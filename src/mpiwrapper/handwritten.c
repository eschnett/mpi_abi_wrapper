/* libmpiwrapper -- the hand-written entry points among the S1 prototype set.
 *
 * These are the ones where per-function judgement is needed, so the generator
 * will never produce them: it lists them in its HAND_WRITTEN ledger and fails
 * if the ledger and the generated set do not together cover all 688
 * (NOTES.md #8). The S1 eight, and what each is here for:
 *
 *   MPI_Init, MPI_Finalize     lifecycle
 *   MPI_Get_count              a status argument in the *in* direction
 *   MPI_Op_create              trampoline pool without extra state
 *   MPI_Comm_create_errhandler trampoline pool, variadic trampoline
 *   MPI_Error_string           output string buffer with no length argument
 *   MPI_Comm_c2f, MPI_Comm_f2c Fortran converters
 *
 * Four of those eight belong to families S4a completed, and each family got a
 * file of its own -- hw_converters.c (the two MPI_Comm converters),
 * hw_status.c (MPI_Get_count), hw_strings.c (MPI_Error_string) -- so those
 * four bodies moved there rather than being left behind as the odd one out of
 * a set of ten. What stays here is what S4b will finish: lifecycle, and the
 * two callback registrars.
 *
 * Every body appears twice, once per name, from one macro. The MPI_ instance
 * calls the implementation's MPI_ name and the PMPI_ instance its PMPI_ name,
 * so that an application calling PMPI_Get_count to bypass profiling bypasses a
 * tool interposed *below* us as well (NOTES.md #2, decision 7).
 *
 * S1's MPI_Waitall and MPI_Ialltoallw were here too, as stand-ins for the two
 * array classes S2 could not yet generate. S3 generates both, with every other
 * member of their families, so the bodies are gone and the ledger no longer
 * names them.
 */

#include "internal.h"

/* ------------------------------------------------------------- MPI_Init ---- */

/* Hand-written for lifecycle reasons rather than argument ones: argc and argv
 * pass through untouched. S4 is where this grows -- initialization state that
 * MPI_Initialized and the MPI_T calls can be asked about before MPI_Init.
 */
#define BODY_MPI_Init(TARGET)                                                  \
  {                                                                            \
    int   *const argc = abi_argc;                                              \
    char ***const argv = abi_argv;                                             \
                                                                               \
    const int ierror = TARGET(argc, argv);                                     \
    return mpiwrapper_errorcode_toabi(ierror);                                 \
  }

int mpiwrapper_w_MPI_Init(int *abi_argc, char ***abi_argv) BODY_MPI_Init(MPI_Init)
int mpiwrapper_w_PMPI_Init(int *abi_argc, char ***abi_argv) BODY_MPI_Init(PMPI_Init)

/* --------------------------------------------------------- MPI_Finalize ---- */

#define BODY_MPI_Finalize(TARGET)                                              \
  {                                                                            \
    const int ierror = TARGET();                                               \
    return mpiwrapper_errorcode_toabi(ierror);                                 \
  }

int mpiwrapper_w_MPI_Finalize(void) BODY_MPI_Finalize(MPI_Finalize)
int mpiwrapper_w_PMPI_Finalize(void) BODY_MPI_Finalize(PMPI_Finalize)

/* -------------------------------------------------------- MPI_Op_create ---- */

/* MPI_User_function has no extra-state argument, so the user's function cannot
 * be identified from inside the callback and a pool of static trampolines is
 * the only mechanism left. callbacks.c owns the pool; this owns the two bodies.
 */
#define BODY_MPI_Op_create(TARGET)                                             \
  {                                                                            \
    const int slot = mpiwrapper_op_slot_alloc(abi_user_fn);                    \
    if (slot < 0) {                                                            \
      *abi_op = MPIABI_OP_NULL;                                                \
      return MPIABI_ERR_INTERN; /* MPIWRAPPER_OP_SLOTS ops in one process */   \
    }                                                                          \
                                                                               \
    const int commute = abi_commute;                                           \
    MPI_Op    op;                                                              \
    const int ierror = TARGET(mpiwrapper_op_tramp(slot), commute, &op);        \
    if (ierror != MPI_SUCCESS) {                                               \
      mpiwrapper_op_slot_release(slot);                                        \
      *abi_op = MPIABI_OP_NULL;                                                \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }                                                                          \
                                                                               \
    *abi_op = mpiwrapper_op_toabi(op);                                         \
    if (mpiwrapper_take_handle_error()) return MPIABI_ERR_INTERN;              \
    return MPIABI_SUCCESS;                                                     \
  }

int mpiwrapper_w_MPI_Op_create(MPIABI_User_function *abi_user_fn, int abi_commute,
                           MPIABI_Op *abi_op) BODY_MPI_Op_create(MPI_Op_create)
int mpiwrapper_w_PMPI_Op_create(MPIABI_User_function *abi_user_fn, int abi_commute,
                            MPIABI_Op *abi_op)
    BODY_MPI_Op_create(PMPI_Op_create)

/* ------------------------------------------- MPI_Comm_create_errhandler ---- */

#define BODY_MPI_Comm_create_errhandler(TARGET)                                \
  {                                                                            \
    const int slot = mpiwrapper_comm_errh_slot_alloc(abi_comm_errhandler_fn);  \
    if (slot < 0) {                                                            \
      *abi_errhandler = MPIABI_ERRHANDLER_NULL;                                \
      return MPIABI_ERR_INTERN; /* MPIWRAPPER_ERRHANDLER_SLOTS in one process */\
    }                                                                          \
                                                                               \
    MPI_Errhandler errhandler;                                                 \
    const int      ierror =                                                    \
        TARGET(mpiwrapper_comm_errh_tramp(slot), &errhandler);                 \
    if (ierror != MPI_SUCCESS) {                                               \
      mpiwrapper_comm_errh_slot_release(slot);                                 \
      *abi_errhandler = MPIABI_ERRHANDLER_NULL;                                \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }                                                                          \
                                                                               \
    *abi_errhandler = mpiwrapper_errhandler_toabi(errhandler);                 \
    if (mpiwrapper_take_handle_error()) return MPIABI_ERR_INTERN;              \
    return MPIABI_SUCCESS;                                                     \
  }

int
mpiwrapper_w_MPI_Comm_create_errhandler(MPIABI_Comm_errhandler_function *abi_comm_errhandler_fn,
                             MPIABI_Errhandler *abi_errhandler)
    BODY_MPI_Comm_create_errhandler(MPI_Comm_create_errhandler)
int
mpiwrapper_w_PMPI_Comm_create_errhandler(MPIABI_Comm_errhandler_function *abi_comm_errhandler_fn,
                              MPIABI_Errhandler *abi_errhandler)
    BODY_MPI_Comm_create_errhandler(PMPI_Comm_create_errhandler)

/* Declared in handwritten.h, which the generated wrappers.c includes to fill
 * its slots. Not static, therefore -- but hidden by
 * -fvisibility=hidden and absent from the export list, so libmpiwrapper still
 * exports exactly one symbol, which oracle 2 checks with nm.
 */
