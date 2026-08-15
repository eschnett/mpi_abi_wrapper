/* libmpiwrapper -- the six dynamic error-code entry points (NOTES.md #5.6,
 * S4b).
 *
 * These are the only writers of the registry in errorcodes.c, exactly as
 * MPI_*_create_keyval are the only writers of keyvals.c. Everything else in
 * the library reads it: every `mpiwrapper_errorcode_toabi` in every generated
 * body reaches the registry through its switch's default arm, which is what
 * makes a user-defined class survive being returned from MPI_Recv,
 * MPI_Error_class or an error handler.
 *
 * The renumbering is not optional and #5.6 gives the reason: MPI_ERR_LASTCODE
 * is 16383 in the ABI and 0x3fffffff in MPICH, and MPI-5.0 9.5 puts every
 * dynamic class above the implementation's own. Passing the value through
 * would hand the application a number its own header says cannot be an error
 * code.
 *
 * **MPI_Add_error_string needs no registry of its own**, and that is worth
 * saying because #5.6 leaves it open: the string is stored by the
 * implementation against the implementation's code, and MPI_Error_string --
 * S1's body, in hw_strings.c -- converts the ABI code back down and asks for
 * it. So the two halves meet in the implementation rather than here.
 *
 * **The three MPI_Remove_error_* forms leave the registry alone.** Removing a
 * class invalidates the implementation's code, not our record of what it once
 * meant, and an implementation may hand the same number out again afterwards
 * -- which is why the reverse direction scans newest-first (errorcodes.c). A
 * removal that also removed the entry would make an ABI code that the
 * application still holds untranslatable, where leaving it makes the
 * implementation reject it, which is what the standard says should happen.
 */

#include "internal.h"

/* --------------------------------------------------- MPI_Add_error_class -- */

/* On failure the out parameter is left alone: MPI defines no value for it, and
 * writing an invented one would be a class the application could go on to use.
 */
#ifdef MPIWRAPPER_HAVE_MPI_Add_error_class
#  define BODY_MPI_Add_error_class(TARGET)                                     \
    {                                                                          \
      int       errorclass = 0;                                                \
      const int ierror     = TARGET(&errorclass);                              \
      if (ierror != MPI_SUCCESS) return mpiwrapper_errorcode_toabi(ierror);    \
                                                                               \
      if (!mpiwrapper_errorcode_add(errorclass, abi_errorclass))               \
        return MPIABI_ERR_INTERN; /* MPIWRAPPER_ERRORCODE_SLOTS per process */ \
      return MPIABI_SUCCESS;                                                   \
    }
#else
#  define BODY_MPI_Add_error_class(TARGET)                                     \
    {                                                                          \
      (void)abi_errorclass;                                                    \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Add_error_class(int *abi_errorclass)
    BODY_MPI_Add_error_class(MPI_Add_error_class)
int mpiwrapper_w_PMPI_Add_error_class(int *abi_errorclass)
    BODY_MPI_Add_error_class(PMPI_Add_error_class)

/* ---------------------------------------------------- MPI_Add_error_code -- */

#ifdef MPIWRAPPER_HAVE_MPI_Add_error_code
#  define BODY_MPI_Add_error_code(TARGET)                                      \
    {                                                                          \
      const int errorclass = mpiwrapper_errorcode_fromabi(abi_errorclass);     \
                                                                               \
      int       errorcode = 0;                                                 \
      const int ierror    = TARGET(errorclass, &errorcode);                    \
      if (ierror != MPI_SUCCESS) return mpiwrapper_errorcode_toabi(ierror);    \
                                                                               \
      if (!mpiwrapper_errorcode_add(errorcode, abi_errorcode))                 \
        return MPIABI_ERR_INTERN;                                              \
      return MPIABI_SUCCESS;                                                   \
    }
#else
#  define BODY_MPI_Add_error_code(TARGET)                                      \
    {                                                                          \
      (void)abi_errorclass;                                                    \
      (void)abi_errorcode;                                                     \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Add_error_code(int abi_errorclass, int *abi_errorcode)
    BODY_MPI_Add_error_code(MPI_Add_error_code)
int mpiwrapper_w_PMPI_Add_error_code(int abi_errorclass, int *abi_errorcode)
    BODY_MPI_Add_error_code(PMPI_Add_error_code)

/* -------------------------------------------------- MPI_Add_error_string -- */

/* The string crosses unconverted, and its length is the implementation's
 * business: MPI truncates at its own MPI_MAX_ERROR_STRING, and a caller that
 * passed a longer one is already truncated on a native MPI.
 */
#ifdef MPIWRAPPER_HAVE_MPI_Add_error_string
#  define BODY_MPI_Add_error_string(TARGET)                                    \
    {                                                                          \
      const int         errorcode = mpiwrapper_errorcode_fromabi(abi_errorcode);\
      const char *const string    = abi_string;                                \
                                                                               \
      const int ierror = TARGET(errorcode, string);                            \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_Add_error_string(TARGET)                                    \
    {                                                                          \
      (void)abi_errorcode;                                                     \
      (void)abi_string;                                                        \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Add_error_string(int abi_errorcode, const char *abi_string)
    BODY_MPI_Add_error_string(MPI_Add_error_string)
int mpiwrapper_w_PMPI_Add_error_string(int abi_errorcode,
                                       const char *abi_string)
    BODY_MPI_Add_error_string(PMPI_Add_error_string)

/* ----------------------------------------------- the three removal forms -- */

/* MPI-5.0 additions, so both implementations available today answer decision
 * 6's report for at least one of them. Each is one conversion and a forward;
 * what makes them hand-written is that the conversion is the registry's, and
 * the registry is this file's subject.
 */
#ifdef MPIWRAPPER_HAVE_MPI_Remove_error_class
#  define BODY_MPI_Remove_error_class(TARGET)                                  \
    {                                                                          \
      const int errorclass = mpiwrapper_errorcode_fromabi(abi_errorclass);     \
                                                                               \
      const int ierror = TARGET(errorclass);                                   \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_Remove_error_class(TARGET)                                  \
    {                                                                          \
      (void)abi_errorclass;                                                    \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Remove_error_class(int abi_errorclass)
    BODY_MPI_Remove_error_class(MPI_Remove_error_class)
int mpiwrapper_w_PMPI_Remove_error_class(int abi_errorclass)
    BODY_MPI_Remove_error_class(PMPI_Remove_error_class)

#ifdef MPIWRAPPER_HAVE_MPI_Remove_error_code
#  define BODY_MPI_Remove_error_code(TARGET)                                   \
    {                                                                          \
      const int errorcode = mpiwrapper_errorcode_fromabi(abi_errorcode);       \
                                                                               \
      const int ierror = TARGET(errorcode);                                    \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_Remove_error_code(TARGET)                                   \
    {                                                                          \
      (void)abi_errorcode;                                                     \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Remove_error_code(int abi_errorcode)
    BODY_MPI_Remove_error_code(MPI_Remove_error_code)
int mpiwrapper_w_PMPI_Remove_error_code(int abi_errorcode)
    BODY_MPI_Remove_error_code(PMPI_Remove_error_code)

#ifdef MPIWRAPPER_HAVE_MPI_Remove_error_string
#  define BODY_MPI_Remove_error_string(TARGET)                                 \
    {                                                                          \
      const int errorcode = mpiwrapper_errorcode_fromabi(abi_errorcode);       \
                                                                               \
      const int ierror = TARGET(errorcode);                                    \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_Remove_error_string(TARGET)                                 \
    {                                                                          \
      (void)abi_errorcode;                                                     \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Remove_error_string(int abi_errorcode)
    BODY_MPI_Remove_error_string(MPI_Remove_error_string)
int mpiwrapper_w_PMPI_Remove_error_string(int abi_errorcode)
    BODY_MPI_Remove_error_string(PMPI_Remove_error_string)
