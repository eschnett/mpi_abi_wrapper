/* libmpi_abi -- the exported entry points.
 *
 * S1 STATUS: hand-written stand-in for gen/mpi_abi/entrypoints.c, covering the
 * 29 entry points of the prototype (STAGES.md S1, NOTES.md #11). S2's generator
 * emits this file for all 688, and is required to reproduce these bodies.
 *
 * Note what is *not* here: no conversion, no temporary, no knowledge of any
 * implementation type, and no initialization check. The arguments pass through
 * untouched, and need no cast because the ABI header's MPI_Comm and mpiabi.h's
 * MPIABI_Comm are the same type -- both `struct MPI_ABI_Comm *`. That identity
 * is rule 2 of the renaming (NOTES.md #2) and it is what keeps 1376 forwarders
 * cast-free; a cast here would silently absorb a genuine type error.
 *
 * MPI_* and PMPI_* are two definitions rather than an alias: macOS aliases need
 * -Wl,-alias or __asm__ labels, and at one line per body an alias saves
 * nothing. They reach *different* slots, so that bypassing a profiling layer at
 * the ABI level also bypasses one interposed at the implementation level.
 */

#include <mpi.h> /* the ABI's */

#include "mpiwrapper_vtable.h"

#include <assert.h>

/* Set by the constructor in bootstrap.c before anything can reach these. */
extern const struct mpiwrapper_vtable *mpi_abi_vt;

/* Cheap insurance in development builds only: in a release build this compiles
 * to nothing and a stuck NULL would be a segfault at a small address, which is
 * what the constructor-ordering argument in bootstrap.c says cannot happen.
 * dev/dispatch-bench measures what the defensive version would cost: 23
 * instructions per entry point instead of 4.
 */
#ifndef NDEBUG
#  define VT() (assert(mpi_abi_vt != NULL), mpi_abi_vt)
#else
#  define VT() mpi_abi_vt
#endif

int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count,
                  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return VT()->MPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
}

int PMPI_Allreduce(const void *sendbuf, void *recvbuf, int count,
                   MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return VT()->PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
}

MPI_Fint MPI_Comm_c2f(MPI_Comm comm) { return VT()->MPI_Comm_c2f(comm); }
MPI_Fint PMPI_Comm_c2f(MPI_Comm comm) { return VT()->PMPI_Comm_c2f(comm); }

int MPI_Comm_create_errhandler(MPI_Comm_errhandler_function *comm_errhandler_fn,
                               MPI_Errhandler *errhandler)
{
  return VT()->MPI_Comm_create_errhandler(comm_errhandler_fn, errhandler);
}

int PMPI_Comm_create_errhandler(MPI_Comm_errhandler_function *comm_errhandler_fn,
                                MPI_Errhandler *errhandler)
{
  return VT()->PMPI_Comm_create_errhandler(comm_errhandler_fn, errhandler);
}

MPI_Comm MPI_Comm_f2c(MPI_Fint comm) { return VT()->MPI_Comm_f2c(comm); }
MPI_Comm PMPI_Comm_f2c(MPI_Fint comm) { return VT()->PMPI_Comm_f2c(comm); }

int MPI_Comm_free(MPI_Comm *comm) { return VT()->MPI_Comm_free(comm); }
int PMPI_Comm_free(MPI_Comm *comm) { return VT()->PMPI_Comm_free(comm); }

int MPI_Comm_rank(MPI_Comm comm, int *rank)
{
  return VT()->MPI_Comm_rank(comm, rank);
}

int PMPI_Comm_rank(MPI_Comm comm, int *rank)
{
  return VT()->PMPI_Comm_rank(comm, rank);
}

int MPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler)
{
  return VT()->MPI_Comm_set_errhandler(comm, errhandler);
}

int PMPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler)
{
  return VT()->PMPI_Comm_set_errhandler(comm, errhandler);
}

int MPI_Comm_size(MPI_Comm comm, int *size)
{
  return VT()->MPI_Comm_size(comm, size);
}

int PMPI_Comm_size(MPI_Comm comm, int *size)
{
  return VT()->PMPI_Comm_size(comm, size);
}

int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm)
{
  return VT()->MPI_Comm_split(comm, color, key, newcomm);
}

int PMPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm)
{
  return VT()->PMPI_Comm_split(comm, color, key, newcomm);
}

int MPI_Error_string(int errorcode, char *string, int *resultlen)
{
  return VT()->MPI_Error_string(errorcode, string, resultlen);
}

int PMPI_Error_string(int errorcode, char *string, int *resultlen)
{
  return VT()->PMPI_Error_string(errorcode, string, resultlen);
}

int MPI_File_close(MPI_File *fh) { return VT()->MPI_File_close(fh); }
int PMPI_File_close(MPI_File *fh) { return VT()->PMPI_File_close(fh); }

int MPI_File_open(MPI_Comm comm, const char *filename, int amode, MPI_Info info,
                  MPI_File *fh)
{
  return VT()->MPI_File_open(comm, filename, amode, info, fh);
}

int PMPI_File_open(MPI_Comm comm, const char *filename, int amode,
                   MPI_Info info, MPI_File *fh)
{
  return VT()->PMPI_File_open(comm, filename, amode, info, fh);
}

int MPI_Finalize(void) { return VT()->MPI_Finalize(); }
int PMPI_Finalize(void) { return VT()->PMPI_Finalize(); }

int MPI_Get_count(const MPI_Status *status, MPI_Datatype datatype, int *count)
{
  return VT()->MPI_Get_count(status, datatype, count);
}

int PMPI_Get_count(const MPI_Status *status, MPI_Datatype datatype, int *count)
{
  return VT()->PMPI_Get_count(status, datatype, count);
}

/* Legal before MPI_Init in every MPI version, which is what makes it the
 * bootstrap's resolution probe (see bootstrap.c). MPI_Wtime would have been the
 * obvious choice and is not: MPICH refuses it before initialization.
 */
int MPI_Get_version(int *version, int *subversion)
{
  return VT()->MPI_Get_version(version, subversion);
}

int PMPI_Get_version(int *version, int *subversion)
{
  return VT()->PMPI_Get_version(version, subversion);
}

int MPI_Ialltoallw(const void *sendbuf, const int sendcounts[],
                   const int sdispls[], const MPI_Datatype sendtypes[],
                   void *recvbuf, const int recvcounts[], const int rdispls[],
                   const MPI_Datatype recvtypes[], MPI_Comm comm,
                   MPI_Request *request)
{
  return VT()->MPI_Ialltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                              recvcounts, rdispls, recvtypes, comm, request);
}

int PMPI_Ialltoallw(const void *sendbuf, const int sendcounts[],
                    const int sdispls[], const MPI_Datatype sendtypes[],
                    void *recvbuf, const int recvcounts[], const int rdispls[],
                    const MPI_Datatype recvtypes[], MPI_Comm comm,
                    MPI_Request *request)
{
  return VT()->PMPI_Ialltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                               recvcounts, rdispls, recvtypes, comm, request);
}

int MPI_Init(int *argc, char ***argv) { return VT()->MPI_Init(argc, argv); }
int PMPI_Init(int *argc, char ***argv) { return VT()->PMPI_Init(argc, argv); }

int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Irecv(buf, count, datatype, source, tag, comm, request);
}

int PMPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
               MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Irecv(buf, count, datatype, source, tag, comm, request);
}

int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest,
              int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Isend(buf, count, datatype, dest, tag, comm, request);
}

int PMPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest,
               int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Op_create(MPI_User_function *user_fn, int commute, MPI_Op *op)
{
  return VT()->MPI_Op_create(user_fn, commute, op);
}

int PMPI_Op_create(MPI_User_function *user_fn, int commute, MPI_Op *op)
{
  return VT()->PMPI_Op_create(user_fn, commute, op);
}

int MPI_Op_free(MPI_Op *op) { return VT()->MPI_Op_free(op); }
int PMPI_Op_free(MPI_Op *op) { return VT()->PMPI_Op_free(op); }

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
             MPI_Comm comm, MPI_Status *status)
{
  return VT()->MPI_Recv(buf, count, datatype, source, tag, comm, status);
}

int PMPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Status *status)
{
  return VT()->PMPI_Recv(buf, count, datatype, source, tag, comm, status);
}

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest,
             int tag, MPI_Comm comm)
{
  return VT()->MPI_Send(buf, count, datatype, dest, tag, comm);
}

int PMPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest,
              int tag, MPI_Comm comm)
{
  return VT()->PMPI_Send(buf, count, datatype, dest, tag, comm);
}

int MPI_Type_commit(MPI_Datatype *datatype)
{
  return VT()->MPI_Type_commit(datatype);
}

int PMPI_Type_commit(MPI_Datatype *datatype)
{
  return VT()->PMPI_Type_commit(datatype);
}

int MPI_Type_create_struct(int count, const int array_of_blocklengths[],
                           const MPI_Aint array_of_displacements[],
                           const MPI_Datatype array_of_types[],
                           MPI_Datatype *newtype)
{
  return VT()->MPI_Type_create_struct(count, array_of_blocklengths,
                                      array_of_displacements, array_of_types,
                                      newtype);
}

int PMPI_Type_create_struct(int count, const int array_of_blocklengths[],
                            const MPI_Aint array_of_displacements[],
                            const MPI_Datatype array_of_types[],
                            MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_create_struct(count, array_of_blocklengths,
                                       array_of_displacements, array_of_types,
                                       newtype);
}

int MPI_Type_create_struct_c(MPI_Count count,
                             const MPI_Count array_of_blocklengths[],
                             const MPI_Count array_of_displacements[],
                             const MPI_Datatype array_of_types[],
                             MPI_Datatype *newtype)
{
  return VT()->MPI_Type_create_struct_c(count, array_of_blocklengths,
                                        array_of_displacements, array_of_types,
                                        newtype);
}

int PMPI_Type_create_struct_c(MPI_Count count,
                              const MPI_Count array_of_blocklengths[],
                              const MPI_Count array_of_displacements[],
                              const MPI_Datatype array_of_types[],
                              MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_create_struct_c(count, array_of_blocklengths,
                                         array_of_displacements, array_of_types,
                                         newtype);
}

int MPI_Type_free(MPI_Datatype *datatype)
{
  return VT()->MPI_Type_free(datatype);
}

int PMPI_Type_free(MPI_Datatype *datatype)
{
  return VT()->PMPI_Type_free(datatype);
}

int MPI_Waitall(int count, MPI_Request array_of_requests[],
                MPI_Status *array_of_statuses)
{
  return VT()->MPI_Waitall(count, array_of_requests, array_of_statuses);
}

int PMPI_Waitall(int count, MPI_Request array_of_requests[],
                 MPI_Status *array_of_statuses)
{
  return VT()->PMPI_Waitall(count, array_of_requests, array_of_statuses);
}

/* Returns double, so there is no error code to map -- one of the handful of
 * entry points whose generated shape differs at all.
 */
double MPI_Wtime(void) { return VT()->MPI_Wtime(); }
double PMPI_Wtime(void) { return VT()->PMPI_Wtime(); }
