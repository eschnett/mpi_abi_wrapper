/* libmpiwrapper -- the hand-written entry points among the S1 prototype set.
 *
 * These are the ones where per-function judgement is needed, so the generator
 * will never produce them: it lists them in its HAND_WRITTEN ledger and fails
 * if the ledger and the generated set do not together cover all 688
 * (NOTES.md #8). The S1 ten, and what each is here for:
 *
 *   MPI_Init, MPI_Finalize     lifecycle
 *   MPI_Get_count              a status argument in the *in* direction
 *   MPI_Op_create              trampoline pool without extra state
 *   MPI_Comm_create_errhandler trampoline pool, variadic trampoline
 *   MPI_Error_string           output string buffer with no length argument
 *   MPI_Comm_c2f, MPI_Comm_f2c Fortran converters
 *   MPI_Ialltoallw             staged temporaries outliving the call
 *   MPI_Waitall                an inout request array, released at completion
 *
 * Every body appears twice, once per name, from one macro. The MPI_ instance
 * calls the implementation's MPI_ name and the PMPI_ instance its PMPI_ name,
 * so that an application calling PMPI_Ialltoallw to bypass profiling bypasses a
 * tool interposed *below* us as well (NOTES.md #2, decision 7).
 *
 * Calls this library makes for its own purposes are a different matter and use
 * PMPI_ unconditionally -- PMPI_Comm_size below, to learn how long
 * MPI_Ialltoallw's arrays are. An internal call is not application traffic and
 * must not be counted as such by a tool.
 */

#include "internal.h"

#include <stdlib.h>
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

/* -------------------------------------------------------- MPI_Waitall ---- */

/* Not generated, and not because of an argument class the generator cannot
 * classify: MPI_Waitall's request array is inout, and its staged temporaries
 * are released *at completion* rather than at return (NOTES.md #5.7). S2
 * generates in-direction arrays only, so this S1 body stays here, named in the
 * ledger with that reason, until S3 generates the class and deletes it.
 */

/* Arrays are staged into temporaries and never converted in place. The request
 * array is writable and MPI_Waitall does write to it, so in-place would even be
 * tempting here -- but the uniform rule is worth more than the saved
 * allocation, and two of NOTES.md #5.7's four reasons still apply.
 *
 * One cleanup path, reached by goto. Two staged arrays and a write-back that
 * has to happen on the error path make the early-return version noticeably
 * harder to check by eye, and every generated array-bearing wrapper has this
 * shape.
 */
#define BODY_MPI_Waitall(TARGET)                                               \
  {                                                                            \
    const int count = abi_count;                                               \
    if (count < 0) return MPIABI_ERR_COUNT; /* before any allocation */        \
                                                                               \
    /* MPI_STATUSES_IGNORE is NULL in the ABI and (MPI_Status *)1 in MPICH, and \
     * the test must come before we allocate room for `count` statuses nobody   \
     * wants.                                                                   \
     */                                                                        \
    const int ignore = abi_statuses == MPIABI_STATUSES_IGNORE;                 \
                                                                               \
    MPI_Request  reqstack[MPIWRAPPER_STAGE_BYTES / sizeof(MPI_Request)];       \
    MPI_Status   ststack[MPIWRAPPER_STAGE_BYTES / sizeof(MPI_Status)];         \
    MPI_Request *requests   = NULL;                                            \
    MPI_Status  *statuses   = NULL;                                            \
    int          abi_ierror = MPIABI_ERR_INTERN; /* if we leave before the     \
                                                  * call */                    \
    int          ierror;                                                       \
                                                                               \
    requests = mpiwrapper_stage(reqstack, sizeof reqstack, (size_t)count,      \
                                sizeof *requests);                             \
    if (!requests) goto done;                                                  \
                                                                               \
    if (!ignore) {                                                             \
      statuses = mpiwrapper_stage(ststack, sizeof ststack, (size_t)count,      \
                                  sizeof *statuses);                           \
      if (!statuses) goto done;                                                \
      memset(statuses, 0, (size_t)count * sizeof *statuses);                   \
    }                                                                          \
                                                                               \
    for (int i = 0; i < count; ++i)                                            \
      requests[i] = mpiwrapper_request_fromabi(abi_requests[i]);               \
                                                                               \
    ierror = TARGET(count, requests, ignore ? MPI_STATUSES_IGNORE : statuses); \
                                                                               \
    /* Temporaries owned by a request die here, and only here: a request that   \
     * the implementation has set to MPI_REQUEST_NULL is complete and           \
     * deallocated, so nothing can still be reading the block. abi_requests     \
     * still holds the pre-call handles at this point, which is why the         \
     * write-back below comes after. Persistent requests are *not* nulled by a  \
     * completion, which is exactly the distinction that makes freeing at       \
     * completion wrong for them (NOTES.md #5.7); they arrive in S3.            \
     */                                                                        \
    if (mpiwrapper_staged_any())                                               \
      for (int i = 0; i < count; ++i)                                          \
        if (requests[i] == MPI_REQUEST_NULL) {                                 \
          const MPI_Request before =                                           \
              mpiwrapper_request_fromabi(abi_requests[i]);                     \
          if (before != MPI_REQUEST_NULL) mpiwrapper_staged_release(before);   \
        }                                                                      \
                                                                               \
    /* Write back unconditionally, *including* on error: MPI_ERR_IN_STATUS      \
     * means the per-request error codes in the status array are the payload,   \
     * and the request array has been partially updated either way. Returning   \
     * early here would be a silent data-loss bug.                              \
     */                                                                        \
    for (int i = 0; i < count; ++i)                                            \
      abi_requests[i] = mpiwrapper_request_toabi(requests[i]);                 \
    if (!ignore)                                                               \
      for (int i = 0; i < count; ++i)                                          \
        mpiwrapper_status_toabi(&statuses[i], &abi_statuses[i]);               \
                                                                               \
    abi_ierror = mpiwrapper_errorcode_toabi(ierror);                           \
    if (mpiwrapper_take_handle_error()) abi_ierror = MPIABI_ERR_INTERN;        \
                                                                               \
  done:                                                                        \
    /* Safe on the early paths: mpiwrapper_unstage ignores NULL and frees only  \
     * when the pointer is not the stack buffer.                                \
     */                                                                        \
    mpiwrapper_unstage(statuses, ststack);                                     \
    mpiwrapper_unstage(requests, reqstack);                                    \
    return abi_ierror;                                                         \
  }

int mpiwrapper_w_MPI_Waitall(int abi_count, MPIABI_Request abi_requests[],
                             MPIABI_Status *abi_statuses)
    BODY_MPI_Waitall(MPI_Waitall)
int mpiwrapper_w_PMPI_Waitall(int abi_count, MPIABI_Request abi_requests[],
                              MPIABI_Status *abi_statuses)
    BODY_MPI_Waitall(PMPI_Waitall)

/* --------------------------------------------------------- MPI_Ialltoallw ---- */

/* The one entry point in the prototype whose temporaries have to outlive the
 * call. The two datatype arrays cannot be converted in place (NOTES.md #5.7:
 * they are `const`, the application may legally read them while the operation
 * is in flight, and there is no restore point), and they must stay converted
 * until the operation completes, so they go on the heap in a single block owned
 * by the request. mpiwrapper_staged_release, called from every completion
 * function, frees it.
 *
 * The array length is the group size, which the ABI call does not carry, so we
 * ask the implementation -- with PMPI_, because this is our traffic and not the
 * application's. On an intercommunicator both arrays are sized by the *remote*
 * group, which is the kind of detail that makes this function hand-written.
 */
#define BODY_MPI_Ialltoallw(TARGET)                                            \
  {                                                                            \
    const MPI_Comm comm = mpiwrapper_comm_fromabi(abi_comm);                   \
                                                                               \
    int inter = 0;                                                             \
    int n     = 0;                                                             \
    int ierror = PMPI_Comm_test_inter(comm, &inter);                           \
    if (ierror == MPI_SUCCESS)                                                 \
      ierror = inter ? PMPI_Comm_remote_size(comm, &n)                         \
                     : PMPI_Comm_size(comm, &n);                               \
    if (ierror != MPI_SUCCESS) {                                               \
      *abi_request = MPIABI_REQUEST_NULL;                                      \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }                                                                          \
                                                                               \
    MPI_Datatype *block = malloc(2 * (size_t)n * sizeof *block);               \
    if (!block && n > 0) {                                                     \
      *abi_request = MPIABI_REQUEST_NULL;                                      \
      return MPIABI_ERR_INTERN;                                                \
    }                                                                          \
    MPI_Datatype *const sendtypes = block;                                     \
    MPI_Datatype *const recvtypes = block + n;                                 \
    for (int i = 0; i < n; ++i) {                                              \
      sendtypes[i] = mpiwrapper_datatype_fromabi(abi_sendtypes[i]);            \
      recvtypes[i] = mpiwrapper_datatype_fromabi(abi_recvtypes[i]);            \
    }                                                                          \
                                                                               \
    const void *const sendbuf    = mpiwrapper_sendbuf_fromabi(abi_sendbuf);    \
    void *const       recvbuf    = mpiwrapper_recvbuf_fromabi(abi_recvbuf);    \
    const int *const  sendcounts = abi_sendcounts;                             \
    const int *const  sdispls    = abi_sdispls;                                \
    const int *const  recvcounts = abi_recvcounts;                             \
    const int *const  rdispls    = abi_rdispls;                                \
                                                                               \
    MPI_Request request;                                                       \
    ierror = TARGET(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,          \
                    recvcounts, rdispls, recvtypes, comm, &request);           \
    if (ierror != MPI_SUCCESS) {                                               \
      free(block);                                                             \
      *abi_request = MPIABI_REQUEST_NULL;                                      \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }                                                                          \
                                                                               \
    *abi_request = mpiwrapper_request_toabi(request);                          \
    if (mpiwrapper_take_handle_error()) return MPIABI_ERR_INTERN;              \
                                                                               \
    /* The operation is already in flight, so a table that cannot take the      \
     * block leaves us with nothing safe to do: freeing it would be a           \
     * use-after-free while the implementation is reading it, and there is no   \
     * way to un-start the operation. Leaking it and saying so is the only      \
     * honest answer, and the limit that produced it is a build-time constant.  \
     */                                                                        \
    if (!mpiwrapper_staged_attach(request, block)) return MPIABI_ERR_INTERN;   \
    return MPIABI_SUCCESS;                                                     \
  }

int mpiwrapper_w_MPI_Ialltoallw(const void *abi_sendbuf, const int abi_sendcounts[],
                            const int abi_sdispls[],
                            const MPIABI_Datatype abi_sendtypes[],
                            void *abi_recvbuf, const int abi_recvcounts[],
                            const int abi_rdispls[],
                            const MPIABI_Datatype abi_recvtypes[],
                            MPIABI_Comm abi_comm, MPIABI_Request *abi_request)
    BODY_MPI_Ialltoallw(MPI_Ialltoallw)
int mpiwrapper_w_PMPI_Ialltoallw(const void *abi_sendbuf,
                             const int abi_sendcounts[], const int abi_sdispls[],
                             const MPIABI_Datatype abi_sendtypes[],
                             void *abi_recvbuf, const int abi_recvcounts[],
                             const int abi_rdispls[],
                             const MPIABI_Datatype abi_recvtypes[],
                             MPIABI_Comm abi_comm, MPIABI_Request *abi_request)
    BODY_MPI_Ialltoallw(PMPI_Ialltoallw)

/* Declared in handwritten.h, which the generated wrappers.c includes to fill
 * these eighteen slots. Not static, therefore -- but hidden by
 * -fvisibility=hidden and absent from the export list, so libmpiwrapper still
 * exports exactly one symbol, which oracle 2 checks with nm.
 */
