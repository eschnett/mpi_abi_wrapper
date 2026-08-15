/* libmpiwrapper -- the ten output string buffers with no length argument
 * (NOTES.md #5.8, S4a; MPI_Error_string is S1's).
 *
 * The danger these ten share: the caller sized its array with the *ABI's*
 * MPI_MAX_*, and the implementation writes up to its *own*. Where the
 * implementation's limit is larger, handing it the caller's array is a buffer
 * overflow -- so every one of them writes into a staged buffer of the
 * implementation's size and copies back under the ABI's. Everything else with
 * an output string passes the size in and is safe by construction; that set is
 * the generator's, and generate.py's STRING_OUT_LENGTH names the bounding
 * parameter per site precisely because naming it is the only thing that
 * separates the two classes.
 *
 * **The staging is unconditional**, not `#if impl_max > ABI_max`. Today no
 * limit of either implementation exceeds the ABI's, so a conditional temporary
 * would never compile here and the first MPI that needed it would be running
 * code nobody had executed. All ten are cold paths, so one exercised shape
 * costs nothing worth measuring.
 *
 * **Truncate or error is a per-parameter judgement**, and this is the table.
 * It lives beside the bodies rather than in the generator, because these
 * bodies are hand-written and a table naming sites nothing reads is a table
 * that goes stale:
 *
 *   truncate silently -- the value is prose, and an implementation truncates
 *   its own too-short buffer the same way:
 *       MPI_Error_string, MPI_Get_library_version,
 *       MPI_Comm_get_name, MPI_Type_get_name, MPI_Win_get_name,
 *       MPI_Get_processor_name
 *
 *   return MPIABI_ERR_INTERN -- the value is an identifier that goes back into
 *   MPI, where a truncated one fails mysteriously and much later:
 *       MPI_Open_port, MPI_Lookup_name (a truncated port name fails at
 *       connect), MPI_Info_get_nthkey (used to look a value up),
 *       MPI_File_get_view's datarep
 *
 * MPI_Get_processor_name is the awkward one and is in the first group with a
 * caveat: it reads as prose, but applications use it for rank-to-node mapping,
 * where truncation can make two nodes indistinguishable -- a silently wrong
 * answer rather than a visibly short one. It truncates anyway, because MPI
 * already permits an implementation to truncate to its own maximum, so a
 * caller that cannot tolerate it is already broken on a native MPI.
 *
 * On the error paths the truncated, NUL-terminated string is written anyway,
 * so a caller that ignores the return code reads a short answer rather than
 * uninitialized memory. And MPIABI_ERR_INTERN rather than MPI_ERR_TRUNCATE:
 * the limitation is this library's, not the caller's.
 */

#include "internal.h"

#include <string.h>

/* Copies at most `abi_max - 1` characters plus a NUL, and reports whether the
 * whole string fitted. `resultlen` is optional because four of the ten have no
 * such parameter -- their callers rely on the NUL alone.
 *
 * *resultlen is what was actually copied rather than what the implementation
 * produced, so that `string[*resultlen] == '\0'` holds for the caller. Telling
 * it the untruncated length would be worse than useless: it would point past
 * the terminator.
 */
static int string_out(char *dst, size_t abi_max, const char *src, int srclen,
                      int *resultlen)
{
  size_t len    = srclen < 0 ? 0 : (size_t)srclen;
  int    fitted = 1;

  if (len > abi_max - 1) {
    len    = abi_max - 1;
    fitted = 0;
  }
  memcpy(dst, src, len);
  dst[len] = '\0';
  if (resultlen) *resultlen = (int)len;
  return fitted;
}

/* ------------------------------------------------------ MPI_Error_string ---- */

/* S1's, and the template for the other nine. Against both implementations
 * today the copy could go straight into the caller's array -- 512 against 512
 * for MPICH, 512 against 256 for Open MPI -- which is exactly why the staging
 * is here rather than behind a test.
 */
#ifdef MPIWRAPPER_HAVE_MPI_Error_string
#  define BODY_MPI_Error_string(TARGET)                                        \
    {                                                                          \
      const int errorcode = mpiwrapper_errorcode_fromabi(abi_errorcode);       \
                                                                               \
      char buf[MPI_MAX_ERROR_STRING]; /* the implementation's maximum */       \
      int  resultlen = 0;                                                      \
                                                                               \
      const int ierror = TARGET(errorcode, buf, &resultlen);                   \
      if (ierror == MPI_SUCCESS)                                               \
        (void)string_out(abi_string, MPIABI_MAX_ERROR_STRING, buf, resultlen,  \
                         abi_resultlen);                                       \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_Error_string(TARGET)                                        \
    {                                                                          \
      (void)abi_errorcode;                                                     \
      (void)abi_string;                                                        \
      (void)abi_resultlen;                                                     \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Error_string(int abi_errorcode, char *abi_string,
                                  int *abi_resultlen)
    BODY_MPI_Error_string(MPI_Error_string)
int mpiwrapper_w_PMPI_Error_string(int abi_errorcode, char *abi_string,
                                   int *abi_resultlen)
    BODY_MPI_Error_string(PMPI_Error_string)

/* ------------------------------------------------ MPI_Get_library_version ---- */

/* The one staged buffer big enough to be worth a word: MPICH's
 * MPI_MAX_LIBRARY_VERSION_STRING is 8192, so this is an 8 KB automatic array.
 * It is a once-per-process call, so the alternative -- a heap allocation with
 * a failure path of its own -- would be more code and more ways to be wrong.
 */
#ifdef MPIWRAPPER_HAVE_MPI_Get_library_version
#  define BODY_MPI_Get_library_version(TARGET)                                 \
    {                                                                          \
      char buf[MPI_MAX_LIBRARY_VERSION_STRING];                                \
      int  resultlen = 0;                                                      \
                                                                               \
      const int ierror = TARGET(buf, &resultlen);                              \
      if (ierror == MPI_SUCCESS)                                               \
        (void)string_out(abi_version, MPIABI_MAX_LIBRARY_VERSION_STRING, buf,  \
                         resultlen, abi_resultlen);                            \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_Get_library_version(TARGET)                                 \
    {                                                                          \
      (void)abi_version;                                                       \
      (void)abi_resultlen;                                                     \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Get_library_version(char *abi_version, int *abi_resultlen)
    BODY_MPI_Get_library_version(MPI_Get_library_version)
int mpiwrapper_w_PMPI_Get_library_version(char *abi_version, int *abi_resultlen)
    BODY_MPI_Get_library_version(PMPI_Get_library_version)

/* ------------------------------------------------ MPI_Get_processor_name ---- */

#ifdef MPIWRAPPER_HAVE_MPI_Get_processor_name
#  define BODY_MPI_Get_processor_name(TARGET)                                  \
    {                                                                          \
      char buf[MPI_MAX_PROCESSOR_NAME];                                        \
      int  resultlen = 0;                                                      \
                                                                               \
      const int ierror = TARGET(buf, &resultlen);                              \
      if (ierror == MPI_SUCCESS)                                               \
        (void)string_out(abi_name, MPIABI_MAX_PROCESSOR_NAME, buf, resultlen,  \
                         abi_resultlen);                                       \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_Get_processor_name(TARGET)                                  \
    {                                                                          \
      (void)abi_name;                                                          \
      (void)abi_resultlen;                                                     \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Get_processor_name(char *abi_name, int *abi_resultlen)
    BODY_MPI_Get_processor_name(MPI_Get_processor_name)
int mpiwrapper_w_PMPI_Get_processor_name(char *abi_name, int *abi_resultlen)
    BODY_MPI_Get_processor_name(PMPI_Get_processor_name)

/* ------------------------------------------------------ the three *_get_name */

/* One shape, three classes, and the only difference is which handle goes in.
 * MPI_MAX_OBJECT_NAME bounds all three -- 128 in the ABI, 128 in MPICH, 64 in
 * Open MPI.
 */
#define BODY_GET_NAME(TARGET, FROMABI, ABIHANDLE, ABINAME)                     \
  {                                                                            \
    char buf[MPI_MAX_OBJECT_NAME];                                             \
    int  resultlen = 0;                                                        \
                                                                               \
    const int ierror = TARGET(FROMABI(ABIHANDLE), buf, &resultlen);            \
    if (ierror == MPI_SUCCESS)                                                 \
      (void)string_out(ABINAME, MPIABI_MAX_OBJECT_NAME, buf, resultlen,        \
                       abi_resultlen);                                         \
    return mpiwrapper_errorcode_toabi(ierror);                                 \
  }

#define STUB_GET_NAME(ABIHANDLE, ABINAME)                                      \
  {                                                                            \
    (void)(ABIHANDLE);                                                         \
    (void)(ABINAME);                                                           \
    (void)abi_resultlen;                                                       \
    return MPIABI_ERR_UNSUPPORTED_OPERATION;                                   \
  }

#ifdef MPIWRAPPER_HAVE_MPI_Comm_get_name
#  define BODY_MPI_Comm_get_name(TARGET)                                       \
     BODY_GET_NAME(TARGET, mpiwrapper_comm_fromabi, abi_comm, abi_comm_name)
#else
#  define BODY_MPI_Comm_get_name(TARGET)                                       \
     STUB_GET_NAME(abi_comm, abi_comm_name)
#endif

int mpiwrapper_w_MPI_Comm_get_name(MPIABI_Comm abi_comm, char *abi_comm_name,
                                   int *abi_resultlen)
    BODY_MPI_Comm_get_name(MPI_Comm_get_name)
int mpiwrapper_w_PMPI_Comm_get_name(MPIABI_Comm abi_comm, char *abi_comm_name,
                                    int *abi_resultlen)
    BODY_MPI_Comm_get_name(PMPI_Comm_get_name)

#ifdef MPIWRAPPER_HAVE_MPI_Type_get_name
#  define BODY_MPI_Type_get_name(TARGET)                                       \
     BODY_GET_NAME(TARGET, mpiwrapper_datatype_fromabi, abi_datatype,          \
                   abi_type_name)
#else
#  define BODY_MPI_Type_get_name(TARGET)                                       \
     STUB_GET_NAME(abi_datatype, abi_type_name)
#endif

int mpiwrapper_w_MPI_Type_get_name(MPIABI_Datatype abi_datatype,
                                   char *abi_type_name, int *abi_resultlen)
    BODY_MPI_Type_get_name(MPI_Type_get_name)
int mpiwrapper_w_PMPI_Type_get_name(MPIABI_Datatype abi_datatype,
                                    char *abi_type_name, int *abi_resultlen)
    BODY_MPI_Type_get_name(PMPI_Type_get_name)

#ifdef MPIWRAPPER_HAVE_MPI_Win_get_name
#  define BODY_MPI_Win_get_name(TARGET)                                        \
     BODY_GET_NAME(TARGET, mpiwrapper_win_fromabi, abi_win, abi_win_name)
#else
#  define BODY_MPI_Win_get_name(TARGET)                                        \
     STUB_GET_NAME(abi_win, abi_win_name)
#endif

int mpiwrapper_w_MPI_Win_get_name(MPIABI_Win abi_win, char *abi_win_name,
                                  int *abi_resultlen)
    BODY_MPI_Win_get_name(MPI_Win_get_name)
int mpiwrapper_w_PMPI_Win_get_name(MPIABI_Win abi_win, char *abi_win_name,
                                   int *abi_resultlen)
    BODY_MPI_Win_get_name(PMPI_Win_get_name)

/* ----------------------------------------------------------- MPI_Open_port ---- */

/* The first of the four that return an error rather than truncating. Note
 * what they do *not* have: a resultlen. The caller finds the end of the string
 * by its NUL, which is why a truncated one is undetectable to it and why
 * failing loudly is the only way it can learn.
 */
#ifdef MPIWRAPPER_HAVE_MPI_Open_port
#  define BODY_MPI_Open_port(TARGET)                                           \
    {                                                                          \
      const MPI_Info info = mpiwrapper_info_fromabi(abi_info);                 \
                                                                               \
      char buf[MPI_MAX_PORT_NAME];                                             \
      buf[0] = '\0';                                                           \
                                                                               \
      const int ierror = TARGET(info, buf);                                    \
      if (ierror != MPI_SUCCESS) return mpiwrapper_errorcode_toabi(ierror);    \
      if (!string_out(abi_port_name, MPIABI_MAX_PORT_NAME, buf,                \
                      (int)strlen(buf), NULL))                                 \
        return MPIABI_ERR_INTERN;                                              \
      return MPIABI_SUCCESS;                                                   \
    }
#else
#  define BODY_MPI_Open_port(TARGET)                                           \
    {                                                                          \
      (void)abi_info;                                                          \
      (void)abi_port_name;                                                     \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Open_port(MPIABI_Info abi_info, char *abi_port_name)
    BODY_MPI_Open_port(MPI_Open_port)
int mpiwrapper_w_PMPI_Open_port(MPIABI_Info abi_info, char *abi_port_name)
    BODY_MPI_Open_port(PMPI_Open_port)

/* --------------------------------------------------------- MPI_Lookup_name ---- */

#ifdef MPIWRAPPER_HAVE_MPI_Lookup_name
#  define BODY_MPI_Lookup_name(TARGET)                                         \
    {                                                                          \
      const char *const service_name = abi_service_name;                       \
      const MPI_Info    info         = mpiwrapper_info_fromabi(abi_info);      \
                                                                               \
      char buf[MPI_MAX_PORT_NAME];                                             \
      buf[0] = '\0';                                                           \
                                                                               \
      const int ierror = TARGET(service_name, info, buf);                      \
      if (ierror != MPI_SUCCESS) return mpiwrapper_errorcode_toabi(ierror);    \
      if (!string_out(abi_port_name, MPIABI_MAX_PORT_NAME, buf,                \
                      (int)strlen(buf), NULL))                                 \
        return MPIABI_ERR_INTERN;                                              \
      return MPIABI_SUCCESS;                                                   \
    }
#else
#  define BODY_MPI_Lookup_name(TARGET)                                         \
    {                                                                          \
      (void)abi_service_name;                                                  \
      (void)abi_info;                                                          \
      (void)abi_port_name;                                                     \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Lookup_name(const char *abi_service_name,
                                 MPIABI_Info abi_info, char *abi_port_name)
    BODY_MPI_Lookup_name(MPI_Lookup_name)
int mpiwrapper_w_PMPI_Lookup_name(const char *abi_service_name,
                                  MPIABI_Info abi_info, char *abi_port_name)
    BODY_MPI_Lookup_name(PMPI_Lookup_name)

/* ------------------------------------------------------ MPI_Info_get_nthkey ---- */

/* The one case where the ABI's limit is the larger by a wide margin: 256
 * against Open MPI's 36. The opposite direction is not fixable and is correct
 * to pass through -- a key longer than the implementation's own maximum is
 * rejected by the implementation with MPI_ERR_INFO_KEY, which we map and
 * return (NOTES.md #5.8).
 */
#ifdef MPIWRAPPER_HAVE_MPI_Info_get_nthkey
#  define BODY_MPI_Info_get_nthkey(TARGET)                                     \
    {                                                                          \
      const MPI_Info info = mpiwrapper_info_fromabi(abi_info);                 \
      const int      n    = abi_n;                                             \
                                                                               \
      char buf[MPI_MAX_INFO_KEY + 1]; /* MPICH's 255 excludes the NUL */       \
      buf[0] = '\0';                                                           \
                                                                               \
      const int ierror = TARGET(info, n, buf);                                 \
      if (ierror != MPI_SUCCESS) return mpiwrapper_errorcode_toabi(ierror);    \
      if (!string_out(abi_key, MPIABI_MAX_INFO_KEY, buf, (int)strlen(buf),     \
                      NULL))                                                   \
        return MPIABI_ERR_INTERN;                                              \
      return MPIABI_SUCCESS;                                                   \
    }
#else
#  define BODY_MPI_Info_get_nthkey(TARGET)                                     \
    {                                                                          \
      (void)abi_info;                                                          \
      (void)abi_n;                                                             \
      (void)abi_key;                                                           \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Info_get_nthkey(MPIABI_Info abi_info, int abi_n,
                                     char *abi_key)
    BODY_MPI_Info_get_nthkey(MPI_Info_get_nthkey)
int mpiwrapper_w_PMPI_Info_get_nthkey(MPIABI_Info abi_info, int abi_n,
                                      char *abi_key)
    BODY_MPI_Info_get_nthkey(PMPI_Info_get_nthkey)

/* -------------------------------------------------------- MPI_File_get_view ---- */

/* The only one of the ten that is here for its string and converts three other
 * parameters besides: an offset that passes through and two out datatypes that
 * do not. The datatypes are written back only on success, and the collision
 * check runs once for both, as everywhere a body produces handles.
 */
#ifdef MPIWRAPPER_HAVE_MPI_File_get_view
#  define BODY_MPI_File_get_view(TARGET)                                       \
    {                                                                          \
      const MPI_File fh = mpiwrapper_file_fromabi(abi_fh);                     \
                                                                               \
      MPI_Offset   disp;                                                       \
      MPI_Datatype etype;                                                      \
      MPI_Datatype filetype;                                                   \
      char         buf[MPI_MAX_DATAREP_STRING];                                \
      buf[0] = '\0';                                                           \
                                                                               \
      const int ierror = TARGET(fh, &disp, &etype, &filetype, buf);            \
      if (ierror != MPI_SUCCESS) return mpiwrapper_errorcode_toabi(ierror);    \
                                                                               \
      *abi_disp     = disp;                                                    \
      *abi_etype    = mpiwrapper_datatype_toabi(etype);                        \
      *abi_filetype = mpiwrapper_datatype_toabi(filetype);                     \
      if (mpiwrapper_take_handle_error()) return MPIABI_ERR_INTERN;            \
                                                                               \
      if (!string_out(abi_datarep, MPIABI_MAX_DATAREP_STRING, buf,             \
                      (int)strlen(buf), NULL))                                 \
        return MPIABI_ERR_INTERN;                                              \
      return MPIABI_SUCCESS;                                                   \
    }
#else
#  define BODY_MPI_File_get_view(TARGET)                                       \
    {                                                                          \
      (void)abi_fh;                                                            \
      (void)abi_disp;                                                          \
      (void)abi_datarep;                                                       \
      *abi_etype    = MPIABI_DATATYPE_NULL;                                    \
      *abi_filetype = MPIABI_DATATYPE_NULL;                                    \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_File_get_view(MPIABI_File abi_fh, MPIABI_Offset *abi_disp,
                                   MPIABI_Datatype *abi_etype,
                                   MPIABI_Datatype *abi_filetype,
                                   char *abi_datarep)
    BODY_MPI_File_get_view(MPI_File_get_view)
int mpiwrapper_w_PMPI_File_get_view(MPIABI_File abi_fh, MPIABI_Offset *abi_disp,
                                    MPIABI_Datatype *abi_etype,
                                    MPIABI_Datatype *abi_filetype,
                                    char *abi_datarep)
    BODY_MPI_File_get_view(PMPI_File_get_view)
