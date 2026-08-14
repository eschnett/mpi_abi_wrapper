/* libmpiwrapper -- the generated wrapper bodies.
 *
 * Excerpt of gen/mpiwrapper/wrappers.c. This is where every conversion happens.
 *
 * Two headers meet here and nowhere else:
 *   <mpi.h>     the *implementation's* header. Included normally, with no
 *               preprocessor games, because this library defines no MPI_* function
 *               and so collides with nothing. It supplies the real MPI_Send, the
 *               real MPI_COMM_WORLD, and -- crucially -- the real declarations,
 *               which check every call below at compile time. That is why no
 *               hand-maintained table of implementation signatures has to exist.
 *   "mpiabi.h"  the MPIABI_ view of the ABI.
 *
 * Naming convention, and it is load-bearing rather than cosmetic: ABI-side names
 * carry an abi_ prefix, implementation-side names are bare. The generator asserts
 * over its own emitted text that **no parameter of an ABI-typed signature appears
 * in the argument list of the implementation call** -- only locally declared
 * converted values may. With this convention that assertion is a grep, and a
 * missing conversion is a hard stop at generation time rather than a wrong answer
 * at 4096 ranks.
 *
 * The bodies are `static`: only mpiwrapper_get_vtable is exported, and `static`
 * enforces that in the language rather than relying on the linker script.
 *
 * Note that the wrapper's own *internal* MPI calls -- in the hand-written ~50, where
 * MPI_Init needs a rank or the error-code registry needs a class -- must use the
 * implementation's PMPI_* names. An internal call is not application traffic and
 * must not be counted as such by an interposed tool. This is the same discipline
 * implementations follow inside themselves.
 */

#include <mpi.h> /* the implementation's */

#include "mpiabi.h"
#include "mpiwrapper_vtable.h"

#include <stddef.h>
#include <string.h>

/* Defined in src/mpiwrapper/, shown in mpiwrapper_convert.c. In the real tree
 * these live in src/mpiwrapper/internal.h.
 */
extern MPI_Comm     mpiwrapper_comm_fromabi(MPIABI_Comm);
extern MPI_Datatype mpiwrapper_datatype_fromabi(MPIABI_Datatype);
extern MPI_Info     mpiwrapper_info_fromabi(MPIABI_Info);
extern MPI_Request  mpiwrapper_request_fromabi(MPIABI_Request);
extern MPIABI_File  mpiwrapper_file_toabi(MPI_File);
extern MPIABI_Request mpiwrapper_request_toabi(MPI_Request);
extern int  mpiwrapper_rank_fromabi(int);
extern int  mpiwrapper_tag_fromabi(int);
extern int  mpiwrapper_filemode_fromabi(int);
extern int  mpiwrapper_errorcode_fromabi(int);
extern int  mpiwrapper_errorcode_toabi(int);
extern void mpiwrapper_status_toabi(const MPI_Status *, MPIABI_Status *);
extern int  mpiwrapper_op_create(MPIABI_User_function *, int, MPIABI_Op *);

/* Staging: a caller-supplied stack buffer when the request fits in it, heap
 * otherwise. Never a VLA (optional in C11, absent in MSVC, and unbounded means a
 * stack overflow at 100k ranks) and never alloca. The threshold is in *bytes*, not
 * elements, so that a 32-byte-per-element status array cannot blow a budget tuned
 * for 8-byte handles.
 */
extern void *mpiwrapper_stage(void *stackbuf, size_t stackbytes, size_t nmemb,
                              size_t size);
extern void  mpiwrapper_unstage(void *p, void *stackbuf);

#define MPIWRAPPER_STAGE_BYTES 1024

/* ------------------------------------------------------------------- MPI_Send */

/* The shape every simple entry point takes: convert each argument into a local,
 * call, map the error code. Nothing is modified in place and nothing is aliased.
 *
 * The body is a template instantiated twice, once per name, differing only in which
 * implementation entry point it calls. The generator holds it once; the doubling is
 * in the emitted text, not in anything maintained by hand.
 *
 * `dest` and `tag` are both plain ints and go through *different* conversions. That
 * is not redundancy: in the ABI, MPI_ANY_TAG is -2 and MPI_PROC_NULL is -3, while
 * MPICH gives both the value -1, so a single int_fromabi would be unimplementable.
 */
#define BODY_MPI_Send(TARGET)                                                     \
  {                                                                               \
    const void *const  buf      = abi_buf; /* sentinel-translated for MPI_BOTTOM */\
    const int          count    = abi_count;                                      \
    const MPI_Datatype datatype = mpiwrapper_datatype_fromabi(abi_datatype);       \
    const int          dest     = mpiwrapper_rank_fromabi(abi_dest);              \
    const int          tag      = mpiwrapper_tag_fromabi(abi_tag);                 \
    const MPI_Comm     comm     = mpiwrapper_comm_fromabi(abi_comm);              \
                                                                                  \
    const int ierror = TARGET(buf, count, datatype, dest, tag, comm);             \
    return mpiwrapper_errorcode_toabi(ierror);                                     \
  }

static int w_MPI_Send(const void *abi_buf, int abi_count,
                      MPIABI_Datatype abi_datatype, int abi_dest, int abi_tag,
                      MPIABI_Comm abi_comm)
    BODY_MPI_Send(MPI_Send)

/* The name-shifted twin, so that an application calling PMPI_Send genuinely
 * bypasses a tool interposed between this library and the implementation. Routing
 * both ABI names to one slot would bypass only the ABI-level profiling layer.
 *
 * MPIWRAPPER_HAVE_PMPI_SEND is a configure-time probe, because the shifted names are
 * not reliably in libmpi: MPICH can place them in a separate libpmpich (PMPILIBNAME)
 * and Open MPI can compile the profiling layer separately. Where the symbol is
 * absent this falls back to the unshifted body for that one function, which is
 * exactly the cheaper behaviour -- so the fallback is a degradation, not a failure.
 */
#if MPIWRAPPER_HAVE_PMPI_SEND
static int w_PMPI_Send(const void *abi_buf, int abi_count,
                       MPIABI_Datatype abi_datatype, int abi_dest, int abi_tag,
                       MPIABI_Comm abi_comm)
    BODY_MPI_Send(PMPI_Send)
#else
#  define w_PMPI_Send w_MPI_Send
#endif

/* ---------------------------------------------------------------- MPI_Waitall */

/* Arrays: staged into temporaries, never converted in place. The request array is
 * writable and MPI_Waitall does write to it, so in-place would even be tempting
 * here -- but see NOTES.md 5.7 for why the uniform rule is worth more than the
 * saved allocation.
 */
static int w_MPI_Waitall(int abi_count, MPIABI_Request *abi_requests,
                         MPIABI_Status *abi_statuses)
{
  const int count = abi_count;
  if (count < 0) return MPIABI_ERR_COUNT; /* before any allocation */

  /* MPI_STATUSES_IGNORE is NULL in the ABI and (MPI_Status *)1 in MPICH, so the
   * translation is real -- and it must short-circuit before we allocate room for
   * `count` statuses nobody wants.
   */
  const int ignore_statuses = (abi_statuses == MPIABI_STATUSES_IGNORE);

  MPI_Request  reqstack[MPIWRAPPER_STAGE_BYTES / sizeof(MPI_Request)];
  MPI_Status   ststack[MPIWRAPPER_STAGE_BYTES / sizeof(MPI_Status)];
  MPI_Request *requests = NULL;
  MPI_Status  *statuses = NULL;
  int          ierror;

  requests = mpiwrapper_stage(reqstack, sizeof reqstack, (size_t)count,
                              sizeof *requests);
  if (!requests) return MPIABI_ERR_INTERN;

  if (!ignore_statuses) {
    statuses = mpiwrapper_stage(ststack, sizeof ststack, (size_t)count,
                                sizeof *statuses);
    if (!statuses) {
      mpiwrapper_unstage(requests, reqstack);
      return MPIABI_ERR_INTERN;
    }
  }

  for (int i = 0; i < count; ++i)
    requests[i] = mpiwrapper_request_fromabi(abi_requests[i]);

  ierror = MPI_Waitall(count, requests,
                       ignore_statuses ? MPI_STATUSES_IGNORE : statuses);

  /* Write back unconditionally, *including* on error: MPI_ERR_IN_STATUS means the
   * per-request error codes in the status array are the payload, and the request
   * array has been partially updated either way. Returning early here would be a
   * silent data-loss bug.
   */
  for (int i = 0; i < count; ++i)
    abi_requests[i] = mpiwrapper_request_toabi(requests[i]);
  if (!ignore_statuses)
    for (int i = 0; i < count; ++i)
      mpiwrapper_status_toabi(&statuses[i], &abi_statuses[i]);

  mpiwrapper_unstage(statuses, ststack);
  mpiwrapper_unstage(requests, reqstack);
  return mpiwrapper_errorcode_toabi(ierror);
}

/* -------------------------------------------------------------- MPI_File_open */

/* An out-handle and a bitmask argument. */
static int w_MPI_File_open(MPIABI_Comm abi_comm, const char *abi_filename,
                           int abi_amode, MPIABI_Info abi_info,
                           MPIABI_File *abi_fh)
{
  const MPI_Comm comm     = mpiwrapper_comm_fromabi(abi_comm);
  const char    *filename = abi_filename; /* passthrough */
  const int      amode    = mpiwrapper_filemode_fromabi(abi_amode);
  const MPI_Info info     = mpiwrapper_info_fromabi(abi_info);

  /* A local of the *implementation's* type. Never `(MPI_File *)abi_fh`: on MPICH
   * that writes a 4-byte handle into an 8-byte ABI slot and leaves the upper half
   * garbage, while on Open MPI it works by accident -- so it would pass every test
   * on one implementation and corrupt on the other.
   */
  MPI_File fh;

  const int ierror = MPI_File_open(comm, filename, amode, info, &fh);

  *abi_fh = (ierror == MPI_SUCCESS) ? mpiwrapper_file_toabi(fh)
                                    : MPIABI_FILE_NULL;
  return mpiwrapper_errorcode_toabi(ierror);
}

/* ----------------------------------------------------------- MPI_Error_string */

/* An output string buffer with no length argument. The caller sized `abi_string`
 * with the *ABI's* MPI_MAX_ERROR_STRING; the implementation will write up to its
 * own. Those are 512 and 512 for MPICH and 512 and 256 for Open MPI, so today the
 * copy could go straight into the caller's array -- but staging is emitted
 * unconditionally rather than under an #if, precisely so that this path is
 * exercised on every run instead of being dead code that the first implementation
 * with a larger limit gets to try out in production.
 */
static int w_MPI_Error_string(int abi_errorcode, char *abi_string,
                             int *abi_resultlen)
{
  const int errorcode = mpiwrapper_errorcode_fromabi(abi_errorcode);

  char buf[MPI_MAX_ERROR_STRING]; /* the implementation's maximum */
  int  resultlen = 0;

  const int ierror = MPI_Error_string(errorcode, buf, &resultlen);
  if (ierror == MPI_SUCCESS) {
    /* An error string is prose, so truncation is the right answer; an identifier
     * that will be handed back to MPI is not, and MPI_Open_port, MPI_Lookup_name,
     * MPI_Info_get_nthkey and MPI_File_get_view's datarep return MPIABI_ERR_INTERN
     * instead. That choice is per-parameter and lives in the generator's named
     * (routine, parameter) table.
     */
    int n = resultlen;
    if (n < 0) n = 0;
    if (n > MPIABI_MAX_ERROR_STRING - 1) n = MPIABI_MAX_ERROR_STRING - 1;
    memcpy(abi_string, buf, (size_t)n);
    abi_string[n] = '\0';
    *abi_resultlen = n; /* so that abi_string[*abi_resultlen] == '\0' holds */
  }
  return mpiwrapper_errorcode_toabi(ierror);
}

/* ------------------------------------------------------------------- MPI_Wtime */

/* No error code to map. */
static double w_MPI_Wtime(void) { return MPI_Wtime(); }

/* --------------------------------------------------------------- the vtable */

/* Designated initializers, so a slot the generator forgets is a NULL pointer
 * rather than a shifted one -- and the generator asserts none is NULL.
 */
const struct mpiwrapper_vtable mpiwrapper_vtable_instance = {
    .MPI_Send         = w_MPI_Send,
    .PMPI_Send        = w_PMPI_Send,
    .MPI_Waitall      = w_MPI_Waitall,
    .MPI_Op_create    = mpiwrapper_op_create, /* hand-written: trampoline pool */
    .MPI_File_open    = w_MPI_File_open,
    .MPI_Error_string = w_MPI_Error_string,
    .MPI_Wtime        = w_MPI_Wtime,
    /* .MPI_Recv = w_MPI_Recv, .PMPI_Recv = w_PMPI_Recv, ... 1368 more ... */
};
