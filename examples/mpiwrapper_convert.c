/* libmpiwrapper -- the hand-written conversion runtime.
 *
 * Excerpt of src/mpiwrapper/. Everything here is hand-written; the bodies in
 * mpiwrapper_wrappers.c are generated and call into it.
 */

#include <mpi.h> /* the implementation's */

#include "mpiabi.h"
#include "mpiwrapper_vtable.h"

#include <dlfcn.h>
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

/* A *perfect* hash, not open addressing. The whole key set is known at
 * initialization, so a multiplier can be searched for until no two keys collide,
 * which removes the probe loop -- and the probe loop is the only data-dependent part.
 * dev/handle-map-bench measures the difference: 1.09 ns flat, against 1.36-1.53 ns
 * for open addressing when the datatype varies from call to call.
 *
 * That benchmark also answers the obvious alternatives. A sorted array with binary
 * search costs 3.4x as much: seven dependent, unpredictable comparisons.
 * Interpolation search is a trap -- O(log log n) assumes uniformly distributed keys,
 * and MPICH's are one value at 0x0c000000, a dense cluster at 0x4c00xxxx, and one at
 * 0x8c000004, on which it degenerates toward a linear scan with a floating-point
 * divide per step and measures *eighty times* slower.
 */

#define RMAP_EMPTY UINT64_MAX /* no real handle: not a valid address, not a valid
                               * MPICH handle. Lets the lookup be a single compare
                               * with no separate "used" test. */

struct rmap_entry {
  uint64_t key;
  uint64_t abi;
};

struct rmap {
  struct rmap_entry *slots;
  size_t             nslots; /* power of two */
  uint64_t           mul;
  unsigned           shift;
};

static struct rmap_entry rmap_datatype_slots[1024];
static struct rmap_entry rmap_comm_slots[64];
static struct rmap_entry rmap_op_slots[64];
static struct rmap_entry rmap_file_slots[16];

static struct rmap rmap_datatype = {rmap_datatype_slots, 1024, 0, 0};
static struct rmap rmap_comm     = {rmap_comm_slots, 64, 0, 0};
static struct rmap rmap_op       = {rmap_op_slots, 64, 0, 0};
static struct rmap rmap_file     = {rmap_file_slots, 16, 0, 0};

/* Search multipliers until the key set is collision-free. Bounded, and reports
 * failure so that mpiwrapper_get_vtable can refuse with a diagnostic: degrading to a
 * probe loop at run time would put back the branch this exists to remove, so the
 * failure has to be loud and at initialization.
 */
static int rmap_build(struct rmap *m, const uint64_t *keys, const uint64_t *abis,
                      size_t n)
{
  unsigned shift = 64;
  for (size_t t = m->nslots; t > 1; t >>= 1) --shift;

  uint64_t mul = 0x9e3779b97f4a7c15u;
  for (int attempt = 0; attempt < 4096; ++attempt, mul += 0x2545f4914f6cdd1du) {
    for (size_t i = 0; i < m->nslots; ++i) m->slots[i].key = RMAP_EMPTY;
    int ok = 1;
    for (size_t i = 0; i < n && ok; ++i) {
      const size_t slot = (size_t)((keys[i] * mul) >> shift);
      if (m->slots[slot].key != RMAP_EMPTY) ok = 0;
      else { m->slots[slot].key = keys[i]; m->slots[slot].abi = abis[i]; }
    }
    if (ok) { m->mul = mul; m->shift = shift; return 1; }
  }
  return 0;
}

/* One multiply, one shift, one load, one compare. No loop. */
static inline int rmap_lookup(const struct rmap *m, uintptr_t key, uint64_t *abi)
{
  const struct rmap_entry *e =
      &m->slots[(size_t)(((uint64_t)key * m->mul) >> m->shift)];
  if (e->key != (uint64_t)key) return 0;
  *abi = e->abi;
  return 1;
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
  /* A switch is fine here, and in every one of these four functions. Within a
   * single role the implementation's magic values are distinct -- for ranks MPICH
   * uses -1, -2, -3, -32766 and Open MPI -2, -1, -4, -32766 -- so the case labels
   * are unique. It is only a *combined* rank-and-tag conversion that could not be
   * written as a switch, because MPICH gives MPI_PROC_NULL and MPI_ANY_TAG the same
   * value (-1). That is the reason the roles are separate functions, and it is not a
   * reason to avoid a switch inside them.
   */
  switch (rank) {
  case MPI_ANY_SOURCE: return MPIABI_ANY_SOURCE;
  case MPI_PROC_NULL:  return MPIABI_PROC_NULL;
  case MPI_ROOT:       return MPIABI_ROOT;
  case MPI_UNDEFINED:  return MPIABI_UNDEFINED;
  default:             return rank;
  }
}

int mpiwrapper_tag_fromabi(int abi_tag)
{
  switch (abi_tag) {
  case MPIABI_ANY_TAG: return MPI_ANY_TAG;
  default:             return abi_tag;
  }
}

int mpiwrapper_tag_toabi(int tag)
{
  switch (tag) {
  case MPI_ANY_TAG: return MPIABI_ANY_TAG;
  default:          return tag;
  }
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
 *
 * File modes and window asserts need *separate* mappers, which is why this one is
 * named for its role. The ABI keeps the two families in one enum with disjoint
 * bits, so one mapper would do on the way in; Open MPI gives its window asserts
 * the same bit values it gives CREATE, RDONLY, WRONLY, RDWR and DELETE_ON_CLOSE,
 * so on the way out an implementation-side 1 is ambiguous. NOTES.md 5.5.
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

static int init_reverse_maps(void)
{
  /* Every entry names the implementation's own macro, so no implementation-side
   * value is ever transcribed -- and a macro that does not exist is a compile error
   * rather than a wrong number. The generator emits these, one line per predefined
   * handle, with #ifdef guards on the optional ones.
   *
   * Note what these arrays are *not*: `static`. An earlier version of this file had
   * them static, which compiles against MPICH, whose handles are integer constants,
   * and does not compile at all against Open MPI, whose handles are the addresses
   * of objects -- a pointer-to-integer cast is not a constant expression, so it
   * cannot initialize static storage. They are built at run time instead, which
   * costs one pass over 103 entries once per process.
   */
  const uint64_t dt_impl[] = {
      (uint64_t)(uintptr_t)MPI_DATATYPE_NULL,
      (uint64_t)(uintptr_t)MPI_INT,
      (uint64_t)(uintptr_t)MPI_DOUBLE,
      (uint64_t)(uintptr_t)MPI_BYTE,
#ifdef MPI_INTEGER16
      (uint64_t)(uintptr_t)MPI_INTEGER16,
#endif
  };
  const uint64_t dt_abi[] = {
      0x00000200, 0x00000209, 0x00000214, 0x00000247,
#ifdef MPI_INTEGER16
      0x0000022f,
#endif
  };
  const uint64_t comm_impl[] = {(uint64_t)(uintptr_t)MPI_COMM_NULL,
                                       (uint64_t)(uintptr_t)MPI_COMM_WORLD,
                                       (uint64_t)(uintptr_t)MPI_COMM_SELF};
  const uint64_t comm_abi[]  = {0x00000100, 0x00000101, 0x00000102};
  const uint64_t op_impl[]   = {(uint64_t)(uintptr_t)MPI_OP_NULL,
                                       (uint64_t)(uintptr_t)MPI_SUM};
  const uint64_t op_abi[]    = {0x00000020, 0x00000021};
  const uint64_t file_impl[] = {(uint64_t)(uintptr_t)MPI_FILE_NULL};
  const uint64_t file_abi[]  = {0x00000118};

#define N(a) (sizeof(a) / sizeof(*(a)))
  return rmap_build(&rmap_datatype, dt_impl, dt_abi, N(dt_impl)) &&
         rmap_build(&rmap_comm, comm_impl, comm_abi, N(comm_impl)) &&
         rmap_build(&rmap_op, op_impl, op_abi, N(op_impl)) &&
         rmap_build(&rmap_file, file_impl, file_abi, N(file_impl));
#undef N
}

/* Did the loader bind our MPI_* calls outward to the implementation, or back into
 * libmpi_abi? On ELF the second is the default outcome, because a dlopen'ed object
 * searches the global scope -- where libmpi_abi lives -- before its own
 * dependencies. dev/dlopen-probe measures this: plain RTLD_LOCAL captures both the
 * wrapper's calls and the implementation's own internal ones, and RTLD_DEEPBIND or
 * dlmopen fixes both.
 *
 * Checking the outcome rather than the mechanism matters: dlinfo(RTLD_DI_LMID)
 * confirms which namespace we got, but not that every reference resolved the way the
 * namespace was supposed to make it resolve. This check does not care which
 * mechanism was used, or whether it propagated to dependencies.
 */
static int resolution_is_outward(const void *abi_probe, const char **diagnostic)
{
  Dl_info abi_info, impl_info;

  if (!abi_probe) return 1; /* caller opted out */

  if (!dladdr(abi_probe, &abi_info) ||
      !dladdr((const void *)(uintptr_t)&MPI_Send, &impl_info)) {
    /* dladdr is best-effort; a static implementation can defeat it. Not fatal. */
    return 1;
  }

  if (abi_info.dli_fbase == impl_info.dli_fbase) {
    *diagnostic =
        "symbol resolution captured: this libmpiwrapper's MPI_* calls resolve "
        "back into libmpi_abi instead of the MPI implementation, which would "
        "recurse until the stack is exhausted. Load the wrapper with dlmopen or "
        "RTLD_DEEPBIND.";
    return 0;
  }
  return 1;
}

const struct mpiwrapper_vtable *
mpiwrapper_get_vtable(uint32_t abi_version, uint32_t abi_subversion,
                      uint32_t layout_hash, size_t size, const void *abi_probe,
                      const char **diagnostic)
{
  static atomic_int initialized;

  /* The ABI header carries both MPI_ABI_VERSION and MPI_ABI_SUBVERSION, so check
   * both. A differing major version is incompatible outright. A differing
   * subversion is not necessarily fatal -- subversions are meant to be additive --
   * but the two halves must agree on which one they were generated from, and the
   * layout hash below would not catch a subversion that added no slot.
   */
  if (abi_version != MPIABI_VERSION) {
    *diagnostic = "MPI ABI major version mismatch between libmpi_abi and "
                  "libmpiwrapper";
    return NULL;
  }
  if (abi_subversion != MPIABI_SUBVERSION) {
    *diagnostic = "MPI ABI subversion mismatch between libmpi_abi and "
                  "libmpiwrapper";
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

  if (!resolution_is_outward(abi_probe, diagnostic)) return NULL;

  int expected = 0;
  if (atomic_compare_exchange_strong_explicit(&initialized, &expected, 1,
                                              memory_order_acq_rel,
                                              memory_order_acquire)) {
    if (!init_reverse_maps()) {
      /* Loud, and at initialization: falling back to a probe loop at run time would
       * reintroduce the branch the perfect hash exists to remove.
       */
      *diagnostic = "could not construct a collision-free predefined-handle map";
      return NULL;
    }
  }

  /* The maps are complete before any slot can be reached, which is the reason this
   * is a getter rather than an exported struct.
   */
  return &mpiwrapper_vtable_instance;
}
