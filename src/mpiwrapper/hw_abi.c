/* libmpiwrapper -- the six MPI_Abi_* entry points (MPI-5.0 20.3 and 20.4.1,
 * S4a).
 *
 * These answer about *this library*, not about the wrapped MPI, and that is
 * the whole of why they are hand-written. Every other entry point here is a
 * conversion around a call to the implementation; these six have no such call
 * to make, because the questions they ask -- which ABI version is this, how
 * wide are its integer types, what does this process's Fortran compiler do
 * with LOGICAL -- are answered by the ABI header the caller compiled against
 * and by what the caller itself told us. Forwarding any of them would be a
 * category error: an implementation that does not implement the ABI has no
 * ABI version to report, and one that does would report its own rather than
 * the one this wrapper presents.
 *
 * MPI_Abi_get_info is the exception that proves it. Its *answer* is ours, but
 * an MPI_Info is an implementation object, so this is the one body here that
 * has to call MPI at all -- to build a container for values it computes
 * itself.
 *
 * The Fortran registration pair (20.4.1) exists so that a Fortran layer can
 * tell a C implementation the properties of its compiler. Over a wrapped MPI
 * that information has no onward destination: MPICH and Open MPI were each
 * built with a Fortran compiler of their own and their MPI_LOGICAL is already
 * settled, so passing ours down would at best be ignored and at worst
 * contradict what they know. So the state is kept here and handed back by the
 * matching getter, which is exactly the contract the standard states -- "when
 * MPI_INFO_NULL is returned, the implementation does not know the properties
 * of the Fortran compiler and they must be set by the application" -- and
 * exactly what mpif needs from it.
 */

#include "internal.h"

#include <stdatomic.h>
#include <string.h>

/* MPI-5.0 20.4.1: "only the first call ... affects the state of the MPI
 * library; all subsequent calls will return the error code MPI_ERR_ABI". One
 * atomic flag per setter makes that a race-free claim rather than a
 * test-then-set, and MPI_ABI_GET_VERSION and MPI_ABI_GET_INFO "must always be
 * thread-safe" whether or not MPI_Init has been called -- which they are here
 * by having nothing to synchronize.
 */
static atomic_flag fortran_info_claimed  = ATOMIC_FLAG_INIT;
static atomic_bool fortran_info_published;
static MPI_Info    fortran_info; /* the implementation-side duplicate */

static atomic_flag fortran_bool_claimed = ATOMIC_FLAG_INIT;
static atomic_bool fortran_bool_published;

/* A Fortran LOGICAL is at most 16 bytes on any target this could run on, and
 * the standard passes the literals by address rather than by value precisely
 * because their representation is not knowable otherwise. Storing the bytes,
 * and the size they came with, is the whole of what "knowing the booleans"
 * means.
 */
#define MPIWRAPPER_MAX_LOGICAL_SIZE 16
static int           fortran_logical_size;
static unsigned char fortran_true[MPIWRAPPER_MAX_LOGICAL_SIZE];
static unsigned char fortran_false[MPIWRAPPER_MAX_LOGICAL_SIZE];

/* --------------------------------------------------- MPI_Abi_get_version ---- */

/* "MPI_ABI_GET_VERSION produces the standard ABI version, if supported.
 * Otherwise, the values of the major and minor version are set to -1." This
 * library's entire purpose is to support it, so the answer is never -1, and it
 * comes from the ABI header's own macros so that the number cannot drift from
 * the header the caller compiled against.
 */
#define BODY_MPI_Abi_get_version(TARGET)                                       \
  {                                                                            \
    *abi_abi_major = MPIABI_ABI_VERSION;                                       \
    *abi_abi_minor = MPIABI_ABI_SUBVERSION;                                    \
    return MPIABI_SUCCESS;                                                     \
  }

int mpiwrapper_w_MPI_Abi_get_version(int *abi_abi_major, int *abi_abi_minor)
    BODY_MPI_Abi_get_version(MPI_Abi_get_version)
int mpiwrapper_w_PMPI_Abi_get_version(int *abi_abi_major, int *abi_abi_minor)
    BODY_MPI_Abi_get_version(PMPI_Abi_get_version)

/* ------------------------------------------------------ MPI_Abi_get_info ---- */

/* The three keys MPI-5.0 20.3 predefines, and their values are properties of
 * the ABI rather than of the implementation. internal.h already static-asserts
 * that the implementation's MPI_Aint, MPI_Count and MPI_Offset are the same
 * widths, so reporting the ABI's is reporting both.
 *
 * The implementation calls here are MPI_Info_*, not the entry point's own
 * name, so decision 7's rule -- the PMPI_ body calls the implementation's
 * PMPI_ names, so that a caller bypassing profiling bypasses a tool below us
 * too -- has to be carried by an explicit prefix rather than by TARGET. That
 * is also why the filler is defined twice: it makes the call.
 */
#define DEFINE_ABI_INFO_FILL(NAME, PREFIX)                                     \
  static int NAME(MPI_Info info)                                               \
  {                                                                            \
    static const char *const keys[] = {"mpi_aint_size", "mpi_count_size",      \
                                       "mpi_offset_size"};                     \
    const size_t sizes[] = {sizeof(MPIABI_Aint), sizeof(MPIABI_Count),         \
                            sizeof(MPIABI_Offset)};                            \
                                                                               \
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; ++i) {                \
      /* A size in bytes is one or two digits, so this writes them rather      \
       * than reaching for snprintf.                                           \
       */                                                                      \
      char   value[24];                                                        \
      char   digits[24];                                                       \
      size_t n       = sizes[i];                                               \
      int    ndigits = 0;                                                      \
      do {                                                                     \
        digits[ndigits++] = (char)('0' + (int)(n % 10));                       \
        n /= 10;                                                               \
      } while (n);                                                             \
      for (int d = 0; d < ndigits; ++d) value[d] = digits[ndigits - 1 - d];    \
      value[ndigits] = '\0';                                                   \
                                                                               \
      const int ierror = PREFIX##Info_set(info, keys[i], value);               \
      if (ierror != MPI_SUCCESS) return ierror;                                \
    }                                                                          \
    return MPI_SUCCESS;                                                        \
  }

/* Guarded on MPI_Info_create rather than on MPI_Abi_get_info: what this body
 * needs from the implementation is an info object, and one that cannot create
 * an info has nothing for this call to hand back. An implementation with an
 * MPI_Abi_get_info of its own is deliberately not consulted, for the reason at
 * the top of this file.
 */
#ifdef MPIWRAPPER_HAVE_MPI_Info_create
DEFINE_ABI_INFO_FILL(abi_info_fill, MPI_)
DEFINE_ABI_INFO_FILL(abi_info_fill_p, PMPI_)

#  define BODY_ABI_GET_INFO(FILL, PREFIX)                                      \
    {                                                                          \
      MPI_Info  info;                                                          \
      const int ierror = PREFIX##Info_create(&info);                           \
      if (ierror != MPI_SUCCESS) {                                             \
        *abi_info = MPIABI_INFO_NULL;                                          \
        return mpiwrapper_errorcode_toabi(ierror);                             \
      }                                                                        \
                                                                               \
      const int filled = FILL(info);                                           \
      if (filled != MPI_SUCCESS) {                                             \
        (void)PREFIX##Info_free(&info);                                        \
        *abi_info = MPIABI_INFO_NULL;                                          \
        return mpiwrapper_errorcode_toabi(filled);                             \
      }                                                                        \
                                                                               \
      *abi_info = mpiwrapper_info_toabi(info);                                 \
      if (mpiwrapper_take_handle_error()) return MPIABI_ERR_INTERN;            \
      return MPIABI_SUCCESS;                                                   \
    }
#else
#  define BODY_ABI_GET_INFO(FILL, PREFIX)                                      \
    {                                                                          \
      *abi_info = MPIABI_INFO_NULL;                                            \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Abi_get_info(MPIABI_Info *abi_info)
    BODY_ABI_GET_INFO(abi_info_fill, MPI_)
int mpiwrapper_w_PMPI_Abi_get_info(MPIABI_Info *abi_info)
    BODY_ABI_GET_INFO(abi_info_fill_p, PMPI_)

/* ---------------------------------------------- MPI_Abi_set_fortran_info ---- */

/* The info is duplicated rather than retained: MPI-5.0 lets the caller free or
 * modify its own handle the moment this returns, and an aliased handle would
 * turn that into a stale answer from the getter much later.
 */
#ifdef MPIWRAPPER_HAVE_MPI_Info_dup
#  define BODY_ABI_SET_FORTRAN_INFO(PREFIX)                                    \
    {                                                                          \
      if (atomic_flag_test_and_set_explicit(&fortran_info_claimed,             \
                                            memory_order_acq_rel))             \
        return MPIABI_ERR_ABI; /* not the first call */                        \
                                                                               \
      const MPI_Info info = mpiwrapper_info_fromabi(abi_info);                 \
      MPI_Info       dup;                                                      \
      const int      ierror = PREFIX##Info_dup(info, &dup);                    \
      if (ierror != MPI_SUCCESS) return mpiwrapper_errorcode_toabi(ierror);    \
                                                                               \
      fortran_info = dup;                                                      \
      atomic_store_explicit(&fortran_info_published, 1,                        \
                            memory_order_release);                             \
      return MPIABI_SUCCESS;                                                   \
    }
#else
#  define BODY_ABI_SET_FORTRAN_INFO(PREFIX)                                    \
    {                                                                          \
      (void)abi_info;                                                          \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Abi_set_fortran_info(MPIABI_Info abi_info)
    BODY_ABI_SET_FORTRAN_INFO(MPI_)
int mpiwrapper_w_PMPI_Abi_set_fortran_info(MPIABI_Info abi_info)
    BODY_ABI_SET_FORTRAN_INFO(PMPI_)

/* ---------------------------------------------- MPI_Abi_get_fortran_info ---- */

/* MPI_INFO_NULL when nothing has been set, which the standard makes the signal
 * that "the implementation does not know the properties of the Fortran
 * compiler and they must be set by the application". A failed
 * MPI_Abi_set_fortran_info leaves the claim taken and nothing published, which
 * is the state the standard tells the user to inspect with this call.
 */
#define BODY_MPI_Abi_get_fortran_info(TARGET)                                  \
  {                                                                            \
    if (!atomic_load_explicit(&fortran_info_published,                         \
                              memory_order_acquire)) {                         \
      *abi_info = MPIABI_INFO_NULL;                                            \
      return MPIABI_SUCCESS;                                                   \
    }                                                                          \
                                                                               \
    *abi_info = mpiwrapper_info_toabi(fortran_info);                           \
    if (mpiwrapper_take_handle_error()) return MPIABI_ERR_INTERN;              \
    return MPIABI_SUCCESS;                                                     \
  }

int mpiwrapper_w_MPI_Abi_get_fortran_info(MPIABI_Info *abi_info)
    BODY_MPI_Abi_get_fortran_info(MPI_Abi_get_fortran_info)
int mpiwrapper_w_PMPI_Abi_get_fortran_info(MPIABI_Info *abi_info)
    BODY_MPI_Abi_get_fortran_info(PMPI_Abi_get_fortran_info)

/* ------------------------------------------ MPI_Abi_set_fortran_booleans ---- */

/* "Boolean literals must be passed directly so that they can be observed by
 * the implementation, since it may not be possible to obtain their literal
 * values directly" -- hence void pointers and a size, and hence a byte copy
 * rather than an interpretation. MPI does not assume Fortran's .TRUE. is C's
 * 1, and neither does this.
 */
#define BODY_MPI_Abi_set_fortran_booleans(TARGET)                              \
  {                                                                            \
    if (abi_logical_size <= 0                                                  \
        || abi_logical_size > MPIWRAPPER_MAX_LOGICAL_SIZE)                     \
      return MPIABI_ERR_ARG;                                                   \
    if (!abi_logical_true || !abi_logical_false) return MPIABI_ERR_ARG;        \
                                                                               \
    if (atomic_flag_test_and_set_explicit(&fortran_bool_claimed,               \
                                          memory_order_acq_rel))               \
      return MPIABI_ERR_ABI; /* not the first call */                          \
                                                                               \
    memcpy(fortran_true, abi_logical_true, (size_t)abi_logical_size);          \
    memcpy(fortran_false, abi_logical_false, (size_t)abi_logical_size);        \
    fortran_logical_size = abi_logical_size;                                   \
    atomic_store_explicit(&fortran_bool_published, 1, memory_order_release);   \
    return MPIABI_SUCCESS;                                                     \
  }

int mpiwrapper_w_MPI_Abi_set_fortran_booleans(int abi_logical_size,
                                              void *abi_logical_true,
                                              void *abi_logical_false)
    BODY_MPI_Abi_set_fortran_booleans(MPI_Abi_set_fortran_booleans)
int mpiwrapper_w_PMPI_Abi_set_fortran_booleans(int abi_logical_size,
                                               void *abi_logical_true,
                                               void *abi_logical_false)
    BODY_MPI_Abi_set_fortran_booleans(PMPI_Abi_set_fortran_booleans)

/* ------------------------------------------ MPI_Abi_get_fortran_booleans ---- */

/* is_set is the answer; the two values are only meaningful when it is true, so
 * they are left untouched otherwise rather than filled with a guess a caller
 * might use. A request for a *different* LOGICAL size than was registered is
 * "not set" for that size, which is the honest answer -- the literals of a
 * LOGICAL(kind=8) are not derivable from those of the default kind.
 */
#define BODY_MPI_Abi_get_fortran_booleans(TARGET)                              \
  {                                                                            \
    *abi_is_set = 0;                                                           \
    if (!atomic_load_explicit(&fortran_bool_published, memory_order_acquire))  \
      return MPIABI_SUCCESS;                                                   \
    if (abi_logical_size != fortran_logical_size) return MPIABI_SUCCESS;       \
    if (!abi_logical_true || !abi_logical_false) return MPIABI_ERR_ARG;        \
                                                                               \
    memcpy(abi_logical_true, fortran_true, (size_t)fortran_logical_size);      \
    memcpy(abi_logical_false, fortran_false, (size_t)fortran_logical_size);    \
    *abi_is_set = 1;                                                           \
    return MPIABI_SUCCESS;                                                     \
  }

int mpiwrapper_w_MPI_Abi_get_fortran_booleans(int abi_logical_size,
                                              void *abi_logical_true,
                                              void *abi_logical_false,
                                              int *abi_is_set)
    BODY_MPI_Abi_get_fortran_booleans(MPI_Abi_get_fortran_booleans)
int mpiwrapper_w_PMPI_Abi_get_fortran_booleans(int abi_logical_size,
                                               void *abi_logical_true,
                                               void *abi_logical_false,
                                               int *abi_is_set)
    BODY_MPI_Abi_get_fortran_booleans(PMPI_Abi_get_fortran_booleans)
