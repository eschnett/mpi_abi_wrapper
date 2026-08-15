/* libmpiwrapper -- the wrapper bodies, and the vtable they fill.
 *
 * S1 STATUS: hand-written stand-in for gen/mpiwrapper/wrappers.c. These are the
 * *mechanical* bodies -- the ones S2's generator has to reproduce from
 * gen/include/mpi.h plus apis.json, byte-identically or with a diff explained
 * line by line (STAGES.md S2). They are written the way a generator would write
 * them, not the way a human would: one local per parameter, in parameter order,
 * declared const, named after the parameter with the abi_ prefix dropped.
 *
 * That convention is load-bearing rather than cosmetic. The generator asserts
 * over its own emitted text that **no parameter of an ABI-typed signature
 * appears in the argument list of the implementation call** -- only locally
 * declared converted values may. With this convention that assertion is a grep,
 * and a missing conversion is a hard stop at generation time rather than a
 * wrong answer at 4096 ranks.
 *
 * Every body is a macro instantiated twice, differing only in which
 * implementation entry point it calls. The doubling is in the emitted text, not
 * in anything maintained by hand.
 *
 * The bodies are static: only mpiwrapper_get_vtable is exported, and static
 * enforces that in the language rather than relying on the linker script.
 */

#include "handwritten.h"
#include "internal.h"

#include <stddef.h>
#include <string.h>

/* ------------------------------------------------------------ MPI_Comm_size */

/* The base case: one handle in, a plain int out, an error code back. */
#define BODY_MPI_Comm_size(TARGET)                                             \
  {                                                                            \
    const MPI_Comm comm = mpiwrapper_comm_fromabi(abi_comm);                   \
    int *const     size = abi_size;                                            \
                                                                               \
    const int ierror = TARGET(comm, size);                                     \
    return mpiwrapper_errorcode_toabi(ierror);                                 \
  }

static int w_MPI_Comm_size(MPIABI_Comm abi_comm, int *abi_size)
    BODY_MPI_Comm_size(MPI_Comm_size)
static int w_PMPI_Comm_size(MPIABI_Comm abi_comm, int *abi_size)
    BODY_MPI_Comm_size(PMPI_Comm_size)

/* ------------------------------------------------------------ MPI_Comm_rank */

#define BODY_MPI_Comm_rank(TARGET)                                             \
  {                                                                            \
    const MPI_Comm comm = mpiwrapper_comm_fromabi(abi_comm);                   \
    int *const     rank = abi_rank;                                            \
                                                                               \
    const int ierror = TARGET(comm, rank);                                     \
    /* The rank of the calling process in its own communicator is a real rank   \
     * and never a sentinel -- MPI_UNDEFINED comes back from                    \
     * MPI_Group_translate_ranks, not from here -- so no out-direction mapping. \
     */                                                                        \
    return mpiwrapper_errorcode_toabi(ierror);                                 \
  }

static int w_MPI_Comm_rank(MPIABI_Comm abi_comm, int *abi_rank)
    BODY_MPI_Comm_rank(MPI_Comm_rank)
static int w_PMPI_Comm_rank(MPIABI_Comm abi_comm, int *abi_rank)
    BODY_MPI_Comm_rank(PMPI_Comm_rank)

/* ----------------------------------------------------------------- MPI_Send */

/* `dest` and `tag` are both plain ints and go through *different* conversions.
 * That is not redundancy: in the ABI MPI_ANY_TAG is -2 and MPI_PROC_NULL is -3,
 * while MPICH gives both the value -1, so a single int_fromabi would be
 * unimplementable. The parameter's kind comes from apis.json, which is why the
 * header alone is not enough (NOTES.md #5.4).
 */
#define BODY_MPI_Send(TARGET)                                                  \
  {                                                                            \
    const void *const  buf      = mpiwrapper_sendbuf_fromabi(abi_buf);         \
    const int          count    = abi_count;                                   \
    const MPI_Datatype datatype = mpiwrapper_datatype_fromabi(abi_datatype);   \
    const int          dest     = mpiwrapper_rank_fromabi(abi_dest);           \
    const int          tag      = mpiwrapper_tag_fromabi(abi_tag);             \
    const MPI_Comm     comm     = mpiwrapper_comm_fromabi(abi_comm);           \
                                                                               \
    const int ierror = TARGET(buf, count, datatype, dest, tag, comm);          \
    return mpiwrapper_errorcode_toabi(ierror);                                 \
  }

static int w_MPI_Send(const void *abi_buf, int abi_count,
                      MPIABI_Datatype abi_datatype, int abi_dest, int abi_tag,
                      MPIABI_Comm abi_comm) BODY_MPI_Send(MPI_Send)
static int w_PMPI_Send(const void *abi_buf, int abi_count,
                       MPIABI_Datatype abi_datatype, int abi_dest, int abi_tag,
                       MPIABI_Comm abi_comm) BODY_MPI_Send(PMPI_Send)

/* ----------------------------------------------------------------- MPI_Recv */

/* MPI_STATUS_IGNORE is NULL in the ABI and (MPI_Status *)1 in MPICH, so the
 * translation is real. The implementation's status is a local of its own type
 * and never the caller's storage reinterpreted: the ABI's is 32 bytes and
 * MPICH's is 20, and writing one through a pointer to the other is the bug
 * class NOTES.md #5.7 ends with.
 */
#define BODY_MPI_Recv(TARGET)                                                  \
  {                                                                            \
    void *const        buf      = mpiwrapper_recvbuf_fromabi(abi_buf);         \
    const int          count    = abi_count;                                   \
    const MPI_Datatype datatype = mpiwrapper_datatype_fromabi(abi_datatype);   \
    const int          source   = mpiwrapper_rank_fromabi(abi_source);         \
    const int          tag      = mpiwrapper_tag_fromabi(abi_tag);             \
    const MPI_Comm     comm     = mpiwrapper_comm_fromabi(abi_comm);           \
    const int          ignore   = abi_status == MPIABI_STATUS_IGNORE;          \
                                                                               \
    /* Zeroed rather than left indeterminate: on an error return the            \
     * implementation may not have written it, and copying its stack garbage    \
     * out to the application is what makes this hard to debug under MSan.      \
     */                                                                        \
    MPI_Status status;                                                         \
    memset(&status, 0, sizeof status);                                         \
                                                                               \
    const int ierror =                                                         \
        TARGET(buf, count, datatype, source, tag, comm,                        \
               ignore ? MPI_STATUS_IGNORE : &status);                          \
    if (!ignore) mpiwrapper_status_toabi(&status, abi_status);                 \
    return mpiwrapper_errorcode_toabi(ierror);                                 \
  }

static int w_MPI_Recv(void *abi_buf, int abi_count, MPIABI_Datatype abi_datatype,
                      int abi_source, int abi_tag, MPIABI_Comm abi_comm,
                      MPIABI_Status *abi_status) BODY_MPI_Recv(MPI_Recv)
static int w_PMPI_Recv(void *abi_buf, int abi_count,
                       MPIABI_Datatype abi_datatype, int abi_source, int abi_tag,
                       MPIABI_Comm abi_comm, MPIABI_Status *abi_status)
    BODY_MPI_Recv(PMPI_Recv)

/* ---------------------------------------------------------- MPI_Get_version */

/* Two plain out-ints, so the simplest body there is -- and the one the ABI side
 * uses to probe its own symbol resolution at load, because it is legal before
 * MPI_Init in every version of the standard.
 */
#define BODY_MPI_Get_version(TARGET)                                           \
  {                                                                            \
    int *const version    = abi_version;                                       \
    int *const subversion = abi_subversion;                                    \
                                                                               \
    const int ierror = TARGET(version, subversion);                            \
    return mpiwrapper_errorcode_toabi(ierror);                                 \
  }

static int w_MPI_Get_version(int *abi_version, int *abi_subversion)
    BODY_MPI_Get_version(MPI_Get_version)
static int w_PMPI_Get_version(int *abi_version, int *abi_subversion)
    BODY_MPI_Get_version(PMPI_Get_version)

/* ---------------------------------------------------------------- MPI_Isend */

#define BODY_MPI_Isend(TARGET)                                                 \
  {                                                                            \
    const void *const  buf      = mpiwrapper_sendbuf_fromabi(abi_buf);         \
    const int          count    = abi_count;                                   \
    const MPI_Datatype datatype = mpiwrapper_datatype_fromabi(abi_datatype);   \
    const int          dest     = mpiwrapper_rank_fromabi(abi_dest);           \
    const int          tag      = mpiwrapper_tag_fromabi(abi_tag);             \
    const MPI_Comm     comm     = mpiwrapper_comm_fromabi(abi_comm);           \
                                                                               \
    MPI_Request request;                                                       \
    const int   ierror =                                                       \
        TARGET(buf, count, datatype, dest, tag, comm, &request);               \
                                                                               \
    *abi_request = (ierror == MPI_SUCCESS)                                     \
                       ? mpiwrapper_request_toabi(request)                     \
                       : MPIABI_REQUEST_NULL;                                  \
    if (mpiwrapper_take_handle_error()) return MPIABI_ERR_INTERN;              \
    return mpiwrapper_errorcode_toabi(ierror);                                 \
  }

static int w_MPI_Isend(const void *abi_buf, int abi_count,
                       MPIABI_Datatype abi_datatype, int abi_dest, int abi_tag,
                       MPIABI_Comm abi_comm, MPIABI_Request *abi_request)
    BODY_MPI_Isend(MPI_Isend)
static int w_PMPI_Isend(const void *abi_buf, int abi_count,
                        MPIABI_Datatype abi_datatype, int abi_dest, int abi_tag,
                        MPIABI_Comm abi_comm, MPIABI_Request *abi_request)
    BODY_MPI_Isend(PMPI_Isend)

/* ---------------------------------------------------------------- MPI_Irecv */

#define BODY_MPI_Irecv(TARGET)                                                 \
  {                                                                            \
    void *const        buf      = mpiwrapper_recvbuf_fromabi(abi_buf);         \
    const int          count    = abi_count;                                   \
    const MPI_Datatype datatype = mpiwrapper_datatype_fromabi(abi_datatype);   \
    const int          source   = mpiwrapper_rank_fromabi(abi_source);         \
    const int          tag      = mpiwrapper_tag_fromabi(abi_tag);             \
    const MPI_Comm     comm     = mpiwrapper_comm_fromabi(abi_comm);           \
                                                                               \
    MPI_Request request;                                                       \
    const int   ierror =                                                       \
        TARGET(buf, count, datatype, source, tag, comm, &request);             \
                                                                               \
    *abi_request = (ierror == MPI_SUCCESS)                                     \
                       ? mpiwrapper_request_toabi(request)                     \
                       : MPIABI_REQUEST_NULL;                                  \
    if (mpiwrapper_take_handle_error()) return MPIABI_ERR_INTERN;              \
    return mpiwrapper_errorcode_toabi(ierror);                                 \
  }

static int w_MPI_Irecv(void *abi_buf, int abi_count,
                       MPIABI_Datatype abi_datatype, int abi_source, int abi_tag,
                       MPIABI_Comm abi_comm, MPIABI_Request *abi_request)
    BODY_MPI_Irecv(MPI_Irecv)
static int w_PMPI_Irecv(void *abi_buf, int abi_count,
                        MPIABI_Datatype abi_datatype, int abi_source,
                        int abi_tag, MPIABI_Comm abi_comm,
                        MPIABI_Request *abi_request) BODY_MPI_Irecv(PMPI_Irecv)

/* -------------------------------------------------------------- MPI_Waitall */

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

static int w_MPI_Waitall(int abi_count, MPIABI_Request abi_requests[],
                         MPIABI_Status *abi_statuses)
    BODY_MPI_Waitall(MPI_Waitall)
static int w_PMPI_Waitall(int abi_count, MPIABI_Request abi_requests[],
                          MPIABI_Status *abi_statuses)
    BODY_MPI_Waitall(PMPI_Waitall)

/* ------------------------------------------------------------ MPI_Allreduce */

/* MPI_IN_PLACE is (void *)1 in the ABI and in Open MPI, and (void *)-1 in
 * MPICH, so the sendbuf test is a real conversion on one of the two -- the kind
 * of difference that would pass every test on the other.
 */
#define BODY_MPI_Allreduce(TARGET)                                             \
  {                                                                            \
    const void *const sendbuf =                                                \
        mpiwrapper_sendbuf_inplace_fromabi(abi_sendbuf);                       \
    void *const        recvbuf  = mpiwrapper_recvbuf_fromabi(abi_recvbuf);     \
    const int          count    = abi_count;                                   \
    const MPI_Datatype datatype = mpiwrapper_datatype_fromabi(abi_datatype);   \
    const MPI_Op       op       = mpiwrapper_op_fromabi(abi_op);               \
    const MPI_Comm     comm     = mpiwrapper_comm_fromabi(abi_comm);           \
                                                                               \
    const int ierror =                                                         \
        TARGET(sendbuf, recvbuf, count, datatype, op, comm);                   \
    return mpiwrapper_errorcode_toabi(ierror);                                 \
  }

static int w_MPI_Allreduce(const void *abi_sendbuf, void *abi_recvbuf,
                           int abi_count, MPIABI_Datatype abi_datatype,
                           MPIABI_Op abi_op, MPIABI_Comm abi_comm)
    BODY_MPI_Allreduce(MPI_Allreduce)
static int w_PMPI_Allreduce(const void *abi_sendbuf, void *abi_recvbuf,
                            int abi_count, MPIABI_Datatype abi_datatype,
                            MPIABI_Op abi_op, MPIABI_Comm abi_comm)
    BODY_MPI_Allreduce(PMPI_Allreduce)

/* ----------------------------------------------------------- MPI_Comm_split */

/* An out handle, so the reverse map. A communicator produced here is dynamic
 * and bit-casts, but MPI_COMM_NULL comes back when the colour is
 * MPI_UNDEFINED, and that one is predefined and must map.
 */
#define BODY_MPI_Comm_split(TARGET)                                            \
  {                                                                            \
    const MPI_Comm comm  = mpiwrapper_comm_fromabi(abi_comm);                  \
    const int      color = abi_color == MPIABI_UNDEFINED ? MPI_UNDEFINED       \
                                                         : abi_color;          \
    const int      key   = abi_key;                                            \
                                                                               \
    MPI_Comm  newcomm;                                                         \
    const int ierror = TARGET(comm, color, key, &newcomm);                     \
                                                                               \
    *abi_newcomm = (ierror == MPI_SUCCESS) ? mpiwrapper_comm_toabi(newcomm)    \
                                           : MPIABI_COMM_NULL;                 \
    if (mpiwrapper_take_handle_error()) return MPIABI_ERR_INTERN;              \
    return mpiwrapper_errorcode_toabi(ierror);                                 \
  }

static int w_MPI_Comm_split(MPIABI_Comm abi_comm, int abi_color, int abi_key,
                            MPIABI_Comm *abi_newcomm)
    BODY_MPI_Comm_split(MPI_Comm_split)
static int w_PMPI_Comm_split(MPIABI_Comm abi_comm, int abi_color, int abi_key,
                             MPIABI_Comm *abi_newcomm)
    BODY_MPI_Comm_split(PMPI_Comm_split)

/* ------------------------------------------------------------ MPI_Comm_free */

/* An inout handle: a local of the implementation's type, never
 * `(MPI_Comm *)abi_comm`. On MPICH that would write a 4-byte handle into an
 * 8-byte ABI slot and leave the upper half garbage, while on Open MPI it works
 * by accident -- so it would pass every test on one implementation and corrupt
 * on the other. No _Static_assert catches this; the naming convention does.
 */
#define BODY_MPI_Comm_free(TARGET)                                             \
  {                                                                            \
    MPI_Comm comm = mpiwrapper_comm_fromabi(*abi_comm);                        \
                                                                               \
    const int ierror = TARGET(&comm);                                          \
                                                                               \
    *abi_comm = mpiwrapper_comm_toabi(comm);                                   \
    if (mpiwrapper_take_handle_error()) return MPIABI_ERR_INTERN;              \
    return mpiwrapper_errorcode_toabi(ierror);                                 \
  }

static int w_MPI_Comm_free(MPIABI_Comm *abi_comm)
    BODY_MPI_Comm_free(MPI_Comm_free)
static int w_PMPI_Comm_free(MPIABI_Comm *abi_comm)
    BODY_MPI_Comm_free(PMPI_Comm_free)

/* --------------------------------------------- MPI_Comm_set_errhandler ---- */

#define BODY_MPI_Comm_set_errhandler(TARGET)                                   \
  {                                                                            \
    const MPI_Comm       comm = mpiwrapper_comm_fromabi(abi_comm);             \
    const MPI_Errhandler errhandler =                                          \
        mpiwrapper_errhandler_fromabi(abi_errhandler);                         \
                                                                               \
    const int ierror = TARGET(comm, errhandler);                               \
    return mpiwrapper_errorcode_toabi(ierror);                                 \
  }

static int w_MPI_Comm_set_errhandler(MPIABI_Comm       abi_comm,
                                     MPIABI_Errhandler abi_errhandler)
    BODY_MPI_Comm_set_errhandler(MPI_Comm_set_errhandler)
static int w_PMPI_Comm_set_errhandler(MPIABI_Comm       abi_comm,
                                      MPIABI_Errhandler abi_errhandler)
    BODY_MPI_Comm_set_errhandler(PMPI_Comm_set_errhandler)

/* ------------------------------------------------- MPI_Type_create_struct -- */

/* A `const` handle array, which is the case that kills in-place conversion
 * outright: an application's `static const MPI_Datatype types[3]` lives in
 * .rodata and writing to it crashes a legal program.
 *
 * The *displacement* array is a different matter and is passed straight
 * through. An earlier version of this body staged it too, on the grounds that
 * the ABI's MPI_Aint and the implementation's may be distinct types. They may
 * be -- on glibc the ABI's MPI_Count is `long` where MPICH's is `long long`,
 * because int64_t is long there -- but distinct *spellings* are not the
 * question. This project implements a system ABI, and what an ABI fixes is
 * representation: dev/type-identity measures size and signedness identical in
 * every implementation and platform tried, and the cast reading correctly at
 * -O3 -fstrict-aliasing even in the single-translation-unit worst case. The
 * _Static_asserts in internal.h are what license the cast, and would fail the
 * build if a target ever disagreed.
 */
#define BODY_MPI_Type_create_struct(TARGET)                                    \
  {                                                                            \
    const int count = abi_count;                                               \
    if (count < 0) return MPIABI_ERR_COUNT;                                    \
                                                                               \
    const int *const      blocklengths  = abi_array_of_blocklengths;           \
    const MPI_Aint *const displacements =                                      \
        (const MPI_Aint *)abi_array_of_displacements;                          \
                                                                               \
    MPI_Datatype  typestack[MPIWRAPPER_STAGE_BYTES / sizeof(MPI_Datatype)];    \
    MPI_Datatype *types      = NULL;                                           \
    int           abi_ierror = MPIABI_ERR_INTERN;                              \
                                                                               \
    types = mpiwrapper_stage(typestack, sizeof typestack, (size_t)count,       \
                             sizeof *types);                                   \
    if (!types) goto done;                                                     \
                                                                               \
    for (int i = 0; i < count; ++i)                                            \
      types[i] = mpiwrapper_datatype_fromabi(abi_array_of_types[i]);           \
                                                                               \
    {                                                                          \
      MPI_Datatype newtype;                                                    \
      const int    ierror =                                                    \
          TARGET(count, blocklengths, displacements, types, &newtype);         \
                                                                               \
      *abi_newtype = (ierror == MPI_SUCCESS)                                   \
                         ? mpiwrapper_datatype_toabi(newtype)                  \
                         : MPIABI_DATATYPE_NULL;                               \
      abi_ierror   = mpiwrapper_errorcode_toabi(ierror);                       \
      if (mpiwrapper_take_handle_error()) abi_ierror = MPIABI_ERR_INTERN;      \
    }                                                                          \
                                                                               \
  done:                                                                        \
    mpiwrapper_unstage(types, typestack);                                      \
    return abi_ierror;                                                         \
  }

static int w_MPI_Type_create_struct(int abi_count,
                                    const int abi_array_of_blocklengths[],
                                    const MPIABI_Aint abi_array_of_displacements[],
                                    const MPIABI_Datatype abi_array_of_types[],
                                    MPIABI_Datatype *abi_newtype)
    BODY_MPI_Type_create_struct(MPI_Type_create_struct)
static int w_PMPI_Type_create_struct(int abi_count,
                                     const int abi_array_of_blocklengths[],
                                     const MPIABI_Aint abi_array_of_displacements[],
                                     const MPIABI_Datatype abi_array_of_types[],
                                     MPIABI_Datatype *abi_newtype)
    BODY_MPI_Type_create_struct(PMPI_Type_create_struct)

/* ----------------------------------------------- MPI_Type_create_struct_c -- */

/* The large-count twin, and the first place decision 6 bites: the _c forms are
 * MPI-4.0, and an implementation that predates them cannot be called. Open MPI
 * 5.0.10 is exactly that case -- it reports MPI_VERSION 3.1 and defines no _c
 * entry point at all -- so the generated body becomes a stub returning
 * MPIABI_ERR_UNSUPPORTED_OPERATION, the slot stays present, and the generator
 * lists it in gen/report.txt. The ABI surface never shrinks; what is missing is
 * discoverable at run time (NOTES.md #1).
 */
#if defined(MPI_VERSION) && MPI_VERSION >= 4
#  define BODY_MPI_Type_create_struct_c(TARGET)                                \
    {                                                                          \
      const MPI_Count count = abi_count;                                       \
      if (count < 0) return MPIABI_ERR_COUNT;                                  \
      if ((uint64_t)count > SIZE_MAX / sizeof(MPI_Datatype))                   \
        return MPIABI_ERR_COUNT;                                               \
                                                                               \
      /* Passed through, not staged: same representation, no value mapping. */ \
      const MPI_Count *const blocklengths =                                    \
          (const MPI_Count *)abi_array_of_blocklengths;                        \
      const MPI_Count *const displacements =                                   \
          (const MPI_Count *)abi_array_of_displacements;                       \
                                                                               \
      MPI_Datatype  typestack[MPIWRAPPER_STAGE_BYTES / sizeof(MPI_Datatype)];  \
      MPI_Datatype *types      = NULL;                                         \
      int           abi_ierror = MPIABI_ERR_INTERN;                            \
                                                                               \
      types = mpiwrapper_stage(typestack, sizeof typestack, (size_t)count,     \
                               sizeof *types);                                 \
      if (!types) goto done;                                                   \
                                                                               \
      for (MPI_Count i = 0; i < count; ++i)                                    \
        types[i] = mpiwrapper_datatype_fromabi(abi_array_of_types[i]);         \
                                                                               \
      {                                                                        \
        MPI_Datatype newtype;                                                  \
        const int    ierror =                                                  \
            TARGET(count, blocklengths, displacements, types, &newtype);       \
                                                                               \
        *abi_newtype = (ierror == MPI_SUCCESS)                                 \
                           ? mpiwrapper_datatype_toabi(newtype)                \
                           : MPIABI_DATATYPE_NULL;                             \
        abi_ierror   = mpiwrapper_errorcode_toabi(ierror);                     \
        if (mpiwrapper_take_handle_error()) abi_ierror = MPIABI_ERR_INTERN;    \
      }                                                                        \
                                                                               \
    done:                                                                      \
      mpiwrapper_unstage(types, typestack);                                    \
      return abi_ierror;                                                       \
    }
#else
#  define BODY_MPI_Type_create_struct_c(TARGET)                                \
    {                                                                          \
      (void)abi_count;                                                         \
      (void)abi_array_of_blocklengths;                                         \
      (void)abi_array_of_displacements;                                        \
      (void)abi_array_of_types;                                                \
      *abi_newtype = MPIABI_DATATYPE_NULL;                                     \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

static int w_MPI_Type_create_struct_c(MPIABI_Count abi_count,
                                      const MPIABI_Count abi_array_of_blocklengths[],
                                      const MPIABI_Count abi_array_of_displacements[],
                                      const MPIABI_Datatype abi_array_of_types[],
                                      MPIABI_Datatype *abi_newtype)
    BODY_MPI_Type_create_struct_c(MPI_Type_create_struct_c)
static int w_PMPI_Type_create_struct_c(MPIABI_Count abi_count,
                                       const MPIABI_Count abi_array_of_blocklengths[],
                                       const MPIABI_Count abi_array_of_displacements[],
                                       const MPIABI_Datatype abi_array_of_types[],
                                       MPIABI_Datatype *abi_newtype)
    BODY_MPI_Type_create_struct_c(PMPI_Type_create_struct_c)

/* ----------------------------------------------------------- MPI_Type_commit */

#define BODY_MPI_Type_commit(TARGET)                                           \
  {                                                                            \
    MPI_Datatype datatype = mpiwrapper_datatype_fromabi(*abi_datatype);        \
                                                                               \
    const int ierror = TARGET(&datatype);                                      \
                                                                               \
    *abi_datatype = mpiwrapper_datatype_toabi(datatype);                       \
    if (mpiwrapper_take_handle_error()) return MPIABI_ERR_INTERN;              \
    return mpiwrapper_errorcode_toabi(ierror);                                 \
  }

static int w_MPI_Type_commit(MPIABI_Datatype *abi_datatype)
    BODY_MPI_Type_commit(MPI_Type_commit)
static int w_PMPI_Type_commit(MPIABI_Datatype *abi_datatype)
    BODY_MPI_Type_commit(PMPI_Type_commit)

/* ------------------------------------------------------------- MPI_Type_free */

#define BODY_MPI_Type_free(TARGET)                                             \
  {                                                                            \
    MPI_Datatype datatype = mpiwrapper_datatype_fromabi(*abi_datatype);        \
                                                                               \
    const int ierror = TARGET(&datatype);                                      \
                                                                               \
    *abi_datatype = mpiwrapper_datatype_toabi(datatype);                       \
    if (mpiwrapper_take_handle_error()) return MPIABI_ERR_INTERN;              \
    return mpiwrapper_errorcode_toabi(ierror);                                 \
  }

static int w_MPI_Type_free(MPIABI_Datatype *abi_datatype)
    BODY_MPI_Type_free(MPI_Type_free)
static int w_PMPI_Type_free(MPIABI_Datatype *abi_datatype)
    BODY_MPI_Type_free(PMPI_Type_free)

/* --------------------------------------------------------------- MPI_Op_free */

#define BODY_MPI_Op_free(TARGET)                                               \
  {                                                                            \
    MPI_Op op = mpiwrapper_op_fromabi(*abi_op);                                \
                                                                               \
    const int ierror = TARGET(&op);                                            \
                                                                               \
    /* The trampoline slot is deliberately *not* released here: a pending       \
     * reduction may still invoke it after this returns (NOTES.md #6.2).        \
     */                                                                        \
    *abi_op = mpiwrapper_op_toabi(op);                                         \
    if (mpiwrapper_take_handle_error()) return MPIABI_ERR_INTERN;              \
    return mpiwrapper_errorcode_toabi(ierror);                                 \
  }

static int w_MPI_Op_free(MPIABI_Op *abi_op) BODY_MPI_Op_free(MPI_Op_free)
static int w_PMPI_Op_free(MPIABI_Op *abi_op) BODY_MPI_Op_free(PMPI_Op_free)

/* ------------------------------------------------------------- MPI_File_open */

/* A bitmask argument and a second handle class. MPI_MODE_RDONLY is 16 in the
 * ABI and 2 in both implementations, so the amode is decomposed bit by bit
 * rather than switched (NOTES.md #5.5).
 */
#define BODY_MPI_File_open(TARGET)                                             \
  {                                                                            \
    const MPI_Comm    comm     = mpiwrapper_comm_fromabi(abi_comm);            \
    const char *const filename = abi_filename;                                 \
    const int         amode    = mpiwrapper_filemode_fromabi(abi_amode);           \
    const MPI_Info    info     = mpiwrapper_info_fromabi(abi_info);            \
                                                                               \
    MPI_File  fh;                                                              \
    const int ierror = TARGET(comm, filename, amode, info, &fh);               \
                                                                               \
    *abi_fh = (ierror == MPI_SUCCESS) ? mpiwrapper_file_toabi(fh)              \
                                      : MPIABI_FILE_NULL;                      \
    if (mpiwrapper_take_handle_error()) return MPIABI_ERR_INTERN;              \
    return mpiwrapper_errorcode_toabi(ierror);                                 \
  }

static int w_MPI_File_open(MPIABI_Comm abi_comm, const char *abi_filename,
                           int abi_amode, MPIABI_Info abi_info,
                           MPIABI_File *abi_fh) BODY_MPI_File_open(MPI_File_open)
static int w_PMPI_File_open(MPIABI_Comm abi_comm, const char *abi_filename,
                            int abi_amode, MPIABI_Info abi_info,
                            MPIABI_File *abi_fh)
    BODY_MPI_File_open(PMPI_File_open)

/* ------------------------------------------------------------ MPI_File_close */

#define BODY_MPI_File_close(TARGET)                                            \
  {                                                                            \
    MPI_File fh = mpiwrapper_file_fromabi(*abi_fh);                            \
                                                                               \
    const int ierror = TARGET(&fh);                                            \
                                                                               \
    *abi_fh = mpiwrapper_file_toabi(fh);                                       \
    if (mpiwrapper_take_handle_error()) return MPIABI_ERR_INTERN;              \
    return mpiwrapper_errorcode_toabi(ierror);                                 \
  }

static int w_MPI_File_close(MPIABI_File *abi_fh)
    BODY_MPI_File_close(MPI_File_close)
static int w_PMPI_File_close(MPIABI_File *abi_fh)
    BODY_MPI_File_close(PMPI_File_close)

/* ---------------------------------------------------------------- MPI_Wtime */

/* Returns double, so there is no error code to map. */
#define BODY_MPI_Wtime(TARGET)                                                 \
  {                                                                            \
    return TARGET();                                                           \
  }

static double w_MPI_Wtime(void) BODY_MPI_Wtime(MPI_Wtime)
static double w_PMPI_Wtime(void) BODY_MPI_Wtime(PMPI_Wtime)

/* ------------------------------------------------------------- the vtable -- */

/* Designated initializers, so a slot the generator forgets is a NULL pointer
 * rather than a shifted one -- and mpiwrapper_get_vtable asserts none is NULL
 * before handing the table out.
 */
const struct mpiwrapper_vtable mpiwrapper_vtable_instance = {
    .MPI_Allreduce               = w_MPI_Allreduce,
    .PMPI_Allreduce              = w_PMPI_Allreduce,
    .MPI_Comm_c2f                = mpiwrapper_w_MPI_Comm_c2f,
    .PMPI_Comm_c2f               = mpiwrapper_w_PMPI_Comm_c2f,
    .MPI_Comm_create_errhandler  = mpiwrapper_w_MPI_Comm_create_errhandler,
    .PMPI_Comm_create_errhandler = mpiwrapper_w_PMPI_Comm_create_errhandler,
    .MPI_Comm_f2c                = mpiwrapper_w_MPI_Comm_f2c,
    .PMPI_Comm_f2c               = mpiwrapper_w_PMPI_Comm_f2c,
    .MPI_Comm_free               = w_MPI_Comm_free,
    .PMPI_Comm_free              = w_PMPI_Comm_free,
    .MPI_Comm_rank               = w_MPI_Comm_rank,
    .PMPI_Comm_rank              = w_PMPI_Comm_rank,
    .MPI_Comm_set_errhandler     = w_MPI_Comm_set_errhandler,
    .PMPI_Comm_set_errhandler    = w_PMPI_Comm_set_errhandler,
    .MPI_Comm_size               = w_MPI_Comm_size,
    .PMPI_Comm_size              = w_PMPI_Comm_size,
    .MPI_Comm_split              = w_MPI_Comm_split,
    .PMPI_Comm_split             = w_PMPI_Comm_split,
    .MPI_Error_string            = mpiwrapper_w_MPI_Error_string,
    .PMPI_Error_string           = mpiwrapper_w_PMPI_Error_string,
    .MPI_File_close              = w_MPI_File_close,
    .PMPI_File_close             = w_PMPI_File_close,
    .MPI_File_open               = w_MPI_File_open,
    .PMPI_File_open              = w_PMPI_File_open,
    .MPI_Finalize                = mpiwrapper_w_MPI_Finalize,
    .PMPI_Finalize               = mpiwrapper_w_PMPI_Finalize,
    .MPI_Get_count               = mpiwrapper_w_MPI_Get_count,
    .PMPI_Get_count              = mpiwrapper_w_PMPI_Get_count,
    .MPI_Get_version             = w_MPI_Get_version,
    .PMPI_Get_version            = w_PMPI_Get_version,
    .MPI_Ialltoallw              = mpiwrapper_w_MPI_Ialltoallw,
    .PMPI_Ialltoallw             = mpiwrapper_w_PMPI_Ialltoallw,
    .MPI_Init                    = mpiwrapper_w_MPI_Init,
    .PMPI_Init                   = mpiwrapper_w_PMPI_Init,
    .MPI_Irecv                   = w_MPI_Irecv,
    .PMPI_Irecv                  = w_PMPI_Irecv,
    .MPI_Isend                   = w_MPI_Isend,
    .PMPI_Isend                  = w_PMPI_Isend,
    .MPI_Op_create               = mpiwrapper_w_MPI_Op_create,
    .PMPI_Op_create              = mpiwrapper_w_PMPI_Op_create,
    .MPI_Op_free                 = w_MPI_Op_free,
    .PMPI_Op_free                = w_PMPI_Op_free,
    .MPI_Recv                    = w_MPI_Recv,
    .PMPI_Recv                   = w_PMPI_Recv,
    .MPI_Send                    = w_MPI_Send,
    .PMPI_Send                   = w_PMPI_Send,
    .MPI_Type_commit             = w_MPI_Type_commit,
    .PMPI_Type_commit            = w_PMPI_Type_commit,
    .MPI_Type_create_struct      = w_MPI_Type_create_struct,
    .PMPI_Type_create_struct     = w_PMPI_Type_create_struct,
    .MPI_Type_create_struct_c    = w_MPI_Type_create_struct_c,
    .PMPI_Type_create_struct_c   = w_PMPI_Type_create_struct_c,
    .MPI_Type_free               = w_MPI_Type_free,
    .PMPI_Type_free              = w_PMPI_Type_free,
    .MPI_Waitall                 = w_MPI_Waitall,
    .PMPI_Waitall                = w_PMPI_Waitall,
    .MPI_Wtime                   = w_MPI_Wtime,
    .PMPI_Wtime                  = w_PMPI_Wtime,
};
