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

#include <string.h>

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

/* -------------------------------------------------------- MPI_Get_count ---- */

/* One of the ten functions that need an implementation status built *from* an
 * ABI status (NOTES.md #5.2). The blob in MPI_internal is the implementation's
 * own private bytes, put there by whichever call filled the status, so this is
 * a restore rather than a synthesis -- there is no validity marker and no
 * fallback, deliberately.
 */
#define BODY_MPI_Get_count(TARGET)                                             \
  {                                                                            \
    MPI_Status status;                                                         \
    mpiwrapper_status_fromabi(abi_status, &status);                            \
    const MPI_Datatype datatype = mpiwrapper_datatype_fromabi(abi_datatype);   \
                                                                               \
    int       count  = 0;                                                      \
    const int ierror = TARGET(&status, datatype, &count);                      \
                                                                               \
    /* The count is MPI_UNDEFINED when the datatype does not divide the         \
     * message evenly, which is a mapped integer rather than a plain one --     \
     * identical in all three today, and written out because that is a          \
     * property of these implementations and not of the ABI.                    \
     */                                                                        \
    if (ierror == MPI_SUCCESS)                                                 \
      *abi_count = (count == MPI_UNDEFINED) ? MPIABI_UNDEFINED : count;        \
    return mpiwrapper_errorcode_toabi(ierror);                                 \
  }

int mpiwrapper_w_MPI_Get_count(const MPIABI_Status *abi_status,
                           MPIABI_Datatype abi_datatype, int *abi_count)
    BODY_MPI_Get_count(MPI_Get_count)
int mpiwrapper_w_PMPI_Get_count(const MPIABI_Status *abi_status,
                            MPIABI_Datatype abi_datatype, int *abi_count)
    BODY_MPI_Get_count(PMPI_Get_count)

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

/* ------------------------------------------------------ MPI_Error_string ---- */

/* One of the ten output-string buffers with no length argument (NOTES.md #5.8).
 * The caller sized abi_string with the *ABI's* MPI_MAX_ERROR_STRING and the
 * implementation writes up to its own, which is 512 against 512 for MPICH and
 * 512 against 256 for Open MPI -- so today the copy could go straight into the
 * caller's array. Staging is emitted unconditionally anyway, precisely so this
 * path is exercised on every run instead of being dead code that the first
 * implementation with a larger limit gets to try out in production.
 *
 * An error string is prose, so truncation is the right answer here; an
 * identifier that will be handed back to MPI is not, and MPI_Open_port,
 * MPI_Lookup_name, MPI_Info_get_nthkey and MPI_File_get_view's datarep return
 * MPIABI_ERR_INTERN instead. That choice is per-parameter and lives in the
 * generator's named (routine, parameter) table.
 */
#define BODY_MPI_Error_string(TARGET)                                          \
  {                                                                            \
    const int errorcode = mpiwrapper_errorcode_fromabi(abi_errorcode);         \
                                                                               \
    char buf[MPI_MAX_ERROR_STRING]; /* the implementation's maximum */         \
    int  resultlen = 0;                                                        \
                                                                               \
    const int ierror = TARGET(errorcode, buf, &resultlen);                     \
    if (ierror == MPI_SUCCESS) {                                               \
      int n = resultlen;                                                       \
      if (n < 0) n = 0;                                                        \
      if (n > MPIABI_MAX_ERROR_STRING - 1) n = MPIABI_MAX_ERROR_STRING - 1;    \
      memcpy(abi_string, buf, (size_t)n);                                      \
      abi_string[n]  = '\0';                                                   \
      *abi_resultlen = n; /* so abi_string[*abi_resultlen] == '\0' holds */    \
    }                                                                          \
    return mpiwrapper_errorcode_toabi(ierror);                                 \
  }

int mpiwrapper_w_MPI_Error_string(int abi_errorcode, char *abi_string,
                              int *abi_resultlen)
    BODY_MPI_Error_string(MPI_Error_string)
int mpiwrapper_w_PMPI_Error_string(int abi_errorcode, char *abi_string,
                               int *abi_resultlen)
    BODY_MPI_Error_string(PMPI_Error_string)

/* ------------------------------------------------ MPI_Comm_c2f / _f2c ---- */

/* The Fortran converters, and the reason mpif can run over any MPI. Note what
 * they do *not* return: an error code. So the handle-collision flag that every
 * other object-producing conversion turns into MPIABI_ERR_INTERN has nowhere to
 * go here, and is cleared rather than left set for the next call to blame. A
 * comm whose bits collide with the ABI's predefined range converts to
 * MPIABI_COMM_NULL, which the caller will find invalid soon enough.
 */
#define BODY_MPI_Comm_c2f(TARGET)                                              \
  {                                                                            \
    const MPI_Comm comm = mpiwrapper_comm_fromabi(abi_comm);                   \
    return (MPIABI_Fint)TARGET(comm);                                          \
  }

MPIABI_Fint mpiwrapper_w_MPI_Comm_c2f(MPIABI_Comm abi_comm)
    BODY_MPI_Comm_c2f(MPI_Comm_c2f)
MPIABI_Fint mpiwrapper_w_PMPI_Comm_c2f(MPIABI_Comm abi_comm)
    BODY_MPI_Comm_c2f(PMPI_Comm_c2f)

#define BODY_MPI_Comm_f2c(TARGET)                                              \
  {                                                                            \
    const MPI_Fint comm = abi_comm;                                            \
                                                                               \
    const MPIABI_Comm abi_result = mpiwrapper_comm_toabi(TARGET(comm));        \
    (void)mpiwrapper_take_handle_error();                                      \
    return abi_result;                                                         \
  }

MPIABI_Comm mpiwrapper_w_MPI_Comm_f2c(MPIABI_Fint abi_comm)
    BODY_MPI_Comm_f2c(MPI_Comm_f2c)
MPIABI_Comm mpiwrapper_w_PMPI_Comm_f2c(MPIABI_Fint abi_comm)
    BODY_MPI_Comm_f2c(PMPI_Comm_f2c)

/* Declared in handwritten.h, which the generated wrappers.c includes to fill
 * these sixteen slots. Not static, therefore -- but hidden by
 * -fvisibility=hidden and absent from the export list, so libmpiwrapper still
 * exports exactly one symbol, which oracle 2 checks with nm.
 */
