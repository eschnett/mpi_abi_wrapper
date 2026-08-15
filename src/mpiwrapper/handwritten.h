/* The hand-written entry-point bodies, for the vtable to point at.
 *
 * The generated wrappers.c holds one vtable initializer covering all 1376
 * slots, so it has to name the bodies the generator did *not* write. This is
 * that list: exactly the HAND_WRITTEN ledger's members, one declaration each,
 * MPI_ and PMPI_ separately. If the two sets ever disagree the initializer
 * fails to compile, which is the cheapest possible version of the ledger check.
 *
 * S1 covers nine entry points (eighteen slots); S4 completes the set (~90; NOTES.md #8).
 */

#ifndef MPIWRAPPER_HANDWRITTEN_H
#define MPIWRAPPER_HANDWRITTEN_H

#include "internal.h"

int mpiwrapper_w_MPI_Init(int *abi_argc, char ***abi_argv);
int mpiwrapper_w_PMPI_Init(int *abi_argc, char ***abi_argv);

int mpiwrapper_w_MPI_Finalize(void);
int mpiwrapper_w_PMPI_Finalize(void);

int mpiwrapper_w_MPI_Get_count(const MPIABI_Status *abi_status,
                               MPIABI_Datatype abi_datatype, int *abi_count);
int mpiwrapper_w_PMPI_Get_count(const MPIABI_Status *abi_status,
                                MPIABI_Datatype abi_datatype, int *abi_count);

int mpiwrapper_w_MPI_Op_create(MPIABI_User_function *abi_user_fn,
                               int abi_commute, MPIABI_Op *abi_op);
int mpiwrapper_w_PMPI_Op_create(MPIABI_User_function *abi_user_fn,
                                int abi_commute, MPIABI_Op *abi_op);

int mpiwrapper_w_MPI_Comm_create_errhandler(
    MPIABI_Comm_errhandler_function *abi_comm_errhandler_fn,
    MPIABI_Errhandler               *abi_errhandler);
int mpiwrapper_w_PMPI_Comm_create_errhandler(
    MPIABI_Comm_errhandler_function *abi_comm_errhandler_fn,
    MPIABI_Errhandler               *abi_errhandler);

int mpiwrapper_w_MPI_Error_string(int abi_errorcode, char *abi_string,
                                  int *abi_resultlen);
int mpiwrapper_w_PMPI_Error_string(int abi_errorcode, char *abi_string,
                                   int *abi_resultlen);

MPIABI_Fint mpiwrapper_w_MPI_Comm_c2f(MPIABI_Comm abi_comm);
MPIABI_Fint mpiwrapper_w_PMPI_Comm_c2f(MPIABI_Comm abi_comm);

MPIABI_Comm mpiwrapper_w_MPI_Comm_f2c(MPIABI_Fint abi_comm);
MPIABI_Comm mpiwrapper_w_PMPI_Comm_f2c(MPIABI_Fint abi_comm);

int mpiwrapper_w_MPI_Ialltoallw(const void *abi_sendbuf,
                                const int abi_sendcounts[],
                                const int abi_sdispls[],
                                const MPIABI_Datatype abi_sendtypes[],
                                void *abi_recvbuf, const int abi_recvcounts[],
                                const int abi_rdispls[],
                                const MPIABI_Datatype abi_recvtypes[],
                                MPIABI_Comm            abi_comm,
                                MPIABI_Request        *abi_request);
int mpiwrapper_w_PMPI_Ialltoallw(const void *abi_sendbuf,
                                 const int abi_sendcounts[],
                                 const int abi_sdispls[],
                                 const MPIABI_Datatype abi_sendtypes[],
                                 void *abi_recvbuf, const int abi_recvcounts[],
                                 const int abi_rdispls[],
                                 const MPIABI_Datatype abi_recvtypes[],
                                 MPIABI_Comm            abi_comm,
                                 MPIABI_Request        *abi_request);

#endif /* MPIWRAPPER_HANDWRITTEN_H */
