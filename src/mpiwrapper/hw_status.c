/* libmpiwrapper -- the ten functions that need an *implementation* status
 * built from an ABI one (NOTES.md #5.2, S4a; MPI_Get_count is S1's).
 *
 * These are the only ten. Everything else that touches a status either
 * receives one from the implementation -- generated, and status_toabi's job --
 * or reads and writes a named field of the caller's own ABI status without the
 * implementation being involved at all, which is the six pure ABI-side
 * accessors S3 generates. What makes this set distinct is the *in* direction:
 * the caller hands us a status the ABI's way and the implementation has to be
 * handed its own.
 *
 * That is a restore rather than a synthesis. The 20 scratch bytes of an ABI
 * status hold the implementation's own private bytes, put there by whichever
 * call filled the status, so status_fromabi reassembles exactly what the
 * implementation last wrote. There is deliberately no validity marker and no
 * fallback for a status the implementation never touched: garbage in, garbage
 * out is what the native implementation does for the same user error, and a
 * marker would cost a branch on every conversion to catch a case MPI already
 * calls erroneous.
 *
 * Four of the ten take the status as *inout* -- the three MPI_Status_set_*
 * forms and MPI_Status_set_cancelled -- so they convert back afterwards. That
 * is the one thing here that is not symmetric with the S1 body: a conversion
 * out as well as in, and the ABI status is rewritten in full rather than
 * patched, since the implementation is free to have moved any of its private
 * bytes.
 */

#include "internal.h"

/* ------------------------------------------------- the two body shapes ---- */

/* The eight query forms: status in, one converted count out. COUNT_T is int or
 * MPIABI_Count, and the MPI_UNDEFINED test is why the result cannot simply be
 * assigned -- it is a mapped integer rather than a plain one. Identical in all
 * three today, and written out because that is a property of these
 * implementations rather than of the ABI.
 */
#define BODY_STATUS_QUERY(TARGET, COUNT_T, IMPL_COUNT_T)                       \
  {                                                                            \
    MPI_Status status;                                                         \
    mpiwrapper_status_fromabi(abi_status, &status);                            \
    const MPI_Datatype datatype = mpiwrapper_datatype_fromabi(abi_datatype);   \
                                                                               \
    IMPL_COUNT_T count  = 0;                                                   \
    const int    ierror = TARGET(&status, datatype, &count);                   \
                                                                               \
    if (ierror == MPI_SUCCESS)                                                 \
      *abi_count = (count == MPI_UNDEFINED) ? (COUNT_T)MPIABI_UNDEFINED        \
                                            : (COUNT_T)count;                  \
    return mpiwrapper_errorcode_toabi(ierror);                                 \
  }

#define STUB_STATUS_QUERY                                                      \
  {                                                                            \
    (void)abi_status;                                                          \
    (void)abi_datatype;                                                        \
    (void)abi_count;                                                           \
    return MPIABI_ERR_UNSUPPORTED_OPERATION;                                   \
  }

/* The three MPI_Status_set_elements forms: status inout, count in. The count
 * is an ordinary value here rather than a mapped one -- the caller is stating
 * how many elements the status describes, and MPI_UNDEFINED is not a legal
 * answer to that question.
 */
#define BODY_STATUS_SET_ELEMENTS(TARGET, IMPL_COUNT_T)                         \
  {                                                                            \
    MPI_Status status;                                                         \
    mpiwrapper_status_fromabi(abi_status, &status);                            \
    const MPI_Datatype   datatype = mpiwrapper_datatype_fromabi(abi_datatype); \
    const IMPL_COUNT_T   count    = (IMPL_COUNT_T)abi_count;                   \
                                                                               \
    const int ierror = TARGET(&status, datatype, count);                       \
                                                                               \
    if (ierror == MPI_SUCCESS) mpiwrapper_status_toabi(&status, abi_status);   \
    return mpiwrapper_errorcode_toabi(ierror);                                 \
  }

#define STUB_STATUS_SET_ELEMENTS                                               \
  {                                                                            \
    (void)abi_status;                                                          \
    (void)abi_datatype;                                                        \
    (void)abi_count;                                                           \
    return MPIABI_ERR_UNSUPPORTED_OPERATION;                                   \
  }

/* -------------------------------------------------------- MPI_Get_count ---- */

#ifdef MPIWRAPPER_HAVE_MPI_Get_count
#  define BODY_MPI_Get_count(TARGET) BODY_STATUS_QUERY(TARGET, int, int)
#else
#  define BODY_MPI_Get_count(TARGET) STUB_STATUS_QUERY
#endif

int mpiwrapper_w_MPI_Get_count(const MPIABI_Status *abi_status,
                               MPIABI_Datatype abi_datatype, int *abi_count)
    BODY_MPI_Get_count(MPI_Get_count)
int mpiwrapper_w_PMPI_Get_count(const MPIABI_Status *abi_status,
                                MPIABI_Datatype abi_datatype, int *abi_count)
    BODY_MPI_Get_count(PMPI_Get_count)

/* ------------------------------------------------------ MPI_Get_count_c ---- */

/* NOTES.md #5.10. MPI_Get_count is the one of these three with no `_x` twin,
 * so the fallback is the small form and the answer widens rather than
 * narrowing -- exact, and needing no MPI_ERR_VALUE_TOO_LARGE arm. What it
 * cannot do is report a count above INT_MAX: the small form answers
 * MPI_UNDEFINED there, which is a legal answer and which the shared macro
 * already maps.
 */
#ifdef MPIWRAPPER_HAVE_MPI_Get_count_c
#  define BODY_MPI_Get_count_c(TARGET, FALLBACK)                               \
     BODY_STATUS_QUERY(TARGET, MPIABI_Count, MPI_Count)
#elif defined(MPIWRAPPER_HAVE_MPI_Get_count)
#  define BODY_MPI_Get_count_c(TARGET, FALLBACK)                               \
     BODY_STATUS_QUERY(FALLBACK, MPIABI_Count, int)
#else
#  define BODY_MPI_Get_count_c(TARGET, FALLBACK) STUB_STATUS_QUERY
#endif

int mpiwrapper_w_MPI_Get_count_c(const MPIABI_Status *abi_status,
                                 MPIABI_Datatype abi_datatype,
                                 MPIABI_Count *abi_count)
    BODY_MPI_Get_count_c(MPI_Get_count_c, MPI_Get_count)
int mpiwrapper_w_PMPI_Get_count_c(const MPIABI_Status *abi_status,
                                  MPIABI_Datatype abi_datatype,
                                  MPIABI_Count *abi_count)
    BODY_MPI_Get_count_c(PMPI_Get_count_c, PMPI_Get_count)

/* ----------------------------------------------------- MPI_Get_elements ---- */

#ifdef MPIWRAPPER_HAVE_MPI_Get_elements
#  define BODY_MPI_Get_elements(TARGET) BODY_STATUS_QUERY(TARGET, int, int)
#else
#  define BODY_MPI_Get_elements(TARGET) STUB_STATUS_QUERY
#endif

int mpiwrapper_w_MPI_Get_elements(const MPIABI_Status *abi_status,
                                  MPIABI_Datatype abi_datatype, int *abi_count)
    BODY_MPI_Get_elements(MPI_Get_elements)
int mpiwrapper_w_PMPI_Get_elements(const MPIABI_Status *abi_status,
                                   MPIABI_Datatype abi_datatype, int *abi_count)
    BODY_MPI_Get_elements(PMPI_Get_elements)

/* --------------------------------------------------- MPI_Get_elements_c ---- */

/* The fallback here is MPI_Get_elements_x and not MPI_Get_elements: the `_x`
 * form has answered in MPI_Count since MPI-3.0, so it is exact and leaves no
 * ceiling, where the small form would answer MPI_UNDEFINED above INT_MAX
 * (NOTES.md #5.10). Deprecated in MPI-4.1 means do not write new code against
 * it, not absent -- and this arm is only ever compiled against an
 * implementation older than that.
 */
#ifdef MPIWRAPPER_HAVE_MPI_Get_elements_c
#  define BODY_MPI_Get_elements_c(TARGET, FALLBACK)                            \
     BODY_STATUS_QUERY(TARGET, MPIABI_Count, MPI_Count)
#elif defined(MPIWRAPPER_HAVE_MPI_Get_elements_x)
#  define BODY_MPI_Get_elements_c(TARGET, FALLBACK)                            \
     BODY_STATUS_QUERY(FALLBACK, MPIABI_Count, MPI_Count)
#else
#  define BODY_MPI_Get_elements_c(TARGET, FALLBACK) STUB_STATUS_QUERY
#endif

int mpiwrapper_w_MPI_Get_elements_c(const MPIABI_Status *abi_status,
                                    MPIABI_Datatype abi_datatype,
                                    MPIABI_Count *abi_count)
    BODY_MPI_Get_elements_c(MPI_Get_elements_c, MPI_Get_elements_x)
int mpiwrapper_w_PMPI_Get_elements_c(const MPIABI_Status *abi_status,
                                     MPIABI_Datatype abi_datatype,
                                     MPIABI_Count *abi_count)
    BODY_MPI_Get_elements_c(PMPI_Get_elements_c, PMPI_Get_elements_x)

/* --------------------------------------------------- MPI_Get_elements_x ---- */

/* Deprecated in MPI-4.1 and still provided, like the other eleven the header
 * marks: deprecated means "do not write new code against it", not "absent".
 */
#ifdef MPIWRAPPER_HAVE_MPI_Get_elements_x
#  define BODY_MPI_Get_elements_x(TARGET)                                      \
     BODY_STATUS_QUERY(TARGET, MPIABI_Count, MPI_Count)
#else
#  define BODY_MPI_Get_elements_x(TARGET) STUB_STATUS_QUERY
#endif

int mpiwrapper_w_MPI_Get_elements_x(const MPIABI_Status *abi_status,
                                    MPIABI_Datatype abi_datatype,
                                    MPIABI_Count *abi_count)
    BODY_MPI_Get_elements_x(MPI_Get_elements_x)
int mpiwrapper_w_PMPI_Get_elements_x(const MPIABI_Status *abi_status,
                                     MPIABI_Datatype abi_datatype,
                                     MPIABI_Count *abi_count)
    BODY_MPI_Get_elements_x(PMPI_Get_elements_x)

/* ---------------------------------------------------- MPI_Test_cancelled ---- */

/* The one query of the ten with no datatype and no count: status in, plain
 * flag out. A flag is a plain int in both directions -- MPI fixes true as
 * non-zero and false as zero in C, so there is nothing to map.
 */
#ifdef MPIWRAPPER_HAVE_MPI_Test_cancelled
#  define BODY_MPI_Test_cancelled(TARGET)                                      \
    {                                                                          \
      MPI_Status status;                                                       \
      mpiwrapper_status_fromabi(abi_status, &status);                          \
                                                                               \
      int       flag   = 0;                                                    \
      const int ierror = TARGET(&status, &flag);                               \
                                                                               \
      if (ierror == MPI_SUCCESS) *abi_flag = flag;                             \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_Test_cancelled(TARGET)                                      \
    {                                                                          \
      (void)abi_status;                                                        \
      (void)abi_flag;                                                          \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Test_cancelled(const MPIABI_Status *abi_status,
                                    int *abi_flag)
    BODY_MPI_Test_cancelled(MPI_Test_cancelled)
int mpiwrapper_w_PMPI_Test_cancelled(const MPIABI_Status *abi_status,
                                     int *abi_flag)
    BODY_MPI_Test_cancelled(PMPI_Test_cancelled)

/* ------------------------------------------------ MPI_Status_set_cancelled ---- */

/* Status inout, flag in -- the mirror of MPI_Test_cancelled, and the reason
 * the whole set exists: a generalized request's query callback is handed an
 * ABI status by us and calls this to fill it in, so the blob has to survive
 * the trip out to the implementation and back.
 */
#ifdef MPIWRAPPER_HAVE_MPI_Status_set_cancelled
#  define BODY_MPI_Status_set_cancelled(TARGET)                                \
    {                                                                          \
      MPI_Status status;                                                       \
      mpiwrapper_status_fromabi(abi_status, &status);                          \
      const int flag = abi_flag;                                               \
                                                                               \
      const int ierror = TARGET(&status, flag);                                \
                                                                               \
      if (ierror == MPI_SUCCESS)                                               \
        mpiwrapper_status_toabi(&status, abi_status);                          \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_Status_set_cancelled(TARGET)                                \
    {                                                                          \
      (void)abi_status;                                                        \
      (void)abi_flag;                                                          \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Status_set_cancelled(MPIABI_Status *abi_status,
                                          int abi_flag)
    BODY_MPI_Status_set_cancelled(MPI_Status_set_cancelled)
int mpiwrapper_w_PMPI_Status_set_cancelled(MPIABI_Status *abi_status,
                                           int abi_flag)
    BODY_MPI_Status_set_cancelled(PMPI_Status_set_cancelled)

/* ------------------------------------------------ MPI_Status_set_elements ---- */

#ifdef MPIWRAPPER_HAVE_MPI_Status_set_elements
#  define BODY_MPI_Status_set_elements(TARGET)                                 \
     BODY_STATUS_SET_ELEMENTS(TARGET, int)
#else
#  define BODY_MPI_Status_set_elements(TARGET) STUB_STATUS_SET_ELEMENTS
#endif

int mpiwrapper_w_MPI_Status_set_elements(MPIABI_Status *abi_status,
                                         MPIABI_Datatype abi_datatype,
                                         int abi_count)
    BODY_MPI_Status_set_elements(MPI_Status_set_elements)
int mpiwrapper_w_PMPI_Status_set_elements(MPIABI_Status *abi_status,
                                          MPIABI_Datatype abi_datatype,
                                          int abi_count)
    BODY_MPI_Status_set_elements(PMPI_Status_set_elements)

/* ---------------------------------------------- MPI_Status_set_elements_c ---- */

/* MPI_Status_set_elements_x takes an MPI_Count in, so this fallback is exact
 * in the in direction too and needs no narrowing check (NOTES.md #5.10).
 */
#ifdef MPIWRAPPER_HAVE_MPI_Status_set_elements_c
#  define BODY_MPI_Status_set_elements_c(TARGET, FALLBACK)                     \
     BODY_STATUS_SET_ELEMENTS(TARGET, MPI_Count)
#elif defined(MPIWRAPPER_HAVE_MPI_Status_set_elements_x)
#  define BODY_MPI_Status_set_elements_c(TARGET, FALLBACK)                     \
     BODY_STATUS_SET_ELEMENTS(FALLBACK, MPI_Count)
#else
#  define BODY_MPI_Status_set_elements_c(TARGET, FALLBACK)                     \
     STUB_STATUS_SET_ELEMENTS
#endif

int mpiwrapper_w_MPI_Status_set_elements_c(MPIABI_Status *abi_status,
                                           MPIABI_Datatype abi_datatype,
                                           MPIABI_Count abi_count)
    BODY_MPI_Status_set_elements_c(MPI_Status_set_elements_c,
                                   MPI_Status_set_elements_x)
int mpiwrapper_w_PMPI_Status_set_elements_c(MPIABI_Status *abi_status,
                                            MPIABI_Datatype abi_datatype,
                                            MPIABI_Count abi_count)
    BODY_MPI_Status_set_elements_c(PMPI_Status_set_elements_c,
                                   PMPI_Status_set_elements_x)

/* ---------------------------------------------- MPI_Status_set_elements_x ---- */

#ifdef MPIWRAPPER_HAVE_MPI_Status_set_elements_x
#  define BODY_MPI_Status_set_elements_x(TARGET)                               \
     BODY_STATUS_SET_ELEMENTS(TARGET, MPI_Count)
#else
#  define BODY_MPI_Status_set_elements_x(TARGET) STUB_STATUS_SET_ELEMENTS
#endif

int mpiwrapper_w_MPI_Status_set_elements_x(MPIABI_Status *abi_status,
                                           MPIABI_Datatype abi_datatype,
                                           MPIABI_Count abi_count)
    BODY_MPI_Status_set_elements_x(MPI_Status_set_elements_x)
int mpiwrapper_w_PMPI_Status_set_elements_x(MPIABI_Status *abi_status,
                                            MPIABI_Datatype abi_datatype,
                                            MPIABI_Count abi_count)
    BODY_MPI_Status_set_elements_x(PMPI_Status_set_elements_x)
