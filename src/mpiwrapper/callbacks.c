/* libmpiwrapper -- callback trampolines.
 *
 * Hand-written and permanent. Of the seven callback families, the ones with an
 * extra-state argument can carry a heap-allocated {user_fn, user_extra} pair;
 * the ones *without* need a pool of static trampolines, each knowing its own
 * index, because there is nothing to smuggle a user pointer through and the
 * user's function cannot be identified from inside the callback.
 *
 * S1 covers the two pool families that the prototype reaches: MPI_Op_create and
 * MPI_Comm_create_errhandler. The error-handler case is the non-obvious one --
 * MPI_Comm_errhandler_function and its three siblings have no extra-state
 * argument either, so MPI_Op_create is not the only exception (NOTES.md #6.1).
 * S4b adds the other four pools of that row -- MPI_Op_create_c and the file,
 * window and session error handlers -- and nothing else belongs here: the
 * families that *do* have an extra-state argument (keyvals, generalized
 * requests, datareps) need no pool and live in extrastate.c, and MPI_T's
 * events need a map rather than either and live in toolevents.c.
 *
 * Six pools, then, and their sizes are the two tunables of #6.2: 1024 op slots
 * per variant and 256 error-handler slots per class.
 *
 * Nothing here is ever reclaimed, and that is a decision rather than an
 * oversight. MPI-5.0 2.5.2: a deallocate call "marks the object for
 * deallocation... Any operation pending (at the time of the deallocate) ... will
 * complete normally; the object will be deallocated afterwards" -- so a pending
 * MPI_Ireduce can invoke an op trampoline after MPI_Op_free returns, and 9.4
 * says an error handler is deallocated only after every object associated with
 * it is. Precise reclamation would need a refcount driven by ~36 reduction
 * entry points plus MPI_Start; a bug in that is a use-after-free surfacing as a
 * wrong reduction result at scale, while not reclaiming merely caps creations
 * at the pool size and returns a clean error naming the macro (NOTES.md #6.2).
 */

#include "internal.h"

#include <stdatomic.h>

/* The pools are built by macro expansion over *hexadecimal* suffixes, which is
 * what makes the index in the body and the position in the array the same
 * number without a second list to keep in step. Decimal suffixes would be worse
 * than they look: `op_tramp_017` would have to pass `017`, and that is octal.
 */
#define MPIWRAPPER_HEX16(X, P)                                                 \
  X(P##0) X(P##1) X(P##2) X(P##3) X(P##4) X(P##5) X(P##6) X(P##7)              \
  X(P##8) X(P##9) X(P##a) X(P##b) X(P##c) X(P##d) X(P##e) X(P##f)
#define MPIWRAPPER_HEX256(X, P)                                                \
  MPIWRAPPER_HEX16(X, P##0) MPIWRAPPER_HEX16(X, P##1)                          \
  MPIWRAPPER_HEX16(X, P##2) MPIWRAPPER_HEX16(X, P##3)                          \
  MPIWRAPPER_HEX16(X, P##4) MPIWRAPPER_HEX16(X, P##5)                          \
  MPIWRAPPER_HEX16(X, P##6) MPIWRAPPER_HEX16(X, P##7)                          \
  MPIWRAPPER_HEX16(X, P##8) MPIWRAPPER_HEX16(X, P##9)                          \
  MPIWRAPPER_HEX16(X, P##a) MPIWRAPPER_HEX16(X, P##b)                          \
  MPIWRAPPER_HEX16(X, P##c) MPIWRAPPER_HEX16(X, P##d)                          \
  MPIWRAPPER_HEX16(X, P##e) MPIWRAPPER_HEX16(X, P##f)

/* 4 x 256 = 1024 op slots, indices 0x000..0x3ff, ~24 KB of text. */
#define MPIWRAPPER_OP_POOL(X)                                                  \
  MPIWRAPPER_HEX256(X, 0)                                                      \
  MPIWRAPPER_HEX256(X, 1) MPIWRAPPER_HEX256(X, 2) MPIWRAPPER_HEX256(X, 3)

/* 256 errhandler slots per class, indices 0x00..0xff. */
#define MPIWRAPPER_ERRHANDLER_POOL(X) MPIWRAPPER_HEX256(X, )

/* ------------------------------------------------------------ user ops ---- */

static _Atomic(MPIABI_User_function *) op_fn[MPIWRAPPER_OP_SLOTS];

static void op_dispatch(int slot, void *invec, void *inoutvec, int *len,
                        MPI_Datatype *datatype)
{
  MPIABI_User_function *fn =
      atomic_load_explicit(&op_fn[slot], memory_order_acquire);

  /* This is why a user-op trampoline needs the reverse handle map: the
   * implementation hands us *its* datatype and the user's function is written
   * against the ABI's.
   */
  MPIABI_Datatype abi_datatype = mpiwrapper_datatype_toabi(*datatype);

  fn(invec, inoutvec, len, &abi_datatype);
}

/* Declared with the implementation's own typedef shape, so the signature is
 * checked against its header rather than written out twice.
 */
#define MPIWRAPPER_DEFINE_OP_TRAMP(SUF)                                        \
  static void op_tramp_##SUF(void *a, void *b, int *n, MPI_Datatype *d)        \
  {                                                                            \
    op_dispatch(0x##SUF, a, b, n, d);                                          \
  }
#define MPIWRAPPER_REF_OP_TRAMP(SUF) op_tramp_##SUF,

MPIWRAPPER_OP_POOL(MPIWRAPPER_DEFINE_OP_TRAMP)

static MPI_User_function *const op_tramps[] = {
    MPIWRAPPER_OP_POOL(MPIWRAPPER_REF_OP_TRAMP)};

_Static_assert(sizeof op_tramps / sizeof *op_tramps == MPIWRAPPER_OP_SLOTS,
               "op trampoline pool size does not match MPIWRAPPER_OP_SLOTS");

/* The registration bodies live in handwritten.c, because each exists twice --
 * once calling MPI_Op_create and once PMPI_Op_create -- and this file must not
 * name either. What the bodies need from the pool is exactly this: take a slot,
 * find its trampoline, and give the slot back on the one path where that is
 * safe.
 */
int mpiwrapper_op_slot_alloc(MPIABI_User_function *fn)
{
  for (int i = 0; i < MPIWRAPPER_OP_SLOTS; ++i) {
    MPIABI_User_function *expected = NULL;
    if (atomic_compare_exchange_strong_explicit(&op_fn[i], &expected, fn,
                                                memory_order_acq_rel,
                                                memory_order_acquire))
      return i;
  }
  return -1; /* pool exhausted */
}

MPI_User_function *mpiwrapper_op_tramp(int slot) { return op_tramps[slot]; }

/* The *only* place a slot is released, and it is safe here and nowhere else:
 * the caller failed to create the op, so it was never handed to the application
 * and nothing can reference it. See the file header for why MPI_Op_free is not
 * such a place.
 */
void mpiwrapper_op_slot_release(int slot)
{
  atomic_store_explicit(&op_fn[slot], NULL, memory_order_release);
}

/* --------------------------------------------------- comm error handlers ---- */

static _Atomic(MPIABI_Comm_errhandler_function *)
    comm_errh_fn[MPIWRAPPER_ERRHANDLER_SLOTS];

static void comm_errh_dispatch(int slot, MPI_Comm *comm, int *error_code)
{
  MPIABI_Comm_errhandler_function *fn =
      atomic_load_explicit(&comm_errh_fn[slot], memory_order_acquire);

  MPIABI_Comm abi_comm       = mpiwrapper_comm_toabi(*comm);
  int         abi_error_code = mpiwrapper_errorcode_toabi(*error_code);

  /* A pool lookup allocates nothing, which matters here more than anywhere
   * else: an error-handler trampoline runs when the process is already in
   * trouble.
   */
  fn(&abi_comm, &abi_error_code);
}

/* The trampolines must be *variadic*, matching MPI_Comm_errhandler_function's
 * `...`. Nothing needs forwarding -- the extra arguments are
 * implementation-specific and the user's ABI-side function is variadic too --
 * but variadic and non-variadic calling conventions differ on arm64 macOS, so a
 * non-variadic declaration would be a silent ABI bug. Taking the shape from the
 * implementation's own typedef gets this right for free.
 */
#define MPIWRAPPER_DEFINE_COMM_ERRH_TRAMP(SUF)                                 \
  static void comm_errh_tramp_##SUF(MPI_Comm *c, int *e, ...)                  \
  {                                                                            \
    comm_errh_dispatch(0x##SUF, c, e);                                         \
  }
#define MPIWRAPPER_REF_COMM_ERRH_TRAMP(SUF) comm_errh_tramp_##SUF,

MPIWRAPPER_ERRHANDLER_POOL(MPIWRAPPER_DEFINE_COMM_ERRH_TRAMP)

static MPI_Comm_errhandler_function *const comm_errh_tramps[] = {
    MPIWRAPPER_ERRHANDLER_POOL(MPIWRAPPER_REF_COMM_ERRH_TRAMP)};

_Static_assert(sizeof comm_errh_tramps / sizeof *comm_errh_tramps ==
                   MPIWRAPPER_ERRHANDLER_SLOTS,
               "errhandler trampoline pool size does not match "
               "MPIWRAPPER_ERRHANDLER_SLOTS");

int mpiwrapper_comm_errh_slot_alloc(MPIABI_Comm_errhandler_function *fn)
{
  for (int i = 0; i < MPIWRAPPER_ERRHANDLER_SLOTS; ++i) {
    MPIABI_Comm_errhandler_function *expected = NULL;
    if (atomic_compare_exchange_strong_explicit(&comm_errh_fn[i], &expected, fn,
                                                memory_order_acq_rel,
                                                memory_order_acquire))
      return i;
  }
  return -1;
}

MPI_Comm_errhandler_function *mpiwrapper_comm_errh_tramp(int slot)
{
  return comm_errh_tramps[slot];
}

void mpiwrapper_comm_errh_slot_release(int slot)
{
  atomic_store_explicit(&comm_errh_fn[slot], NULL, memory_order_release);
}

/* ------------------------------------------------ large-count user ops ---- */

/* MPI_Op_create_c's callback differs from MPI_Op_create's in one parameter --
 * `MPI_Count *len` where the other has `int *` -- and that is enough to make
 * it a second pool rather than a second entry in the first: the trampoline's
 * *type* is what the implementation stores, so one pool cannot serve both.
 *
 * Guarded on the registrar, because MPI_User_function_c is a typedef an
 * MPI-3.1 implementation does not declare and a pool of a type that does not
 * exist is a compile error rather than decision 6's run-time report.
 */
#ifdef MPIWRAPPER_HAVE_MPI_Op_create_c

static _Atomic(MPIABI_User_function_c *) op_c_fn[MPIWRAPPER_OP_SLOTS];

static void op_c_dispatch(int slot, void *invec, void *inoutvec,
                          MPI_Count *len, MPI_Datatype *datatype)
{
  MPIABI_User_function_c *fn =
      atomic_load_explicit(&op_c_fn[slot], memory_order_acquire);

  MPIABI_Datatype abi_datatype = mpiwrapper_datatype_toabi(*datatype);
  MPIABI_Count    abi_len      = (MPIABI_Count)*len;

  fn(invec, inoutvec, &abi_len, &abi_datatype);
}

#  define MPIWRAPPER_DEFINE_OP_C_TRAMP(SUF)                                    \
    static void op_c_tramp_##SUF(void *a, void *b, MPI_Count *n,               \
                                 MPI_Datatype *d)                              \
    {                                                                          \
      op_c_dispatch(0x##SUF, a, b, n, d);                                      \
    }
#  define MPIWRAPPER_REF_OP_C_TRAMP(SUF) op_c_tramp_##SUF,

MPIWRAPPER_OP_POOL(MPIWRAPPER_DEFINE_OP_C_TRAMP)

static MPI_User_function_c *const op_c_tramps[] = {
    MPIWRAPPER_OP_POOL(MPIWRAPPER_REF_OP_C_TRAMP)};

_Static_assert(sizeof op_c_tramps / sizeof *op_c_tramps == MPIWRAPPER_OP_SLOTS,
               "large-count op trampoline pool size does not match "
               "MPIWRAPPER_OP_SLOTS");

int mpiwrapper_op_c_slot_alloc(MPIABI_User_function_c *fn)
{
  for (int i = 0; i < MPIWRAPPER_OP_SLOTS; ++i) {
    MPIABI_User_function_c *expected = NULL;
    if (atomic_compare_exchange_strong_explicit(&op_c_fn[i], &expected, fn,
                                                memory_order_acq_rel,
                                                memory_order_acquire))
      return i;
  }
  return -1;
}

MPI_User_function_c *mpiwrapper_op_c_tramp(int slot) { return op_c_tramps[slot]; }

void mpiwrapper_op_c_slot_release(int slot)
{
  atomic_store_explicit(&op_c_fn[slot], NULL, memory_order_release);
}

#endif /* MPIWRAPPER_HAVE_MPI_Op_create_c */

/* ------------------------------- large-count user ops, over a small MPI ---- */

/* The fallback's pool (NOTES.md #5.10): the caller registered an
 * MPI_User_function_c through MPI_Op_create_c, and the implementation has only
 * MPI_Op_create, whose callback takes `int *len`. So the trampoline has the
 * *small* shape -- that is what the implementation stores and will call -- and
 * widens the length on its way into the caller's function.
 *
 * **This adapter cannot fail, and that is worth stating because every other
 * part of #5.10 can.** The narrowing everywhere else is on a value the caller
 * supplies; here the value comes from the implementation, which produced it as
 * an `int`, so widening it to MPIABI_Count is exact. MPI_Op_create itself
 * takes no count, so the registration cannot overflow either. There is no
 * MPI_ERR_VALUE_TOO_LARGE arm anywhere in this pool.
 *
 * It is a third pool rather than a third entry in one of the others for the
 * reason the second one gives: the trampoline's type is what the
 * implementation stores. It costs another 1024 function bodies of text, and
 * only in a build that takes the fallback arm -- an implementation with
 * MPI_Op_create_c compiles none of this.
 */
#if !defined(MPIWRAPPER_HAVE_MPI_Op_create_c) &&                               \
    defined(MPIWRAPPER_HAVE_MPI_Op_create)

static _Atomic(MPIABI_User_function_c *) op_ca_fn[MPIWRAPPER_OP_SLOTS];

static void op_ca_dispatch(int slot, void *invec, void *inoutvec, int *len,
                           MPI_Datatype *datatype)
{
  MPIABI_User_function_c *fn =
      atomic_load_explicit(&op_ca_fn[slot], memory_order_acquire);

  MPIABI_Datatype abi_datatype = mpiwrapper_datatype_toabi(*datatype);
  MPIABI_Count    abi_len      = (MPIABI_Count)*len;

  fn(invec, inoutvec, &abi_len, &abi_datatype);
}

#  define MPIWRAPPER_DEFINE_OP_CA_TRAMP(SUF)                                   \
    static void op_ca_tramp_##SUF(void *a, void *b, int *n, MPI_Datatype *d)   \
    {                                                                          \
      op_ca_dispatch(0x##SUF, a, b, n, d);                                     \
    }
#  define MPIWRAPPER_REF_OP_CA_TRAMP(SUF) op_ca_tramp_##SUF,

MPIWRAPPER_OP_POOL(MPIWRAPPER_DEFINE_OP_CA_TRAMP)

static MPI_User_function *const op_ca_tramps[] = {
    MPIWRAPPER_OP_POOL(MPIWRAPPER_REF_OP_CA_TRAMP)};

_Static_assert(sizeof op_ca_tramps / sizeof *op_ca_tramps
                   == MPIWRAPPER_OP_SLOTS,
               "large-count op fallback trampoline pool size does not match "
               "MPIWRAPPER_OP_SLOTS");

int mpiwrapper_op_ca_slot_alloc(MPIABI_User_function_c *fn)
{
  for (int i = 0; i < MPIWRAPPER_OP_SLOTS; ++i) {
    MPIABI_User_function_c *expected = NULL;
    if (atomic_compare_exchange_strong_explicit(&op_ca_fn[i], &expected, fn,
                                                memory_order_acq_rel,
                                                memory_order_acquire))
      return i;
  }
  return -1;
}

MPI_User_function *mpiwrapper_op_ca_tramp(int slot)
{
  return op_ca_tramps[slot];
}

void mpiwrapper_op_ca_slot_release(int slot)
{
  atomic_store_explicit(&op_ca_fn[slot], NULL, memory_order_release);
}

#endif /* the large-count op fallback */

/* ------------------------------------ file, window and session handlers ---- */

/* The remaining three error-handler classes. Each is the communicator pool
 * above with one type changed, so they are one macro rather than three
 * transcriptions -- and the macro takes the *implementation's* typedef names,
 * which is what keeps the variadic declaration of #6.1 right on every
 * platform without anyone having to remember it three more times.
 *
 * The handle conversion inside the dispatch is the reason these are
 * trampolines at all: the implementation invokes the handler with its own
 * MPI_File, and the application's function is written against the ABI's.
 */
#define MPIWRAPPER_DEFINE_ERRH_POOL(TAG, ABI_FN_T, IMPL_FN_T, ABI_HANDLE_T,    \
                                    IMPL_HANDLE_T, TOABI)                      \
  static _Atomic(ABI_FN_T *) TAG##_errh_fn[MPIWRAPPER_ERRHANDLER_SLOTS];       \
                                                                               \
  static void TAG##_errh_dispatch(int slot, IMPL_HANDLE_T *handle,             \
                                  int *error_code)                             \
  {                                                                            \
    ABI_FN_T *fn =                                                             \
        atomic_load_explicit(&TAG##_errh_fn[slot], memory_order_acquire);      \
                                                                               \
    ABI_HANDLE_T abi_handle     = TOABI(*handle);                              \
    int          abi_error_code = mpiwrapper_errorcode_toabi(*error_code);     \
                                                                               \
    fn(&abi_handle, &abi_error_code);                                          \
  }                                                                            \
                                                                               \
  MPIWRAPPER_ERRHANDLER_POOL(MPIWRAPPER_DEFINE_ERRH_TRAMP_##TAG)               \
                                                                               \
  static IMPL_FN_T *const TAG##_errh_tramps[] = {                              \
      MPIWRAPPER_ERRHANDLER_POOL(MPIWRAPPER_REF_ERRH_TRAMP_##TAG)};            \
                                                                               \
  _Static_assert(sizeof TAG##_errh_tramps / sizeof *TAG##_errh_tramps          \
                     == MPIWRAPPER_ERRHANDLER_SLOTS,                           \
                 #TAG " errhandler pool size does not match "                  \
                      "MPIWRAPPER_ERRHANDLER_SLOTS");                          \
                                                                               \
  int mpiwrapper_##TAG##_errh_slot_alloc(ABI_FN_T *fn)                         \
  {                                                                            \
    for (int i = 0; i < MPIWRAPPER_ERRHANDLER_SLOTS; ++i) {                    \
      ABI_FN_T *expected = NULL;                                               \
      if (atomic_compare_exchange_strong_explicit(&TAG##_errh_fn[i],           \
                                                  &expected, fn,               \
                                                  memory_order_acq_rel,        \
                                                  memory_order_acquire))       \
        return i;                                                              \
    }                                                                          \
    return -1;                                                                 \
  }                                                                            \
                                                                               \
  IMPL_FN_T *mpiwrapper_##TAG##_errh_tramp(int slot)                           \
  {                                                                            \
    return TAG##_errh_tramps[slot];                                            \
  }                                                                            \
                                                                               \
  void mpiwrapper_##TAG##_errh_slot_release(int slot)                          \
  {                                                                            \
    atomic_store_explicit(&TAG##_errh_fn[slot], NULL, memory_order_release);   \
  }

#ifdef MPIWRAPPER_HAVE_MPI_File_create_errhandler
#  define MPIWRAPPER_DEFINE_ERRH_TRAMP_file(SUF)                               \
    static void file_errh_tramp_##SUF(MPI_File *h, int *e, ...)                \
    {                                                                          \
      file_errh_dispatch(0x##SUF, h, e);                                       \
    }
#  define MPIWRAPPER_REF_ERRH_TRAMP_file(SUF) file_errh_tramp_##SUF,
MPIWRAPPER_DEFINE_ERRH_POOL(file, MPIABI_File_errhandler_function,
                            MPI_File_errhandler_function, MPIABI_File, MPI_File,
                            mpiwrapper_file_toabi)
#endif

#ifdef MPIWRAPPER_HAVE_MPI_Win_create_errhandler
#  define MPIWRAPPER_DEFINE_ERRH_TRAMP_win(SUF)                                \
    static void win_errh_tramp_##SUF(MPI_Win *h, int *e, ...)                  \
    {                                                                          \
      win_errh_dispatch(0x##SUF, h, e);                                        \
    }
#  define MPIWRAPPER_REF_ERRH_TRAMP_win(SUF) win_errh_tramp_##SUF,
MPIWRAPPER_DEFINE_ERRH_POOL(win, MPIABI_Win_errhandler_function,
                            MPI_Win_errhandler_function, MPIABI_Win, MPI_Win,
                            mpiwrapper_win_toabi)
#endif

#ifdef MPIWRAPPER_HAVE_MPI_Session_create_errhandler
#  define MPIWRAPPER_DEFINE_ERRH_TRAMP_session(SUF)                            \
    static void session_errh_tramp_##SUF(MPI_Session *h, int *e, ...)          \
    {                                                                          \
      session_errh_dispatch(0x##SUF, h, e);                                    \
    }
#  define MPIWRAPPER_REF_ERRH_TRAMP_session(SUF) session_errh_tramp_##SUF,
MPIWRAPPER_DEFINE_ERRH_POOL(session, MPIABI_Session_errhandler_function,
                            MPI_Session_errhandler_function, MPIABI_Session,
                            MPI_Session, mpiwrapper_session_toabi)
#endif
