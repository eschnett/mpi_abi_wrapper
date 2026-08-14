/* libmpiwrapper -- the hand-written conversion runtime.
 *
 * Excerpt of src/mpiwrapper/. Everything here is hand-written; the bodies in
 * mpiwrapper_wrappers.c are generated and call into it.
 */

#include <mpi.h> /* the implementation's */

#include "mpiabi.h"
#include "mpiwrapper_vtable.h"

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================ ABI -> impl ==== */

/* A dense switch over 0x20..0x2eb, which the compiler turns into a jump table.
 *
 * The case labels are numeric with the symbolic name in a comment, and that is
 * deliberate: MPIABI_INT expands to ((MPIABI_Datatype)0x00000209), and casting an
 * integer constant to a pointer type and back is not an integer constant
 * expression in standard C -- gcc and clang accept it, but a case label is exactly
 * where that extension is not worth relying on. The generator knows the numeric
 * value because it parsed the header, so it emits the number and never transcribes
 * anything by hand either way.
 */
MPI_Datatype mpiwrapper_datatype_fromabi(MPIABI_Datatype abi)
{
  switch ((uintptr_t)abi) {
  case 0x00000200: return MPI_DATATYPE_NULL; /* MPIABI_DATATYPE_NULL */
  case 0x00000209: return MPI_INT;           /* MPIABI_INT */
  case 0x00000214: return MPI_DOUBLE;        /* MPIABI_DOUBLE */
  case 0x00000247: return MPI_BYTE;          /* MPIABI_BYTE */
  /* ... the other predefined datatypes ...
   *
   * Optional ones are #ifdef-guarded on the implementation's own macro name, so
   * the implementation-side value is never written down here:
   *
   *   #ifdef MPI_INTEGER16
   *   case 0x0000022f: return MPI_INTEGER16;
   *   #endif
   */
  default: break;
  }
  /* Not predefined: the bits are an implementation handle we produced earlier. */
  return (MPI_Datatype)(uintptr_t)abi;
}

MPI_Comm mpiwrapper_comm_fromabi(MPIABI_Comm abi)
{
  switch ((uintptr_t)abi) {
  case 0x00000100: return MPI_COMM_NULL;  /* MPIABI_COMM_NULL */
  case 0x00000101: return MPI_COMM_WORLD; /* MPIABI_COMM_WORLD */
  case 0x00000102: return MPI_COMM_SELF;  /* MPIABI_COMM_SELF */
  default: break;
  }
  return (MPI_Comm)(uintptr_t)abi;
}

/* MPI_Info, MPI_Request, MPI_Op, MPI_File, ... all take the same shape. */
MPI_Info mpiwrapper_info_fromabi(MPIABI_Info abi)
{
  if ((uintptr_t)abi == 0x00000130) return MPI_INFO_NULL;
  return (MPI_Info)(uintptr_t)abi;
}

MPI_Request mpiwrapper_request_fromabi(MPIABI_Request abi)
{
  if ((uintptr_t)abi == 0x00000180) return MPI_REQUEST_NULL;
  return (MPI_Request)(uintptr_t)abi;
}

/* ============================================================ impl -> ABI ==== */

/* The hard direction. Predefined *implementation* handle values are not
 * compile-time constants in general -- Open MPI's are the addresses of static
 * objects -- so this map cannot be a switch and cannot be built at compile time.
 * It is built once, inside mpiwrapper_get_vtable, before any slot can be called.
 *
 * A linear scan over the 104 predefined handles would be far too slow: this runs
 * on every out-handle and inside every user-op trampoline, and about 60 of the 104
 * are datatypes, which is the hot case.
 *
 * One table per class, so that an implementation handle value shared across
 * classes cannot alias.
 */

struct rmap_entry {
  uintptr_t key;
  uint64_t  abi;
  int       used;
};

struct rmap {
  struct rmap_entry *slots;
  size_t             mask; /* capacity - 1, capacity a power of two */
};

static struct rmap_entry rmap_datatype_slots[256];
static struct rmap_entry rmap_comm_slots[32];
static struct rmap_entry rmap_op_slots[32];
static struct rmap_entry rmap_file_slots[8];

static struct rmap rmap_datatype = {rmap_datatype_slots, 255};
static struct rmap rmap_comm     = {rmap_comm_slots, 31};
static struct rmap rmap_op       = {rmap_op_slots, 31};
static struct rmap rmap_file     = {rmap_file_slots, 7};

static size_t rmap_hash(uintptr_t k)
{
  /* Fibonacci hashing: implementation handles are dense in their low bits
   * (MPICH) or 8/16-byte-aligned addresses (Open MPI), and both would collide
   * badly under a plain mask.
   */
  uint64_t x = (uint64_t)k;
  x *= 0x9e3779b97f4a7c15u;
  return (size_t)(x >> 32);
}

/* Insert-only, called from initialization while single-threaded. */
static void rmap_insert(struct rmap *m, uintptr_t key, uint64_t abi)
{
  size_t i = rmap_hash(key) & m->mask;
  while (m->slots[i].used) {
    if (m->slots[i].key == key) return; /* aliased predefined handles: first wins */
    i = (i + 1) & m->mask;
  }
  m->slots[i].key  = key;
  m->slots[i].abi  = abi;
  m->slots[i].used = 1;
}

/* Read-only afterwards, so no synchronization is needed on lookup. */
static int rmap_lookup(const struct rmap *m, uintptr_t key, uint64_t *abi)
{
  size_t i = rmap_hash(key) & m->mask;
  while (m->slots[i].used) {
    if (m->slots[i].key == key) {
      *abi = m->slots[i].abi;
      return 1;
    }
    i = (i + 1) & m->mask;
  }
  return 0;
}

/* A dynamically created implementation handle becomes an ABI handle by preserving
 * its bits. That is only correct if it cannot land inside the ABI's predefined
 * range, and it cannot for either implementation: MPICH's handles carry a kind
 * field in the high bits so all real handles are >= 0x04000000, and Open MPI's are
 * object addresses.
 *
 * We cannot prove that at configure time -- cross-compiling forbids running a
 * probe -- and we cannot tag the value, because ABI handles are pointer-sized and
 * a 32-bit target has no spare high bits. So the check is here, at run time, in
 * the toabi direction only: that is object *creation*, not every MPI_Send.
 */
static int dynamic_ok(uintptr_t bits)
{
  return !(bits >= MPIABI_PREDEFINED_FIRST && bits <= MPIABI_PREDEFINED_LAST);
}

MPIABI_Datatype mpiwrapper_datatype_toabi(MPI_Datatype dt)
{
  uint64_t abi;
  if (rmap_lookup(&rmap_datatype, (uintptr_t)dt, &abi))
    return (MPIABI_Datatype)(uintptr_t)abi;
  if (!dynamic_ok((uintptr_t)dt)) return MPIABI_DATATYPE_NULL; /* caller returns
                                                                 MPIABI_ERR_INTERN */
  return (MPIABI_Datatype)(uintptr_t)dt;
}

MPIABI_Request mpiwrapper_request_toabi(MPI_Request req)
{
  if (req == MPI_REQUEST_NULL) return MPIABI_REQUEST_NULL;
  return (MPIABI_Request)(uintptr_t)req;
}

MPIABI_File mpiwrapper_file_toabi(MPI_File fh)
{
  uint64_t abi;
  if (rmap_lookup(&rmap_file, (uintptr_t)fh, &abi))
    return (MPIABI_File)(uintptr_t)abi;
  return (MPIABI_File)(uintptr_t)fh;
}

/* ====================================================== ranks, tags, codes ==== */

/* Separate functions for ranks and tags, and this is the reason: in the ABI,
 * MPI_ANY_TAG is -2 and MPI_PROC_NULL is -3; in MPICH, MPI_ANY_TAG and
 * MPI_PROC_NULL are *both* -1. An int argument therefore cannot be translated
 * without knowing its role, which is why the generator needs each parameter's kind
 * from apis.json and the header alone is not enough.
 */
int mpiwrapper_rank_fromabi(int abi_rank)
{
  switch (abi_rank) {
  case MPIABI_ANY_SOURCE: return MPI_ANY_SOURCE;
  case MPIABI_PROC_NULL:  return MPI_PROC_NULL;
  case MPIABI_ROOT:       return MPI_ROOT;
  case MPIABI_UNDEFINED:  return MPI_UNDEFINED;
  default:                return abi_rank;
  }
}

int mpiwrapper_rank_toabi(int rank)
{
  if (rank == MPI_ANY_SOURCE) return MPIABI_ANY_SOURCE;
  if (rank == MPI_PROC_NULL)  return MPIABI_PROC_NULL;
  if (rank == MPI_ROOT)       return MPIABI_ROOT;
  if (rank == MPI_UNDEFINED)  return MPIABI_UNDEFINED;
  return rank;
  /* Not a switch: MPICH gives MPI_PROC_NULL and MPI_ANY_TAG the same value, so
   * the case labels of a combined switch would not be unique. The order of the
   * tests is what disambiguates, and it is only correct because this function
   * knows it is looking at a rank.
   */
}

int mpiwrapper_tag_fromabi(int abi_tag)
{
  if (abi_tag == MPIABI_ANY_TAG) return MPI_ANY_TAG;
  return abi_tag;
}

int mpiwrapper_tag_toabi(int tag)
{
  if (tag == MPI_ANY_TAG) return MPIABI_ANY_TAG;
  return tag;
}

/* Error codes: the common case is MPI_SUCCESS, which is 0 everywhere, so it costs
 * one compare. Dynamically added codes (MPI_Add_error_class/_code) need a
 * bidirectional table, not shown here: the ABI caps MPI_ERR_LASTCODE at 16383
 * against MPICH's 0x3fffffff, so they must be *renumbered* rather than passed
 * through. Atomic-append, capped at 16383, falling back to MPIABI_ERR_OTHER when
 * an unknown code arrives.
 */
extern int mpiwrapper_dynamic_errorcode_toabi(int ierror);
extern int mpiwrapper_dynamic_errorcode_fromabi(int abi_ierror);

int mpiwrapper_errorcode_toabi(int ierror)
{
  if (ierror == MPI_SUCCESS) return MPIABI_SUCCESS;
  switch (ierror) {
  case MPI_ERR_COUNT:     return MPIABI_ERR_COUNT;
  case MPI_ERR_INTERN:    return MPIABI_ERR_INTERN;
  case MPI_ERR_IN_STATUS: return MPIABI_ERR_IN_STATUS;
  case MPI_ERR_INFO_KEY:  return MPIABI_ERR_INFO_KEY;
  case MPI_ERR_PORT:      return MPIABI_ERR_PORT;
  /* ... the remaining predefined classes ... */
  default: return mpiwrapper_dynamic_errorcode_toabi(ierror);
  }
}

int mpiwrapper_errorcode_fromabi(int abi_ierror)
{
  if (abi_ierror == MPIABI_SUCCESS) return MPI_SUCCESS;
  switch (abi_ierror) {
  case MPIABI_ERR_COUNT:  return MPI_ERR_COUNT;
  case MPIABI_ERR_INTERN: return MPI_ERR_INTERN;
  /* ... */
  default: return mpiwrapper_dynamic_errorcode_fromabi(abi_ierror);
  }
}

/* ==================================================================== status === */

/* The ABI status is 32 bytes: three named ints plus 20 bytes of scratch. The
 * implementation's is smaller, and its private part is the complement of the
 * named-field block -- at the *front* in MPICH (count_lo, count_hi_and_cancelled,
 * then the named fields) and at the *back* in Open MPI (named fields, then
 * _cancelled and _ucount).
 *
 * So the private part is a head range and a tail range, both of compile-time
 * constant length, and one of the two is empty in each known implementation: this
 * compiles to a single 8-byte copy for MPICH and a single 12-byte copy for Open
 * MPI.
 */

#define IMPL_NAMED_OFF offsetof(MPI_Status, MPI_SOURCE)
#define IMPL_NAMED_LEN (3 * sizeof(int))
#define IMPL_HEAD_LEN  IMPL_NAMED_OFF
#define IMPL_TAIL_OFF  (IMPL_NAMED_OFF + IMPL_NAMED_LEN)
#define IMPL_TAIL_LEN  (sizeof(MPI_Status) - IMPL_TAIL_OFF)
#define ABI_SCRATCH    sizeof(((MPIABI_Status *)0)->MPI_internal)

/* The three named fields must be contiguous and in order. True for both known
 * implementations; an implementation that interleaves private bytes between them
 * fails the build here rather than being silently mishandled.
 */
_Static_assert(offsetof(MPI_Status, MPI_TAG) == IMPL_NAMED_OFF + sizeof(int),
               "MPI_Status: MPI_TAG does not follow MPI_SOURCE");
_Static_assert(offsetof(MPI_Status, MPI_ERROR) == IMPL_NAMED_OFF + 2 * sizeof(int),
               "MPI_Status: MPI_ERROR does not follow MPI_TAG");

/* This one has no runtime recourse: there would be nowhere to put the overflow,
 * and side storage keyed on the status address is unsound because applications
 * copy statuses freely. Hence a build failure rather than a check.
 */
_Static_assert(IMPL_HEAD_LEN + IMPL_TAIL_LEN <= ABI_SCRATCH,
               "MPI_Status: private part exceeds the ABI's 20 scratch bytes");

void mpiwrapper_status_toabi(const MPI_Status *st, MPIABI_Status *abi)
{
  const unsigned char *src = (const unsigned char *)st;

  /* Zeroed first, so that the bytes we do not use are reproducible and no
   * implementation stack garbage reaches the application. Not needed for
   * correctness; it keeps valgrind and MSan quiet, which matters because this shim
   * will be debugged under both.
   */
  unsigned char blob[ABI_SCRATCH];
  memset(blob, 0, sizeof blob);
  memcpy(blob, src, IMPL_HEAD_LEN);
  memcpy(blob + IMPL_HEAD_LEN, src + IMPL_TAIL_OFF, IMPL_TAIL_LEN);

  /* MPI_SOURCE can be MPI_PROC_NULL or MPI_ANY_SOURCE, and MPI_TAG can be
   * MPI_ANY_TAG, so both go through their own role-specific conversion here too.
   */
  abi->MPI_SOURCE = mpiwrapper_rank_toabi(st->MPI_SOURCE);
  abi->MPI_TAG    = mpiwrapper_tag_toabi(st->MPI_TAG);
  abi->MPI_ERROR  = mpiwrapper_errorcode_toabi(st->MPI_ERROR);
  memcpy(abi->MPI_internal, blob, sizeof blob);
}

void mpiwrapper_status_fromabi(const MPIABI_Status *abi, MPI_Status *st)
{
  unsigned char *dst = (unsigned char *)st;

  memset(st, 0, sizeof *st);
  memcpy(dst, abi->MPI_internal, IMPL_HEAD_LEN);
  memcpy(dst + IMPL_TAIL_OFF, (const unsigned char *)abi->MPI_internal + IMPL_HEAD_LEN,
         IMPL_TAIL_LEN);

  st->MPI_SOURCE = mpiwrapper_rank_fromabi(abi->MPI_SOURCE);
  st->MPI_TAG    = mpiwrapper_tag_fromabi(abi->MPI_TAG);
  st->MPI_ERROR  = mpiwrapper_errorcode_fromabi(abi->MPI_ERROR);

  /* There is deliberately no validity marker and no fallback for a status the
   * implementation never produced. In the generalized-request flow the blob is
   * always implementation-backed, and for a genuinely uninitialized status garbage
   * out is the correct behaviour -- it is what the native implementation does for
   * the same user error. Neither implementation dereferences anything from these
   * bytes, and MPI already requires statuses to be freely copyable, which forces
   * them to be position-independent and self-contained.
   */
}

/* ================================================================= bitmasks === */

/* MPI_MODE_* are OR-combined and the bit assignments are unrelated: the ABI's
 * RDONLY is 16 where both implementations use 2, and NOCHECK is 1024 where Open
 * MPI uses 1. So this is a decomposition, not a switch.
 */
int mpiwrapper_filemode_fromabi(int abi_amode)
{
  int amode = 0;
  if (abi_amode & MPIABI_MODE_APPEND)          amode |= MPI_MODE_APPEND;
  if (abi_amode & MPIABI_MODE_CREATE)          amode |= MPI_MODE_CREATE;
  if (abi_amode & MPIABI_MODE_DELETE_ON_CLOSE) amode |= MPI_MODE_DELETE_ON_CLOSE;
  if (abi_amode & MPIABI_MODE_EXCL)            amode |= MPI_MODE_EXCL;
  if (abi_amode & MPIABI_MODE_RDONLY)          amode |= MPI_MODE_RDONLY;
  if (abi_amode & MPIABI_MODE_RDWR)            amode |= MPI_MODE_RDWR;
  if (abi_amode & MPIABI_MODE_SEQUENTIAL)      amode |= MPI_MODE_SEQUENTIAL;
  if (abi_amode & MPIABI_MODE_UNIQUE_OPEN)     amode |= MPI_MODE_UNIQUE_OPEN;
  if (abi_amode & MPIABI_MODE_WRONLY)          amode |= MPI_MODE_WRONLY;
  return amode;
}

/* ================================================================== staging === */

void *mpiwrapper_stage(void *stackbuf, size_t stackbytes, size_t nmemb,
                       size_t size)
{
  if (nmemb == 0) return stackbuf; /* never NULL, so callers need no special case */
  if (nmemb <= stackbytes / size) return stackbuf;
  if (nmemb > SIZE_MAX / size) return NULL;
  return malloc(nmemb * size);
}

void mpiwrapper_unstage(void *p, void *stackbuf)
{
  if (p && p != stackbuf) free(p);
}

/* ========================================================= op trampolines ===== */

/* MPI_User_function has no extra-state argument, so there is nothing to smuggle a
 * user pointer through and the user's function cannot be identified from inside
 * the callback. Hence a pool of static trampolines, each knowing its own index.
 *
 * The four *_create_errhandler functions need the same treatment for the same
 * reason -- MPI_Op_create is not the only callback without extra state. Their
 * trampolines must additionally be declared *variadic*, matching
 * MPI_Comm_errhandler_function's `...`: nothing needs forwarding, but variadic and
 * non-variadic calling conventions differ on arm64 macOS, so a non-variadic
 * declaration would be a silent ABI bug. Taking the type from the implementation's
 * own typedef gets that right for free.
 */

#define MPIWRAPPER_OP_SLOTS 1024 /* ~24 KB of text; tunable at build time */

static _Atomic(MPIABI_User_function *) op_fn[MPIWRAPPER_OP_SLOTS];

static void op_dispatch(int slot, void *invec, void *inoutvec, int *len,
                        MPI_Datatype *datatype)
{
  MPIABI_User_function *fn =
      atomic_load_explicit(&op_fn[slot], memory_order_acquire);

  /* This is why a user-op trampoline needs the reverse handle map. */
  MPIABI_Datatype abi_datatype = mpiwrapper_datatype_toabi(*datatype);

  fn(invec, inoutvec, len, &abi_datatype);
}

/* Declared with the implementation's own typedef, so the signature is checked
 * against the header rather than written out twice.
 */
#define OP_TRAMP(i)                                                              \
  static void op_tramp_##i(void *a, void *b, int *n, MPI_Datatype *d)            \
  {                                                                              \
    op_dispatch((i), a, b, n, d);                                                \
  }

OP_TRAMP(0)
OP_TRAMP(1)
OP_TRAMP(2)
OP_TRAMP(3)
/* ... the generator emits MPIWRAPPER_OP_SLOTS of these ... */

static MPI_User_function *const op_tramps[] = {
    op_tramp_0, op_tramp_1, op_tramp_2, op_tramp_3,
    /* ... */
};

static int op_slot_alloc(MPIABI_User_function *fn)
{
  for (int i = 0; i < (int)(sizeof op_tramps / sizeof *op_tramps); ++i) {
    MPIABI_User_function *expected = NULL;
    if (atomic_compare_exchange_strong_explicit(&op_fn[i], &expected, fn,
                                               memory_order_acq_rel,
                                               memory_order_acquire))
      return i;
  }
  return -1; /* pool exhausted */
}

int mpiwrapper_op_create(MPIABI_User_function *abi_user_fn, int abi_commute,
                         MPIABI_Op *abi_op)
{
  const int slot = op_slot_alloc(abi_user_fn);
  if (slot < 0) {
    *abi_op = MPIABI_OP_NULL;
    return MPIABI_ERR_INTERN; /* names MPIWRAPPER_OP_SLOTS in the diagnostic */
  }

  MPI_Op    op;
  const int ierror = MPI_Op_create(op_tramps[slot], abi_commute, &op);
  if (ierror != MPI_SUCCESS) {
    /* The *only* place a slot is ever released. Safe here and nowhere else: the
     * op was never handed to the application, so nothing can reference it.
     *
     * MPI_Op_free is not a release point. MPI-5.0 2.5.2: a deallocate call "marks
     * the object for deallocation... Any operation pending (at the time of the
     * deallocate) ... will complete normally; the object will be deallocated
     * afterwards." So a pending MPI_Ireduce can still invoke this trampoline after
     * MPI_Op_free returns. Reclaiming would need a per-slot refcount driven by all
     * ~36 reduction entry points plus MPI_Start; v1 does not, because a refcount
     * bug is a use-after-free that surfaces as a wrong reduction result at scale,
     * while not reclaiming merely caps op creations at MPIWRAPPER_OP_SLOTS.
     */
    atomic_store_explicit(&op_fn[slot], NULL, memory_order_release);
    *abi_op = MPIABI_OP_NULL;
    return mpiwrapper_errorcode_toabi(ierror);
  }

  uint64_t abi;
  *abi_op = rmap_lookup(&rmap_op, (uintptr_t)op, &abi)
                ? (MPIABI_Op)(uintptr_t)abi
                : (MPIABI_Op)(uintptr_t)op;
  return MPIABI_SUCCESS;
}

/* =============================================================== the getter === */

extern const struct mpiwrapper_vtable mpiwrapper_vtable_instance;

static void init_reverse_maps(void)
{
  /* Every entry names the implementation's own macro, so no implementation-side
   * value is ever transcribed -- and a macro that does not exist is a compile
   * error rather than a wrong number.
   */
  rmap_insert(&rmap_datatype, (uintptr_t)MPI_DATATYPE_NULL, 0x00000200);
  rmap_insert(&rmap_datatype, (uintptr_t)MPI_INT, 0x00000209);
  rmap_insert(&rmap_datatype, (uintptr_t)MPI_DOUBLE, 0x00000214);
  rmap_insert(&rmap_datatype, (uintptr_t)MPI_BYTE, 0x00000247);
#ifdef MPI_INTEGER16
  rmap_insert(&rmap_datatype, (uintptr_t)MPI_INTEGER16, 0x0000022f);
#endif
  /* ... all 104 predefined handles, one line each, generated ... */

  rmap_insert(&rmap_comm, (uintptr_t)MPI_COMM_NULL, 0x00000100);
  rmap_insert(&rmap_comm, (uintptr_t)MPI_COMM_WORLD, 0x00000101);
  rmap_insert(&rmap_comm, (uintptr_t)MPI_COMM_SELF, 0x00000102);

  rmap_insert(&rmap_op, (uintptr_t)MPI_OP_NULL, 0x00000020);
  rmap_insert(&rmap_op, (uintptr_t)MPI_SUM, 0x00000021);

  rmap_insert(&rmap_file, (uintptr_t)MPI_FILE_NULL, 0x00000118);
}

const struct mpiwrapper_vtable *
mpiwrapper_get_vtable(uint32_t abi_version, uint32_t layout_hash, size_t size,
                      const char **diagnostic)
{
  static atomic_int initialized;

  if (abi_version != MPIABI_VERSION) {
    *diagnostic = "MPI ABI version mismatch between libmpi_abi and libmpiwrapper";
    return NULL;
  }
  if (layout_hash != MPIWRAPPER_LAYOUT_HASH) {
    *diagnostic = "vtable layout mismatch: libmpi_abi and libmpiwrapper were "
                  "generated from different slot lists";
    return NULL;
  }
  /* A larger caller struct would read past the end of ours. A smaller one is fine:
   * it sees a prefix, and the layout hash already established that the prefix
   * agrees.
   */
  if (size > sizeof(struct mpiwrapper_vtable)) {
    *diagnostic = "libmpi_abi expects a larger vtable than this libmpiwrapper "
                  "provides";
    return NULL;
  }

  int expected = 0;
  if (atomic_compare_exchange_strong_explicit(&initialized, &expected, 1,
                                              memory_order_acq_rel,
                                              memory_order_acquire))
    init_reverse_maps();

  /* The maps are complete before any slot can be reached, which is the reason this
   * is a getter rather than an exported struct.
   */
  return &mpiwrapper_vtable_instance;
}
