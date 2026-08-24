/* libmpiwrapper -- the eight lifecycle entry points (NOTES.md #8, S1 and S4b).
 *
 * MPI_Init and MPI_Finalize are S1's and were in handwritten.c until S4b gave
 * their family a file, exactly as S4a did for the converters and the strings.
 * What S4b adds is the state they were always supposed to be keeping.
 *
 * **Why MPI_Initialized and MPI_Finalized are not forwarded.** Every other
 * entry point in this library is a conversion around a call to the
 * implementation. These two are questions about *us*, and MPI-5.0 11.4.1
 * defines them that way: MPI_INITIALIZED "returns flag = true if MPI_INIT or
 * MPI_INIT_THREAD has been called", which is a statement about a call the
 * application made -- through this library, since there is no other way in --
 * and not about whatever internal state the wrapped implementation happens to
 * be in.
 *
 * The two answers differ, and the session model is where. An application may
 * use MPI_Session_init and nothing else, never calling MPI_Init at all; the
 * standard then requires MPI_Initialized to answer false while MPI is
 * perfectly usable. An implementation that brings its world state up when a
 * session is created -- which nothing forbids, since the question it is
 * answering is its own -- would answer true. Owning the state makes the answer
 * exactly what the standard says it is: did the application call MPI_Init
 * through us, and did MPI_Finalize complete.
 *
 * It also makes the answer honest when the implementation's own MPI_Init
 * *fails*: nothing is recorded, so MPI_Initialized stays false rather than
 * reporting an initialization that did not happen.
 *
 * **The thread level is not kept here, and that is a decision.** An earlier
 * plan had the wrapper answering `provided` from a level of its own. It has
 * nothing to answer it with and nothing to cap: every shared table in this
 * library is fixed-capacity and lock-free (decision 11), so the wrapper
 * supports MPI_THREAD_MULTIPLE whenever the implementation does and degrades
 * nothing when it does not -- and MPI_Query_thread, which is the entry point
 * that reports the level afterwards, is generated and answers from the
 * implementation. A remembered copy would have no reader, so there is none.
 */

#include "internal.h"

#include <stdatomic.h>

/* Ordered rather than two flags, because the transitions are ordered:
 * MPI_Finalize is erroneous before MPI_Init, and after MPI_Finalize
 * MPI_Initialized still answers true (11.4.1 -- these two and MPI_Get_version
 * are the calls that remain legal there). So "initialized" is state >= 1 and
 * "finalized" is state == 2, and neither needs its own variable.
 */
#define MPIWRAPPER_LIFE_NONE        0
#define MPIWRAPPER_LIFE_INITIALIZED 1
#define MPIWRAPPER_LIFE_FINALIZED   2

static atomic_int lifecycle_state;

/* --------------------------------------------------------------- MPI_Init -- */

/* argc and argv pass through untouched; what makes this hand-written is the
 * state above, which no argument class describes.
 */
#define BODY_MPI_Init(TARGET)                                                  \
  {                                                                            \
    int   *const argc = abi_argc;                                              \
    char ***const argv = abi_argv;                                             \
                                                                               \
    const int ierror = TARGET(argc, argv);                                     \
    if (ierror == MPI_SUCCESS)                                                 \
      atomic_store_explicit(&lifecycle_state, MPIWRAPPER_LIFE_INITIALIZED,     \
                            memory_order_release);                             \
    return mpiwrapper_errorcode_toabi(ierror);                                 \
  }

int mpiwrapper_w_MPI_Init(int *abi_argc, char ***abi_argv) BODY_MPI_Init(MPI_Init)
int mpiwrapper_w_PMPI_Init(int *abi_argc, char ***abi_argv) BODY_MPI_Init(PMPI_Init)

/* -------------------------------------------------------- MPI_Init_thread -- */

/* The thread level is a mapped integer family like any other: Open MPI spells
 * MPI_THREAD_* as enumerators, which is one of the two cases that made
 * dev/probe_impl.py necessary, and the generated switch handles it.
 */
#ifdef MPIWRAPPER_HAVE_MPI_Init_thread
#  define BODY_MPI_Init_thread(TARGET)                                         \
    {                                                                          \
      int   *const argc     = abi_argc;                                        \
      char ***const argv    = abi_argv;                                        \
      const int    required = mpiwrapper_threadlevel_fromabi(abi_required);    \
                                                                               \
      int       provided = 0;                                                  \
      const int ierror   = TARGET(argc, argv, required, &provided);            \
      if (ierror == MPI_SUCCESS)                                               \
        atomic_store_explicit(&lifecycle_state, MPIWRAPPER_LIFE_INITIALIZED,   \
                              memory_order_release);                           \
                                                                               \
      *abi_provided = mpiwrapper_threadlevel_toabi(provided);                  \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_Init_thread(TARGET)                                         \
    {                                                                          \
      (void)abi_argc;                                                          \
      (void)abi_argv;                                                          \
      (void)abi_required;                                                      \
      (void)abi_provided;                                                      \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Init_thread(int *abi_argc, char ***abi_argv,
                                 int abi_required, int *abi_provided)
    BODY_MPI_Init_thread(MPI_Init_thread)
int mpiwrapper_w_PMPI_Init_thread(int *abi_argc, char ***abi_argv,
                                  int abi_required, int *abi_provided)
    BODY_MPI_Init_thread(PMPI_Init_thread)

/* ----------------------------------------------------------- MPI_Finalize -- */

#define BODY_MPI_Finalize(TARGET)                                              \
  {                                                                            \
    const int ierror = TARGET();                                               \
    if (ierror == MPI_SUCCESS)                                                 \
      atomic_store_explicit(&lifecycle_state, MPIWRAPPER_LIFE_FINALIZED,       \
                            memory_order_release);                             \
    return mpiwrapper_errorcode_toabi(ierror);                                 \
  }

int mpiwrapper_w_MPI_Finalize(void) BODY_MPI_Finalize(MPI_Finalize)
int mpiwrapper_w_PMPI_Finalize(void) BODY_MPI_Finalize(PMPI_Finalize)

/* -------------------------------------- MPI_Initialized, MPI_Finalized ----- */

/* No MPIWRAPPER_HAVE_ guard on either, for the reason the generated status
 * accessors have none (#5.2): the body never reaches the implementation, so
 * decision 6's stub would replace a correct answer with an error. These two
 * are also legal before MPI_Init and after MPI_Finalize, which is exactly
 * where forwarding them would be least safe.
 */
#define BODY_MPI_Initialized(TARGET)                                           \
  {                                                                            \
    *abi_flag = atomic_load_explicit(&lifecycle_state, memory_order_acquire)   \
                != MPIWRAPPER_LIFE_NONE;                                       \
    return MPIABI_SUCCESS;                                                     \
  }

int mpiwrapper_w_MPI_Initialized(int *abi_flag)
    BODY_MPI_Initialized(MPI_Initialized)
int mpiwrapper_w_PMPI_Initialized(int *abi_flag)
    BODY_MPI_Initialized(PMPI_Initialized)

/* ---------------------------------------------------- MPI_Get_version ---- */

/* **Reports the version of the standard this library presents, which is the
 * ABI's, not the wrapped implementation's** (decision 24). gen/include/mpi.h
 * says MPI_VERSION 5 and MPI_SUBVERSION 0, and MPI-5.0 2.7 requires the macros
 * and this call to agree -- an application that finds them disagreeing has been
 * told its library is older than the header it compiled against, which for a
 * wrapper is not merely surprising but unactionable: the surface really is
 * complete MPI-5.0, and what varies is which parts of it answer
 * MPI_ERR_UNSUPPORTED_OPERATION. Decision 3 makes that a *run-time* discovery
 * by error code rather than a version comparison, and forwarding put a second,
 * contradictory answer in front of it.
 *
 * Nothing is lost: MPI_Get_library_version still forwards, and every
 * implementation's banner names its own version there. mpif's
 * mpif_check_environment reads exactly these two and aborted the process over a
 * wrapped MPICH 4.3.1 -- eight of its tests, and the reason this changed.
 *
 * **The call to the implementation stays, and must.** It looks like dead code
 * and is not: src/mpi_abi/bootstrap.c makes this call, through the vtable,
 * as the behavioural half of the isolation check (NOTES.md #2). That check
 * works by seeing whether the wrapper's *outward* call comes back into
 * libmpi_abi, so a body that returned two constants without calling anything
 * would leave the probe with nothing to detect and silently retire the one
 * check that catches macOS weak-definition capture (HISTORY.md #2.3). The MPI_
 * slot calls MPI_Get_version and the PMPI_ slot PMPI_Get_version, per decision
 * 7, because it is the unshifted name that gets captured.
 *
 * Its answer is discarded; its error code is not, so an implementation that
 * somehow fails here still says so.
 */
#define BODY_MPI_Get_version(TARGET)                                           \
  {                                                                            \
    int impl_version    = 0;                                                   \
    int impl_subversion = 0;                                                   \
    const int ierror = TARGET(&impl_version, &impl_subversion);                \
    if (ierror != MPI_SUCCESS) return mpiwrapper_errorcode_toabi(ierror);      \
                                                                               \
    *abi_version    = MPIABI_VERSION;                                          \
    *abi_subversion = MPIABI_SUBVERSION;                                       \
    return MPIABI_SUCCESS;                                                     \
  }

int mpiwrapper_w_MPI_Get_version(int *abi_version, int *abi_subversion)
    BODY_MPI_Get_version(MPI_Get_version)
int mpiwrapper_w_PMPI_Get_version(int *abi_version, int *abi_subversion)
    BODY_MPI_Get_version(PMPI_Get_version)

#define BODY_MPI_Finalized(TARGET)                                             \
  {                                                                            \
    *abi_flag = atomic_load_explicit(&lifecycle_state, memory_order_acquire)   \
                == MPIWRAPPER_LIFE_FINALIZED;                                  \
    return MPIABI_SUCCESS;                                                     \
  }

int mpiwrapper_w_MPI_Finalized(int *abi_flag) BODY_MPI_Finalized(MPI_Finalized)
int mpiwrapper_w_PMPI_Finalized(int *abi_flag)
    BODY_MPI_Finalized(PMPI_Finalized)

/* -------------------------------------------------------------- MPI_Abort -- */

/* Mechanical, and hand-written only because #8 groups it with the lifecycle:
 * a communicator and an error code, both of which have conversion rules
 * already. The error code is an *in*-direction one, so it goes down through
 * the fromabi direction -- including a dynamic code the application obtained
 * from MPI_Add_error_class, which errorcodes.c resolves.
 */
#ifdef MPIWRAPPER_HAVE_MPI_Abort
#  define BODY_MPI_Abort(TARGET)                                               \
    {                                                                          \
      const MPI_Comm comm      = mpiwrapper_comm_fromabi(abi_comm);            \
      const int      errorcode = mpiwrapper_errorcode_fromabi(abi_errorcode);  \
                                                                               \
      const int ierror = TARGET(comm, errorcode);                              \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_Abort(TARGET)                                               \
    {                                                                          \
      (void)abi_comm;                                                          \
      (void)abi_errorcode;                                                     \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Abort(MPIABI_Comm abi_comm, int abi_errorcode)
    BODY_MPI_Abort(MPI_Abort)
int mpiwrapper_w_PMPI_Abort(MPIABI_Comm abi_comm, int abi_errorcode)
    BODY_MPI_Abort(PMPI_Abort)

/* ------------------------------------- MPI_Session_init, _finalize -------- */

/* The session pair is mechanical too, and here for the same reason: #8's
 * lifecycle bullet is eight entry points and these are two of them. They do
 * *not* touch the state above, and that is the point made at the top of this
 * file -- a session is not the world model, and MPI_Initialized must keep
 * answering false for a program that only ever uses sessions.
 */
#if defined(MPIWRAPPER_HAVE_MPI_Session_init) &&                               \
    defined(MPIWRAPPER_HAVE_MPI_SESSION_NULL)
#  define BODY_MPI_Session_init(TARGET)                                        \
    {                                                                          \
      const MPI_Info       info       = mpiwrapper_info_fromabi(abi_info);     \
      const MPI_Errhandler errhandler =                                        \
          mpiwrapper_errhandler_fromabi(abi_errhandler);                       \
                                                                               \
      MPI_Session session;                                                     \
      const int   ierror = TARGET(info, errhandler, &session);                 \
                                                                               \
      *abi_session = (ierror == MPI_SUCCESS)                                   \
                         ? mpiwrapper_session_toabi(session)                   \
                         : MPIABI_SESSION_NULL;                                \
      if (mpiwrapper_take_handle_error()) return MPIABI_ERR_INTERN;            \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_Session_init(TARGET)                                        \
    {                                                                          \
      (void)abi_info;                                                          \
      (void)abi_errhandler;                                                    \
      *abi_session = MPIABI_SESSION_NULL;                                      \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Session_init(MPIABI_Info abi_info,
                                  MPIABI_Errhandler abi_errhandler,
                                  MPIABI_Session   *abi_session)
    BODY_MPI_Session_init(MPI_Session_init)
int mpiwrapper_w_PMPI_Session_init(MPIABI_Info abi_info,
                                   MPIABI_Errhandler abi_errhandler,
                                   MPIABI_Session   *abi_session)
    BODY_MPI_Session_init(PMPI_Session_init)

#if defined(MPIWRAPPER_HAVE_MPI_Session_finalize) &&                           \
    defined(MPIWRAPPER_HAVE_MPI_SESSION_NULL)
#  define BODY_MPI_Session_finalize(TARGET)                                    \
    {                                                                          \
      MPI_Session session = mpiwrapper_session_fromabi(*abi_session);          \
                                                                               \
      const int ierror = TARGET(&session);                                     \
      *abi_session     = mpiwrapper_session_toabi(session);                    \
      if (mpiwrapper_take_handle_error()) return MPIABI_ERR_INTERN;            \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_Session_finalize(TARGET)                                    \
    {                                                                          \
      (void)abi_session;                                                       \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Session_finalize(MPIABI_Session *abi_session)
    BODY_MPI_Session_finalize(MPI_Session_finalize)
int mpiwrapper_w_PMPI_Session_finalize(MPIABI_Session *abi_session)
    BODY_MPI_Session_finalize(PMPI_Session_finalize)
