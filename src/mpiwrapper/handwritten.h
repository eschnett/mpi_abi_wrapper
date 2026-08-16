/* The hand-written entry-point bodies, for the vtable to point at.
 *
 * The generated wrappers.c holds one vtable initializer covering all 1376
 * slots, so it has to name the bodies the generator did *not* write. This is
 * that list: exactly the members of the HAND_WRITTEN ledger that have a body,
 * one declaration each, MPI_ and PMPI_ separately. If the two sets ever
 * disagree the initializer fails to compile, which is the cheapest possible
 * version of the ledger check -- and dev/generate.py reads this file to decide
 * which ledger entries are still stubs, so a declaration added here without a
 * definition is a link error rather than a silent claim.
 *
 * S1 covered eight entry points. S4a added 70 more -- the converter face: the
 * 44 handle converters and 4 status converters (hw_converters.c), the ten
 * status-consuming functions (hw_status.c), the ten output-string buffers with
 * no length argument (hw_strings.c) and the six MPI_Abi_* calls (hw_abi.c).
 * S4b finished the ledger with the 40 that need state the wrapper owns: the
 * lifecycle (hw_lifecycle.c), the fifteen callback registrars
 * (hw_callbacks.c), the twelve buffer forms (hw_buffers.c), the six dynamic
 * error-code forms (hw_errors.c), the two spawn forms (hw_spawn.c) and
 * MPI_Pcontrol (hw_pcontrol.c). S7 added the two attribute getters whose
 * *value* is a converted class (hw_attr.c), found by the MPICH suite rather
 * than by any assertion here. All 120 ledger entries have a body, which
 * dev/generate.py freezes as a tally of its own (NOTES.md #8).
 *
 * S1's handwritten.c is gone: each of its eight bodies belongs to a family
 * that now has a file, and a file named after the stage that wrote it was
 * never going to survive the stage after that.
 */

#ifndef MPIWRAPPER_HANDWRITTEN_H
#define MPIWRAPPER_HANDWRITTEN_H

#include "internal.h"

/* ------------------------------------------- lifecycle (hw_lifecycle.c) --- */

int mpiwrapper_w_MPI_Init(int *abi_argc, char ***abi_argv);
int mpiwrapper_w_PMPI_Init(int *abi_argc, char ***abi_argv);

int mpiwrapper_w_MPI_Init_thread(int *abi_argc, char ***abi_argv,
                                 int abi_required, int *abi_provided);
int mpiwrapper_w_PMPI_Init_thread(int *abi_argc, char ***abi_argv,
                                  int abi_required, int *abi_provided);

int mpiwrapper_w_MPI_Finalize(void);
int mpiwrapper_w_PMPI_Finalize(void);

int mpiwrapper_w_MPI_Initialized(int *abi_flag);
int mpiwrapper_w_PMPI_Initialized(int *abi_flag);

int mpiwrapper_w_MPI_Finalized(int *abi_flag);
int mpiwrapper_w_PMPI_Finalized(int *abi_flag);

int mpiwrapper_w_MPI_Abort(MPIABI_Comm abi_comm, int abi_errorcode);
int mpiwrapper_w_PMPI_Abort(MPIABI_Comm abi_comm, int abi_errorcode);

int mpiwrapper_w_MPI_Session_init(MPIABI_Info       abi_info,
                                  MPIABI_Errhandler abi_errhandler,
                                  MPIABI_Session   *abi_session);
int mpiwrapper_w_PMPI_Session_init(MPIABI_Info       abi_info,
                                   MPIABI_Errhandler abi_errhandler,
                                   MPIABI_Session   *abi_session);

int mpiwrapper_w_MPI_Session_finalize(MPIABI_Session *abi_session);
int mpiwrapper_w_PMPI_Session_finalize(MPIABI_Session *abi_session);

/* ----------------------------- callback registration (hw_callbacks.c) ----- */

int mpiwrapper_w_MPI_Op_create(MPIABI_User_function *abi_user_fn,
                               int abi_commute, MPIABI_Op *abi_op);
int mpiwrapper_w_PMPI_Op_create(MPIABI_User_function *abi_user_fn,
                                int abi_commute, MPIABI_Op *abi_op);

int mpiwrapper_w_MPI_Op_create_c(MPIABI_User_function_c *abi_user_fn,
                                 int abi_commute, MPIABI_Op *abi_op);
int mpiwrapper_w_PMPI_Op_create_c(MPIABI_User_function_c *abi_user_fn,
                                  int abi_commute, MPIABI_Op *abi_op);

int mpiwrapper_w_MPI_Comm_create_errhandler(
    MPIABI_Comm_errhandler_function *abi_comm_errhandler_fn,
    MPIABI_Errhandler               *abi_errhandler);
int mpiwrapper_w_PMPI_Comm_create_errhandler(
    MPIABI_Comm_errhandler_function *abi_comm_errhandler_fn,
    MPIABI_Errhandler               *abi_errhandler);

int mpiwrapper_w_MPI_File_create_errhandler(
    MPIABI_File_errhandler_function *abi_file_errhandler_fn,
    MPIABI_Errhandler               *abi_errhandler);
int mpiwrapper_w_PMPI_File_create_errhandler(
    MPIABI_File_errhandler_function *abi_file_errhandler_fn,
    MPIABI_Errhandler               *abi_errhandler);

int mpiwrapper_w_MPI_Win_create_errhandler(
    MPIABI_Win_errhandler_function *abi_win_errhandler_fn,
    MPIABI_Errhandler              *abi_errhandler);
int mpiwrapper_w_PMPI_Win_create_errhandler(
    MPIABI_Win_errhandler_function *abi_win_errhandler_fn,
    MPIABI_Errhandler              *abi_errhandler);

int mpiwrapper_w_MPI_Session_create_errhandler(
    MPIABI_Session_errhandler_function *abi_session_errhandler_fn,
    MPIABI_Errhandler                  *abi_errhandler);
int mpiwrapper_w_PMPI_Session_create_errhandler(
    MPIABI_Session_errhandler_function *abi_session_errhandler_fn,
    MPIABI_Errhandler                  *abi_errhandler);

int mpiwrapper_w_MPI_Comm_create_keyval(
    MPIABI_Comm_copy_attr_function   *abi_comm_copy_attr_fn,
    MPIABI_Comm_delete_attr_function *abi_comm_delete_attr_fn,
    int *abi_comm_keyval, void *abi_extra_state);
int mpiwrapper_w_PMPI_Comm_create_keyval(
    MPIABI_Comm_copy_attr_function   *abi_comm_copy_attr_fn,
    MPIABI_Comm_delete_attr_function *abi_comm_delete_attr_fn,
    int *abi_comm_keyval, void *abi_extra_state);

int mpiwrapper_w_MPI_Type_create_keyval(
    MPIABI_Type_copy_attr_function   *abi_type_copy_attr_fn,
    MPIABI_Type_delete_attr_function *abi_type_delete_attr_fn,
    int *abi_type_keyval, void *abi_extra_state);
int mpiwrapper_w_PMPI_Type_create_keyval(
    MPIABI_Type_copy_attr_function   *abi_type_copy_attr_fn,
    MPIABI_Type_delete_attr_function *abi_type_delete_attr_fn,
    int *abi_type_keyval, void *abi_extra_state);

int mpiwrapper_w_MPI_Win_create_keyval(
    MPIABI_Win_copy_attr_function   *abi_win_copy_attr_fn,
    MPIABI_Win_delete_attr_function *abi_win_delete_attr_fn,
    int *abi_win_keyval, void *abi_extra_state);
int mpiwrapper_w_PMPI_Win_create_keyval(
    MPIABI_Win_copy_attr_function   *abi_win_copy_attr_fn,
    MPIABI_Win_delete_attr_function *abi_win_delete_attr_fn,
    int *abi_win_keyval, void *abi_extra_state);

int mpiwrapper_w_MPI_Grequest_start(
    MPIABI_Grequest_query_function  *abi_query_fn,
    MPIABI_Grequest_free_function   *abi_free_fn,
    MPIABI_Grequest_cancel_function *abi_cancel_fn, void *abi_extra_state,
    MPIABI_Request                  *abi_request);
int mpiwrapper_w_PMPI_Grequest_start(
    MPIABI_Grequest_query_function  *abi_query_fn,
    MPIABI_Grequest_free_function   *abi_free_fn,
    MPIABI_Grequest_cancel_function *abi_cancel_fn, void *abi_extra_state,
    MPIABI_Request                  *abi_request);

int mpiwrapper_w_MPI_Register_datarep(
    const char                         *abi_datarep,
    MPIABI_Datarep_conversion_function *abi_read_conversion_fn,
    MPIABI_Datarep_conversion_function *abi_write_conversion_fn,
    MPIABI_Datarep_extent_function     *abi_dtype_file_extent_fn,
    void                               *abi_extra_state);
int mpiwrapper_w_PMPI_Register_datarep(
    const char                         *abi_datarep,
    MPIABI_Datarep_conversion_function *abi_read_conversion_fn,
    MPIABI_Datarep_conversion_function *abi_write_conversion_fn,
    MPIABI_Datarep_extent_function     *abi_dtype_file_extent_fn,
    void                               *abi_extra_state);

int mpiwrapper_w_MPI_Register_datarep_c(
    const char                           *abi_datarep,
    MPIABI_Datarep_conversion_function_c *abi_read_conversion_fn,
    MPIABI_Datarep_conversion_function_c *abi_write_conversion_fn,
    MPIABI_Datarep_extent_function       *abi_dtype_file_extent_fn,
    void                                 *abi_extra_state);
int mpiwrapper_w_PMPI_Register_datarep_c(
    const char                           *abi_datarep,
    MPIABI_Datarep_conversion_function_c *abi_read_conversion_fn,
    MPIABI_Datarep_conversion_function_c *abi_write_conversion_fn,
    MPIABI_Datarep_extent_function       *abi_dtype_file_extent_fn,
    void                                 *abi_extra_state);

int mpiwrapper_w_MPI_T_event_register_callback(
    MPIABI_T_event_registration abi_event_registration,
    MPIABI_T_cb_safety abi_cb_safety, MPIABI_Info abi_info,
    void *abi_user_data, MPIABI_T_event_cb_function *abi_event_cb_function);
int mpiwrapper_w_PMPI_T_event_register_callback(
    MPIABI_T_event_registration abi_event_registration,
    MPIABI_T_cb_safety abi_cb_safety, MPIABI_Info abi_info,
    void *abi_user_data, MPIABI_T_event_cb_function *abi_event_cb_function);

int mpiwrapper_w_MPI_T_event_set_dropped_handler(
    MPIABI_T_event_registration         abi_event_registration,
    MPIABI_T_event_dropped_cb_function *abi_dropped_cb_function);
int mpiwrapper_w_PMPI_T_event_set_dropped_handler(
    MPIABI_T_event_registration         abi_event_registration,
    MPIABI_T_event_dropped_cb_function *abi_dropped_cb_function);

int mpiwrapper_w_MPI_T_event_handle_free(
    MPIABI_T_event_registration      abi_event_registration,
    void *abi_user_data, MPIABI_T_event_free_cb_function *abi_free_cb_function);
int mpiwrapper_w_PMPI_T_event_handle_free(
    MPIABI_T_event_registration      abi_event_registration,
    void *abi_user_data, MPIABI_T_event_free_cb_function *abi_free_cb_function);

/* ------------------------------ buffer attach and detach (hw_buffers.c) --- */

int mpiwrapper_w_MPI_Buffer_attach(void *abi_buffer, int abi_size);
int mpiwrapper_w_PMPI_Buffer_attach(void *abi_buffer, int abi_size);

int mpiwrapper_w_MPI_Buffer_attach_c(void *abi_buffer, MPIABI_Count abi_size);
int mpiwrapper_w_PMPI_Buffer_attach_c(void *abi_buffer, MPIABI_Count abi_size);

int mpiwrapper_w_MPI_Buffer_detach(void *abi_buffer_addr, int *abi_size);
int mpiwrapper_w_PMPI_Buffer_detach(void *abi_buffer_addr, int *abi_size);

int mpiwrapper_w_MPI_Buffer_detach_c(void         *abi_buffer_addr,
                                     MPIABI_Count *abi_size);
int mpiwrapper_w_PMPI_Buffer_detach_c(void         *abi_buffer_addr,
                                      MPIABI_Count *abi_size);

int mpiwrapper_w_MPI_Comm_attach_buffer(MPIABI_Comm abi_comm, void *abi_buffer,
                                        int abi_size);
int mpiwrapper_w_PMPI_Comm_attach_buffer(MPIABI_Comm abi_comm, void *abi_buffer,
                                         int abi_size);

int mpiwrapper_w_MPI_Comm_attach_buffer_c(MPIABI_Comm abi_comm,
                                          void        *abi_buffer,
                                          MPIABI_Count abi_size);
int mpiwrapper_w_PMPI_Comm_attach_buffer_c(MPIABI_Comm abi_comm,
                                           void        *abi_buffer,
                                           MPIABI_Count abi_size);

int mpiwrapper_w_MPI_Comm_detach_buffer(MPIABI_Comm abi_comm,
                                        void *abi_buffer_addr, int *abi_size);
int mpiwrapper_w_PMPI_Comm_detach_buffer(MPIABI_Comm abi_comm,
                                         void *abi_buffer_addr, int *abi_size);

int mpiwrapper_w_MPI_Comm_detach_buffer_c(MPIABI_Comm   abi_comm,
                                          void         *abi_buffer_addr,
                                          MPIABI_Count *abi_size);
int mpiwrapper_w_PMPI_Comm_detach_buffer_c(MPIABI_Comm   abi_comm,
                                           void         *abi_buffer_addr,
                                           MPIABI_Count *abi_size);

int mpiwrapper_w_MPI_Session_attach_buffer(MPIABI_Session abi_session,
                                           void *abi_buffer, int abi_size);
int mpiwrapper_w_PMPI_Session_attach_buffer(MPIABI_Session abi_session,
                                            void *abi_buffer, int abi_size);

int mpiwrapper_w_MPI_Session_attach_buffer_c(MPIABI_Session abi_session,
                                             void        *abi_buffer,
                                             MPIABI_Count abi_size);
int mpiwrapper_w_PMPI_Session_attach_buffer_c(MPIABI_Session abi_session,
                                              void        *abi_buffer,
                                              MPIABI_Count abi_size);

int mpiwrapper_w_MPI_Session_detach_buffer(MPIABI_Session abi_session,
                                           void *abi_buffer_addr,
                                           int  *abi_size);
int mpiwrapper_w_PMPI_Session_detach_buffer(MPIABI_Session abi_session,
                                            void *abi_buffer_addr,
                                            int  *abi_size);

int mpiwrapper_w_MPI_Session_detach_buffer_c(MPIABI_Session abi_session,
                                             void         *abi_buffer_addr,
                                             MPIABI_Count *abi_size);
int mpiwrapper_w_PMPI_Session_detach_buffer_c(MPIABI_Session abi_session,
                                              void         *abi_buffer_addr,
                                              MPIABI_Count *abi_size);

/* -------------------------------- dynamic error codes (hw_errors.c) ------- */

int mpiwrapper_w_MPI_Add_error_class(int *abi_errorclass);
int mpiwrapper_w_PMPI_Add_error_class(int *abi_errorclass);

int mpiwrapper_w_MPI_Add_error_code(int abi_errorclass, int *abi_errorcode);
int mpiwrapper_w_PMPI_Add_error_code(int abi_errorclass, int *abi_errorcode);

int mpiwrapper_w_MPI_Add_error_string(int abi_errorcode, const char *abi_string);
int mpiwrapper_w_PMPI_Add_error_string(int         abi_errorcode,
                                       const char *abi_string);

int mpiwrapper_w_MPI_Remove_error_class(int abi_errorclass);
int mpiwrapper_w_PMPI_Remove_error_class(int abi_errorclass);

int mpiwrapper_w_MPI_Remove_error_code(int abi_errorcode);
int mpiwrapper_w_PMPI_Remove_error_code(int abi_errorcode);

int mpiwrapper_w_MPI_Remove_error_string(int abi_errorcode);
int mpiwrapper_w_PMPI_Remove_error_string(int abi_errorcode);

/* --------------------------------------------------- spawn (hw_spawn.c) --- */

int mpiwrapper_w_MPI_Comm_spawn(const char *abi_command, char *abi_argv[],
                                int abi_maxprocs, MPIABI_Info abi_info,
                                int abi_root, MPIABI_Comm abi_comm,
                                MPIABI_Comm *abi_intercomm,
                                int          abi_array_of_errcodes[]);
int mpiwrapper_w_PMPI_Comm_spawn(const char *abi_command, char *abi_argv[],
                                 int abi_maxprocs, MPIABI_Info abi_info,
                                 int abi_root, MPIABI_Comm abi_comm,
                                 MPIABI_Comm *abi_intercomm,
                                 int          abi_array_of_errcodes[]);

int mpiwrapper_w_MPI_Comm_spawn_multiple(
    int abi_count, char *abi_array_of_commands[], char **abi_array_of_argv[],
    const int abi_array_of_maxprocs[], const MPIABI_Info abi_array_of_info[],
    int abi_root, MPIABI_Comm abi_comm, MPIABI_Comm *abi_intercomm,
    int abi_array_of_errcodes[]);
int mpiwrapper_w_PMPI_Comm_spawn_multiple(
    int abi_count, char *abi_array_of_commands[], char **abi_array_of_argv[],
    const int abi_array_of_maxprocs[], const MPIABI_Info abi_array_of_info[],
    int abi_root, MPIABI_Comm abi_comm, MPIABI_Comm *abi_intercomm,
    int abi_array_of_errcodes[]);

/* ------------------------------------------ MPI_Pcontrol (hw_pcontrol.c) -- */

int mpiwrapper_w_MPI_Pcontrol(const int abi_level, ...);
int mpiwrapper_w_PMPI_Pcontrol(const int abi_level, ...);

/* ---------------------- the 44 handle converters (hw_converters.c) ------ */


MPIABI_Fint mpiwrapper_w_MPI_Comm_c2f(MPIABI_Comm abi_comm);
MPIABI_Fint mpiwrapper_w_PMPI_Comm_c2f(MPIABI_Comm abi_comm);
MPIABI_Comm mpiwrapper_w_MPI_Comm_f2c(MPIABI_Fint abi_comm);
MPIABI_Comm mpiwrapper_w_PMPI_Comm_f2c(MPIABI_Fint abi_comm);
int mpiwrapper_w_MPI_Comm_toint(MPIABI_Comm abi_comm);
int mpiwrapper_w_PMPI_Comm_toint(MPIABI_Comm abi_comm);
MPIABI_Comm mpiwrapper_w_MPI_Comm_fromint(int abi_comm);
MPIABI_Comm mpiwrapper_w_PMPI_Comm_fromint(int abi_comm);

MPIABI_Fint mpiwrapper_w_MPI_Errhandler_c2f(MPIABI_Errhandler abi_errhandler);
MPIABI_Fint mpiwrapper_w_PMPI_Errhandler_c2f(MPIABI_Errhandler abi_errhandler);
MPIABI_Errhandler mpiwrapper_w_MPI_Errhandler_f2c(MPIABI_Fint abi_errhandler);
MPIABI_Errhandler mpiwrapper_w_PMPI_Errhandler_f2c(MPIABI_Fint abi_errhandler);
int mpiwrapper_w_MPI_Errhandler_toint(MPIABI_Errhandler abi_errhandler);
int mpiwrapper_w_PMPI_Errhandler_toint(MPIABI_Errhandler abi_errhandler);
MPIABI_Errhandler mpiwrapper_w_MPI_Errhandler_fromint(int abi_errhandler);
MPIABI_Errhandler mpiwrapper_w_PMPI_Errhandler_fromint(int abi_errhandler);

MPIABI_Fint mpiwrapper_w_MPI_File_c2f(MPIABI_File abi_file);
MPIABI_Fint mpiwrapper_w_PMPI_File_c2f(MPIABI_File abi_file);
MPIABI_File mpiwrapper_w_MPI_File_f2c(MPIABI_Fint abi_file);
MPIABI_File mpiwrapper_w_PMPI_File_f2c(MPIABI_Fint abi_file);
int mpiwrapper_w_MPI_File_toint(MPIABI_File abi_file);
int mpiwrapper_w_PMPI_File_toint(MPIABI_File abi_file);
MPIABI_File mpiwrapper_w_MPI_File_fromint(int abi_file);
MPIABI_File mpiwrapper_w_PMPI_File_fromint(int abi_file);

MPIABI_Fint mpiwrapper_w_MPI_Group_c2f(MPIABI_Group abi_group);
MPIABI_Fint mpiwrapper_w_PMPI_Group_c2f(MPIABI_Group abi_group);
MPIABI_Group mpiwrapper_w_MPI_Group_f2c(MPIABI_Fint abi_group);
MPIABI_Group mpiwrapper_w_PMPI_Group_f2c(MPIABI_Fint abi_group);
int mpiwrapper_w_MPI_Group_toint(MPIABI_Group abi_group);
int mpiwrapper_w_PMPI_Group_toint(MPIABI_Group abi_group);
MPIABI_Group mpiwrapper_w_MPI_Group_fromint(int abi_group);
MPIABI_Group mpiwrapper_w_PMPI_Group_fromint(int abi_group);

MPIABI_Fint mpiwrapper_w_MPI_Info_c2f(MPIABI_Info abi_info);
MPIABI_Fint mpiwrapper_w_PMPI_Info_c2f(MPIABI_Info abi_info);
MPIABI_Info mpiwrapper_w_MPI_Info_f2c(MPIABI_Fint abi_info);
MPIABI_Info mpiwrapper_w_PMPI_Info_f2c(MPIABI_Fint abi_info);
int mpiwrapper_w_MPI_Info_toint(MPIABI_Info abi_info);
int mpiwrapper_w_PMPI_Info_toint(MPIABI_Info abi_info);
MPIABI_Info mpiwrapper_w_MPI_Info_fromint(int abi_info);
MPIABI_Info mpiwrapper_w_PMPI_Info_fromint(int abi_info);

MPIABI_Fint mpiwrapper_w_MPI_Message_c2f(MPIABI_Message abi_message);
MPIABI_Fint mpiwrapper_w_PMPI_Message_c2f(MPIABI_Message abi_message);
MPIABI_Message mpiwrapper_w_MPI_Message_f2c(MPIABI_Fint abi_message);
MPIABI_Message mpiwrapper_w_PMPI_Message_f2c(MPIABI_Fint abi_message);
int mpiwrapper_w_MPI_Message_toint(MPIABI_Message abi_message);
int mpiwrapper_w_PMPI_Message_toint(MPIABI_Message abi_message);
MPIABI_Message mpiwrapper_w_MPI_Message_fromint(int abi_message);
MPIABI_Message mpiwrapper_w_PMPI_Message_fromint(int abi_message);

MPIABI_Fint mpiwrapper_w_MPI_Op_c2f(MPIABI_Op abi_op);
MPIABI_Fint mpiwrapper_w_PMPI_Op_c2f(MPIABI_Op abi_op);
MPIABI_Op mpiwrapper_w_MPI_Op_f2c(MPIABI_Fint abi_op);
MPIABI_Op mpiwrapper_w_PMPI_Op_f2c(MPIABI_Fint abi_op);
int mpiwrapper_w_MPI_Op_toint(MPIABI_Op abi_op);
int mpiwrapper_w_PMPI_Op_toint(MPIABI_Op abi_op);
MPIABI_Op mpiwrapper_w_MPI_Op_fromint(int abi_op);
MPIABI_Op mpiwrapper_w_PMPI_Op_fromint(int abi_op);

MPIABI_Fint mpiwrapper_w_MPI_Request_c2f(MPIABI_Request abi_request);
MPIABI_Fint mpiwrapper_w_PMPI_Request_c2f(MPIABI_Request abi_request);
MPIABI_Request mpiwrapper_w_MPI_Request_f2c(MPIABI_Fint abi_request);
MPIABI_Request mpiwrapper_w_PMPI_Request_f2c(MPIABI_Fint abi_request);
int mpiwrapper_w_MPI_Request_toint(MPIABI_Request abi_request);
int mpiwrapper_w_PMPI_Request_toint(MPIABI_Request abi_request);
MPIABI_Request mpiwrapper_w_MPI_Request_fromint(int abi_request);
MPIABI_Request mpiwrapper_w_PMPI_Request_fromint(int abi_request);

MPIABI_Fint mpiwrapper_w_MPI_Session_c2f(MPIABI_Session abi_session);
MPIABI_Fint mpiwrapper_w_PMPI_Session_c2f(MPIABI_Session abi_session);
MPIABI_Session mpiwrapper_w_MPI_Session_f2c(MPIABI_Fint abi_session);
MPIABI_Session mpiwrapper_w_PMPI_Session_f2c(MPIABI_Fint abi_session);
int mpiwrapper_w_MPI_Session_toint(MPIABI_Session abi_session);
int mpiwrapper_w_PMPI_Session_toint(MPIABI_Session abi_session);
MPIABI_Session mpiwrapper_w_MPI_Session_fromint(int abi_session);
MPIABI_Session mpiwrapper_w_PMPI_Session_fromint(int abi_session);

MPIABI_Fint mpiwrapper_w_MPI_Type_c2f(MPIABI_Datatype abi_datatype);
MPIABI_Fint mpiwrapper_w_PMPI_Type_c2f(MPIABI_Datatype abi_datatype);
MPIABI_Datatype mpiwrapper_w_MPI_Type_f2c(MPIABI_Fint abi_datatype);
MPIABI_Datatype mpiwrapper_w_PMPI_Type_f2c(MPIABI_Fint abi_datatype);
int mpiwrapper_w_MPI_Type_toint(MPIABI_Datatype abi_datatype);
int mpiwrapper_w_PMPI_Type_toint(MPIABI_Datatype abi_datatype);
MPIABI_Datatype mpiwrapper_w_MPI_Type_fromint(int abi_datatype);
MPIABI_Datatype mpiwrapper_w_PMPI_Type_fromint(int abi_datatype);

MPIABI_Fint mpiwrapper_w_MPI_Win_c2f(MPIABI_Win abi_win);
MPIABI_Fint mpiwrapper_w_PMPI_Win_c2f(MPIABI_Win abi_win);
MPIABI_Win mpiwrapper_w_MPI_Win_f2c(MPIABI_Fint abi_win);
MPIABI_Win mpiwrapper_w_PMPI_Win_f2c(MPIABI_Fint abi_win);
int mpiwrapper_w_MPI_Win_toint(MPIABI_Win abi_win);
int mpiwrapper_w_PMPI_Win_toint(MPIABI_Win abi_win);
MPIABI_Win mpiwrapper_w_MPI_Win_fromint(int abi_win);
MPIABI_Win mpiwrapper_w_PMPI_Win_fromint(int abi_win);

/* ------------------------- the four status converters (hw_converters.c) --- */

int mpiwrapper_w_MPI_Status_c2f(const MPIABI_Status *abi_c_status,
                                MPIABI_Fint *abi_f_status);
int mpiwrapper_w_PMPI_Status_c2f(const MPIABI_Status *abi_c_status,
                                 MPIABI_Fint *abi_f_status);

int mpiwrapper_w_MPI_Status_f2c(const MPIABI_Fint *abi_f_status,
                                MPIABI_Status *abi_c_status);
int mpiwrapper_w_PMPI_Status_f2c(const MPIABI_Fint *abi_f_status,
                                 MPIABI_Status *abi_c_status);

int mpiwrapper_w_MPI_Status_c2f08(const MPIABI_Status *abi_c_status,
                                  MPIABI_F08_Status *abi_f08_status);
int mpiwrapper_w_PMPI_Status_c2f08(const MPIABI_Status *abi_c_status,
                                   MPIABI_F08_Status *abi_f08_status);

int mpiwrapper_w_MPI_Status_f082c(const MPIABI_F08_Status *abi_f08_status,
                                  MPIABI_Status *abi_c_status);
int mpiwrapper_w_PMPI_Status_f082c(const MPIABI_F08_Status *abi_f08_status,
                                   MPIABI_Status *abi_c_status);

/* -------------------- the ten status-consuming functions (hw_status.c) ---- */

int mpiwrapper_w_MPI_Get_count(const MPIABI_Status *abi_status,
                               MPIABI_Datatype abi_datatype, int *abi_count);
int mpiwrapper_w_PMPI_Get_count(const MPIABI_Status *abi_status,
                                MPIABI_Datatype abi_datatype, int *abi_count);

int mpiwrapper_w_MPI_Get_count_c(const MPIABI_Status *abi_status,
                                 MPIABI_Datatype abi_datatype,
                                 MPIABI_Count *abi_count);
int mpiwrapper_w_PMPI_Get_count_c(const MPIABI_Status *abi_status,
                                  MPIABI_Datatype abi_datatype,
                                  MPIABI_Count *abi_count);

int mpiwrapper_w_MPI_Get_elements(const MPIABI_Status *abi_status,
                                  MPIABI_Datatype abi_datatype, int *abi_count);
int mpiwrapper_w_PMPI_Get_elements(const MPIABI_Status *abi_status,
                                   MPIABI_Datatype abi_datatype,
                                   int *abi_count);

int mpiwrapper_w_MPI_Get_elements_c(const MPIABI_Status *abi_status,
                                    MPIABI_Datatype abi_datatype,
                                    MPIABI_Count *abi_count);
int mpiwrapper_w_PMPI_Get_elements_c(const MPIABI_Status *abi_status,
                                     MPIABI_Datatype abi_datatype,
                                     MPIABI_Count *abi_count);

int mpiwrapper_w_MPI_Get_elements_x(const MPIABI_Status *abi_status,
                                    MPIABI_Datatype abi_datatype,
                                    MPIABI_Count *abi_count);
int mpiwrapper_w_PMPI_Get_elements_x(const MPIABI_Status *abi_status,
                                     MPIABI_Datatype abi_datatype,
                                     MPIABI_Count *abi_count);

int mpiwrapper_w_MPI_Test_cancelled(const MPIABI_Status *abi_status,
                                    int *abi_flag);
int mpiwrapper_w_PMPI_Test_cancelled(const MPIABI_Status *abi_status,
                                     int *abi_flag);

int mpiwrapper_w_MPI_Status_set_cancelled(MPIABI_Status *abi_status,
                                          int abi_flag);
int mpiwrapper_w_PMPI_Status_set_cancelled(MPIABI_Status *abi_status,
                                           int abi_flag);

int mpiwrapper_w_MPI_Status_set_elements(MPIABI_Status *abi_status,
                                         MPIABI_Datatype abi_datatype,
                                         int abi_count);
int mpiwrapper_w_PMPI_Status_set_elements(MPIABI_Status *abi_status,
                                          MPIABI_Datatype abi_datatype,
                                          int abi_count);

int mpiwrapper_w_MPI_Status_set_elements_c(MPIABI_Status *abi_status,
                                           MPIABI_Datatype abi_datatype,
                                           MPIABI_Count abi_count);
int mpiwrapper_w_PMPI_Status_set_elements_c(MPIABI_Status *abi_status,
                                            MPIABI_Datatype abi_datatype,
                                            MPIABI_Count abi_count);

int mpiwrapper_w_MPI_Status_set_elements_x(MPIABI_Status *abi_status,
                                           MPIABI_Datatype abi_datatype,
                                           MPIABI_Count abi_count);
int mpiwrapper_w_PMPI_Status_set_elements_x(MPIABI_Status *abi_status,
                                            MPIABI_Datatype abi_datatype,
                                            MPIABI_Count abi_count);

/* ------------------- the ten output-string buffers (hw_strings.c) -------- */

int mpiwrapper_w_MPI_Error_string(int abi_errorcode, char *abi_string,
                                  int *abi_resultlen);
int mpiwrapper_w_PMPI_Error_string(int abi_errorcode, char *abi_string,
                                   int *abi_resultlen);

int mpiwrapper_w_MPI_Get_library_version(char *abi_version,
                                         int *abi_resultlen);
int mpiwrapper_w_PMPI_Get_library_version(char *abi_version,
                                          int *abi_resultlen);

int mpiwrapper_w_MPI_Get_processor_name(char *abi_name, int *abi_resultlen);
int mpiwrapper_w_PMPI_Get_processor_name(char *abi_name, int *abi_resultlen);

int mpiwrapper_w_MPI_Comm_get_name(MPIABI_Comm abi_comm, char *abi_comm_name,
                                   int *abi_resultlen);
int mpiwrapper_w_PMPI_Comm_get_name(MPIABI_Comm abi_comm, char *abi_comm_name,
                                    int *abi_resultlen);

int mpiwrapper_w_MPI_Type_get_name(MPIABI_Datatype abi_datatype,
                                   char *abi_type_name, int *abi_resultlen);
int mpiwrapper_w_PMPI_Type_get_name(MPIABI_Datatype abi_datatype,
                                    char *abi_type_name, int *abi_resultlen);

int mpiwrapper_w_MPI_Win_get_name(MPIABI_Win abi_win, char *abi_win_name,
                                  int *abi_resultlen);
int mpiwrapper_w_PMPI_Win_get_name(MPIABI_Win abi_win, char *abi_win_name,
                                   int *abi_resultlen);

int mpiwrapper_w_MPI_Open_port(MPIABI_Info abi_info, char *abi_port_name);
int mpiwrapper_w_PMPI_Open_port(MPIABI_Info abi_info, char *abi_port_name);

int mpiwrapper_w_MPI_Lookup_name(const char *abi_service_name,
                                 MPIABI_Info abi_info, char *abi_port_name);
int mpiwrapper_w_PMPI_Lookup_name(const char *abi_service_name,
                                  MPIABI_Info abi_info, char *abi_port_name);

int mpiwrapper_w_MPI_Info_get_nthkey(MPIABI_Info abi_info, int abi_n,
                                     char *abi_key);
int mpiwrapper_w_PMPI_Info_get_nthkey(MPIABI_Info abi_info, int abi_n,
                                      char *abi_key);

int mpiwrapper_w_MPI_File_get_view(MPIABI_File abi_fh, MPIABI_Offset *abi_disp,
                                   MPIABI_Datatype *abi_etype,
                                   MPIABI_Datatype *abi_filetype,
                                   char *abi_datarep);
int mpiwrapper_w_PMPI_File_get_view(MPIABI_File abi_fh, MPIABI_Offset *abi_disp,
                                    MPIABI_Datatype *abi_etype,
                                    MPIABI_Datatype *abi_filetype,
                                    char *abi_datarep);

/* --------------------------- ABI introspection (hw_abi.c) --------------- */

int mpiwrapper_w_MPI_Abi_get_version(int *abi_abi_major, int *abi_abi_minor);
int mpiwrapper_w_PMPI_Abi_get_version(int *abi_abi_major, int *abi_abi_minor);

int mpiwrapper_w_MPI_Abi_get_info(MPIABI_Info *abi_info);
int mpiwrapper_w_PMPI_Abi_get_info(MPIABI_Info *abi_info);

int mpiwrapper_w_MPI_Abi_get_fortran_info(MPIABI_Info *abi_info);
int mpiwrapper_w_PMPI_Abi_get_fortran_info(MPIABI_Info *abi_info);

int mpiwrapper_w_MPI_Abi_set_fortran_info(MPIABI_Info abi_info);
int mpiwrapper_w_PMPI_Abi_set_fortran_info(MPIABI_Info abi_info);

int mpiwrapper_w_MPI_Abi_get_fortran_booleans(int abi_logical_size,
                                              void *abi_logical_true,
                                              void *abi_logical_false,
                                              int *abi_is_set);
int mpiwrapper_w_PMPI_Abi_get_fortran_booleans(int abi_logical_size,
                                               void *abi_logical_true,
                                               void *abi_logical_false,
                                               int *abi_is_set);

int mpiwrapper_w_MPI_Abi_set_fortran_booleans(int abi_logical_size,
                                              void *abi_logical_true,
                                              void *abi_logical_false);
int mpiwrapper_w_PMPI_Abi_set_fortran_booleans(int abi_logical_size,
                                               void *abi_logical_true,
                                               void *abi_logical_false);

/* ------------------- attribute values that convert (hw_attr.c) ----------- */

int mpiwrapper_w_MPI_Comm_get_attr(MPIABI_Comm abi_comm, int abi_comm_keyval,
                                   void *abi_attribute_val, int *abi_flag);
int mpiwrapper_w_PMPI_Comm_get_attr(MPIABI_Comm abi_comm, int abi_comm_keyval,
                                    void *abi_attribute_val, int *abi_flag);

int mpiwrapper_w_MPI_Win_get_attr(MPIABI_Win abi_win, int abi_win_keyval,
                                  void *abi_attribute_val, int *abi_flag);
int mpiwrapper_w_PMPI_Win_get_attr(MPIABI_Win abi_win, int abi_win_keyval,
                                   void *abi_attribute_val, int *abi_flag);

#endif /* MPIWRAPPER_HANDWRITTEN_H */
