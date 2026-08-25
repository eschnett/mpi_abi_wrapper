/* libmpiwrapper -- the 46 converters: 44 handle converters and the four
 * Fortran status converters (NOTES.md #8, S4a).
 *
 * Two families that look like one and are not:
 *
 *   _c2f / _f2c   MPI-2's C-Fortran handle conversion. MPI-5.0 20.4 puts these
 *                 *outside* the ABI -- "the functions defined in Section 19.3.4
 *                 and Section 19.3.5 ... are not part of this ABI", because
 *                 they depend on MPI_Fint and so on the Fortran compiler's
 *                 INTEGER. Nothing therefore fixes what a Fortran handle value
 *                 has to be, and the wrapper forwards to the implementation's
 *                 own converter, which is what S1 chose for MPI_Comm and what
 *                 the other ten follow. mpif is the consumer and the oracle
 *                 (S8): a round trip through our own code in both directions
 *                 cannot see a Fortran integer the implementation would reject.
 *
 *   _toint /      MPI-5.0 20.4.5's handle serialization, which *is* part of the
 *   _fromint      ABI and which pins the answer: "for all predefined handles,
 *                 the integer value must be the same as the values listed in
 *                 Section A". Those are the ABI's own values, so forwarding to
 *                 an implementation's _toint would be wrong even where one
 *                 exists. These never call the implementation at all -- they
 *                 are pure ABI-side, like the six status accessors of #5.2 --
 *                 and serialize.c holds the table that makes a 64-bit dynamic
 *                 handle survive the trip through an int.
 *
 * Both families return a handle or an integer rather than an error code, so
 * they have nowhere to report a failure. A c2f/f2c pair over an implementation
 * that lacks the entry point answers 0 and the class's null handle
 * respectively; a _fromint given an integer this library never issued answers
 * the class's null handle. In each case the caller finds it invalid at the next
 * call, which is the same answer S1 settled on for a handle whose bits collide
 * with the ABI's predefined range.
 *
 * The four status converters are the third thing here, and NOTES.md #4.4 is
 * why they are memcpy-shaped: the ABI's Fortran status *is* the ABI's C status
 * -- eight ints, the three named fields at indices 0, 1 and 2 -- so the
 * conversion is a copy and the implementation is not involved. Forwarding to
 * the implementation's MPI_Status_c2f would be actively wrong: MPICH puts the
 * named fields at indices 2, 3 and 4, and a caller reading status(MPI_F_SOURCE)
 * out of the result would read a private byte.
 */

#include "internal.h"

#include <string.h>

/* ------------------------------------------------ the four body shapes ---- */

/* Instantiated once per handle class below. The class-specific parts are the
 * two conversion functions, the ABI type and the ABI null handle; everything
 * else is identical eleven times over, which is what makes a macro the honest
 * spelling here rather than a copy.
 */


/* The collision flag every other object-producing conversion turns into
 * MPIABI_ERR_INTERN has nowhere to go here, so it is cleared rather than left
 * set for the next call to blame -- a handle whose bits collide with the ABI's
 * predefined range converts to the class's null handle instead (S1's note on
 * MPI_Comm_f2c, now the rule for all eleven).
 */


/* No TARGET and no guard: serialization is defined against the ABI's own
 * handle values, so there is nothing to ask the implementation and nothing for
 * decision 6 to stub. The MPI_ and PMPI_ instances are therefore identical,
 * and are still two functions because the vtable has two slots (decision 7).
 */
#define BODY_TOINT(ABIARG)                                                     \
  {                                                                            \
    return mpiwrapper_handle_toint(MPIWRAPPER_BITS(ABIARG));                   \
  }

#define BODY_FROMINT(ABI_T, ABINULL, ABIARG)                                   \
  {                                                                            \
    uint64_t bits;                                                             \
    if (!mpiwrapper_handle_fromint(ABIARG, &bits)) return ABINULL;             \
    return MPIWRAPPER_HANDLE(ABI_T, bits);                                     \
  }


/* -------------------------------- MPI_Comm_c2f / _f2c / _toint / _fromint */

#define BODY_MPI_Comm_c2f(TARGET) BODY_TOINT(abi_comm)
#define BODY_MPI_Comm_f2c(TARGET) \
    BODY_FROMINT(MPIABI_Comm, MPIABI_COMM_NULL, abi_comm)

MPIABI_Fint mpiwrapper_w_MPI_Comm_c2f(MPIABI_Comm abi_comm)
    BODY_MPI_Comm_c2f(MPI_Comm_c2f)
MPIABI_Fint mpiwrapper_w_PMPI_Comm_c2f(MPIABI_Comm abi_comm)
    BODY_MPI_Comm_c2f(PMPI_Comm_c2f)

MPIABI_Comm mpiwrapper_w_MPI_Comm_f2c(MPIABI_Fint abi_comm)
    BODY_MPI_Comm_f2c(MPI_Comm_f2c)
MPIABI_Comm mpiwrapper_w_PMPI_Comm_f2c(MPIABI_Fint abi_comm)
    BODY_MPI_Comm_f2c(PMPI_Comm_f2c)

int mpiwrapper_w_MPI_Comm_toint(MPIABI_Comm abi_comm)
    BODY_TOINT(abi_comm)
int mpiwrapper_w_PMPI_Comm_toint(MPIABI_Comm abi_comm)
    BODY_TOINT(abi_comm)

MPIABI_Comm mpiwrapper_w_MPI_Comm_fromint(int abi_comm)
    BODY_FROMINT(MPIABI_Comm, MPIABI_COMM_NULL, abi_comm)
MPIABI_Comm mpiwrapper_w_PMPI_Comm_fromint(int abi_comm)
    BODY_FROMINT(MPIABI_Comm, MPIABI_COMM_NULL, abi_comm)

/* -------------------------- MPI_Errhandler_c2f / _f2c / _toint / _fromint */

#define BODY_MPI_Errhandler_c2f(TARGET) BODY_TOINT(abi_errhandler)
#define BODY_MPI_Errhandler_f2c(TARGET) \
    BODY_FROMINT(MPIABI_Errhandler, MPIABI_ERRHANDLER_NULL, abi_errhandler)

MPIABI_Fint mpiwrapper_w_MPI_Errhandler_c2f(MPIABI_Errhandler abi_errhandler)
    BODY_MPI_Errhandler_c2f(MPI_Errhandler_c2f)
MPIABI_Fint mpiwrapper_w_PMPI_Errhandler_c2f(MPIABI_Errhandler abi_errhandler)
    BODY_MPI_Errhandler_c2f(PMPI_Errhandler_c2f)

MPIABI_Errhandler mpiwrapper_w_MPI_Errhandler_f2c(MPIABI_Fint abi_errhandler)
    BODY_MPI_Errhandler_f2c(MPI_Errhandler_f2c)
MPIABI_Errhandler mpiwrapper_w_PMPI_Errhandler_f2c(MPIABI_Fint abi_errhandler)
    BODY_MPI_Errhandler_f2c(PMPI_Errhandler_f2c)

int mpiwrapper_w_MPI_Errhandler_toint(MPIABI_Errhandler abi_errhandler)
    BODY_TOINT(abi_errhandler)
int mpiwrapper_w_PMPI_Errhandler_toint(MPIABI_Errhandler abi_errhandler)
    BODY_TOINT(abi_errhandler)

MPIABI_Errhandler mpiwrapper_w_MPI_Errhandler_fromint(int abi_errhandler)
    BODY_FROMINT(MPIABI_Errhandler, MPIABI_ERRHANDLER_NULL, abi_errhandler)
MPIABI_Errhandler mpiwrapper_w_PMPI_Errhandler_fromint(int abi_errhandler)
    BODY_FROMINT(MPIABI_Errhandler, MPIABI_ERRHANDLER_NULL, abi_errhandler)

/* -------------------------------- MPI_File_c2f / _f2c / _toint / _fromint */

#define BODY_MPI_File_c2f(TARGET) BODY_TOINT(abi_file)
#define BODY_MPI_File_f2c(TARGET) \
    BODY_FROMINT(MPIABI_File, MPIABI_FILE_NULL, abi_file)

MPIABI_Fint mpiwrapper_w_MPI_File_c2f(MPIABI_File abi_file)
    BODY_MPI_File_c2f(MPI_File_c2f)
MPIABI_Fint mpiwrapper_w_PMPI_File_c2f(MPIABI_File abi_file)
    BODY_MPI_File_c2f(PMPI_File_c2f)

MPIABI_File mpiwrapper_w_MPI_File_f2c(MPIABI_Fint abi_file)
    BODY_MPI_File_f2c(MPI_File_f2c)
MPIABI_File mpiwrapper_w_PMPI_File_f2c(MPIABI_Fint abi_file)
    BODY_MPI_File_f2c(PMPI_File_f2c)

int mpiwrapper_w_MPI_File_toint(MPIABI_File abi_file)
    BODY_TOINT(abi_file)
int mpiwrapper_w_PMPI_File_toint(MPIABI_File abi_file)
    BODY_TOINT(abi_file)

MPIABI_File mpiwrapper_w_MPI_File_fromint(int abi_file)
    BODY_FROMINT(MPIABI_File, MPIABI_FILE_NULL, abi_file)
MPIABI_File mpiwrapper_w_PMPI_File_fromint(int abi_file)
    BODY_FROMINT(MPIABI_File, MPIABI_FILE_NULL, abi_file)

/* ------------------------------- MPI_Group_c2f / _f2c / _toint / _fromint */

#define BODY_MPI_Group_c2f(TARGET) BODY_TOINT(abi_group)
#define BODY_MPI_Group_f2c(TARGET) \
    BODY_FROMINT(MPIABI_Group, MPIABI_GROUP_NULL, abi_group)

MPIABI_Fint mpiwrapper_w_MPI_Group_c2f(MPIABI_Group abi_group)
    BODY_MPI_Group_c2f(MPI_Group_c2f)
MPIABI_Fint mpiwrapper_w_PMPI_Group_c2f(MPIABI_Group abi_group)
    BODY_MPI_Group_c2f(PMPI_Group_c2f)

MPIABI_Group mpiwrapper_w_MPI_Group_f2c(MPIABI_Fint abi_group)
    BODY_MPI_Group_f2c(MPI_Group_f2c)
MPIABI_Group mpiwrapper_w_PMPI_Group_f2c(MPIABI_Fint abi_group)
    BODY_MPI_Group_f2c(PMPI_Group_f2c)

int mpiwrapper_w_MPI_Group_toint(MPIABI_Group abi_group)
    BODY_TOINT(abi_group)
int mpiwrapper_w_PMPI_Group_toint(MPIABI_Group abi_group)
    BODY_TOINT(abi_group)

MPIABI_Group mpiwrapper_w_MPI_Group_fromint(int abi_group)
    BODY_FROMINT(MPIABI_Group, MPIABI_GROUP_NULL, abi_group)
MPIABI_Group mpiwrapper_w_PMPI_Group_fromint(int abi_group)
    BODY_FROMINT(MPIABI_Group, MPIABI_GROUP_NULL, abi_group)

/* -------------------------------- MPI_Info_c2f / _f2c / _toint / _fromint */

#define BODY_MPI_Info_c2f(TARGET) BODY_TOINT(abi_info)
#define BODY_MPI_Info_f2c(TARGET) \
    BODY_FROMINT(MPIABI_Info, MPIABI_INFO_NULL, abi_info)

MPIABI_Fint mpiwrapper_w_MPI_Info_c2f(MPIABI_Info abi_info)
    BODY_MPI_Info_c2f(MPI_Info_c2f)
MPIABI_Fint mpiwrapper_w_PMPI_Info_c2f(MPIABI_Info abi_info)
    BODY_MPI_Info_c2f(PMPI_Info_c2f)

MPIABI_Info mpiwrapper_w_MPI_Info_f2c(MPIABI_Fint abi_info)
    BODY_MPI_Info_f2c(MPI_Info_f2c)
MPIABI_Info mpiwrapper_w_PMPI_Info_f2c(MPIABI_Fint abi_info)
    BODY_MPI_Info_f2c(PMPI_Info_f2c)

int mpiwrapper_w_MPI_Info_toint(MPIABI_Info abi_info)
    BODY_TOINT(abi_info)
int mpiwrapper_w_PMPI_Info_toint(MPIABI_Info abi_info)
    BODY_TOINT(abi_info)

MPIABI_Info mpiwrapper_w_MPI_Info_fromint(int abi_info)
    BODY_FROMINT(MPIABI_Info, MPIABI_INFO_NULL, abi_info)
MPIABI_Info mpiwrapper_w_PMPI_Info_fromint(int abi_info)
    BODY_FROMINT(MPIABI_Info, MPIABI_INFO_NULL, abi_info)

/* ----------------------------- MPI_Message_c2f / _f2c / _toint / _fromint */

#define BODY_MPI_Message_c2f(TARGET) BODY_TOINT(abi_message)
#define BODY_MPI_Message_f2c(TARGET) \
    BODY_FROMINT(MPIABI_Message, MPIABI_MESSAGE_NULL, abi_message)

MPIABI_Fint mpiwrapper_w_MPI_Message_c2f(MPIABI_Message abi_message)
    BODY_MPI_Message_c2f(MPI_Message_c2f)
MPIABI_Fint mpiwrapper_w_PMPI_Message_c2f(MPIABI_Message abi_message)
    BODY_MPI_Message_c2f(PMPI_Message_c2f)

MPIABI_Message mpiwrapper_w_MPI_Message_f2c(MPIABI_Fint abi_message)
    BODY_MPI_Message_f2c(MPI_Message_f2c)
MPIABI_Message mpiwrapper_w_PMPI_Message_f2c(MPIABI_Fint abi_message)
    BODY_MPI_Message_f2c(PMPI_Message_f2c)

int mpiwrapper_w_MPI_Message_toint(MPIABI_Message abi_message)
    BODY_TOINT(abi_message)
int mpiwrapper_w_PMPI_Message_toint(MPIABI_Message abi_message)
    BODY_TOINT(abi_message)

MPIABI_Message mpiwrapper_w_MPI_Message_fromint(int abi_message)
    BODY_FROMINT(MPIABI_Message, MPIABI_MESSAGE_NULL, abi_message)
MPIABI_Message mpiwrapper_w_PMPI_Message_fromint(int abi_message)
    BODY_FROMINT(MPIABI_Message, MPIABI_MESSAGE_NULL, abi_message)

/* ---------------------------------- MPI_Op_c2f / _f2c / _toint / _fromint */

#define BODY_MPI_Op_c2f(TARGET) BODY_TOINT(abi_op)
#define BODY_MPI_Op_f2c(TARGET) \
    BODY_FROMINT(MPIABI_Op, MPIABI_OP_NULL, abi_op)

MPIABI_Fint mpiwrapper_w_MPI_Op_c2f(MPIABI_Op abi_op)
    BODY_MPI_Op_c2f(MPI_Op_c2f)
MPIABI_Fint mpiwrapper_w_PMPI_Op_c2f(MPIABI_Op abi_op)
    BODY_MPI_Op_c2f(PMPI_Op_c2f)

MPIABI_Op mpiwrapper_w_MPI_Op_f2c(MPIABI_Fint abi_op)
    BODY_MPI_Op_f2c(MPI_Op_f2c)
MPIABI_Op mpiwrapper_w_PMPI_Op_f2c(MPIABI_Fint abi_op)
    BODY_MPI_Op_f2c(PMPI_Op_f2c)

int mpiwrapper_w_MPI_Op_toint(MPIABI_Op abi_op)
    BODY_TOINT(abi_op)
int mpiwrapper_w_PMPI_Op_toint(MPIABI_Op abi_op)
    BODY_TOINT(abi_op)

MPIABI_Op mpiwrapper_w_MPI_Op_fromint(int abi_op)
    BODY_FROMINT(MPIABI_Op, MPIABI_OP_NULL, abi_op)
MPIABI_Op mpiwrapper_w_PMPI_Op_fromint(int abi_op)
    BODY_FROMINT(MPIABI_Op, MPIABI_OP_NULL, abi_op)

/* ----------------------------- MPI_Request_c2f / _f2c / _toint / _fromint */

#define BODY_MPI_Request_c2f(TARGET) BODY_TOINT(abi_request)
#define BODY_MPI_Request_f2c(TARGET) \
    BODY_FROMINT(MPIABI_Request, MPIABI_REQUEST_NULL, abi_request)

MPIABI_Fint mpiwrapper_w_MPI_Request_c2f(MPIABI_Request abi_request)
    BODY_MPI_Request_c2f(MPI_Request_c2f)
MPIABI_Fint mpiwrapper_w_PMPI_Request_c2f(MPIABI_Request abi_request)
    BODY_MPI_Request_c2f(PMPI_Request_c2f)

MPIABI_Request mpiwrapper_w_MPI_Request_f2c(MPIABI_Fint abi_request)
    BODY_MPI_Request_f2c(MPI_Request_f2c)
MPIABI_Request mpiwrapper_w_PMPI_Request_f2c(MPIABI_Fint abi_request)
    BODY_MPI_Request_f2c(PMPI_Request_f2c)

int mpiwrapper_w_MPI_Request_toint(MPIABI_Request abi_request)
    BODY_TOINT(abi_request)
int mpiwrapper_w_PMPI_Request_toint(MPIABI_Request abi_request)
    BODY_TOINT(abi_request)

MPIABI_Request mpiwrapper_w_MPI_Request_fromint(int abi_request)
    BODY_FROMINT(MPIABI_Request, MPIABI_REQUEST_NULL, abi_request)
MPIABI_Request mpiwrapper_w_PMPI_Request_fromint(int abi_request)
    BODY_FROMINT(MPIABI_Request, MPIABI_REQUEST_NULL, abi_request)

/* ----------------------------- MPI_Session_c2f / _f2c / _toint / _fromint */

#define BODY_MPI_Session_c2f(TARGET) BODY_TOINT(abi_session)
#define BODY_MPI_Session_f2c(TARGET) \
    BODY_FROMINT(MPIABI_Session, MPIABI_SESSION_NULL, abi_session)

MPIABI_Fint mpiwrapper_w_MPI_Session_c2f(MPIABI_Session abi_session)
    BODY_MPI_Session_c2f(MPI_Session_c2f)
MPIABI_Fint mpiwrapper_w_PMPI_Session_c2f(MPIABI_Session abi_session)
    BODY_MPI_Session_c2f(PMPI_Session_c2f)

MPIABI_Session mpiwrapper_w_MPI_Session_f2c(MPIABI_Fint abi_session)
    BODY_MPI_Session_f2c(MPI_Session_f2c)
MPIABI_Session mpiwrapper_w_PMPI_Session_f2c(MPIABI_Fint abi_session)
    BODY_MPI_Session_f2c(PMPI_Session_f2c)

int mpiwrapper_w_MPI_Session_toint(MPIABI_Session abi_session)
    BODY_TOINT(abi_session)
int mpiwrapper_w_PMPI_Session_toint(MPIABI_Session abi_session)
    BODY_TOINT(abi_session)

MPIABI_Session mpiwrapper_w_MPI_Session_fromint(int abi_session)
    BODY_FROMINT(MPIABI_Session, MPIABI_SESSION_NULL, abi_session)
MPIABI_Session mpiwrapper_w_PMPI_Session_fromint(int abi_session)
    BODY_FROMINT(MPIABI_Session, MPIABI_SESSION_NULL, abi_session)

/* -------------------------------- MPI_Type_c2f / _f2c / _toint / _fromint */

#define BODY_MPI_Type_c2f(TARGET) BODY_TOINT(abi_datatype)
#define BODY_MPI_Type_f2c(TARGET) \
    BODY_FROMINT(MPIABI_Datatype, MPIABI_DATATYPE_NULL, abi_datatype)

MPIABI_Fint mpiwrapper_w_MPI_Type_c2f(MPIABI_Datatype abi_datatype)
    BODY_MPI_Type_c2f(MPI_Type_c2f)
MPIABI_Fint mpiwrapper_w_PMPI_Type_c2f(MPIABI_Datatype abi_datatype)
    BODY_MPI_Type_c2f(PMPI_Type_c2f)

MPIABI_Datatype mpiwrapper_w_MPI_Type_f2c(MPIABI_Fint abi_datatype)
    BODY_MPI_Type_f2c(MPI_Type_f2c)
MPIABI_Datatype mpiwrapper_w_PMPI_Type_f2c(MPIABI_Fint abi_datatype)
    BODY_MPI_Type_f2c(PMPI_Type_f2c)

int mpiwrapper_w_MPI_Type_toint(MPIABI_Datatype abi_datatype)
    BODY_TOINT(abi_datatype)
int mpiwrapper_w_PMPI_Type_toint(MPIABI_Datatype abi_datatype)
    BODY_TOINT(abi_datatype)

MPIABI_Datatype mpiwrapper_w_MPI_Type_fromint(int abi_datatype)
    BODY_FROMINT(MPIABI_Datatype, MPIABI_DATATYPE_NULL, abi_datatype)
MPIABI_Datatype mpiwrapper_w_PMPI_Type_fromint(int abi_datatype)
    BODY_FROMINT(MPIABI_Datatype, MPIABI_DATATYPE_NULL, abi_datatype)

/* --------------------------------- MPI_Win_c2f / _f2c / _toint / _fromint */

#define BODY_MPI_Win_c2f(TARGET) BODY_TOINT(abi_win)
#define BODY_MPI_Win_f2c(TARGET) \
    BODY_FROMINT(MPIABI_Win, MPIABI_WIN_NULL, abi_win)

MPIABI_Fint mpiwrapper_w_MPI_Win_c2f(MPIABI_Win abi_win)
    BODY_MPI_Win_c2f(MPI_Win_c2f)
MPIABI_Fint mpiwrapper_w_PMPI_Win_c2f(MPIABI_Win abi_win)
    BODY_MPI_Win_c2f(PMPI_Win_c2f)

MPIABI_Win mpiwrapper_w_MPI_Win_f2c(MPIABI_Fint abi_win)
    BODY_MPI_Win_f2c(MPI_Win_f2c)
MPIABI_Win mpiwrapper_w_PMPI_Win_f2c(MPIABI_Fint abi_win)
    BODY_MPI_Win_f2c(PMPI_Win_f2c)

int mpiwrapper_w_MPI_Win_toint(MPIABI_Win abi_win)
    BODY_TOINT(abi_win)
int mpiwrapper_w_PMPI_Win_toint(MPIABI_Win abi_win)
    BODY_TOINT(abi_win)

MPIABI_Win mpiwrapper_w_MPI_Win_fromint(int abi_win)
    BODY_FROMINT(MPIABI_Win, MPIABI_WIN_NULL, abi_win)
MPIABI_Win mpiwrapper_w_PMPI_Win_fromint(int abi_win)
    BODY_FROMINT(MPIABI_Win, MPIABI_WIN_NULL, abi_win)


/* ----------------------------------------- the four status converters ---- */

/* NOTES.md #4.4: MPI_F_STATUS_SIZE is 8 in the ABI and the named fields are at
 * indices 0, 1 and 2 -- which is exactly the layout of the ABI's C status, 32
 * bytes with three named ints followed by 20 bytes of scratch. So an ABI
 * Fortran status and an ABI C status are the same 32 bytes, MPIABI_F08_Status
 * is a typedef of MPIABI_Status, and all four conversions are one copy.
 *
 * The asserts below are what make that a checked property rather than a
 * remembered one: they fail the build if the ABI header ever moves a named
 * field or resizes the Fortran status, which is the only way this copy could
 * become wrong.
 *
 * What survives the copy matters as much as what it costs: the 20 scratch
 * bytes carry the implementation's own private status bytes, so a status that
 * has been to Fortran and back still answers MPI_Get_count correctly.
 */
_Static_assert(sizeof(MPIABI_Status)
                   == (size_t)MPIABI_F_STATUS_SIZE * sizeof(MPIABI_Fint),
               "the ABI Fortran status is not the ABI C status");
_Static_assert(offsetof(MPIABI_Status, MPI_SOURCE)
                   == (size_t)MPIABI_F_SOURCE * sizeof(MPIABI_Fint),
               "MPI_F_SOURCE does not index the ABI status's MPI_SOURCE");
_Static_assert(offsetof(MPIABI_Status, MPI_TAG)
                   == (size_t)MPIABI_F_TAG * sizeof(MPIABI_Fint),
               "MPI_F_TAG does not index the ABI status's MPI_TAG");
_Static_assert(offsetof(MPIABI_Status, MPI_ERROR)
                   == (size_t)MPIABI_F_ERROR * sizeof(MPIABI_Fint),
               "MPI_F_ERROR does not index the ABI status's MPI_ERROR");

/* TARGET goes unused in all four, as it does in _toint and _fromint above and
 * for the same reason: nothing here calls the implementation, so the MPI_ and
 * PMPI_ instances really are the same body. Naming the implementation's own
 * function -- even only to take its address -- would be worse than useless: it
 * would make the wrapper fail to *compile* against an MPI that does not
 * declare a converter it never needs to provide.
 */
#define BODY_MPI_Status_c2f(TARGET)                                            \
  {                                                                            \
    memcpy(abi_f_status, abi_c_status, sizeof(MPIABI_Status));                 \
    return MPIABI_SUCCESS;                                                     \
  }

int mpiwrapper_w_MPI_Status_c2f(const MPIABI_Status *abi_c_status,
                                MPIABI_Fint *abi_f_status)
    BODY_MPI_Status_c2f(MPI_Status_c2f)
int mpiwrapper_w_PMPI_Status_c2f(const MPIABI_Status *abi_c_status,
                                 MPIABI_Fint *abi_f_status)
    BODY_MPI_Status_c2f(PMPI_Status_c2f)

#define BODY_MPI_Status_f2c(TARGET)                                            \
  {                                                                            \
    memcpy(abi_c_status, abi_f_status, sizeof(MPIABI_Status));                 \
    return MPIABI_SUCCESS;                                                     \
  }

int mpiwrapper_w_MPI_Status_f2c(const MPIABI_Fint *abi_f_status,
                                MPIABI_Status *abi_c_status)
    BODY_MPI_Status_f2c(MPI_Status_f2c)
int mpiwrapper_w_PMPI_Status_f2c(const MPIABI_Fint *abi_f_status,
                                 MPIABI_Status *abi_c_status)
    BODY_MPI_Status_f2c(PMPI_Status_f2c)

/* MPIABI_F08_Status is MPIABI_Status, so these two are the identity -- written
 * as a copy anyway, because the ABI header is free to stop making them the
 * same type and a struct assignment would then be the thing that changed
 * meaning silently.
 */
#define BODY_MPI_Status_c2f08(TARGET)                                          \
  {                                                                            \
    memcpy(abi_f08_status, abi_c_status, sizeof(MPIABI_Status));               \
    return MPIABI_SUCCESS;                                                     \
  }

int mpiwrapper_w_MPI_Status_c2f08(const MPIABI_Status *abi_c_status,
                                  MPIABI_F08_Status *abi_f08_status)
    BODY_MPI_Status_c2f08(MPI_Status_c2f08)
int mpiwrapper_w_PMPI_Status_c2f08(const MPIABI_Status *abi_c_status,
                                   MPIABI_F08_Status *abi_f08_status)
    BODY_MPI_Status_c2f08(PMPI_Status_c2f08)

#define BODY_MPI_Status_f082c(TARGET)                                          \
  {                                                                            \
    memcpy(abi_c_status, abi_f08_status, sizeof(MPIABI_Status));               \
    return MPIABI_SUCCESS;                                                     \
  }

int mpiwrapper_w_MPI_Status_f082c(const MPIABI_F08_Status *abi_f08_status,
                                  MPIABI_Status *abi_c_status)
    BODY_MPI_Status_f082c(MPI_Status_f082c)
int mpiwrapper_w_PMPI_Status_f082c(const MPIABI_F08_Status *abi_f08_status,
                                   MPIABI_Status *abi_c_status)
    BODY_MPI_Status_f082c(PMPI_Status_f082c)
