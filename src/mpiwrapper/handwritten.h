/* The hand-written entry-point bodies, for the vtable to point at.
 *
 * The generated wrappers.c holds one vtable initializer covering all 1376
 * slots, so it has to name the bodies the generator did *not* write. This is
 * that list: exactly the HAND_WRITTEN ledger's members, one declaration each,
 * MPI_ and PMPI_ separately. If the two sets ever disagree the initializer
 * fails to compile, which is the cheapest possible version of the ledger check.
 *
 * S1 covers eight entry points (sixteen slots). S3 took MPI_Waitall and
 * MPI_Ialltoallw back, which S1 had written and S2 kept here as stand-ins for
 * the two array classes it could not yet generate; S4 completes the rest of
 * the ledger (NOTES.md #8).
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

#endif /* MPIWRAPPER_HANDWRITTEN_H */
