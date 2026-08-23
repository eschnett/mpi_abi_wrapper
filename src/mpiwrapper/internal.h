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

#include <limits.h> /* CHAR_BIT, for the large-count narrowing checks */
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

/* -------------------------------------------------- large-count narrowing */

/* NOTES.md #5.10. Where the implementation has no `_c` entry point, the
 * wrapper calls its small twin, and every in-direction value has to be checked
 * against the type that twin takes. These three are the only place that check
 * is written, so what "will not fit" means is one definition rather than a
 * hundred emitted comparisons.
 *
 * They are *not* the assertion above being relitigated. That one says the ABI's
 * MPI_Count and the implementation's are the same width, which is still true
 * and still what makes the ordinary path a pointer cast. This is the different
 * question of whether a value fits the *small* twin's `int` -- and it has no
 * build-time answer, because whether the small twin is called at all depends on
 * what the implementation has.
 */
static inline int mpiwrapper_fits(MPIABI_Count v, size_t bytes)
{
  /* Unsigned throughout, and phrased as a magnitude rather than as a pair of
   * limits.h names: MPI_Aint is a typedef whose spelling differs between
   * implementations and has no MPI_AINT_MAX beside it, and the case where the
   * destination is as wide as MPIABI_Count -- every 64-bit host, for
   * mpiwrapper_narrow_aint -- must not overflow while being folded away.
   */
  const uint64_t magnitude = (uint64_t)1 << (bytes * CHAR_BIT - 1);
  return v >= 0 ? (uint64_t)v < magnitude
                : (uint64_t)(-(v + 1)) < magnitude;
}

static inline int mpiwrapper_narrow_int(MPIABI_Count v, int *out)
{
  if (!mpiwrapper_fits(v, sizeof(int))) return 0;
  *out = (int)v;
  return 1;
}

static inline int mpiwrapper_narrow_aint(MPIABI_Count v, MPI_Aint *out)
{
  /* Folds to an unconditional store wherever MPI_Aint is 64 bits, which is
   * everywhere this project builds except the i386 row -- and that row is the
   * reason this is a check and not a cast.
   */
  if (!mpiwrapper_fits(v, sizeof(MPI_Aint))) return 0;
  *out = (MPI_Aint)v;
  return 1;
}

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

/* The same idea for MPI_T's six handle classes, which are not among the eleven
 * and have their own, much smaller, predefined range: the ABI gives every
 * MPI_T null handle the value 0 and MPI_T_PVAR_ALL_HANDLES the value 1, and
 * nothing else. A dynamic implementation handle whose bits landed there would
 * come back out as a sentinel, so the toabi direction rejects it -- one
 * compare, on a path that allocates a tool handle.
 */
#define MPIWRAPPER_TOOL_PREDEF_LAST 0x1u

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
#ifndef MPIWRAPPER_SERIAL_SLOTS
#  define MPIWRAPPER_SERIAL_SLOTS 4096
#endif
/* Larger than the keyval table it is otherwise a copy of, because this one
 * absorbs the implementation's own error codes as well as the application's:
 * MPICH answers essentially every error with an instance-specific code, and
 * interning them is what keeps their class reachable (errorcodes.c). Here
 * rather than in that file because test/mpiwrapper_selftest.c fills it.
 */
#ifndef MPIWRAPPER_ERRORCODE_SLOTS
#  define MPIWRAPPER_ERRORCODE_SLOTS 4096
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

/* Sets the same flag. Exists because the MPI_T handle conversions are
 * generated into constants.c -- their guards have to appear in a generated
 * source for dev/probe_impl.py to ask about them -- while the flag itself is
 * the thread-local in handles.c and stays owned there.
 */
void mpiwrapper_set_handle_error(void);

/* ------------------------------------------------- MPI_T's handle classes */

/* Not the eleven above, and deliberately not their machinery: at most two
 * predefined values apiece, so each direction is one or two compares rather
 * than a perfect-hash lookup. Generated into constants.c from the ABI header's
 * own sentinels (NOTES.md #5.3), and guarded on the *type*, since an
 * implementation without MPI_T -- or with MPI_T but without MPI-4.0's events
 * -- does not declare it.
 *
 * The sentinels really do differ: the ABI fixes MPI_T_PVAR_ALL_HANDLES at 1,
 * Open MPI 5.0.6 spells it -1, and MPICH 4.3.1 makes it an `extern ... * const`
 * whose value is not a constant expression, so it cannot be a case label
 * anywhere and both directions are run-time compares.
 */
#ifdef MPIWRAPPER_HAVE_MPI_T_enum
MPI_T_enum            mpiwrapper_t_enum_fromabi(MPIABI_T_enum abi);
MPIABI_T_enum         mpiwrapper_t_enum_toabi(MPI_T_enum h);
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_cvar_handle
MPI_T_cvar_handle     mpiwrapper_t_cvar_handle_fromabi(MPIABI_T_cvar_handle abi);
MPIABI_T_cvar_handle  mpiwrapper_t_cvar_handle_toabi(MPI_T_cvar_handle h);
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_pvar_handle
MPI_T_pvar_handle     mpiwrapper_t_pvar_handle_fromabi(MPIABI_T_pvar_handle abi);
MPIABI_T_pvar_handle  mpiwrapper_t_pvar_handle_toabi(MPI_T_pvar_handle h);
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_pvar_session
MPI_T_pvar_session    mpiwrapper_t_pvar_session_fromabi(MPIABI_T_pvar_session abi);
MPIABI_T_pvar_session mpiwrapper_t_pvar_session_toabi(MPI_T_pvar_session h);
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_event_registration
MPI_T_event_registration
mpiwrapper_t_event_registration_fromabi(MPIABI_T_event_registration abi);
MPIABI_T_event_registration
mpiwrapper_t_event_registration_toabi(MPI_T_event_registration h);
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_event_instance
MPI_T_event_instance
mpiwrapper_t_event_instance_fromabi(MPIABI_T_event_instance abi);
MPIABI_T_event_instance
mpiwrapper_t_event_instance_toabi(MPI_T_event_instance h);
#endif

/* ------------------------------------------------------ MPI_T's obj_handle */

/* MPI_T's three handle allocators take `void *obj_handle`, which MPI-5.0
 * 15.3.6 defines as the address of a local variable holding an MPI handle --
 * of a class that is nowhere in the argument list. What decides it is the
 * `bind` a prior get_info reported, so the wrapper asks for that first
 * (toolobj.c below) and then converts through this union, whose address is
 * what the implementation is given in place of the caller's.
 *
 * The storage is a local of the generated body and lives exactly as long as
 * the call: MPI-5.0 says the object is bound, not the pointer, and neither
 * implementation retains the address.
 */
union mpiwrapper_tool_obj {
  MPI_Comm       comm;
  MPI_Datatype   datatype;
  MPI_Errhandler errhandler;
  MPI_File       file;
  MPI_Group      group;
  MPI_Info       info;
  MPI_Message    message;
  MPI_Op         op;
  MPI_Request    request;
#ifdef MPIWRAPPER_HAVE_MPI_SESSION_NULL
  MPI_Session    session;
#endif
  MPI_Win        win;
};

/* Generated (constants.c): the switch from a bind value onto a handle class.
 * Generated rather than hand-written so that the MPIWRAPPER_HAVE_ guards its
 * cases need are probed like every other -- dev/probe_impl.py reads the
 * generated sources, not these. Returns NULL for a null obj_handle and for
 * MPI_T_BIND_NO_OBJECT, which the standard makes the same thing.
 */
void *mpiwrapper_tool_obj_fromabi(int bind, void *abi_obj,
                                  union mpiwrapper_tool_obj *out);

/* Hand-written (toolobj.c): what class of object a control variable,
 * performance variable or event type must be bound to. One per routine,
 * because the query differs per routine and is not derivable from the
 * parameter. Each returns an implementation error code, like the extents
 * above and for the same reason.
 */
int mpiwrapper_cvar_bind(int cvar_index, int *bind);
int mpiwrapper_pvar_bind(int pvar_index, int *bind);
int mpiwrapper_event_bind(int event_index, int *bind);

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

/* The two families no parameter has (S7). MPI_WIN_CREATE_FLAVOR and
 * MPI_WIN_MODEL reach an application only as the *value* of a window
 * attribute, which is a void * in every signature that carries one, so
 * apis.json marks nothing and only src/mpiwrapper/hw_attr.c calls these. The
 * fromabi halves exist because the generator emits families in pairs; nothing
 * hands one of these back to an implementation, since the attribute is
 * read-only.
 */
/* The one sentinel that is an integer (S7): MPI_DISPLACEMENT_CURRENT, which
 * MPI_File_set_view's `disp` may be and no other OFFSET-kind parameter may.
 * In the ABI it is (MPI_Offset)-1 and in ROMIO -54278278. Only the in
 * direction exists -- MPI_File_get_view answers with a real displacement.
 */
MPI_Offset mpiwrapper_displacement_fromabi(MPIABI_Offset abi_disp);

int mpiwrapper_winflavor_fromabi(int abi_winflavor);
int mpiwrapper_winflavor_toabi(int winflavor);
int mpiwrapper_winmodel_fromabi(int abi_winmodel);
int mpiwrapper_winmodel_toabi(int winmodel);

/* MPI_T's six enumerated families. Every case is guarded, because the whole
 * tool interface is optional; a family whose members are all absent still
 * compiles, as a function whose switch is only its default arm.
 */
int mpiwrapper_tbind_fromabi(int abi_tbind);
int mpiwrapper_tbind_toabi(int tbind);
int mpiwrapper_tcbsafety_fromabi(int abi_tcbsafety);
int mpiwrapper_tcbsafety_toabi(int tcbsafety);
int mpiwrapper_tpvarclass_fromabi(int abi_tpvarclass);
int mpiwrapper_tpvarclass_toabi(int tpvarclass);
int mpiwrapper_tscope_fromabi(int abi_tscope);
int mpiwrapper_tscope_toabi(int tscope);
int mpiwrapper_tsourceorder_fromabi(int abi_tsourceorder);
int mpiwrapper_tsourceorder_toabi(int tsourceorder);
int mpiwrapper_tverbosity_fromabi(int abi_tverbosity);
int mpiwrapper_tverbosity_toabi(int tverbosity);

/* --------------------------------------------------------------- keyvals */

/* The one mapped integer family with a *dynamic* half. The thirteen
 * predefined attribute keys convert through a generated switch like any other
 * family; a keyval the implementation handed out at run time cannot, because
 * it is an int with no structure and it can land anywhere -- including on top
 * of the ABI's predefined 501-507 and 601-605, which Open MPI's small
 * sequential keyvals could in principle reach (NOTES.md #5.6).
 *
 * So the switch's default arm asks the registry below instead of passing the
 * value through. Every dynamic keyval an application can hold came out of one
 * of the three MPI_*_create_keyval calls, which is where mpiwrapper_keyval_add
 * is called from (S4b, hw_callbacks.c) -- MPI_Keyval_create is the fourth
 * spelling and libmpi_abi forwards it to the first. So a value neither
 * predefined nor registered is one
 * this library never issued, and both directions answer MPI_KEYVAL_INVALID
 * rather than invent a mapping.
 */
int mpiwrapper_keyval_fromabi(int abi_keyval);
int mpiwrapper_keyval_toabi(int keyval);
int mpiwrapper_keyval_dynamic_fromabi(int abi_keyval);
int mpiwrapper_keyval_dynamic_toabi(int keyval);

/* Registers a keyval the implementation just created and returns the ABI-side
 * value to hand back to the caller, or 0 if the table is full -- which the
 * caller turns into MPIABI_ERR_INTERN, as every fixed-capacity table here
 * does. The ABI-side values are drawn from a high range that no predefined key
 * can reach, so the two halves of the family cannot collide by construction
 * rather than by luck.
 */
int mpiwrapper_keyval_add(int keyval, int *abi_keyval);

/* ----------------------------------------------- dynamic error codes ---- */

/* The error-code family's dynamic half, and the exact shape of the keyval one
 * above (S4b, NOTES.md #5.6). MPI_Add_error_class and MPI_Add_error_code hand
 * out values above the implementation's own MPI_ERR_LASTCODE -- 0x3fffffff on
 * MPICH -- and the ABI's is 16383, so passing one through would hand the
 * application a number its header says cannot be an error code.
 *
 * So the generated switches' default arms ask here instead of passing the
 * value on, and the registry answers MPIABI_ERR_OTHER / MPI_ERR_OTHER for a
 * code it never issued: that is a legal class for an error this ABI cannot
 * name, and it is what the switch answered before the registry existed.
 */
int mpiwrapper_errorcode_add(int ierror, int *abi_ierror);
int mpiwrapper_errorcode_dynamic_toabi(int ierror);
int mpiwrapper_errorcode_dynamic_fromabi(int abi_ierror);

/* What MPI_LASTUSEDCODE answers (S7): the largest error code this library can
 * hand an application, which is this table's business rather than the
 * implementation's, since every code the application sees has been through
 * the conversion above. src/mpiwrapper/hw_attr.c is the only caller.
 */
int mpiwrapper_errorcode_lastused(void);

/* ------------------------------------------------- the attached buffer ---- */

/* MPI_BUFFER_AUTOMATIC where the implementation does not have it (S4b). MPI
 * lets an application attach *no* buffer and have the library provide one of
 * "sufficient size" itself; an implementation predating MPI-4.1 has no such
 * mode, so the wrapper attaches a buffer of its own and remembers that the
 * block is ours rather than the caller's -- which is the whole of what the
 * detach side needs to know, since it must then answer MPI_BUFFER_AUTOMATIC
 * rather than an address the caller never gave us.
 *
 * One record per buffering scope: the process-wide buffer of
 * MPI_Buffer_attach, or a communicator's or session's. The key is the
 * implementation handle's bits, and MPIWRAPPER_AUTOBUF_PROCESS is the
 * process-wide scope, which no handle can collide with.
 *
 * The emulation is an approximation and is documented as one: "sufficient
 * size" becomes MPIWRAPPER_AUTOBUF_BYTES, so a program that would have run
 * against a real automatic buffer can still exhaust ours and see
 * MPI_ERR_BUFFER. Nothing better is available without reimplementing buffered
 * mode.
 */
#define MPIWRAPPER_AUTOBUF_PROCESS UINT64_MAX

/* Allocates the block, records it against the scope, and reports its size.
 * NULL when the table is full or the allocation failed, which the caller
 * turns into MPIABI_ERR_INTERN.
 */
void *mpiwrapper_autobuf_claim(uint64_t scope, size_t *bytes);

/* Frees the block recorded against the scope and clears the record. Returns 1
 * if there was one -- which is how the detach side learns that the address the
 * implementation just handed back is ours and not the application's -- and 0
 * otherwise. Also the undo path when the attach itself fails.
 */
int mpiwrapper_autobuf_release(uint64_t scope);

/* ------------------------------------------------- handle serialization ---- */

/* MPI-5.0 20.4.5's MPI_<class>_toint / _fromint, which are the ABI's own
 * replacement for the c2f/f2c pair and, unlike it, are part of the ABI. Two
 * requirements, and they are why this cannot be a cast:
 *
 *  - "For all predefined handles, the integer value must be the same as the
 *    values listed in Section A" -- the ABI's own predefined values, i.e. the
 *    0x20..0x2eb this file already knows.
 *  - "For user-defined handles, the implementation must return the same
 *    integer for every call with the same handle, which does not conflict with
 *    the reserved range for predefined handles."
 *
 * A dynamic ABI handle is the implementation's own handle bits (NOTES.md
 * #5.1), and on Open MPI those are an object address: 64 bits, which does not
 * fit in an int and cannot be recovered from a truncation. So dynamic handles
 * are *interned* -- the ABI-side integer is an index into an append-only table
 * drawn from a base far above the predefined range, exactly as keyvals.c draws
 * the dynamic half of its family (NOTES.md #5.6). Predefined handles are not
 * interned, because the first requirement fixes their value.
 *
 * These take and return raw bits rather than a handle type, so one table
 * serves all eleven classes. That is not a shortcut: the integer identifies
 * the *bits*, and the class comes from which _fromint the caller reached for,
 * so an int handed to the wrong class's _fromint is the caller's error and not
 * an ambiguity here.
 *
 * Nothing is ever removed, for the same reason as everywhere else in this
 * library: MPI-5.0 20.4.5 makes an integer whose handle has been freed
 * erroneous to use, but a *re*-created object may reuse the implementation's
 * handle value, and the entry then still describes it correctly.
 */

/* 0 when the table is full -- never a valid serialized handle, since the
 * predefined range starts at 0x20, so the matching _fromint fails and the
 * caller gets the class's null handle rather than a fabricated one.
 */
int mpiwrapper_handle_toint(uint64_t abi_bits);

/* 0 when the integer is one this library never issued. */
int mpiwrapper_handle_fromint(int value, uint64_t *abi_bits);

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

/* The graph-topology weight arrays. MPI_UNWEIGHTED and MPI_WEIGHTS_EMPTY are
 * (int *)10 and (int *)11 in the ABI and are ordinary extern objects in both
 * implementations, so the pointer needs translating even though the weights
 * themselves are plain ints that pass straight through. Two roles again,
 * because MPI_Dist_graph_neighbors takes the same sentinels at an *out*
 * parameter, where the array must not be const.
 */
const int *mpiwrapper_weights_fromabi(const int *abi_weights);
int       *mpiwrapper_weights_out_fromabi(int *abi_weights);

/* ------------------------------------------------------------------ status */

void mpiwrapper_status_toabi(const MPI_Status *st, MPIABI_Status *abi);
/* The same, leaving the ABI status's MPI_ERROR alone: MPI-5.0 3.2.5 says the
 * error field is set only by the multiple-completion calls, so a single OUT
 * status must come back with the caller's value intact (S7).
 */
void mpiwrapper_status_toabi_keep_error(const MPI_Status *st,
                                        MPIABI_Status *abi);
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
/* Why attach reports *how* it failed: the two ways are not the same problem.
 * A full table is the capacity limit every fixed table here has, answered with
 * MPIABI_ERR_INTERN naming a build-time constant. A duplicate key is a
 * conformance bug -- the call was legal and the implementation merely shared a
 * request -- and must not fail the call (NOTES.md #13.2).
 */
enum mpiwrapper_staged_fate {
  MPIWRAPPER_STAGED_STORED,
  MPIWRAPPER_STAGED_DUPLICATE,
  MPIWRAPPER_STAGED_FULL
};

enum mpiwrapper_staged_fate mpiwrapper_staged_attach(MPI_Request request,
                                                     void       *block);
void mpiwrapper_staged_release(MPI_Request request);
int  mpiwrapper_staged_any(void);

/* Whether a staged entry point's request can already be complete when it comes
 * back. A nonblocking form's can, and both implementations then hand back a
 * *shared built-in* request rather than allocating one; a persistent form's
 * cannot, because MPI-5.0 3.9 makes it inactive from creation and an inactive
 * request answers "complete" to every test there is. That difference is what
 * mpiwrapper_staged_keep needs and what it must never guess (NOTES.md #13.2).
 *
 * The generator derives it from the *signature* -- the persistent inits are the
 * ones that take an MPI_Info -- and cross-checks the name, so a ninth entry
 * point cannot be misclassified silently (NOTES.md #5.7's rule about spelling).
 */
enum mpiwrapper_staged_kind {
  MPIWRAPPER_STAGED_NONBLOCKING,
  MPIWRAPPER_STAGED_PERSISTENT
};

/* What a staged entry point does with its block once the call has returned.
 * The whole policy is here rather than in the emitted text because it is a
 * policy: see staging.c. Returns 0 only for the one outcome that is still the
 * caller's error, a full table.
 */
int mpiwrapper_staged_keep(MPI_Request request, void *block, size_t nstaged,
                           enum mpiwrapper_staged_kind kind);

/* How many blocks have been leaked because the table could not take them --
 * either fate above. Zero in every run that has not hit one, and the oracle
 * mpiwrapper_selftest uses for the capacity behaviour of a table whose error
 * channel #13.2's (c) deliberately narrowed.
 */
unsigned long mpiwrapper_staged_leaked(void);

/* ----------------------------------------------------------------- extents */

/* How long an array parameter is, where `apis.json` answers `*` -- i.e. where
 * the length is a property of an object rather than of the argument list, so
 * the only place to ask is the implementation (extents.c). Each returns an
 * implementation error code, which the generated body maps like any other; a
 * body runs its extent queries before the call it wraps, and the two errors
 * coincide.
 */
int mpiwrapper_comm_extent(MPI_Comm comm, int *n);
int mpiwrapper_neighbor_extents(MPI_Comm comm, int *indegree, int *outdegree);
int mpiwrapper_dist_graph_extents(MPI_Comm comm, int *indegree, int *outdegree);
int mpiwrapper_graph_nedges(MPI_Comm comm, int *nedges);
int mpiwrapper_graph_nneighbors(MPI_Comm comm, int rank, int *nneighbors);
int mpiwrapper_root_extent(MPI_Comm comm, int root, int *n);
int mpiwrapper_type_ndatatypes(MPI_Datatype datatype, int *ndatatypes);
int mpiwrapper_type_ndatatypes_c(MPI_Datatype datatype, MPI_Count *ndatatypes);

/* Not an implementation error code: a plain 0 for a negative degree or an
 * overflowing sum, which the caller turns into MPIABI_ERR_ARG before anything
 * is allocated from it.
 */
int mpiwrapper_sum_degrees(const int *degrees, int n, int *total);

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

/* S4b completes the pool families of #6.1: the large-count user reduction,
 * which has a `MPI_Count *len` where the other has an `int *`, and the three
 * error-handler classes beside the communicator's. Each is guarded on the
 * registrar that fills it, because a pool for a class the implementation does
 * not have would not compile -- MPI_File_errhandler_function is a typedef, not
 * a name a stub can invent.
 */
#ifdef MPIWRAPPER_HAVE_MPI_Op_create_c
int                  mpiwrapper_op_c_slot_alloc(MPIABI_User_function_c *fn);
MPI_User_function_c *mpiwrapper_op_c_tramp(int slot);
void                 mpiwrapper_op_c_slot_release(int slot);
#endif

#ifdef MPIWRAPPER_HAVE_MPI_File_create_errhandler
int mpiwrapper_file_errh_slot_alloc(MPIABI_File_errhandler_function *fn);
MPI_File_errhandler_function *mpiwrapper_file_errh_tramp(int slot);
void                          mpiwrapper_file_errh_slot_release(int slot);
#endif

#ifdef MPIWRAPPER_HAVE_MPI_Win_create_errhandler
int mpiwrapper_win_errh_slot_alloc(MPIABI_Win_errhandler_function *fn);
MPI_Win_errhandler_function *mpiwrapper_win_errh_tramp(int slot);
void                         mpiwrapper_win_errh_slot_release(int slot);
#endif

#ifdef MPIWRAPPER_HAVE_MPI_Session_create_errhandler
int mpiwrapper_session_errh_slot_alloc(MPIABI_Session_errhandler_function *fn);
MPI_Session_errhandler_function *mpiwrapper_session_errh_tramp(int slot);
void                             mpiwrapper_session_errh_slot_release(int slot);
#endif

/* ------------------------------------------------ callbacks: extra state */

/* #6.1's other mechanism: a family whose registrar takes an extra-state
 * argument needs no pool, because the {user_fn, user_extra} pair can travel
 * through the implementation as that argument (extrastate.c). Each builder
 * heap-allocates the pair, answers the implementation-side function pointers
 * to register, and answers the pointer to hand the implementation as its
 * extra_state. 0 means the allocation failed and nothing was registered.
 *
 * `state` is the pair for a family whose functions are all ours, and the
 * caller's own extra_state where none of them is -- the predefined attribute
 * functions of #6.1's sentinel row, where the implementation's own
 * MPI_COMM_DUP_FN does the work and there is nothing of ours to smuggle.
 */
int mpiwrapper_comm_attr_fns(MPIABI_Comm_copy_attr_function   *abi_copy_fn,
                             MPIABI_Comm_delete_attr_function *abi_delete_fn,
                             void                             *abi_extra_state,
                             MPI_Comm_copy_attr_function     **copy_fn,
                             MPI_Comm_delete_attr_function   **delete_fn,
                             void                            **state);
int mpiwrapper_type_attr_fns(MPIABI_Type_copy_attr_function   *abi_copy_fn,
                             MPIABI_Type_delete_attr_function *abi_delete_fn,
                             void                             *abi_extra_state,
                             MPI_Type_copy_attr_function     **copy_fn,
                             MPI_Type_delete_attr_function   **delete_fn,
                             void                            **state);
int mpiwrapper_win_attr_fns(MPIABI_Win_copy_attr_function   *abi_copy_fn,
                            MPIABI_Win_delete_attr_function *abi_delete_fn,
                            void                            *abi_extra_state,
                            MPI_Win_copy_attr_function     **copy_fn,
                            MPI_Win_delete_attr_function   **delete_fn,
                            void                           **state);

/* Generalized requests are the one family with an observable reclamation
 * point: MPI deallocates the object after our free trampoline returns, so the
 * pair is freed there (#6.2). `discard` is the other end -- the path where
 * MPI_Grequest_start itself failed and no callback will ever run.
 */
#ifdef MPIWRAPPER_HAVE_MPI_Grequest_start
int mpiwrapper_grequest_fns(MPIABI_Grequest_query_function  *abi_query_fn,
                            MPIABI_Grequest_free_function   *abi_free_fn,
                            MPIABI_Grequest_cancel_function *abi_cancel_fn,
                            void                            *abi_extra_state,
                            MPI_Grequest_query_function     **query_fn,
                            MPI_Grequest_free_function      **free_fn,
                            MPI_Grequest_cancel_function    **cancel_fn,
                            void                            **state);
void mpiwrapper_grequest_discard(void *state);
#endif

/* Datareps are the opposite end of the same question: MPI has no
 * deregistration call at all, so the pair is process-lifetime by construction
 * (#6.2). `discard` exists only for a failed registration.
 */
#ifdef MPIWRAPPER_HAVE_MPI_Register_datarep
int mpiwrapper_datarep_fns(MPIABI_Datarep_conversion_function *abi_read_fn,
                           MPIABI_Datarep_conversion_function *abi_write_fn,
                           MPIABI_Datarep_extent_function     *abi_extent_fn,
                           void                               *abi_extra_state,
                           MPI_Datarep_conversion_function   **read_fn,
                           MPI_Datarep_conversion_function   **write_fn,
                           MPI_Datarep_extent_function       **extent_fn,
                           void                              **state);
#endif
#ifdef MPIWRAPPER_HAVE_MPI_Register_datarep_c
int mpiwrapper_datarep_c_fns(MPIABI_Datarep_conversion_function_c *abi_read_fn,
                             MPIABI_Datarep_conversion_function_c *abi_write_fn,
                             MPIABI_Datarep_extent_function *abi_extent_fn,
                             void                           *abi_extra_state,
                             MPI_Datarep_conversion_function_c **read_fn,
                             MPI_Datarep_conversion_function_c **write_fn,
                             MPI_Datarep_extent_function      **extent_fn,
                             void                             **state);
#endif
#if defined(MPIWRAPPER_HAVE_MPI_Register_datarep) ||                           \
    defined(MPIWRAPPER_HAVE_MPI_Register_datarep_c)
void mpiwrapper_datarep_discard(void *state);
#endif

/* ------------------------------------------------------- MPI_T's events */

/* #6.1's third mechanism, and the reason it is neither of the other two:
 * MPI_T_event_set_dropped_handler has no user_data parameter of its own, so
 * nothing can be smuggled through the registration -- but every one of the
 * three callbacks receives the event-registration handle, so one map keyed on
 * the implementation's handle serves all three (toolevents.c).
 *
 * Each setter answers 0 when the map is full, which the caller turns into
 * MPIABI_ERR_INTERN. The trampolines are declared through the
 * implementation's own typedefs, so their signatures are checked against its
 * header rather than written out twice.
 */
#ifdef MPIWRAPPER_HAVE_MPI_T_event_registration
int mpiwrapper_t_event_set_cb(MPI_T_event_registration    reg,
                              int                         abi_cb_safety,
                              MPIABI_T_event_cb_function *abi_fn,
                              void                       *user_data);
int mpiwrapper_t_event_set_dropped(
    MPI_T_event_registration reg, MPIABI_T_event_dropped_cb_function *abi_fn);
int mpiwrapper_t_event_set_free(MPI_T_event_registration            reg,
                                MPIABI_T_event_free_cb_function    *abi_fn,
                                void                               *user_data);

/* `MPI_T_event_cb_function f;` declares a function *of that type*, which is
 * how a trampoline takes its signature from the implementation's own typedef
 * instead of repeating it.
 */
#  ifdef MPIWRAPPER_HAVE_MPI_T_event_register_callback
MPI_T_event_cb_function mpiwrapper_t_event_cb_tramp;
#  endif
#  ifdef MPIWRAPPER_HAVE_MPI_T_event_set_dropped_handler
MPI_T_event_dropped_cb_function mpiwrapper_t_event_dropped_tramp;
#  endif
#  ifdef MPIWRAPPER_HAVE_MPI_T_event_handle_free
MPI_T_event_free_cb_function mpiwrapper_t_event_free_tramp;
#  endif
#endif

#endif /* MPIWRAPPER_INTERNAL_H */
