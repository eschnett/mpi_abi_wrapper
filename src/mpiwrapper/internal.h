/* libmpiwrapper -- the interface between the hand-written conversion runtime
 * (src/mpiwrapper/) and the wrapper bodies (gen/mpiwrapper/, S1: wrappers.c
 * beside this file).
 *
 * Two headers meet in every translation unit that includes this one, and
 * nowhere else in the project:
 *   <mpi.h>     the *implementation's*. Included normally, with no preprocessor
 *               games, because this library defines no MPI_* function and so
 *               collides with nothing. It supplies the real MPI_Send, the real
 *               MPI_COMM_WORLD and -- crucially -- the real declarations, which
 *               check every call in the wrapper bodies at compile time. That is
 *               why no hand-maintained table of implementation signatures has
 *               to exist.
 *   "mpiabi.h"  the MPIABI_ view of the ABI.
 *
 * Naming: ABI-side names carry an abi_ prefix, implementation-side names are
 * bare (NOTES.md #3).
 */

#ifndef MPIWRAPPER_INTERNAL_H
#define MPIWRAPPER_INTERNAL_H

#include <mpi.h> /* the implementation's */

/* ...and it really has to be the implementation's. gen/include holds both
 * mpiabi.h and the ABI's own mpi.h, so an include path that lists it before the
 * MPI's own directory silently turns every wrapper body into a call back into
 * the ABI header's declarations. That configuration compiles and links -- the
 * ABI header is a complete, valid mpi.h, and the implementation exports the
 * names -- and then fails at run time with the implementation rejecting
 * `comm=0x101`, which is MPIABI_COMM_WORLD arriving unconverted. S1 hit exactly
 * that, and a build-ordering mistake that only shows up as a wrong handle at
 * run time is worth one #error.
 *
 * The discriminator is the ABI status guard, which only the ABI header defines
 * at this point (mpiabi.h has not been included yet on this line). Wrapping an
 * MPI that genuinely implements the ABI is a legitimate configuration -- it is
 * oracle 5, where every conversion becomes an identity and any remaining
 * difference is a bug in the plumbing -- so it is permitted, explicitly.
 */
#if defined(MPI_ABI_STATUS_DEFINED) && !defined(MPIWRAPPER_WRAP_ABI_IMPL)
#  error "<mpi.h> resolved to the ABI header rather than an implementation's. \
Put the MPI's include directory ahead of gen/include, or define \
MPIWRAPPER_WRAP_ABI_IMPL if you really are wrapping an ABI-implementing MPI."
#endif

#include "mpiabi.h"
#include "mpiwrapper_vtable.h"

/* What this implementation actually has, written per build tree by
 * dev/probe_impl.py: one MPIWRAPPER_HAVE_<name> per entry point it declares
 * and per optional constant it defines. Every guard in the generated sources
 * tests one of those and nothing else -- never `#ifdef <the implementation's
 * own name>`, which sees macros and not enumerators and is therefore quietly
 * false for MPI_COMBINER_* on MPICH and MPI_THREAD_* on Open MPI.
 *
 * Without the probe every generated body would take its #else branch and the
 * library would be a complete set of stubs -- which links, loads, and answers
 * MPI_ERR_UNSUPPORTED_OPERATION to everything. A missing probe has to be a
 * build failure, not that.
 */
#include "mpiwrapper_impl_config.h"
#ifndef MPIWRAPPER_IMPL_PROBED
#  error "mpiwrapper_impl_config.h did not come from dev/probe_impl.py"
#endif

#include <stddef.h>
#include <stdint.h>

/* ------------------------------------------------------ compile-time checks */

/* NOTES.md #5.9: static-assert where a runtime check would cost something on a
 * hot path, handle at run time where the check is free and the degradation
 * announces itself. A narrowing check on MPI_Count would land on MPI_Send_c and
 * every large-count call, so it is asserted here instead.
 */
_Static_assert(sizeof(MPI_Aint) == sizeof(MPIABI_Aint),
               "MPI_Aint differs in size from the ABI's");
_Static_assert(sizeof(MPI_Count) == sizeof(MPIABI_Count),
               "MPI_Count differs in size from the ABI's");
_Static_assert(sizeof(MPI_Offset) == sizeof(MPIABI_Offset),
               "MPI_Offset differs in size from the ABI's");
_Static_assert(sizeof(MPI_Fint) == sizeof(MPIABI_Fint),
               "MPI_Fint differs in size from the ABI's");
_Static_assert((MPI_Aint)-1 < 0 && (MPI_Count)-1 < 0 && (MPI_Offset)-1 < 0,
               "MPI_Aint/Count/Offset must be signed, as the ABI's are");
/* Every implementation handle must fit in a pointer-sized ABI handle, in both
 * directions and without loss. MPICH's are ints, Open MPI's are addresses.
 */
_Static_assert(sizeof(MPI_Comm) <= sizeof(MPIABI_Comm),
               "implementation handles do not fit in an ABI handle");

/* ------------------------------------------------------------- handle bits */

/* An implementation handle is either an integer (MPICH) or a pointer (Open
 * MPI), so the only portable way to move it through a uint64_t is a cast pair.
 * Signed integer handles sign-extend -- MPICH's MPI_LONG_DOUBLE_INT is
 * 0x8c000004, i.e. negative as an int, and becomes 0xffffffff8c000004 here --
 * which is harmless as long as *both* directions go through these macros, so
 * that the round trip is exact and map keys are formed the same way at build
 * and at lookup.
 */
#define MPIWRAPPER_BITS(h) ((uint64_t)(uintptr_t)(h))
#define MPIWRAPPER_HANDLE(type, bits) ((type)(uintptr_t)(bits))

/* The ABI's predefined handle range. Used in both directions: toabi to reject a
 * dynamic implementation handle that would collide with it (NOTES.md #5.1), and
 * fromabi to recognize an ABI predefined handle this implementation does not
 * provide. It has to be a compile-time constant to stay off the hot path, so it
 * is written here rather than read out of a generated table -- and
 * test/mpiwrapper_selftest.c asserts it against the minimum and maximum over
 * every generated predefined-handle table, so a new ABI header that widened the
 * range fails a test rather than silently narrowing this check.
 */
#define MPIWRAPPER_PREDEF_FIRST 0x00000020u
#define MPIWRAPPER_PREDEF_LAST  0x000002ebu

static inline int mpiwrapper_in_predef_range(uint64_t bits)
{
  return bits >= MPIWRAPPER_PREDEF_FIRST && bits <= MPIWRAPPER_PREDEF_LAST;
}

/* ---------------------------------------------------------- tunable limits */

/* All shared tables are fixed-capacity and lock-free (NOTES.md #6.3). Overflow
 * is a documented limit that returns MPIABI_ERR_INTERN naming the macro.
 */
#ifndef MPIWRAPPER_OP_SLOTS
#  define MPIWRAPPER_OP_SLOTS 1024
#endif
#ifndef MPIWRAPPER_ERRHANDLER_SLOTS
#  define MPIWRAPPER_ERRHANDLER_SLOTS 256
#endif
#ifndef MPIWRAPPER_STAGED_REQUEST_SLOTS
#  define MPIWRAPPER_STAGED_REQUEST_SLOTS 1024
#endif
/* Staging threshold in *bytes*, not elements, so that a 32-byte-per-element
 * status array cannot blow a budget tuned for 8-byte handles.
 */
#ifndef MPIWRAPPER_STAGE_BYTES
#  define MPIWRAPPER_STAGE_BYTES 1024
#endif

/* ------------------------------------------------- handles: ABI -> impl ---- */

/* Generated (S1: constants.c). A dense switch over the predefined range, which
 * the compiler turns into a jump table, then a bit-cast.
 */
MPI_Comm       mpiwrapper_comm_fromabi(MPIABI_Comm abi);
MPI_Datatype   mpiwrapper_datatype_fromabi(MPIABI_Datatype abi);
MPI_Errhandler mpiwrapper_errhandler_fromabi(MPIABI_Errhandler abi);
MPI_File       mpiwrapper_file_fromabi(MPIABI_File abi);
MPI_Group      mpiwrapper_group_fromabi(MPIABI_Group abi);
MPI_Info       mpiwrapper_info_fromabi(MPIABI_Info abi);
MPI_Message    mpiwrapper_message_fromabi(MPIABI_Message abi);
MPI_Op         mpiwrapper_op_fromabi(MPIABI_Op abi);
MPI_Request    mpiwrapper_request_fromabi(MPIABI_Request abi);
/* MPI_Session is MPI-4.0, and an implementation may simply not have it: Ubuntu's
 * Open MPI 4.1 does not. Nothing can test for a *type*, so the test is the
 * class's null handle -- asked of the compiler by dev/probe_impl.py, not of the
 * preprocessor, since an implementation is free to spell it as an enumerator.
 * Testing MPI_VERSION >= 4 instead would be wrong -- Open MPI 5.0 reports
 * MPI-3.1 and does have sessions.
 */
#ifdef MPIWRAPPER_HAVE_MPI_SESSION_NULL
MPI_Session    mpiwrapper_session_fromabi(MPIABI_Session abi);
#endif
MPI_Win        mpiwrapper_win_fromabi(MPIABI_Win abi);

/* ------------------------------------------------- handles: impl -> ABI ---- */

/* Hand-written (handles.c): a perfect hash built at initialization, since
 * predefined implementation handle values are not compile-time constants in
 * general (Open MPI's are addresses of objects).
 *
 * A dynamic handle that would collide with the ABI's predefined range is
 * rejected: these return the class's ABI null handle and set the flag read by
 * mpiwrapper_take_handle_error(), which the caller turns into
 * MPIABI_ERR_INTERN. Checking only in this direction is what keeps it off the
 * hot path -- this is object creation, not every MPI_Send (NOTES.md #5.1).
 */
MPIABI_Comm       mpiwrapper_comm_toabi(MPI_Comm h);
MPIABI_Datatype   mpiwrapper_datatype_toabi(MPI_Datatype h);
MPIABI_Errhandler mpiwrapper_errhandler_toabi(MPI_Errhandler h);
MPIABI_File       mpiwrapper_file_toabi(MPI_File h);
MPIABI_Group      mpiwrapper_group_toabi(MPI_Group h);
MPIABI_Info       mpiwrapper_info_toabi(MPI_Info h);
MPIABI_Message    mpiwrapper_message_toabi(MPI_Message h);
MPIABI_Op         mpiwrapper_op_toabi(MPI_Op h);
MPIABI_Request    mpiwrapper_request_toabi(MPI_Request h);
#ifdef MPIWRAPPER_HAVE_MPI_SESSION_NULL
MPIABI_Session    mpiwrapper_session_toabi(MPI_Session h);
#endif
MPIABI_Win        mpiwrapper_win_toabi(MPI_Win h);

/* Returns and clears the "a handle could not be represented" flag set by the
 * toabi conversions above. Thread-local, so a collision reported by one thread
 * cannot be attributed to another's call.
 */
int mpiwrapper_take_handle_error(void);

/* ---------------------------------------------------- the reverse handle map */

/* One table per class, so that an implementation handle value shared across
 * classes cannot alias. RMAP_EMPTY is neither a valid address nor a valid MPICH
 * handle, which lets a lookup be a single compare with no separate "used" test.
 */
#define MPIWRAPPER_RMAP_EMPTY UINT64_MAX

struct mpiwrapper_rmap_entry {
  uint64_t key; /* the implementation handle's bits */
  uint64_t abi; /* the ABI handle's bits */
};

struct mpiwrapper_rmap {
  struct mpiwrapper_rmap_entry *slots;
  size_t                        nslots; /* power of two */
  uint64_t                      mul;
  unsigned                      shift;
  size_t                        nkeys;
};

/* Search multipliers until the key set is collision-free; returns 0 if none was
 * found, so that mpiwrapper_get_vtable can refuse with a diagnostic. Degrading
 * to a probe loop at run time would put back the only data-dependent branch the
 * perfect hash exists to remove, so the failure has to be loud and at
 * initialization (NOTES.md #5.1).
 *
 * Two keys with the *same* value are an alias, not a collision: an
 * implementation may give two ABI-distinct predefined handles the same value
 * (an unsupported optional datatype answering MPI_DATATYPE_NULL, for one). The
 * first entry wins and the second is dropped, since the reverse direction has
 * to pick one and the ABI's own order is the canonical one.
 */
int mpiwrapper_rmap_build(struct mpiwrapper_rmap *m, const uint64_t *keys,
                          const uint64_t *abis, size_t n);

/* One multiply, one shift, one load, one compare. No loop. */
static inline int mpiwrapper_rmap_lookup(const struct mpiwrapper_rmap *m,
                                         uint64_t key, uint64_t *abi)
{
  const struct mpiwrapper_rmap_entry *e =
      &m->slots[(size_t)((key * m->mul) >> m->shift)];
  if (e->key != key) return 0;
  *abi = e->abi;
  return 1;
}

/* Built by init_reverse_maps() (constants.c) before any slot can be called. */
int mpiwrapper_init_reverse_maps(const char **diagnostic);

/* --------------------------------------------------- predefined-handle tables */

/* Generated (S1: constants.c). The same tables feed the reverse map and
 * test/mpiwrapper_selftest.c, which walks every predefined handle in both
 * directions -- so the test cannot drift from what the maps were built from.
 *
 * Filled at run time rather than declared as static data, because Open MPI's
 * predefined handles are addresses and a pointer-to-integer cast is not a
 * constant expression: `static const uint64_t t[] = {(uint64_t)(uintptr_t)
 * MPI_INT}` does not compile there, only against an integer-handle MPI.
 */
struct mpiwrapper_predef {
  uint64_t    abi;
  uint64_t    impl;
  const char *name; /* the ABI spelling, for diagnostics */
};

struct mpiwrapper_predef_class {
  const char *name;
  size_t (*fill)(struct mpiwrapper_predef *out, size_t max);
  uint64_t (*toabi_bits)(uint64_t impl_bits);
  uint64_t (*fromabi_bits)(uint64_t abi_bits);
  struct mpiwrapper_rmap *map;
};

extern struct mpiwrapper_rmap mpiwrapper_rmap_comm;
extern struct mpiwrapper_rmap mpiwrapper_rmap_datatype;
extern struct mpiwrapper_rmap mpiwrapper_rmap_errhandler;
extern struct mpiwrapper_rmap mpiwrapper_rmap_file;
extern struct mpiwrapper_rmap mpiwrapper_rmap_group;
extern struct mpiwrapper_rmap mpiwrapper_rmap_info;
extern struct mpiwrapper_rmap mpiwrapper_rmap_message;
extern struct mpiwrapper_rmap mpiwrapper_rmap_op;
extern struct mpiwrapper_rmap mpiwrapper_rmap_request;
#ifdef MPIWRAPPER_HAVE_MPI_SESSION_NULL
extern struct mpiwrapper_rmap mpiwrapper_rmap_session;
#endif
extern struct mpiwrapper_rmap mpiwrapper_rmap_win;

/* NULL-terminated; walked by the selftest. */
extern const struct mpiwrapper_predef_class mpiwrapper_predef_classes[];

/* ------------------------------------------------- integer constant classes */

/* Generated (S1: constants.c). Ranks and tags are different classes because
 * MPICH gives MPI_PROC_NULL and MPI_ANY_TAG the same value (-1) while the ABI
 * gives them -3 and -2: an int cannot be translated without knowing its role
 * (NOTES.md #5.4).
 */
int mpiwrapper_rank_fromabi(int abi_rank);
int mpiwrapper_rank_toabi(int rank);
int mpiwrapper_tag_fromabi(int abi_tag);
int mpiwrapper_tag_toabi(int tag);

int mpiwrapper_errorcode_fromabi(int abi_ierror);
int mpiwrapper_errorcode_toabi(int ierror);

/* MPI_MODE_* are OR-combined and the bit assignments are unrelated between the
 * ABI and either implementation, so these are decompositions, not switches.
 *
 * Two mappers, not the one NOTES.md #5.5 originally called for: the ABI keeps
 * file modes (1..256) and window asserts (1024..16384) in one enum with
 * disjoint bits, but Open MPI gives its window asserts the values 1, 2, 4, 8,
 * 16 -- the same bits it gives MPI_MODE_CREATE, RDONLY, WRONLY, RDWR and
 * DELETE_ON_CLOSE. So an implementation-side value cannot be interpreted
 * without knowing which parameter it came from, exactly as for ranks and tags.
 * MPICH keeps the families disjoint, which is how a single mapper passes every
 * test on one implementation.
 */
int mpiwrapper_filemode_fromabi(int abi_mode);
int mpiwrapper_filemode_toabi(int mode);
int mpiwrapper_winassert_fromabi(int abi_mode);
int mpiwrapper_winassert_toabi(int mode);

/* The remaining mapped integer constants, one family per apis.json kind, all
 * generated (gen/mpiwrapper/constants.c). They are switches rather than
 * decompositions: unlike MPI_MODE_*, these are single values, and unlike ranks
 * and tags they carry no sentinel that has to survive the round trip. The
 * default arm passes an unrecognized value through for the implementation to
 * reject, which is what an implementation missing an optional member does.
 */
int mpiwrapper_combiner_fromabi(int abi_combiner);
int mpiwrapper_combiner_toabi(int combiner);
int mpiwrapper_compare_fromabi(int abi_compare);
int mpiwrapper_compare_toabi(int compare);
int mpiwrapper_darg_fromabi(int abi_darg);
int mpiwrapper_darg_toabi(int darg);
int mpiwrapper_distribute_fromabi(int abi_distribute);
int mpiwrapper_distribute_toabi(int distribute);
int mpiwrapper_locktype_fromabi(int abi_locktype);
int mpiwrapper_locktype_toabi(int locktype);
int mpiwrapper_order_fromabi(int abi_order);
int mpiwrapper_order_toabi(int order);
int mpiwrapper_seek_fromabi(int abi_seek);
int mpiwrapper_seek_toabi(int seek);
int mpiwrapper_splittype_fromabi(int abi_splittype);
int mpiwrapper_splittype_toabi(int splittype);
int mpiwrapper_threadlevel_fromabi(int abi_threadlevel);
int mpiwrapper_threadlevel_toabi(int threadlevel);
int mpiwrapper_topology_fromabi(int abi_topology);
int mpiwrapper_topology_toabi(int topology);
int mpiwrapper_typeclass_fromabi(int abi_typeclass);
int mpiwrapper_typeclass_toabi(int typeclass);

/* --------------------------------------------------------------- sentinels */

/* Pointer values with special meaning, fixed in the ABI and possibly
 * non-constant in the implementation. One test per site (NOTES.md #5.3).
 * Separate functions per role because which sentinels are legal is a property
 * of the parameter, not of the type: MPI_IN_PLACE is not accepted where only
 * MPI_BOTTOM is.
 */
const void *mpiwrapper_sendbuf_fromabi(const void *abi_buf);
const void *mpiwrapper_sendbuf_inplace_fromabi(const void *abi_buf);
void       *mpiwrapper_recvbuf_fromabi(void *abi_buf);
void       *mpiwrapper_recvbuf_inplace_fromabi(void *abi_buf);

/* ------------------------------------------------------------------ status */

void mpiwrapper_status_toabi(const MPI_Status *st, MPIABI_Status *abi);
void mpiwrapper_status_fromabi(const MPIABI_Status *abi, MPI_Status *st);

/* ----------------------------------------------------------------- staging */

/* A fixed-size caller buffer when the request fits in it, heap otherwise. Never
 * a VLA (optional in C11, absent in MSVC, and unbounded means a stack overflow
 * at 100k ranks) and never alloca. Returns NULL only on allocation failure,
 * which the caller turns into MPIABI_ERR_INTERN.
 */
void *mpiwrapper_stage(void *stackbuf, size_t stackbytes, size_t nmemb,
                       size_t size);
void  mpiwrapper_unstage(void *p, void *stackbuf);

/* Temporaries that must outlive their call -- the MPI_Ialltoallw family and the
 * persistent _init forms -- live in a request-keyed table, guarded by a global
 * atomic count so that completion calls pay one relaxed load and a compare
 * against zero when the application never uses those routines (NOTES.md #6.3,
 * decision 10).
 */
int  mpiwrapper_staged_attach(MPI_Request request, void *block);
void mpiwrapper_staged_release(MPI_Request request);
int  mpiwrapper_staged_any(void);

/* ------------------------------------------------------------- trampolines */

/* Callbacks without an extra-state argument need a pool of static trampolines,
 * each knowing its own index; there is nothing to smuggle a user pointer
 * through. MPI_Op_create is the familiar case and the four *_create_errhandler
 * functions are the non-obvious one (NOTES.md #6.1).
 *
 * The registration bodies are in handwritten.c rather than here, because each
 * exists twice -- once calling MPI_Op_create, once PMPI_Op_create -- and
 * callbacks.c must name neither. `alloc` returns -1 when the pool is full;
 * `release` is only ever legal on the path where the implementation refused to
 * create the object, so nothing can hold a reference to it (NOTES.md #6.2).
 */
int                mpiwrapper_op_slot_alloc(MPIABI_User_function *fn);
MPI_User_function *mpiwrapper_op_tramp(int slot);
void               mpiwrapper_op_slot_release(int slot);

int mpiwrapper_comm_errh_slot_alloc(MPIABI_Comm_errhandler_function *fn);
MPI_Comm_errhandler_function *mpiwrapper_comm_errh_tramp(int slot);
void                          mpiwrapper_comm_errh_slot_release(int slot);

#endif /* MPIWRAPPER_INTERNAL_H */
