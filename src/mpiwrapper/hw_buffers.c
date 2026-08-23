/* libmpiwrapper -- the twelve buffer attach and detach forms (NOTES.md #8,
 * S4b).
 *
 * Eighteen entry points make up MPI-4.1's buffering chapter and only these
 * twelve need judgement; the six flush forms are mechanical and the generator
 * emits them. What the twelve share is one sentinel and one question of
 * ownership.
 *
 * **The sentinel.** MPI_BUFFER_AUTOMATIC is (void *)2 in the ABI and
 * (void *)-2 in MPICH, so it is a #5.3 sentinel like MPI_BOTTOM: recognized on
 * the way in and produced on the way out, one test per site. It is also the
 * one sentinel here that an implementation may not *have* at all -- it is
 * MPI-4.1, and Open MPI 5.0.6 predates it -- which is what buffers.c exists
 * for: where the implementation cannot express "MPI, provide the buffer", the
 * wrapper attaches a buffer of its own and remembers that it did.
 *
 * **The ownership.** Once the wrapper may have substituted a buffer, the
 * detach side can no longer pass the implementation's answer through: it must
 * hand back MPI_BUFFER_AUTOMATIC, because that is what the application
 * attached, and free the block. Both halves of that are one call into
 * buffers.c, and neither is reachable where the implementation has the mode
 * itself.
 *
 * **What crosses unconverted.** An ordinary attached buffer is the
 * application's own memory and its address means the same thing on both sides,
 * so nothing but the sentinel test happens to it. The size is a plain count of
 * bytes.
 */

#include "internal.h"

/* The three scopes' keys. A communicator or session key is the implementation
 * handle's bits, formed with the same macro as everywhere else.
 */
#define MPIWRAPPER_AUTOBUF_COMM(comm)       MPIWRAPPER_BITS(comm)
#define MPIWRAPPER_AUTOBUF_SESSION(session) MPIWRAPPER_BITS(session)

/* The two directions of the sentinel, in the two configurations. Where the
 * implementation has MPI_BUFFER_AUTOMATIC this is a translation and nothing
 * more; where it does not, the same two macros are the whole of the
 * emulation, so the bodies below read identically either way.
 */
#ifdef MPIWRAPPER_HAVE_MPI_BUFFER_AUTOMATIC
#  define MPIWRAPPER_AUTOBUF_IN(SCOPE, BUF, SIZE, SIZE_T)                      \
    do {                                                                       \
      (BUF) = MPI_BUFFER_AUTOMATIC;                                            \
    } while (0)
#  define MPIWRAPPER_AUTOBUF_UNDO(SCOPE)                                       \
    do {                                                                       \
    } while (0)
#  define MPIWRAPPER_AUTOBUF_OUT(SCOPE, ADDR)                                  \
    ((ADDR) == MPI_BUFFER_AUTOMATIC ? MPIABI_BUFFER_AUTOMATIC : (ADDR))
#else
/* The emulation. `bytes` comes back from buffers.c rather than from the
 * caller, whose own size argument the standard says is irrelevant here.
 */
#  define MPIWRAPPER_AUTOBUF_IN(SCOPE, BUF, SIZE, SIZE_T)                      \
    do {                                                                       \
      size_t bytes = 0;                                                        \
      (BUF)        = mpiwrapper_autobuf_claim((SCOPE), &bytes);                \
      if (!(BUF)) return MPIABI_ERR_INTERN;                                    \
      (SIZE) = (SIZE_T)bytes;                                                  \
    } while (0)
#  define MPIWRAPPER_AUTOBUF_UNDO(SCOPE)                                       \
    do {                                                                       \
      (void)mpiwrapper_autobuf_release(SCOPE);                                 \
    } while (0)
/* Ours or the application's? The record answers, and releasing it is also how
 * the block is freed -- which is safe exactly here, since MPI-5.0 3.6 makes
 * the detach wait until every buffered message has been transmitted.
 */
#  define MPIWRAPPER_AUTOBUF_OUT(SCOPE, ADDR)                                  \
    (mpiwrapper_autobuf_release(SCOPE) ? MPIABI_BUFFER_AUTOMATIC : (ADDR))
#endif

/* ------------------------------------------------ the process-wide buffer -- */

#ifdef MPIWRAPPER_HAVE_MPI_Buffer_attach
#  define BODY_MPI_Buffer_attach(TARGET)                                       \
    {                                                                          \
      void *buffer = abi_buffer;                                               \
      int   size   = abi_size;                                                 \
      if (abi_buffer == MPIABI_BUFFER_AUTOMATIC)                               \
        MPIWRAPPER_AUTOBUF_IN(MPIWRAPPER_AUTOBUF_PROCESS, buffer, size, int);  \
                                                                               \
      const int ierror = TARGET(buffer, size);                                 \
      if (ierror != MPI_SUCCESS && abi_buffer == MPIABI_BUFFER_AUTOMATIC)      \
        MPIWRAPPER_AUTOBUF_UNDO(MPIWRAPPER_AUTOBUF_PROCESS);                   \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_Buffer_attach(TARGET)                                       \
    {                                                                          \
      (void)abi_buffer;                                                        \
      (void)abi_size;                                                          \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Buffer_attach(void *abi_buffer, int abi_size)
    BODY_MPI_Buffer_attach(MPI_Buffer_attach)
int mpiwrapper_w_PMPI_Buffer_attach(void *abi_buffer, int abi_size)
    BODY_MPI_Buffer_attach(PMPI_Buffer_attach)

#ifdef MPIWRAPPER_HAVE_MPI_Buffer_attach_c
#  define BODY_MPI_Buffer_attach_c(TARGET, FALLBACK)                           \
    {                                                                          \
      void     *buffer = abi_buffer;                                           \
      MPI_Count size   = abi_size;                                             \
      if (abi_buffer == MPIABI_BUFFER_AUTOMATIC)                               \
        MPIWRAPPER_AUTOBUF_IN(MPIWRAPPER_AUTOBUF_PROCESS, buffer, size,        \
                              MPI_Count);                                      \
                                                                               \
      const int ierror = TARGET(buffer, size);                                 \
      if (ierror != MPI_SUCCESS && abi_buffer == MPIABI_BUFFER_AUTOMATIC)      \
        MPIWRAPPER_AUTOBUF_UNDO(MPIWRAPPER_AUTOBUF_PROCESS);                   \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#elif defined(MPIWRAPPER_HAVE_MPI_Buffer_attach)
#  define BODY_MPI_Buffer_attach_c(TARGET, FALLBACK)                           \
    {                                                                          \
      void          *buffer = abi_buffer;                                      \
      int            size   = 0;                                               \
      /* MPI_BUFFER_AUTOMATIC ignores the caller's size, so the narrowing      \
       * check belongs on the other arm of this branch and not before it:      \
       * refusing a size the standard says is irrelevant would reject a legal  \
       * call (NOTES.md #5.10).                                                \
       */                                                                      \
      if (abi_buffer == MPIABI_BUFFER_AUTOMATIC)                               \
        MPIWRAPPER_AUTOBUF_IN(MPIWRAPPER_AUTOBUF_PROCESS, buffer, size, int);  \
      else if (!mpiwrapper_narrow_int(abi_size, &size))                        \
        return MPIABI_ERR_VALUE_TOO_LARGE;                                     \
                                                                               \
      const int ierror = FALLBACK(buffer, size);                               \
      if (ierror != MPI_SUCCESS && abi_buffer == MPIABI_BUFFER_AUTOMATIC)      \
        MPIWRAPPER_AUTOBUF_UNDO(MPIWRAPPER_AUTOBUF_PROCESS);                   \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_Buffer_attach_c(TARGET, FALLBACK)                           \
    {                                                                          \
      (void)abi_buffer;                                                        \
      (void)abi_size;                                                          \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Buffer_attach_c(void *abi_buffer, MPIABI_Count abi_size)
    BODY_MPI_Buffer_attach_c(MPI_Buffer_attach_c, MPI_Buffer_attach)
int mpiwrapper_w_PMPI_Buffer_attach_c(void *abi_buffer, MPIABI_Count abi_size)
    BODY_MPI_Buffer_attach_c(PMPI_Buffer_attach_c, PMPI_Buffer_attach)

/* `buffer_addr` is declared `void *` and is really a `void **` -- the standard's
 * own C binding, and the one place in this file where the caller's pointer is
 * written through rather than passed on. It goes through a local because the
 * address coming back may be ours rather than the caller's.
 */
#ifdef MPIWRAPPER_HAVE_MPI_Buffer_detach
#  define BODY_MPI_Buffer_detach(TARGET)                                       \
    {                                                                          \
      void *buffer_addr = NULL;                                                \
      int   size        = 0;                                                   \
                                                                               \
      const int ierror = TARGET(&buffer_addr, &size);                          \
      if (ierror != MPI_SUCCESS) return mpiwrapper_errorcode_toabi(ierror);    \
                                                                               \
      *(void **)abi_buffer_addr =                                              \
          MPIWRAPPER_AUTOBUF_OUT(MPIWRAPPER_AUTOBUF_PROCESS, buffer_addr);     \
      *abi_size = size;                                                        \
      return MPIABI_SUCCESS;                                                   \
    }
#else
#  define BODY_MPI_Buffer_detach(TARGET)                                       \
    {                                                                          \
      (void)abi_buffer_addr;                                                   \
      (void)abi_size;                                                          \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Buffer_detach(void *abi_buffer_addr, int *abi_size)
    BODY_MPI_Buffer_detach(MPI_Buffer_detach)
int mpiwrapper_w_PMPI_Buffer_detach(void *abi_buffer_addr, int *abi_size)
    BODY_MPI_Buffer_detach(PMPI_Buffer_detach)

#ifdef MPIWRAPPER_HAVE_MPI_Buffer_detach_c
#  define BODY_MPI_Buffer_detach_c(TARGET, FALLBACK)                           \
    {                                                                          \
      void     *buffer_addr = NULL;                                            \
      MPI_Count size        = 0;                                               \
                                                                               \
      const int ierror = TARGET(&buffer_addr, &size);                          \
      if (ierror != MPI_SUCCESS) return mpiwrapper_errorcode_toabi(ierror);    \
                                                                               \
      *(void **)abi_buffer_addr =                                              \
          MPIWRAPPER_AUTOBUF_OUT(MPIWRAPPER_AUTOBUF_PROCESS, buffer_addr);     \
      *abi_size = (MPIABI_Count)size;                                          \
      return MPIABI_SUCCESS;                                                   \
    }
#elif defined(MPIWRAPPER_HAVE_MPI_Buffer_detach)
#  define BODY_MPI_Buffer_detach_c(TARGET, FALLBACK)                           \
    {                                                                          \
      void *buffer_addr = NULL;                                                \
      int   size        = 0;                                                   \
                                                                               \
      const int ierror = FALLBACK(&buffer_addr, &size);                        \
      if (ierror != MPI_SUCCESS) return mpiwrapper_errorcode_toabi(ierror);    \
                                                                               \
      /* The small form reports an int, so this widens and cannot lose. */     \
      *(void **)abi_buffer_addr =                                              \
          MPIWRAPPER_AUTOBUF_OUT(MPIWRAPPER_AUTOBUF_PROCESS, buffer_addr);     \
      *abi_size = (MPIABI_Count)size;                                          \
      return MPIABI_SUCCESS;                                                   \
    }
#else
#  define BODY_MPI_Buffer_detach_c(TARGET, FALLBACK)                           \
    {                                                                          \
      (void)abi_buffer_addr;                                                   \
      (void)abi_size;                                                          \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Buffer_detach_c(void *abi_buffer_addr,
                                     MPIABI_Count *abi_size)
    BODY_MPI_Buffer_detach_c(MPI_Buffer_detach_c, MPI_Buffer_detach)
int mpiwrapper_w_PMPI_Buffer_detach_c(void *abi_buffer_addr,
                                      MPIABI_Count *abi_size)
    BODY_MPI_Buffer_detach_c(PMPI_Buffer_detach_c, PMPI_Buffer_detach)

/* -------------------------------------------- the communicator's buffer ---- */

#ifdef MPIWRAPPER_HAVE_MPI_Comm_attach_buffer
#  define BODY_MPI_Comm_attach_buffer(TARGET)                                  \
    {                                                                          \
      const MPI_Comm comm   = mpiwrapper_comm_fromabi(abi_comm);               \
      void          *buffer = abi_buffer;                                      \
      int            size   = abi_size;                                        \
      if (abi_buffer == MPIABI_BUFFER_AUTOMATIC)                               \
        MPIWRAPPER_AUTOBUF_IN(MPIWRAPPER_AUTOBUF_COMM(comm), buffer, size,     \
                              int);                                            \
                                                                               \
      const int ierror = TARGET(comm, buffer, size);                           \
      if (ierror != MPI_SUCCESS && abi_buffer == MPIABI_BUFFER_AUTOMATIC)      \
        MPIWRAPPER_AUTOBUF_UNDO(MPIWRAPPER_AUTOBUF_COMM(comm));                \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_Comm_attach_buffer(TARGET)                                  \
    {                                                                          \
      (void)abi_comm;                                                          \
      (void)abi_buffer;                                                        \
      (void)abi_size;                                                          \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Comm_attach_buffer(MPIABI_Comm abi_comm, void *abi_buffer,
                                        int abi_size)
    BODY_MPI_Comm_attach_buffer(MPI_Comm_attach_buffer)
int mpiwrapper_w_PMPI_Comm_attach_buffer(MPIABI_Comm abi_comm, void *abi_buffer,
                                         int abi_size)
    BODY_MPI_Comm_attach_buffer(PMPI_Comm_attach_buffer)

#ifdef MPIWRAPPER_HAVE_MPI_Comm_attach_buffer_c
#  define BODY_MPI_Comm_attach_buffer_c(TARGET, FALLBACK)                      \
    {                                                                          \
      const MPI_Comm comm   = mpiwrapper_comm_fromabi(abi_comm);               \
      void          *buffer = abi_buffer;                                      \
      MPI_Count      size   = abi_size;                                        \
      if (abi_buffer == MPIABI_BUFFER_AUTOMATIC)                               \
        MPIWRAPPER_AUTOBUF_IN(MPIWRAPPER_AUTOBUF_COMM(comm), buffer, size,     \
                              MPI_Count);                                      \
                                                                               \
      const int ierror = TARGET(comm, buffer, size);                           \
      if (ierror != MPI_SUCCESS && abi_buffer == MPIABI_BUFFER_AUTOMATIC)      \
        MPIWRAPPER_AUTOBUF_UNDO(MPIWRAPPER_AUTOBUF_COMM(comm));                \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#elif defined(MPIWRAPPER_HAVE_MPI_Comm_attach_buffer)
#  define BODY_MPI_Comm_attach_buffer_c(TARGET, FALLBACK)                      \
    {                                                                          \
      const MPI_Comm comm   = mpiwrapper_comm_fromabi(abi_comm);               \
      void          *buffer = abi_buffer;                                      \
      int            size   = 0;                                               \
      /* MPI_BUFFER_AUTOMATIC ignores the caller's size, so the narrowing      \
       * check belongs on the other arm of this branch and not before it:      \
       * refusing a size the standard says is irrelevant would reject a legal  \
       * call (NOTES.md #5.10).                                                \
       */                                                                      \
      if (abi_buffer == MPIABI_BUFFER_AUTOMATIC)                               \
        MPIWRAPPER_AUTOBUF_IN(MPIWRAPPER_AUTOBUF_COMM(comm), buffer, size,     \
                              int);                                            \
      else if (!mpiwrapper_narrow_int(abi_size, &size))                        \
        return MPIABI_ERR_VALUE_TOO_LARGE;                                     \
                                                                               \
      const int ierror = FALLBACK(comm, buffer, size);                         \
      if (ierror != MPI_SUCCESS && abi_buffer == MPIABI_BUFFER_AUTOMATIC)      \
        MPIWRAPPER_AUTOBUF_UNDO(MPIWRAPPER_AUTOBUF_COMM(comm));                \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_Comm_attach_buffer_c(TARGET, FALLBACK)                      \
    {                                                                          \
      (void)abi_comm;                                                          \
      (void)abi_buffer;                                                        \
      (void)abi_size;                                                          \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Comm_attach_buffer_c(MPIABI_Comm abi_comm,
                                          void *abi_buffer,
                                          MPIABI_Count abi_size)
    BODY_MPI_Comm_attach_buffer_c(MPI_Comm_attach_buffer_c,
                                  MPI_Comm_attach_buffer)
int mpiwrapper_w_PMPI_Comm_attach_buffer_c(MPIABI_Comm abi_comm,
                                           void *abi_buffer,
                                           MPIABI_Count abi_size)
    BODY_MPI_Comm_attach_buffer_c(PMPI_Comm_attach_buffer_c,
                                  PMPI_Comm_attach_buffer)

#ifdef MPIWRAPPER_HAVE_MPI_Comm_detach_buffer
#  define BODY_MPI_Comm_detach_buffer(TARGET)                                  \
    {                                                                          \
      const MPI_Comm comm        = mpiwrapper_comm_fromabi(abi_comm);          \
      void          *buffer_addr = NULL;                                       \
      int            size        = 0;                                          \
                                                                               \
      const int ierror = TARGET(comm, &buffer_addr, &size);                    \
      if (ierror != MPI_SUCCESS) return mpiwrapper_errorcode_toabi(ierror);    \
                                                                               \
      *(void **)abi_buffer_addr =                                              \
          MPIWRAPPER_AUTOBUF_OUT(MPIWRAPPER_AUTOBUF_COMM(comm), buffer_addr);  \
      *abi_size = size;                                                        \
      return MPIABI_SUCCESS;                                                   \
    }
#else
#  define BODY_MPI_Comm_detach_buffer(TARGET)                                  \
    {                                                                          \
      (void)abi_comm;                                                          \
      (void)abi_buffer_addr;                                                   \
      (void)abi_size;                                                          \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Comm_detach_buffer(MPIABI_Comm abi_comm,
                                        void *abi_buffer_addr, int *abi_size)
    BODY_MPI_Comm_detach_buffer(MPI_Comm_detach_buffer)
int mpiwrapper_w_PMPI_Comm_detach_buffer(MPIABI_Comm abi_comm,
                                         void *abi_buffer_addr, int *abi_size)
    BODY_MPI_Comm_detach_buffer(PMPI_Comm_detach_buffer)

#ifdef MPIWRAPPER_HAVE_MPI_Comm_detach_buffer_c
#  define BODY_MPI_Comm_detach_buffer_c(TARGET, FALLBACK)                      \
    {                                                                          \
      const MPI_Comm comm        = mpiwrapper_comm_fromabi(abi_comm);          \
      void          *buffer_addr = NULL;                                       \
      MPI_Count      size        = 0;                                          \
                                                                               \
      const int ierror = TARGET(comm, &buffer_addr, &size);                    \
      if (ierror != MPI_SUCCESS) return mpiwrapper_errorcode_toabi(ierror);    \
                                                                               \
      *(void **)abi_buffer_addr =                                              \
          MPIWRAPPER_AUTOBUF_OUT(MPIWRAPPER_AUTOBUF_COMM(comm), buffer_addr);  \
      *abi_size = (MPIABI_Count)size;                                          \
      return MPIABI_SUCCESS;                                                   \
    }
#elif defined(MPIWRAPPER_HAVE_MPI_Comm_detach_buffer)
#  define BODY_MPI_Comm_detach_buffer_c(TARGET, FALLBACK)                      \
    {                                                                          \
      const MPI_Comm comm = mpiwrapper_comm_fromabi(abi_comm);                 \
      void *buffer_addr = NULL;                                                \
      int   size        = 0;                                                   \
                                                                               \
      const int ierror = FALLBACK(comm, &buffer_addr, &size);                  \
      if (ierror != MPI_SUCCESS) return mpiwrapper_errorcode_toabi(ierror);    \
                                                                               \
      /* The small form reports an int, so this widens and cannot lose. */     \
      *(void **)abi_buffer_addr =                                              \
          MPIWRAPPER_AUTOBUF_OUT(MPIWRAPPER_AUTOBUF_COMM(comm), buffer_addr);  \
      *abi_size = (MPIABI_Count)size;                                          \
      return MPIABI_SUCCESS;                                                   \
    }
#else
#  define BODY_MPI_Comm_detach_buffer_c(TARGET, FALLBACK)                      \
    {                                                                          \
      (void)abi_comm;                                                          \
      (void)abi_buffer_addr;                                                   \
      (void)abi_size;                                                          \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Comm_detach_buffer_c(MPIABI_Comm abi_comm,
                                          void         *abi_buffer_addr,
                                          MPIABI_Count *abi_size)
    BODY_MPI_Comm_detach_buffer_c(MPI_Comm_detach_buffer_c,
                                  MPI_Comm_detach_buffer)
int mpiwrapper_w_PMPI_Comm_detach_buffer_c(MPIABI_Comm abi_comm,
                                           void         *abi_buffer_addr,
                                           MPIABI_Count *abi_size)
    BODY_MPI_Comm_detach_buffer_c(PMPI_Comm_detach_buffer_c,
                                  PMPI_Comm_detach_buffer)

/* ------------------------------------------------ the session's buffer ----- */

#if defined(MPIWRAPPER_HAVE_MPI_Session_attach_buffer) &&                      \
    defined(MPIWRAPPER_HAVE_MPI_SESSION_NULL)
#  define BODY_MPI_Session_attach_buffer(TARGET)                               \
    {                                                                          \
      const MPI_Session session = mpiwrapper_session_fromabi(abi_session);     \
      void             *buffer  = abi_buffer;                                  \
      int               size    = abi_size;                                    \
      if (abi_buffer == MPIABI_BUFFER_AUTOMATIC)                               \
        MPIWRAPPER_AUTOBUF_IN(MPIWRAPPER_AUTOBUF_SESSION(session), buffer,     \
                              size, int);                                      \
                                                                               \
      const int ierror = TARGET(session, buffer, size);                        \
      if (ierror != MPI_SUCCESS && abi_buffer == MPIABI_BUFFER_AUTOMATIC)      \
        MPIWRAPPER_AUTOBUF_UNDO(MPIWRAPPER_AUTOBUF_SESSION(session));          \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_Session_attach_buffer(TARGET)                               \
    {                                                                          \
      (void)abi_session;                                                       \
      (void)abi_buffer;                                                        \
      (void)abi_size;                                                          \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Session_attach_buffer(MPIABI_Session abi_session,
                                           void *abi_buffer, int abi_size)
    BODY_MPI_Session_attach_buffer(MPI_Session_attach_buffer)
int mpiwrapper_w_PMPI_Session_attach_buffer(MPIABI_Session abi_session,
                                            void *abi_buffer, int abi_size)
    BODY_MPI_Session_attach_buffer(PMPI_Session_attach_buffer)

#if defined(MPIWRAPPER_HAVE_MPI_Session_attach_buffer_c) &&                    \
    defined(MPIWRAPPER_HAVE_MPI_SESSION_NULL)
#  define BODY_MPI_Session_attach_buffer_c(TARGET, FALLBACK)                   \
    {                                                                          \
      const MPI_Session session = mpiwrapper_session_fromabi(abi_session);     \
      void             *buffer  = abi_buffer;                                  \
      MPI_Count         size    = abi_size;                                    \
      if (abi_buffer == MPIABI_BUFFER_AUTOMATIC)                               \
        MPIWRAPPER_AUTOBUF_IN(MPIWRAPPER_AUTOBUF_SESSION(session), buffer,     \
                              size, MPI_Count);                                \
                                                                               \
      const int ierror = TARGET(session, buffer, size);                        \
      if (ierror != MPI_SUCCESS && abi_buffer == MPIABI_BUFFER_AUTOMATIC)      \
        MPIWRAPPER_AUTOBUF_UNDO(MPIWRAPPER_AUTOBUF_SESSION(session));          \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#elif defined(MPIWRAPPER_HAVE_MPI_Session_attach_buffer) &&                    \
    defined(MPIWRAPPER_HAVE_MPI_SESSION_NULL)
#  define BODY_MPI_Session_attach_buffer_c(TARGET, FALLBACK)                   \
    {                                                                          \
      const MPI_Session session =                                              \
          mpiwrapper_session_fromabi(abi_session);                             \
      void          *buffer = abi_buffer;                                      \
      int            size   = 0;                                               \
      /* MPI_BUFFER_AUTOMATIC ignores the caller's size, so the narrowing      \
       * check belongs on the other arm of this branch and not before it:      \
       * refusing a size the standard says is irrelevant would reject a legal  \
       * call (NOTES.md #5.10).                                                \
       */                                                                      \
      if (abi_buffer == MPIABI_BUFFER_AUTOMATIC)                               \
        MPIWRAPPER_AUTOBUF_IN(MPIWRAPPER_AUTOBUF_SESSION(session), buffer,     \
                              size, int);                                      \
      else if (!mpiwrapper_narrow_int(abi_size, &size))                        \
        return MPIABI_ERR_VALUE_TOO_LARGE;                                     \
                                                                               \
      const int ierror = FALLBACK(session, buffer, size);                      \
      if (ierror != MPI_SUCCESS && abi_buffer == MPIABI_BUFFER_AUTOMATIC)      \
        MPIWRAPPER_AUTOBUF_UNDO(MPIWRAPPER_AUTOBUF_SESSION(session));          \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_Session_attach_buffer_c(TARGET, FALLBACK)                   \
    {                                                                          \
      (void)abi_session;                                                       \
      (void)abi_buffer;                                                        \
      (void)abi_size;                                                          \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Session_attach_buffer_c(MPIABI_Session abi_session,
                                             void        *abi_buffer,
                                             MPIABI_Count abi_size)
    BODY_MPI_Session_attach_buffer_c(MPI_Session_attach_buffer_c,
                                     MPI_Session_attach_buffer)
int mpiwrapper_w_PMPI_Session_attach_buffer_c(MPIABI_Session abi_session,
                                              void        *abi_buffer,
                                              MPIABI_Count abi_size)
    BODY_MPI_Session_attach_buffer_c(PMPI_Session_attach_buffer_c,
                                     PMPI_Session_attach_buffer)

#if defined(MPIWRAPPER_HAVE_MPI_Session_detach_buffer) &&                      \
    defined(MPIWRAPPER_HAVE_MPI_SESSION_NULL)
#  define BODY_MPI_Session_detach_buffer(TARGET)                               \
    {                                                                          \
      const MPI_Session session     = mpiwrapper_session_fromabi(abi_session); \
      void             *buffer_addr = NULL;                                    \
      int               size        = 0;                                       \
                                                                               \
      const int ierror = TARGET(session, &buffer_addr, &size);                 \
      if (ierror != MPI_SUCCESS) return mpiwrapper_errorcode_toabi(ierror);    \
                                                                               \
      *(void **)abi_buffer_addr = MPIWRAPPER_AUTOBUF_OUT(                      \
          MPIWRAPPER_AUTOBUF_SESSION(session), buffer_addr);                   \
      *abi_size = size;                                                        \
      return MPIABI_SUCCESS;                                                   \
    }
#else
#  define BODY_MPI_Session_detach_buffer(TARGET)                               \
    {                                                                          \
      (void)abi_session;                                                       \
      (void)abi_buffer_addr;                                                   \
      (void)abi_size;                                                          \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Session_detach_buffer(MPIABI_Session abi_session,
                                           void *abi_buffer_addr,
                                           int  *abi_size)
    BODY_MPI_Session_detach_buffer(MPI_Session_detach_buffer)
int mpiwrapper_w_PMPI_Session_detach_buffer(MPIABI_Session abi_session,
                                            void *abi_buffer_addr,
                                            int  *abi_size)
    BODY_MPI_Session_detach_buffer(PMPI_Session_detach_buffer)

#if defined(MPIWRAPPER_HAVE_MPI_Session_detach_buffer_c) &&                    \
    defined(MPIWRAPPER_HAVE_MPI_SESSION_NULL)
#  define BODY_MPI_Session_detach_buffer_c(TARGET, FALLBACK)                   \
    {                                                                          \
      const MPI_Session session     = mpiwrapper_session_fromabi(abi_session); \
      void             *buffer_addr = NULL;                                    \
      MPI_Count         size        = 0;                                       \
                                                                               \
      const int ierror = TARGET(session, &buffer_addr, &size);                 \
      if (ierror != MPI_SUCCESS) return mpiwrapper_errorcode_toabi(ierror);    \
                                                                               \
      *(void **)abi_buffer_addr = MPIWRAPPER_AUTOBUF_OUT(                      \
          MPIWRAPPER_AUTOBUF_SESSION(session), buffer_addr);                   \
      *abi_size = (MPIABI_Count)size;                                          \
      return MPIABI_SUCCESS;                                                   \
    }
#elif defined(MPIWRAPPER_HAVE_MPI_Session_detach_buffer) &&                    \
    defined(MPIWRAPPER_HAVE_MPI_SESSION_NULL)
#  define BODY_MPI_Session_detach_buffer_c(TARGET, FALLBACK)                   \
    {                                                                          \
      const MPI_Session session =                                              \
          mpiwrapper_session_fromabi(abi_session);                             \
      void *buffer_addr = NULL;                                                \
      int   size        = 0;                                                   \
                                                                               \
      const int ierror = FALLBACK(session, &buffer_addr, &size);               \
      if (ierror != MPI_SUCCESS) return mpiwrapper_errorcode_toabi(ierror);    \
                                                                               \
      /* The small form reports an int, so this widens and cannot lose. */     \
      *(void **)abi_buffer_addr =                                              \
          MPIWRAPPER_AUTOBUF_OUT(MPIWRAPPER_AUTOBUF_SESSION(session),          \
                                 buffer_addr);                                 \
      *abi_size = (MPIABI_Count)size;                                          \
      return MPIABI_SUCCESS;                                                   \
    }
#else
#  define BODY_MPI_Session_detach_buffer_c(TARGET, FALLBACK)                   \
    {                                                                          \
      (void)abi_session;                                                       \
      (void)abi_buffer_addr;                                                   \
      (void)abi_size;                                                          \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Session_detach_buffer_c(MPIABI_Session abi_session,
                                             void         *abi_buffer_addr,
                                             MPIABI_Count *abi_size)
    BODY_MPI_Session_detach_buffer_c(MPI_Session_detach_buffer_c,
                                     MPI_Session_detach_buffer)
int mpiwrapper_w_PMPI_Session_detach_buffer_c(MPIABI_Session abi_session,
                                              void         *abi_buffer_addr,
                                              MPIABI_Count *abi_size)
    BODY_MPI_Session_detach_buffer_c(PMPI_Session_detach_buffer_c,
                                     PMPI_Session_detach_buffer)
