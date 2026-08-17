/* libmpiwrapper -- the fifteen callback registrars (NOTES.md #6.1, S1 and
 * S4b).
 *
 * MPI_Op_create and MPI_Comm_create_errhandler are S1's and were in
 * handwritten.c until S4b gave the family a file of its own. The other
 * thirteen are S4b's, and every one of them is the same shape: install
 * something of ours where the application gave us a function of its own, and
 * let the machinery in callbacks.c, extrastate.c or toolevents.c decide what
 * "something of ours" is.
 *
 * The registrar is hand-written and the *mechanism* is not, which is the split
 * this file exists to hold. A body here never touches a pool's array or a
 * pair's fields; it asks for a trampoline, hands it to the implementation, and
 * gives the slot back on the one path where that is safe -- the path where the
 * implementation refused to create the object, so nothing can hold a reference
 * to it (#6.2).
 *
 * Why the registrars cannot live beside their mechanism: each of these exists
 * *twice*, once calling the implementation's MPI_ name and once its PMPI_ one
 * (decision 7), and the files that own the trampolines must name neither.
 */

#include "internal.h"

/* ---------------------------------------------------------- user ops ------ */

/* MPI_User_function has no extra-state argument, so the user's function cannot
 * be identified from inside the callback and a pool of static trampolines is
 * the only mechanism left. callbacks.c owns the pool; this owns the bodies.
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
int mpiwrapper_w_PMPI_Op_create(MPIABI_User_function *abi_user_fn,
                            int abi_commute, MPIABI_Op *abi_op)
    BODY_MPI_Op_create(PMPI_Op_create)

/* The same against the large-count callback, which is a second pool because
 * the trampoline's type is what the implementation stores (callbacks.c).
 */
#ifdef MPIWRAPPER_HAVE_MPI_Op_create_c
#  define BODY_MPI_Op_create_c(TARGET)                                         \
    {                                                                          \
      const int slot = mpiwrapper_op_c_slot_alloc(abi_user_fn);                \
      if (slot < 0) {                                                          \
        *abi_op = MPIABI_OP_NULL;                                              \
        return MPIABI_ERR_INTERN;                                              \
      }                                                                        \
                                                                               \
      const int commute = abi_commute;                                         \
      MPI_Op    op;                                                            \
      const int ierror = TARGET(mpiwrapper_op_c_tramp(slot), commute, &op);    \
      if (ierror != MPI_SUCCESS) {                                             \
        mpiwrapper_op_c_slot_release(slot);                                    \
        *abi_op = MPIABI_OP_NULL;                                              \
        return mpiwrapper_errorcode_toabi(ierror);                             \
      }                                                                        \
                                                                               \
      *abi_op = mpiwrapper_op_toabi(op);                                       \
      if (mpiwrapper_take_handle_error()) return MPIABI_ERR_INTERN;            \
      return MPIABI_SUCCESS;                                                   \
    }
#else
#  define BODY_MPI_Op_create_c(TARGET)                                         \
    {                                                                          \
      (void)abi_user_fn;                                                       \
      (void)abi_commute;                                                       \
      *abi_op = MPIABI_OP_NULL;                                                \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Op_create_c(MPIABI_User_function_c *abi_user_fn,
                                 int abi_commute, MPIABI_Op *abi_op)
    BODY_MPI_Op_create_c(MPI_Op_create_c)
int mpiwrapper_w_PMPI_Op_create_c(MPIABI_User_function_c *abi_user_fn,
                                  int abi_commute, MPIABI_Op *abi_op)
    BODY_MPI_Op_create_c(PMPI_Op_create_c)

/* -------------------------------------------------------- error handlers -- */

/* S1's MPI_Comm_create_errhandler body, now instantiated four times: the four
 * classes differ only in which pool they draw from, and the pool is what
 * knows the handle type. Writing the body once is not tidiness -- the
 * error-return path here releases a slot, and three transcriptions of that
 * are three chances to forget it.
 */
#define BODY_CREATE_ERRHANDLER(TARGET, ABI_FN, ALLOC, TRAMP, RELEASE)          \
  {                                                                            \
    const int slot = ALLOC(ABI_FN);                                            \
    if (slot < 0) {                                                            \
      *abi_errhandler = MPIABI_ERRHANDLER_NULL;                                \
      return MPIABI_ERR_INTERN; /* MPIWRAPPER_ERRHANDLER_SLOTS in one process */\
    }                                                                          \
                                                                               \
    MPI_Errhandler errhandler;                                                 \
    const int      ierror = TARGET(TRAMP(slot), &errhandler);                  \
    if (ierror != MPI_SUCCESS) {                                               \
      RELEASE(slot);                                                           \
      *abi_errhandler = MPIABI_ERRHANDLER_NULL;                                \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }                                                                          \
                                                                               \
    *abi_errhandler = mpiwrapper_errhandler_toabi(errhandler);                 \
    if (mpiwrapper_take_handle_error()) return MPIABI_ERR_INTERN;              \
    return MPIABI_SUCCESS;                                                     \
  }

#define BODY_UNSUPPORTED_ERRHANDLER(ABI_FN)                                    \
  {                                                                            \
    (void)ABI_FN;                                                              \
    *abi_errhandler = MPIABI_ERRHANDLER_NULL;                                  \
    return MPIABI_ERR_UNSUPPORTED_OPERATION;                                   \
  }

#ifdef MPIWRAPPER_HAVE_MPI_Comm_create_errhandler
#  define BODY_MPI_Comm_create_errhandler(TARGET)                              \
    BODY_CREATE_ERRHANDLER(TARGET, abi_comm_errhandler_fn,                     \
                           mpiwrapper_comm_errh_slot_alloc,                    \
                           mpiwrapper_comm_errh_tramp,                         \
                           mpiwrapper_comm_errh_slot_release)
#else
#  define BODY_MPI_Comm_create_errhandler(TARGET)                              \
    BODY_UNSUPPORTED_ERRHANDLER(abi_comm_errhandler_fn)
#endif

int
mpiwrapper_w_MPI_Comm_create_errhandler(MPIABI_Comm_errhandler_function *abi_comm_errhandler_fn,
                             MPIABI_Errhandler *abi_errhandler)
    BODY_MPI_Comm_create_errhandler(MPI_Comm_create_errhandler)
int
mpiwrapper_w_PMPI_Comm_create_errhandler(MPIABI_Comm_errhandler_function *abi_comm_errhandler_fn,
                              MPIABI_Errhandler *abi_errhandler)
    BODY_MPI_Comm_create_errhandler(PMPI_Comm_create_errhandler)

#ifdef MPIWRAPPER_HAVE_MPI_File_create_errhandler
#  define BODY_MPI_File_create_errhandler(TARGET)                              \
    BODY_CREATE_ERRHANDLER(TARGET, abi_file_errhandler_fn,                     \
                           mpiwrapper_file_errh_slot_alloc,                    \
                           mpiwrapper_file_errh_tramp,                         \
                           mpiwrapper_file_errh_slot_release)
#else
#  define BODY_MPI_File_create_errhandler(TARGET)                              \
    BODY_UNSUPPORTED_ERRHANDLER(abi_file_errhandler_fn)
#endif

int
mpiwrapper_w_MPI_File_create_errhandler(MPIABI_File_errhandler_function *abi_file_errhandler_fn,
                             MPIABI_Errhandler *abi_errhandler)
    BODY_MPI_File_create_errhandler(MPI_File_create_errhandler)
int
mpiwrapper_w_PMPI_File_create_errhandler(MPIABI_File_errhandler_function *abi_file_errhandler_fn,
                              MPIABI_Errhandler *abi_errhandler)
    BODY_MPI_File_create_errhandler(PMPI_File_create_errhandler)

#ifdef MPIWRAPPER_HAVE_MPI_Win_create_errhandler
#  define BODY_MPI_Win_create_errhandler(TARGET)                               \
    BODY_CREATE_ERRHANDLER(TARGET, abi_win_errhandler_fn,                      \
                           mpiwrapper_win_errh_slot_alloc,                     \
                           mpiwrapper_win_errh_tramp,                          \
                           mpiwrapper_win_errh_slot_release)
#else
#  define BODY_MPI_Win_create_errhandler(TARGET)                               \
    BODY_UNSUPPORTED_ERRHANDLER(abi_win_errhandler_fn)
#endif

int
mpiwrapper_w_MPI_Win_create_errhandler(MPIABI_Win_errhandler_function *abi_win_errhandler_fn,
                            MPIABI_Errhandler *abi_errhandler)
    BODY_MPI_Win_create_errhandler(MPI_Win_create_errhandler)
int
mpiwrapper_w_PMPI_Win_create_errhandler(MPIABI_Win_errhandler_function *abi_win_errhandler_fn,
                             MPIABI_Errhandler *abi_errhandler)
    BODY_MPI_Win_create_errhandler(PMPI_Win_create_errhandler)

#ifdef MPIWRAPPER_HAVE_MPI_Session_create_errhandler
#  define BODY_MPI_Session_create_errhandler(TARGET)                           \
    BODY_CREATE_ERRHANDLER(TARGET, abi_session_errhandler_fn,                  \
                           mpiwrapper_session_errh_slot_alloc,                 \
                           mpiwrapper_session_errh_tramp,                      \
                           mpiwrapper_session_errh_slot_release)
#else
#  define BODY_MPI_Session_create_errhandler(TARGET)                           \
    BODY_UNSUPPORTED_ERRHANDLER(abi_session_errhandler_fn)
#endif

int
mpiwrapper_w_MPI_Session_create_errhandler(MPIABI_Session_errhandler_function *abi_session_errhandler_fn,
                                MPIABI_Errhandler *abi_errhandler)
    BODY_MPI_Session_create_errhandler(MPI_Session_create_errhandler)
int
mpiwrapper_w_PMPI_Session_create_errhandler(MPIABI_Session_errhandler_function *abi_session_errhandler_fn,
                                 MPIABI_Errhandler *abi_errhandler)
    BODY_MPI_Session_create_errhandler(PMPI_Session_create_errhandler)

/* ------------------------------------------------------- attribute keys --- */

/* Three families, one shape: build the implementation-side pair of functions
 * (extrastate.c, which is also where the MPI_COMM_DUP_FN family of sentinels
 * is recognized), create the key, and register what the implementation handed
 * back with keyvals.c so that both directions of the keyval family can
 * translate it afterwards.
 *
 * The one thing worth stating about the error paths: when the key is created
 * and the registry is full, the key exists in the implementation and this
 * library cannot name it. Nothing here frees it -- MPI_*_free_keyval would
 * have to be called through the right one of the two name prefixes, and a key
 * that leaks is a slot, while a key freed under a caller that still holds it
 * would be a use-after-free. So it answers MPIABI_ERR_INTERN and leaks, which
 * is what every other fixed-capacity table here does when it fills.
 */
#define BODY_CREATE_KEYVAL(TARGET, BUILD, COPY_T, DELETE_T, ABI_COPY,          \
                           ABI_DELETE, ABI_EXTRA, ABI_KEYVAL)                  \
  {                                                                            \
    COPY_T   *copy_fn;                                                         \
    DELETE_T *delete_fn;                                                       \
    void     *state;                                                           \
    if (!BUILD(ABI_COPY, ABI_DELETE, ABI_EXTRA, &copy_fn, &delete_fn,          \
               &state)) {                                                      \
      *ABI_KEYVAL = MPIABI_KEYVAL_INVALID;                                     \
      return MPIABI_ERR_INTERN;                                                \
    }                                                                          \
                                                                               \
    int       keyval = MPI_KEYVAL_INVALID;                                     \
    const int ierror = TARGET(copy_fn, delete_fn, &keyval, state);             \
    if (ierror != MPI_SUCCESS) {                                               \
      *ABI_KEYVAL = MPIABI_KEYVAL_INVALID;                                     \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }                                                                          \
                                                                               \
    if (!mpiwrapper_keyval_add(keyval, ABI_KEYVAL)) {                          \
      *ABI_KEYVAL = MPIABI_KEYVAL_INVALID;                                     \
      return MPIABI_ERR_INTERN; /* MPIWRAPPER_KEYVAL_SLOTS in one process */   \
    }                                                                          \
    return MPIABI_SUCCESS;                                                     \
  }

#ifdef MPIWRAPPER_HAVE_MPI_Comm_create_keyval
#  define BODY_MPI_Comm_create_keyval(TARGET)                                  \
    BODY_CREATE_KEYVAL(TARGET, mpiwrapper_comm_attr_fns,                       \
                       MPI_Comm_copy_attr_function,                            \
                       MPI_Comm_delete_attr_function, abi_comm_copy_attr_fn,   \
                       abi_comm_delete_attr_fn, abi_extra_state,               \
                       abi_comm_keyval)
#else
#  define BODY_MPI_Comm_create_keyval(TARGET)                                  \
    {                                                                          \
      (void)abi_comm_copy_attr_fn;                                             \
      (void)abi_comm_delete_attr_fn;                                           \
      (void)abi_extra_state;                                                   \
      *abi_comm_keyval = MPIABI_KEYVAL_INVALID;                                \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Comm_create_keyval(
    MPIABI_Comm_copy_attr_function   *abi_comm_copy_attr_fn,
    MPIABI_Comm_delete_attr_function *abi_comm_delete_attr_fn,
    int *abi_comm_keyval, void *abi_extra_state)
    BODY_MPI_Comm_create_keyval(MPI_Comm_create_keyval)
int mpiwrapper_w_PMPI_Comm_create_keyval(
    MPIABI_Comm_copy_attr_function   *abi_comm_copy_attr_fn,
    MPIABI_Comm_delete_attr_function *abi_comm_delete_attr_fn,
    int *abi_comm_keyval, void *abi_extra_state)
    BODY_MPI_Comm_create_keyval(PMPI_Comm_create_keyval)

#ifdef MPIWRAPPER_HAVE_MPI_Type_create_keyval
#  define BODY_MPI_Type_create_keyval(TARGET)                                  \
    BODY_CREATE_KEYVAL(TARGET, mpiwrapper_type_attr_fns,                       \
                       MPI_Type_copy_attr_function,                            \
                       MPI_Type_delete_attr_function, abi_type_copy_attr_fn,   \
                       abi_type_delete_attr_fn, abi_extra_state,               \
                       abi_type_keyval)
#else
#  define BODY_MPI_Type_create_keyval(TARGET)                                  \
    {                                                                          \
      (void)abi_type_copy_attr_fn;                                             \
      (void)abi_type_delete_attr_fn;                                           \
      (void)abi_extra_state;                                                   \
      *abi_type_keyval = MPIABI_KEYVAL_INVALID;                                \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Type_create_keyval(
    MPIABI_Type_copy_attr_function   *abi_type_copy_attr_fn,
    MPIABI_Type_delete_attr_function *abi_type_delete_attr_fn,
    int *abi_type_keyval, void *abi_extra_state)
    BODY_MPI_Type_create_keyval(MPI_Type_create_keyval)
int mpiwrapper_w_PMPI_Type_create_keyval(
    MPIABI_Type_copy_attr_function   *abi_type_copy_attr_fn,
    MPIABI_Type_delete_attr_function *abi_type_delete_attr_fn,
    int *abi_type_keyval, void *abi_extra_state)
    BODY_MPI_Type_create_keyval(PMPI_Type_create_keyval)

#ifdef MPIWRAPPER_HAVE_MPI_Win_create_keyval
#  define BODY_MPI_Win_create_keyval(TARGET)                                   \
    BODY_CREATE_KEYVAL(TARGET, mpiwrapper_win_attr_fns,                        \
                       MPI_Win_copy_attr_function,                             \
                       MPI_Win_delete_attr_function, abi_win_copy_attr_fn,     \
                       abi_win_delete_attr_fn, abi_extra_state,                \
                       abi_win_keyval)
#else
#  define BODY_MPI_Win_create_keyval(TARGET)                                   \
    {                                                                          \
      (void)abi_win_copy_attr_fn;                                              \
      (void)abi_win_delete_attr_fn;                                            \
      (void)abi_extra_state;                                                   \
      *abi_win_keyval = MPIABI_KEYVAL_INVALID;                                 \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Win_create_keyval(
    MPIABI_Win_copy_attr_function   *abi_win_copy_attr_fn,
    MPIABI_Win_delete_attr_function *abi_win_delete_attr_fn,
    int *abi_win_keyval, void *abi_extra_state)
    BODY_MPI_Win_create_keyval(MPI_Win_create_keyval)
int mpiwrapper_w_PMPI_Win_create_keyval(
    MPIABI_Win_copy_attr_function   *abi_win_copy_attr_fn,
    MPIABI_Win_delete_attr_function *abi_win_delete_attr_fn,
    int *abi_win_keyval, void *abi_extra_state)
    BODY_MPI_Win_create_keyval(PMPI_Win_create_keyval)

/* --------------------------------------------------- generalized requests -- */

#ifdef MPIWRAPPER_HAVE_MPI_Grequest_start
#  define BODY_MPI_Grequest_start(TARGET)                                      \
    {                                                                          \
      MPI_Grequest_query_function  *query_fn;                                  \
      MPI_Grequest_free_function   *free_fn;                                   \
      MPI_Grequest_cancel_function *cancel_fn;                                 \
      void                         *state;                                     \
      if (!mpiwrapper_grequest_fns(abi_query_fn, abi_free_fn, abi_cancel_fn,   \
                                   abi_extra_state, &query_fn, &free_fn,       \
                                   &cancel_fn, &state)) {                      \
        *abi_request = MPIABI_REQUEST_NULL;                                    \
        return MPIABI_ERR_INTERN;                                              \
      }                                                                        \
                                                                               \
      MPI_Request request;                                                     \
      const int   ierror =                                                     \
          TARGET(query_fn, free_fn, cancel_fn, state, &request);               \
      if (ierror != MPI_SUCCESS) {                                             \
        /* No callback will ever run, so this is the one path on which the     \
         * pair is ours to free (#6.2).                                        \
         */                                                                    \
        mpiwrapper_grequest_discard(state);                                    \
        *abi_request = MPIABI_REQUEST_NULL;                                    \
        return mpiwrapper_errorcode_toabi(ierror);                             \
      }                                                                        \
                                                                               \
      *abi_request = mpiwrapper_request_toabi(request);                        \
      if (mpiwrapper_take_handle_error()) return MPIABI_ERR_INTERN;            \
      return MPIABI_SUCCESS;                                                   \
    }
#else
#  define BODY_MPI_Grequest_start(TARGET)                                      \
    {                                                                          \
      (void)abi_query_fn;                                                      \
      (void)abi_free_fn;                                                       \
      (void)abi_cancel_fn;                                                     \
      (void)abi_extra_state;                                                   \
      *abi_request = MPIABI_REQUEST_NULL;                                      \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Grequest_start(
    MPIABI_Grequest_query_function *abi_query_fn,
    MPIABI_Grequest_free_function  *abi_free_fn,
    MPIABI_Grequest_cancel_function *abi_cancel_fn, void *abi_extra_state,
    MPIABI_Request *abi_request) BODY_MPI_Grequest_start(MPI_Grequest_start)
int mpiwrapper_w_PMPI_Grequest_start(
    MPIABI_Grequest_query_function *abi_query_fn,
    MPIABI_Grequest_free_function  *abi_free_fn,
    MPIABI_Grequest_cancel_function *abi_cancel_fn, void *abi_extra_state,
    MPIABI_Request *abi_request) BODY_MPI_Grequest_start(PMPI_Grequest_start)

/* --------------------------------------------------------------- datareps -- */

/* The datarep *name* crosses unconverted: it is a string the application
 * chose, and MPI_File_set_view will hand the same string back to the
 * implementation. Only the three functions and the extra state need a pair --
 * and nothing here is ever reclaimed, because MPI has no call that
 * deregisters a datarep (#6.2).
 */
#ifdef MPIWRAPPER_HAVE_MPI_Register_datarep
#  define BODY_MPI_Register_datarep(TARGET)                                    \
    {                                                                          \
      const char *const datarep = abi_datarep;                                 \
                                                                               \
      MPI_Datarep_conversion_function *read_fn;                                \
      MPI_Datarep_conversion_function *write_fn;                               \
      MPI_Datarep_extent_function     *extent_fn;                              \
      void                            *state;                                  \
      if (!mpiwrapper_datarep_fns(abi_read_conversion_fn,                      \
                                  abi_write_conversion_fn,                     \
                                  abi_dtype_file_extent_fn, abi_extra_state,   \
                                  &read_fn, &write_fn, &extent_fn, &state))    \
        return MPIABI_ERR_INTERN;                                              \
                                                                               \
      const int ierror =                                                       \
          TARGET(datarep, read_fn, write_fn, extent_fn, state);                \
      if (ierror != MPI_SUCCESS) mpiwrapper_datarep_discard(state);            \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_Register_datarep(TARGET)                                    \
    {                                                                          \
      (void)abi_datarep;                                                       \
      (void)abi_read_conversion_fn;                                            \
      (void)abi_write_conversion_fn;                                           \
      (void)abi_dtype_file_extent_fn;                                          \
      (void)abi_extra_state;                                                   \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Register_datarep(
    const char *abi_datarep,
    MPIABI_Datarep_conversion_function *abi_read_conversion_fn,
    MPIABI_Datarep_conversion_function *abi_write_conversion_fn,
    MPIABI_Datarep_extent_function     *abi_dtype_file_extent_fn,
    void *abi_extra_state) BODY_MPI_Register_datarep(MPI_Register_datarep)
int mpiwrapper_w_PMPI_Register_datarep(
    const char *abi_datarep,
    MPIABI_Datarep_conversion_function *abi_read_conversion_fn,
    MPIABI_Datarep_conversion_function *abi_write_conversion_fn,
    MPIABI_Datarep_extent_function     *abi_dtype_file_extent_fn,
    void *abi_extra_state) BODY_MPI_Register_datarep(PMPI_Register_datarep)

#ifdef MPIWRAPPER_HAVE_MPI_Register_datarep_c
#  define BODY_MPI_Register_datarep_c(TARGET)                                  \
    {                                                                          \
      const char *const datarep = abi_datarep;                                 \
                                                                               \
      MPI_Datarep_conversion_function_c *read_fn;                              \
      MPI_Datarep_conversion_function_c *write_fn;                             \
      MPI_Datarep_extent_function       *extent_fn;                            \
      void                              *state;                                \
      if (!mpiwrapper_datarep_c_fns(abi_read_conversion_fn,                    \
                                    abi_write_conversion_fn,                   \
                                    abi_dtype_file_extent_fn, abi_extra_state, \
                                    &read_fn, &write_fn, &extent_fn, &state))  \
        return MPIABI_ERR_INTERN;                                              \
                                                                               \
      const int ierror =                                                       \
          TARGET(datarep, read_fn, write_fn, extent_fn, state);                \
      if (ierror != MPI_SUCCESS) mpiwrapper_datarep_discard(state);            \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_Register_datarep_c(TARGET)                                  \
    {                                                                          \
      (void)abi_datarep;                                                       \
      (void)abi_read_conversion_fn;                                            \
      (void)abi_write_conversion_fn;                                           \
      (void)abi_dtype_file_extent_fn;                                          \
      (void)abi_extra_state;                                                   \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_Register_datarep_c(
    const char *abi_datarep,
    MPIABI_Datarep_conversion_function_c *abi_read_conversion_fn,
    MPIABI_Datarep_conversion_function_c *abi_write_conversion_fn,
    MPIABI_Datarep_extent_function       *abi_dtype_file_extent_fn,
    void *abi_extra_state) BODY_MPI_Register_datarep_c(MPI_Register_datarep_c)
int mpiwrapper_w_PMPI_Register_datarep_c(
    const char *abi_datarep,
    MPIABI_Datarep_conversion_function_c *abi_read_conversion_fn,
    MPIABI_Datarep_conversion_function_c *abi_write_conversion_fn,
    MPIABI_Datarep_extent_function       *abi_dtype_file_extent_fn,
    void *abi_extra_state) BODY_MPI_Register_datarep_c(PMPI_Register_datarep_c)

/* ------------------------------------------------------------ MPI_T events */

/* The map in toolevents.c is filled *before* the implementation is told about
 * the trampoline, because an event may be raised the moment the registration
 * exists -- 15.3.6's own advice to users is about exactly that ordering.
 *
 * A null callback is passed on as a null callback rather than as a trampoline
 * with nothing behind it: the standard makes it the way to *remove* an
 * association, and a trampoline that returns immediately would leave the
 * implementation still raising events into us.
 */
#ifdef MPIWRAPPER_HAVE_MPI_T_event_register_callback
#  define BODY_MPI_T_event_register_callback(TARGET)                           \
    {                                                                          \
      const MPI_T_event_registration event_registration =                      \
          mpiwrapper_t_event_registration_fromabi(abi_event_registration);     \
      const MPI_T_cb_safety cb_safety =                                        \
          (MPI_T_cb_safety)mpiwrapper_tcbsafety_fromabi(abi_cb_safety);        \
      const MPI_Info info = mpiwrapper_info_fromabi(abi_info);                 \
                                                                               \
      if (!mpiwrapper_t_event_set_cb(event_registration, abi_cb_safety,        \
                                     abi_event_cb_function, abi_user_data))    \
        return MPIABI_ERR_INTERN; /* MPIWRAPPER_T_EVENT_SLOTS registrations */ \
                                                                               \
      const int ierror = TARGET(event_registration, cb_safety, info, NULL,     \
                                abi_event_cb_function                          \
                                    ? mpiwrapper_t_event_cb_tramp              \
                                    : NULL);                                   \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_T_event_register_callback(TARGET)                           \
    {                                                                          \
      (void)abi_event_registration;                                            \
      (void)abi_cb_safety;                                                     \
      (void)abi_info;                                                          \
      (void)abi_user_data;                                                     \
      (void)abi_event_cb_function;                                             \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_T_event_register_callback(
    MPIABI_T_event_registration abi_event_registration,
    MPIABI_T_cb_safety abi_cb_safety, MPIABI_Info abi_info,
    void *abi_user_data, MPIABI_T_event_cb_function *abi_event_cb_function)
    BODY_MPI_T_event_register_callback(MPI_T_event_register_callback)
int mpiwrapper_w_PMPI_T_event_register_callback(
    MPIABI_T_event_registration abi_event_registration,
    MPIABI_T_cb_safety abi_cb_safety, MPIABI_Info abi_info,
    void *abi_user_data, MPIABI_T_event_cb_function *abi_event_cb_function)
    BODY_MPI_T_event_register_callback(PMPI_T_event_register_callback)

#ifdef MPIWRAPPER_HAVE_MPI_T_event_set_dropped_handler
#  define BODY_MPI_T_event_set_dropped_handler(TARGET)                         \
    {                                                                          \
      const MPI_T_event_registration event_registration =                      \
          mpiwrapper_t_event_registration_fromabi(abi_event_registration);     \
                                                                               \
      if (!mpiwrapper_t_event_set_dropped(event_registration,                  \
                                          abi_dropped_cb_function))            \
        return MPIABI_ERR_INTERN;                                              \
                                                                               \
      const int ierror =                                                       \
          TARGET(event_registration, abi_dropped_cb_function                   \
                                         ? mpiwrapper_t_event_dropped_tramp    \
                                         : NULL);                              \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_T_event_set_dropped_handler(TARGET)                         \
    {                                                                          \
      (void)abi_event_registration;                                            \
      (void)abi_dropped_cb_function;                                           \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_T_event_set_dropped_handler(
    MPIABI_T_event_registration          abi_event_registration,
    MPIABI_T_event_dropped_cb_function *abi_dropped_cb_function)
    BODY_MPI_T_event_set_dropped_handler(MPI_T_event_set_dropped_handler)
int mpiwrapper_w_PMPI_T_event_set_dropped_handler(
    MPIABI_T_event_registration          abi_event_registration,
    MPIABI_T_event_dropped_cb_function *abi_dropped_cb_function)
    BODY_MPI_T_event_set_dropped_handler(PMPI_T_event_set_dropped_handler)

/* The free callback is #6.1's sixteenth registrar, found by the rule rather
 * than by the list: it takes a callback-*typed* parameter, so it converts a
 * registration handle and a safety level on the way back into user code and
 * needs a trampoline like the two above.
 *
 * Ours is installed even when the application passes none, because it is also
 * the map's reclamation point (#6.2) -- a tool that allocates and frees
 * registrations in a loop would otherwise fill the map with dead entries.
 */
#ifdef MPIWRAPPER_HAVE_MPI_T_event_handle_free
#  define BODY_MPI_T_event_handle_free(TARGET)                                 \
    {                                                                          \
      const MPI_T_event_registration event_registration =                      \
          mpiwrapper_t_event_registration_fromabi(abi_event_registration);     \
                                                                               \
      if (!mpiwrapper_t_event_set_free(event_registration,                     \
                                       abi_free_cb_function, abi_user_data))   \
        return MPIABI_ERR_INTERN;                                              \
                                                                               \
      const int ierror = TARGET(event_registration, NULL,                      \
                                mpiwrapper_t_event_free_tramp);                \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_T_event_handle_free(TARGET)                                 \
    {                                                                          \
      (void)abi_event_registration;                                            \
      (void)abi_user_data;                                                     \
      (void)abi_free_cb_function;                                              \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

int mpiwrapper_w_MPI_T_event_handle_free(
    MPIABI_T_event_registration       abi_event_registration,
    void *abi_user_data, MPIABI_T_event_free_cb_function *abi_free_cb_function)
    BODY_MPI_T_event_handle_free(MPI_T_event_handle_free)
int mpiwrapper_w_PMPI_T_event_handle_free(
    MPIABI_T_event_registration       abi_event_registration,
    void *abi_user_data, MPIABI_T_event_free_cb_function *abi_free_cb_function)
    BODY_MPI_T_event_handle_free(PMPI_T_event_handle_free)
