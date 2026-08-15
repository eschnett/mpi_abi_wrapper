/* GENERATED FILE -- do not edit by hand.
 *
 * Produced by dev/generate.py. libmpi_abi -- the exported entry points, all
 * 1376 of them.
 *
 * Note what is *not* here: no conversion, no temporary, no knowledge of any
 * implementation type, and no initialization check. The arguments pass through
 * untouched, and need no cast because the ABI header's MPI_Comm and mpiabi.h's
 * MPIABI_Comm are the same type -- both `struct MPI_ABI_Comm *`. That identity
 * is rule 2 of the renaming (NOTES.md #2) and it is what keeps 1376
 * forwarders cast-free; a cast here would silently absorb a genuine type
 * error.
 *
 * MPI_* and PMPI_* are two definitions rather than an alias: macOS aliases
 * need -Wl,-alias or __asm__ labels, and at one line per body an alias saves
 * nothing. They reach *different* slots, so that bypassing a profiling layer
 * at the ABI level also bypasses one interposed at the implementation level.
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

int MPI_Abi_get_fortran_booleans(int logical_size, void *logical_true,
                                 void *logical_false, int *is_set)
{
  return VT()->MPI_Abi_get_fortran_booleans(logical_size, logical_true,
                                            logical_false, is_set);
}

int MPI_Abi_get_fortran_info(MPI_Info *info)
{
  return VT()->MPI_Abi_get_fortran_info(info);
}

int MPI_Abi_get_info(MPI_Info *info) { return VT()->MPI_Abi_get_info(info); }

int MPI_Abi_get_version(int *abi_major, int *abi_minor)
{
  return VT()->MPI_Abi_get_version(abi_major, abi_minor);
}

int MPI_Abi_set_fortran_booleans(int logical_size, void *logical_true,
                                 void *logical_false)
{
  return VT()->MPI_Abi_set_fortran_booleans(logical_size, logical_true,
                                            logical_false);
}

int MPI_Abi_set_fortran_info(MPI_Info info)
{
  return VT()->MPI_Abi_set_fortran_info(info);
}

int MPI_Abort(MPI_Comm comm, int errorcode)
{
  return VT()->MPI_Abort(comm, errorcode);
}

int MPI_Accumulate(const void *origin_addr, int origin_count,
                   MPI_Datatype origin_datatype, int target_rank,
                   MPI_Aint target_disp, int target_count,
                   MPI_Datatype target_datatype, MPI_Op op, MPI_Win win)
{
  return VT()->MPI_Accumulate(origin_addr, origin_count, origin_datatype,
                              target_rank, target_disp, target_count,
                              target_datatype, op, win);
}

int MPI_Accumulate_c(const void *origin_addr, MPI_Count origin_count,
                     MPI_Datatype origin_datatype, int target_rank,
                     MPI_Aint target_disp, MPI_Count target_count,
                     MPI_Datatype target_datatype, MPI_Op op, MPI_Win win)
{
  return VT()->MPI_Accumulate_c(origin_addr, origin_count, origin_datatype,
                                target_rank, target_disp, target_count,
                                target_datatype, op, win);
}

int MPI_Add_error_class(int *errorclass)
{
  return VT()->MPI_Add_error_class(errorclass);
}

int MPI_Add_error_code(int errorclass, int *errorcode)
{
  return VT()->MPI_Add_error_code(errorclass, errorcode);
}

int MPI_Add_error_string(int errorcode, const char *string)
{
  return VT()->MPI_Add_error_string(errorcode, string);
}

int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  MPI_Comm comm)
{
  return VT()->MPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                             recvtype, comm);
}

int MPI_Allgather_c(const void *sendbuf, MPI_Count sendcount,
                    MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount,
                    MPI_Datatype recvtype, MPI_Comm comm)
{
  return VT()->MPI_Allgather_c(sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, comm);
}

int MPI_Allgather_init(const void *sendbuf, int sendcount,
                       MPI_Datatype sendtype, void *recvbuf, int recvcount,
                       MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info,
                       MPI_Request *request)
{
  return VT()->MPI_Allgather_init(sendbuf, sendcount, sendtype, recvbuf,
                                  recvcount, recvtype, comm, info, request);
}

int MPI_Allgather_init_c(const void *sendbuf, MPI_Count sendcount,
                         MPI_Datatype sendtype, void *recvbuf,
                         MPI_Count recvcount, MPI_Datatype recvtype,
                         MPI_Comm comm, MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Allgather_init_c(sendbuf, sendcount, sendtype, recvbuf,
                                    recvcount, recvtype, comm, info, request);
}

int MPI_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, const int recvcounts[], const int displs[],
                   MPI_Datatype recvtype, MPI_Comm comm)
{
  return VT()->MPI_Allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                              recvcounts, displs, recvtype, comm);
}

int MPI_Allgatherv_c(const void *sendbuf, MPI_Count sendcount,
                     MPI_Datatype sendtype, void *recvbuf,
                     const MPI_Count recvcounts[], const MPI_Aint displs[],
                     MPI_Datatype recvtype, MPI_Comm comm)
{
  return VT()->MPI_Allgatherv_c(sendbuf, sendcount, sendtype, recvbuf,
                                recvcounts, displs, recvtype, comm);
}

int MPI_Allgatherv_init(const void *sendbuf, int sendcount,
                        MPI_Datatype sendtype, void *recvbuf,
                        const int recvcounts[], const int displs[],
                        MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info,
                        MPI_Request *request)
{
  return VT()->MPI_Allgatherv_init(sendbuf, sendcount, sendtype, recvbuf,
                                   recvcounts, displs, recvtype, comm, info,
                                   request);
}

int MPI_Allgatherv_init_c(const void *sendbuf, MPI_Count sendcount,
                          MPI_Datatype sendtype, void *recvbuf,
                          const MPI_Count recvcounts[],
                          const MPI_Aint displs[], MPI_Datatype recvtype,
                          MPI_Comm comm, MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Allgatherv_init_c(sendbuf, sendcount, sendtype, recvbuf,
                                     recvcounts, displs, recvtype, comm, info,
                                     request);
}

int MPI_Alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr)
{
  return VT()->MPI_Alloc_mem(size, info, baseptr);
}

int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count,
                  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return VT()->MPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
}

int MPI_Allreduce_c(const void *sendbuf, void *recvbuf, MPI_Count count,
                    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return VT()->MPI_Allreduce_c(sendbuf, recvbuf, count, datatype, op, comm);
}

int MPI_Allreduce_init(const void *sendbuf, void *recvbuf, int count,
                       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                       MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Allreduce_init(sendbuf, recvbuf, count, datatype, op, comm,
                                  info, request);
}

int MPI_Allreduce_init_c(const void *sendbuf, void *recvbuf, MPI_Count count,
                         MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                         MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Allreduce_init_c(sendbuf, recvbuf, count, datatype, op,
                                    comm, info, request);
}

int MPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype,
                 MPI_Comm comm)
{
  return VT()->MPI_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                            recvtype, comm);
}

int MPI_Alltoall_c(const void *sendbuf, MPI_Count sendcount,
                   MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount,
                   MPI_Datatype recvtype, MPI_Comm comm)
{
  return VT()->MPI_Alltoall_c(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                              recvtype, comm);
}

int MPI_Alltoall_init(const void *sendbuf, int sendcount,
                      MPI_Datatype sendtype, void *recvbuf, int recvcount,
                      MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info,
                      MPI_Request *request)
{
  return VT()->MPI_Alltoall_init(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcount, recvtype, comm, info, request);
}

int MPI_Alltoall_init_c(const void *sendbuf, MPI_Count sendcount,
                        MPI_Datatype sendtype, void *recvbuf,
                        MPI_Count recvcount, MPI_Datatype recvtype,
                        MPI_Comm comm, MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Alltoall_init_c(sendbuf, sendcount, sendtype, recvbuf,
                                   recvcount, recvtype, comm, info, request);
}

int MPI_Alltoallv(const void *sendbuf, const int sendcounts[],
                  const int sdispls[], MPI_Datatype sendtype, void *recvbuf,
                  const int recvcounts[], const int rdispls[],
                  MPI_Datatype recvtype, MPI_Comm comm)
{
  return VT()->MPI_Alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                             recvcounts, rdispls, recvtype, comm);
}

int MPI_Alltoallv_c(const void *sendbuf, const MPI_Count sendcounts[],
                    const MPI_Aint sdispls[], MPI_Datatype sendtype,
                    void *recvbuf, const MPI_Count recvcounts[],
                    const MPI_Aint rdispls[], MPI_Datatype recvtype,
                    MPI_Comm comm)
{
  return VT()->MPI_Alltoallv_c(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                               recvcounts, rdispls, recvtype, comm);
}

int MPI_Alltoallv_init(const void *sendbuf, const int sendcounts[],
                       const int sdispls[], MPI_Datatype sendtype,
                       void *recvbuf, const int recvcounts[],
                       const int rdispls[], MPI_Datatype recvtype,
                       MPI_Comm comm, MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Alltoallv_init(sendbuf, sendcounts, sdispls, sendtype,
                                  recvbuf, recvcounts, rdispls, recvtype, comm,
                                  info, request);
}

int MPI_Alltoallv_init_c(const void *sendbuf, const MPI_Count sendcounts[],
                         const MPI_Aint sdispls[], MPI_Datatype sendtype,
                         void *recvbuf, const MPI_Count recvcounts[],
                         const MPI_Aint rdispls[], MPI_Datatype recvtype,
                         MPI_Comm comm, MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Alltoallv_init_c(sendbuf, sendcounts, sdispls, sendtype,
                                    recvbuf, recvcounts, rdispls, recvtype,
                                    comm, info, request);
}

int MPI_Alltoallw(const void *sendbuf, const int sendcounts[],
                  const int sdispls[], const MPI_Datatype sendtypes[],
                  void *recvbuf, const int recvcounts[], const int rdispls[],
                  const MPI_Datatype recvtypes[], MPI_Comm comm)
{
  return VT()->MPI_Alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                             recvcounts, rdispls, recvtypes, comm);
}

int MPI_Alltoallw_c(const void *sendbuf, const MPI_Count sendcounts[],
                    const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                    void *recvbuf, const MPI_Count recvcounts[],
                    const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                    MPI_Comm comm)
{
  return VT()->MPI_Alltoallw_c(sendbuf, sendcounts, sdispls, sendtypes,
                               recvbuf, recvcounts, rdispls, recvtypes, comm);
}

int MPI_Alltoallw_init(const void *sendbuf, const int sendcounts[],
                       const int sdispls[], const MPI_Datatype sendtypes[],
                       void *recvbuf, const int recvcounts[],
                       const int rdispls[], const MPI_Datatype recvtypes[],
                       MPI_Comm comm, MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Alltoallw_init(sendbuf, sendcounts, sdispls, sendtypes,
                                  recvbuf, recvcounts, rdispls, recvtypes,
                                  comm, info, request);
}

int MPI_Alltoallw_init_c(const void *sendbuf, const MPI_Count sendcounts[],
                         const MPI_Aint sdispls[],
                         const MPI_Datatype sendtypes[], void *recvbuf,
                         const MPI_Count recvcounts[],
                         const MPI_Aint rdispls[],
                         const MPI_Datatype recvtypes[], MPI_Comm comm,
                         MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Alltoallw_init_c(sendbuf, sendcounts, sdispls, sendtypes,
                                    recvbuf, recvcounts, rdispls, recvtypes,
                                    comm, info, request);
}

int MPI_Attr_delete(MPI_Comm comm, int keyval)
{
  return VT()->MPI_Comm_delete_attr(comm, keyval);
}

int MPI_Attr_get(MPI_Comm comm, int keyval, void *attribute_val, int *flag)
{
  return VT()->MPI_Comm_get_attr(comm, keyval, attribute_val, flag);
}

int MPI_Attr_put(MPI_Comm comm, int keyval, void *attribute_val)
{
  return VT()->MPI_Comm_set_attr(comm, keyval, attribute_val);
}

int MPI_Barrier(MPI_Comm comm) { return VT()->MPI_Barrier(comm); }

int MPI_Barrier_init(MPI_Comm comm, MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Barrier_init(comm, info, request);
}

int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root,
              MPI_Comm comm)
{
  return VT()->MPI_Bcast(buffer, count, datatype, root, comm);
}

int MPI_Bcast_c(void *buffer, MPI_Count count, MPI_Datatype datatype, int root,
                MPI_Comm comm)
{
  return VT()->MPI_Bcast_c(buffer, count, datatype, root, comm);
}

int MPI_Bcast_init(void *buffer, int count, MPI_Datatype datatype, int root,
                   MPI_Comm comm, MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Bcast_init(buffer, count, datatype, root, comm, info,
                              request);
}

int MPI_Bcast_init_c(void *buffer, MPI_Count count, MPI_Datatype datatype,
                     int root, MPI_Comm comm, MPI_Info info,
                     MPI_Request *request)
{
  return VT()->MPI_Bcast_init_c(buffer, count, datatype, root, comm, info,
                                request);
}

int MPI_Bsend(const void *buf, int count, MPI_Datatype datatype, int dest,
              int tag, MPI_Comm comm)
{
  return VT()->MPI_Bsend(buf, count, datatype, dest, tag, comm);
}

int MPI_Bsend_c(const void *buf, MPI_Count count, MPI_Datatype datatype,
                int dest, int tag, MPI_Comm comm)
{
  return VT()->MPI_Bsend_c(buf, count, datatype, dest, tag, comm);
}

int MPI_Bsend_init(const void *buf, int count, MPI_Datatype datatype, int dest,
                   int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Bsend_init(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Bsend_init_c(const void *buf, MPI_Count count, MPI_Datatype datatype,
                     int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Bsend_init_c(buf, count, datatype, dest, tag, comm,
                                request);
}

int MPI_Buffer_attach(void *buffer, int size)
{
  return VT()->MPI_Buffer_attach(buffer, size);
}

int MPI_Buffer_attach_c(void *buffer, MPI_Count size)
{
  return VT()->MPI_Buffer_attach_c(buffer, size);
}

int MPI_Buffer_detach(void *buffer_addr, int *size)
{
  return VT()->MPI_Buffer_detach(buffer_addr, size);
}

int MPI_Buffer_detach_c(void *buffer_addr, MPI_Count *size)
{
  return VT()->MPI_Buffer_detach_c(buffer_addr, size);
}

int MPI_Buffer_flush(void) { return VT()->MPI_Buffer_flush(); }

int MPI_Buffer_iflush(MPI_Request *request)
{
  return VT()->MPI_Buffer_iflush(request);
}

int MPI_Cancel(MPI_Request *request) { return VT()->MPI_Cancel(request); }

int MPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int coords[])
{
  return VT()->MPI_Cart_coords(comm, rank, maxdims, coords);
}

int MPI_Cart_create(MPI_Comm comm_old, int ndims, const int dims[],
                    const int periods[], int reorder, MPI_Comm *comm_cart)
{
  return VT()->MPI_Cart_create(comm_old, ndims, dims, periods, reorder,
                               comm_cart);
}

int MPI_Cart_get(MPI_Comm comm, int maxdims, int dims[], int periods[],
                 int coords[])
{
  return VT()->MPI_Cart_get(comm, maxdims, dims, periods, coords);
}

int MPI_Cart_map(MPI_Comm comm, int ndims, const int dims[],
                 const int periods[], int *newrank)
{
  return VT()->MPI_Cart_map(comm, ndims, dims, periods, newrank);
}

int MPI_Cart_rank(MPI_Comm comm, const int coords[], int *rank)
{
  return VT()->MPI_Cart_rank(comm, coords, rank);
}

int MPI_Cart_shift(MPI_Comm comm, int direction, int disp, int *rank_source,
                   int *rank_dest)
{
  return VT()->MPI_Cart_shift(comm, direction, disp, rank_source, rank_dest);
}

int MPI_Cart_sub(MPI_Comm comm, const int remain_dims[], MPI_Comm *newcomm)
{
  return VT()->MPI_Cart_sub(comm, remain_dims, newcomm);
}

int MPI_Cartdim_get(MPI_Comm comm, int *ndims)
{
  return VT()->MPI_Cartdim_get(comm, ndims);
}

int MPI_Close_port(const char *port_name)
{
  return VT()->MPI_Close_port(port_name);
}

int MPI_Comm_accept(const char *port_name, MPI_Info info, int root,
                    MPI_Comm comm, MPI_Comm *newcomm)
{
  return VT()->MPI_Comm_accept(port_name, info, root, comm, newcomm);
}

int MPI_Comm_attach_buffer(MPI_Comm comm, void *buffer, int size)
{
  return VT()->MPI_Comm_attach_buffer(comm, buffer, size);
}

int MPI_Comm_attach_buffer_c(MPI_Comm comm, void *buffer, MPI_Count size)
{
  return VT()->MPI_Comm_attach_buffer_c(comm, buffer, size);
}

int MPI_Comm_call_errhandler(MPI_Comm comm, int errorcode)
{
  return VT()->MPI_Comm_call_errhandler(comm, errorcode);
}

int MPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result)
{
  return VT()->MPI_Comm_compare(comm1, comm2, result);
}

int MPI_Comm_connect(const char *port_name, MPI_Info info, int root,
                     MPI_Comm comm, MPI_Comm *newcomm)
{
  return VT()->MPI_Comm_connect(port_name, info, root, comm, newcomm);
}

int MPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm)
{
  return VT()->MPI_Comm_create(comm, group, newcomm);
}

int MPI_Comm_create_errhandler(
    MPI_Comm_errhandler_function *comm_errhandler_fn,
    MPI_Errhandler *errhandler)
{
  return VT()->MPI_Comm_create_errhandler(comm_errhandler_fn, errhandler);
}

int MPI_Comm_create_from_group(MPI_Group group, const char *stringtag,
                               MPI_Info info, MPI_Errhandler errhandler,
                               MPI_Comm *newcomm)
{
  return VT()->MPI_Comm_create_from_group(group, stringtag, info, errhandler,
                                          newcomm);
}

int MPI_Comm_create_group(MPI_Comm comm, MPI_Group group, int tag,
                          MPI_Comm *newcomm)
{
  return VT()->MPI_Comm_create_group(comm, group, tag, newcomm);
}

int MPI_Comm_create_keyval(MPI_Comm_copy_attr_function *comm_copy_attr_fn,
                           MPI_Comm_delete_attr_function *comm_delete_attr_fn,
                           int *comm_keyval, void *extra_state)
{
  return VT()->MPI_Comm_create_keyval(comm_copy_attr_fn, comm_delete_attr_fn,
                                      comm_keyval, extra_state);
}

int MPI_Comm_delete_attr(MPI_Comm comm, int comm_keyval)
{
  return VT()->MPI_Comm_delete_attr(comm, comm_keyval);
}

int MPI_Comm_detach_buffer(MPI_Comm comm, void *buffer_addr, int *size)
{
  return VT()->MPI_Comm_detach_buffer(comm, buffer_addr, size);
}

int MPI_Comm_detach_buffer_c(MPI_Comm comm, void *buffer_addr, MPI_Count *size)
{
  return VT()->MPI_Comm_detach_buffer_c(comm, buffer_addr, size);
}

int MPI_Comm_disconnect(MPI_Comm *comm)
{
  return VT()->MPI_Comm_disconnect(comm);
}

int MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm)
{
  return VT()->MPI_Comm_dup(comm, newcomm);
}

int MPI_Comm_dup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm *newcomm)
{
  return VT()->MPI_Comm_dup_with_info(comm, info, newcomm);
}

int MPI_Comm_flush_buffer(MPI_Comm comm)
{
  return VT()->MPI_Comm_flush_buffer(comm);
}

int MPI_Comm_free(MPI_Comm *comm) { return VT()->MPI_Comm_free(comm); }

int MPI_Comm_free_keyval(int *comm_keyval)
{
  return VT()->MPI_Comm_free_keyval(comm_keyval);
}

int MPI_Comm_get_attr(MPI_Comm comm, int comm_keyval, void *attribute_val,
                      int *flag)
{
  return VT()->MPI_Comm_get_attr(comm, comm_keyval, attribute_val, flag);
}

int MPI_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler *errhandler)
{
  return VT()->MPI_Comm_get_errhandler(comm, errhandler);
}

int MPI_Comm_get_info(MPI_Comm comm, MPI_Info *info_used)
{
  return VT()->MPI_Comm_get_info(comm, info_used);
}

int MPI_Comm_get_name(MPI_Comm comm, char *comm_name, int *resultlen)
{
  return VT()->MPI_Comm_get_name(comm, comm_name, resultlen);
}

int MPI_Comm_get_parent(MPI_Comm *parent)
{
  return VT()->MPI_Comm_get_parent(parent);
}

int MPI_Comm_group(MPI_Comm comm, MPI_Group *group)
{
  return VT()->MPI_Comm_group(comm, group);
}

int MPI_Comm_idup(MPI_Comm comm, MPI_Comm *newcomm, MPI_Request *request)
{
  return VT()->MPI_Comm_idup(comm, newcomm, request);
}

int MPI_Comm_idup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm *newcomm,
                            MPI_Request *request)
{
  return VT()->MPI_Comm_idup_with_info(comm, info, newcomm, request);
}

int MPI_Comm_iflush_buffer(MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Comm_iflush_buffer(comm, request);
}

int MPI_Comm_join(int fd, MPI_Comm *intercomm)
{
  return VT()->MPI_Comm_join(fd, intercomm);
}

int MPI_Comm_rank(MPI_Comm comm, int *rank)
{
  return VT()->MPI_Comm_rank(comm, rank);
}

int MPI_Comm_remote_group(MPI_Comm comm, MPI_Group *group)
{
  return VT()->MPI_Comm_remote_group(comm, group);
}

int MPI_Comm_remote_size(MPI_Comm comm, int *size)
{
  return VT()->MPI_Comm_remote_size(comm, size);
}

int MPI_Comm_set_attr(MPI_Comm comm, int comm_keyval, void *attribute_val)
{
  return VT()->MPI_Comm_set_attr(comm, comm_keyval, attribute_val);
}

int MPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler)
{
  return VT()->MPI_Comm_set_errhandler(comm, errhandler);
}

int MPI_Comm_set_info(MPI_Comm comm, MPI_Info info)
{
  return VT()->MPI_Comm_set_info(comm, info);
}

int MPI_Comm_set_name(MPI_Comm comm, const char *comm_name)
{
  return VT()->MPI_Comm_set_name(comm, comm_name);
}

int MPI_Comm_size(MPI_Comm comm, int *size)
{
  return VT()->MPI_Comm_size(comm, size);
}

int MPI_Comm_spawn(const char *command, char *argv[], int maxprocs,
                   MPI_Info info, int root, MPI_Comm comm, MPI_Comm *intercomm,
                   int array_of_errcodes[])
{
  return VT()->MPI_Comm_spawn(command, argv, maxprocs, info, root, comm,
                              intercomm, array_of_errcodes);
}

int MPI_Comm_spawn_multiple(int count, char *array_of_commands[],
                            char **array_of_argv[],
                            const int array_of_maxprocs[],
                            const MPI_Info array_of_info[], int root,
                            MPI_Comm comm, MPI_Comm *intercomm,
                            int array_of_errcodes[])
{
  return VT()->MPI_Comm_spawn_multiple(count, array_of_commands, array_of_argv,
                                       array_of_maxprocs, array_of_info, root,
                                       comm, intercomm, array_of_errcodes);
}

int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm)
{
  return VT()->MPI_Comm_split(comm, color, key, newcomm);
}

int MPI_Comm_split_type(MPI_Comm comm, int split_type, int key, MPI_Info info,
                        MPI_Comm *newcomm)
{
  return VT()->MPI_Comm_split_type(comm, split_type, key, info, newcomm);
}

int MPI_Comm_test_inter(MPI_Comm comm, int *flag)
{
  return VT()->MPI_Comm_test_inter(comm, flag);
}

int MPI_Compare_and_swap(const void *origin_addr, const void *compare_addr,
                         void *result_addr, MPI_Datatype datatype,
                         int target_rank, MPI_Aint target_disp, MPI_Win win)
{
  return VT()->MPI_Compare_and_swap(origin_addr, compare_addr, result_addr,
                                    datatype, target_rank, target_disp, win);
}

int MPI_Dims_create(int nnodes, int ndims, int dims[])
{
  return VT()->MPI_Dims_create(nnodes, ndims, dims);
}

int MPI_Dist_graph_create(MPI_Comm comm_old, int n, const int sources[],
                          const int degrees[], const int destinations[],
                          const int weights[], MPI_Info info, int reorder,
                          MPI_Comm *comm_dist_graph)
{
  return VT()->MPI_Dist_graph_create(comm_old, n, sources, degrees,
                                     destinations, weights, info, reorder,
                                     comm_dist_graph);
}

int MPI_Dist_graph_create_adjacent(MPI_Comm comm_old, int indegree,
                                   const int sources[],
                                   const int sourceweights[], int outdegree,
                                   const int destinations[],
                                   const int destweights[], MPI_Info info,
                                   int reorder, MPI_Comm *comm_dist_graph)
{
  return VT()->MPI_Dist_graph_create_adjacent(comm_old, indegree, sources,
                                              sourceweights, outdegree,
                                              destinations, destweights, info,
                                              reorder, comm_dist_graph);
}

int MPI_Dist_graph_neighbors(MPI_Comm comm, int maxindegree, int sources[],
                             int sourceweights[], int maxoutdegree,
                             int destinations[], int destweights[])
{
  return VT()->MPI_Dist_graph_neighbors(comm, maxindegree, sources,
                                        sourceweights, maxoutdegree,
                                        destinations, destweights);
}

int MPI_Dist_graph_neighbors_count(MPI_Comm comm, int *indegree,
                                   int *outdegree, int *weighted)
{
  return VT()->MPI_Dist_graph_neighbors_count(comm, indegree, outdegree,
                                              weighted);
}

int MPI_Errhandler_free(MPI_Errhandler *errhandler)
{
  return VT()->MPI_Errhandler_free(errhandler);
}

int MPI_Error_class(int errorcode, int *errorclass)
{
  return VT()->MPI_Error_class(errorcode, errorclass);
}

int MPI_Error_string(int errorcode, char *string, int *resultlen)
{
  return VT()->MPI_Error_string(errorcode, string, resultlen);
}

int MPI_Exscan(const void *sendbuf, void *recvbuf, int count,
               MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return VT()->MPI_Exscan(sendbuf, recvbuf, count, datatype, op, comm);
}

int MPI_Exscan_c(const void *sendbuf, void *recvbuf, MPI_Count count,
                 MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return VT()->MPI_Exscan_c(sendbuf, recvbuf, count, datatype, op, comm);
}

int MPI_Exscan_init(const void *sendbuf, void *recvbuf, int count,
                    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                    MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Exscan_init(sendbuf, recvbuf, count, datatype, op, comm,
                               info, request);
}

int MPI_Exscan_init_c(const void *sendbuf, void *recvbuf, MPI_Count count,
                      MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                      MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Exscan_init_c(sendbuf, recvbuf, count, datatype, op, comm,
                                 info, request);
}

int MPI_Fetch_and_op(const void *origin_addr, void *result_addr,
                     MPI_Datatype datatype, int target_rank,
                     MPI_Aint target_disp, MPI_Op op, MPI_Win win)
{
  return VT()->MPI_Fetch_and_op(origin_addr, result_addr, datatype,
                                target_rank, target_disp, op, win);
}

int MPI_File_call_errhandler(MPI_File fh, int errorcode)
{
  return VT()->MPI_File_call_errhandler(fh, errorcode);
}

int MPI_File_close(MPI_File *fh) { return VT()->MPI_File_close(fh); }

int MPI_File_create_errhandler(
    MPI_File_errhandler_function *file_errhandler_fn,
    MPI_Errhandler *errhandler)
{
  return VT()->MPI_File_create_errhandler(file_errhandler_fn, errhandler);
}

int MPI_File_delete(const char *filename, MPI_Info info)
{
  return VT()->MPI_File_delete(filename, info);
}

int MPI_File_get_amode(MPI_File fh, int *amode)
{
  return VT()->MPI_File_get_amode(fh, amode);
}

int MPI_File_get_atomicity(MPI_File fh, int *flag)
{
  return VT()->MPI_File_get_atomicity(fh, flag);
}

int MPI_File_get_byte_offset(MPI_File fh, MPI_Offset offset, MPI_Offset *disp)
{
  return VT()->MPI_File_get_byte_offset(fh, offset, disp);
}

int MPI_File_get_errhandler(MPI_File file, MPI_Errhandler *errhandler)
{
  return VT()->MPI_File_get_errhandler(file, errhandler);
}

int MPI_File_get_group(MPI_File fh, MPI_Group *group)
{
  return VT()->MPI_File_get_group(fh, group);
}

int MPI_File_get_info(MPI_File fh, MPI_Info *info_used)
{
  return VT()->MPI_File_get_info(fh, info_used);
}

int MPI_File_get_position(MPI_File fh, MPI_Offset *offset)
{
  return VT()->MPI_File_get_position(fh, offset);
}

int MPI_File_get_position_shared(MPI_File fh, MPI_Offset *offset)
{
  return VT()->MPI_File_get_position_shared(fh, offset);
}

int MPI_File_get_size(MPI_File fh, MPI_Offset *size)
{
  return VT()->MPI_File_get_size(fh, size);
}

int MPI_File_get_type_extent(MPI_File fh, MPI_Datatype datatype,
                             MPI_Aint *extent)
{
  return VT()->MPI_File_get_type_extent(fh, datatype, extent);
}

int MPI_File_get_type_extent_c(MPI_File fh, MPI_Datatype datatype,
                               MPI_Count *extent)
{
  return VT()->MPI_File_get_type_extent_c(fh, datatype, extent);
}

int MPI_File_get_view(MPI_File fh, MPI_Offset *disp, MPI_Datatype *etype,
                      MPI_Datatype *filetype, char *datarep)
{
  return VT()->MPI_File_get_view(fh, disp, etype, filetype, datarep);
}

int MPI_File_iread(MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                   MPI_Request *request)
{
  return VT()->MPI_File_iread(fh, buf, count, datatype, request);
}

int MPI_File_iread_c(MPI_File fh, void *buf, MPI_Count count,
                     MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->MPI_File_iread_c(fh, buf, count, datatype, request);
}

int MPI_File_iread_all(MPI_File fh, void *buf, int count,
                       MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->MPI_File_iread_all(fh, buf, count, datatype, request);
}

int MPI_File_iread_all_c(MPI_File fh, void *buf, MPI_Count count,
                         MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->MPI_File_iread_all_c(fh, buf, count, datatype, request);
}

int MPI_File_iread_at(MPI_File fh, MPI_Offset offset, void *buf, int count,
                      MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->MPI_File_iread_at(fh, offset, buf, count, datatype, request);
}

int MPI_File_iread_at_c(MPI_File fh, MPI_Offset offset, void *buf,
                        MPI_Count count, MPI_Datatype datatype,
                        MPI_Request *request)
{
  return VT()->MPI_File_iread_at_c(fh, offset, buf, count, datatype, request);
}

int MPI_File_iread_at_all(MPI_File fh, MPI_Offset offset, void *buf, int count,
                          MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->MPI_File_iread_at_all(fh, offset, buf, count, datatype,
                                     request);
}

int MPI_File_iread_at_all_c(MPI_File fh, MPI_Offset offset, void *buf,
                            MPI_Count count, MPI_Datatype datatype,
                            MPI_Request *request)
{
  return VT()->MPI_File_iread_at_all_c(fh, offset, buf, count, datatype,
                                       request);
}

int MPI_File_iread_shared(MPI_File fh, void *buf, int count,
                          MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->MPI_File_iread_shared(fh, buf, count, datatype, request);
}

int MPI_File_iread_shared_c(MPI_File fh, void *buf, MPI_Count count,
                            MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->MPI_File_iread_shared_c(fh, buf, count, datatype, request);
}

int MPI_File_iwrite(MPI_File fh, const void *buf, int count,
                    MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->MPI_File_iwrite(fh, buf, count, datatype, request);
}

int MPI_File_iwrite_c(MPI_File fh, const void *buf, MPI_Count count,
                      MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->MPI_File_iwrite_c(fh, buf, count, datatype, request);
}

int MPI_File_iwrite_all(MPI_File fh, const void *buf, int count,
                        MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->MPI_File_iwrite_all(fh, buf, count, datatype, request);
}

int MPI_File_iwrite_all_c(MPI_File fh, const void *buf, MPI_Count count,
                          MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->MPI_File_iwrite_all_c(fh, buf, count, datatype, request);
}

int MPI_File_iwrite_at(MPI_File fh, MPI_Offset offset, const void *buf,
                       int count, MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->MPI_File_iwrite_at(fh, offset, buf, count, datatype, request);
}

int MPI_File_iwrite_at_c(MPI_File fh, MPI_Offset offset, const void *buf,
                         MPI_Count count, MPI_Datatype datatype,
                         MPI_Request *request)
{
  return VT()->MPI_File_iwrite_at_c(fh, offset, buf, count, datatype, request);
}

int MPI_File_iwrite_at_all(MPI_File fh, MPI_Offset offset, const void *buf,
                           int count, MPI_Datatype datatype,
                           MPI_Request *request)
{
  return VT()->MPI_File_iwrite_at_all(fh, offset, buf, count, datatype,
                                      request);
}

int MPI_File_iwrite_at_all_c(MPI_File fh, MPI_Offset offset, const void *buf,
                             MPI_Count count, MPI_Datatype datatype,
                             MPI_Request *request)
{
  return VT()->MPI_File_iwrite_at_all_c(fh, offset, buf, count, datatype,
                                        request);
}

int MPI_File_iwrite_shared(MPI_File fh, const void *buf, int count,
                           MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->MPI_File_iwrite_shared(fh, buf, count, datatype, request);
}

int MPI_File_iwrite_shared_c(MPI_File fh, const void *buf, MPI_Count count,
                             MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->MPI_File_iwrite_shared_c(fh, buf, count, datatype, request);
}

int MPI_File_open(MPI_Comm comm, const char *filename, int amode,
                  MPI_Info info, MPI_File *fh)
{
  return VT()->MPI_File_open(comm, filename, amode, info, fh);
}

int MPI_File_preallocate(MPI_File fh, MPI_Offset size)
{
  return VT()->MPI_File_preallocate(fh, size);
}

int MPI_File_read(MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                  MPI_Status *status)
{
  return VT()->MPI_File_read(fh, buf, count, datatype, status);
}

int MPI_File_read_c(MPI_File fh, void *buf, MPI_Count count,
                    MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->MPI_File_read_c(fh, buf, count, datatype, status);
}

int MPI_File_read_all(MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                      MPI_Status *status)
{
  return VT()->MPI_File_read_all(fh, buf, count, datatype, status);
}

int MPI_File_read_all_c(MPI_File fh, void *buf, MPI_Count count,
                        MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->MPI_File_read_all_c(fh, buf, count, datatype, status);
}

int MPI_File_read_all_begin(MPI_File fh, void *buf, int count,
                            MPI_Datatype datatype)
{
  return VT()->MPI_File_read_all_begin(fh, buf, count, datatype);
}

int MPI_File_read_all_begin_c(MPI_File fh, void *buf, MPI_Count count,
                              MPI_Datatype datatype)
{
  return VT()->MPI_File_read_all_begin_c(fh, buf, count, datatype);
}

int MPI_File_read_all_end(MPI_File fh, void *buf, MPI_Status *status)
{
  return VT()->MPI_File_read_all_end(fh, buf, status);
}

int MPI_File_read_at(MPI_File fh, MPI_Offset offset, void *buf, int count,
                     MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->MPI_File_read_at(fh, offset, buf, count, datatype, status);
}

int MPI_File_read_at_c(MPI_File fh, MPI_Offset offset, void *buf,
                       MPI_Count count, MPI_Datatype datatype,
                       MPI_Status *status)
{
  return VT()->MPI_File_read_at_c(fh, offset, buf, count, datatype, status);
}

int MPI_File_read_at_all(MPI_File fh, MPI_Offset offset, void *buf, int count,
                         MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->MPI_File_read_at_all(fh, offset, buf, count, datatype, status);
}

int MPI_File_read_at_all_c(MPI_File fh, MPI_Offset offset, void *buf,
                           MPI_Count count, MPI_Datatype datatype,
                           MPI_Status *status)
{
  return VT()->MPI_File_read_at_all_c(fh, offset, buf, count, datatype,
                                      status);
}

int MPI_File_read_at_all_begin(MPI_File fh, MPI_Offset offset, void *buf,
                               int count, MPI_Datatype datatype)
{
  return VT()->MPI_File_read_at_all_begin(fh, offset, buf, count, datatype);
}

int MPI_File_read_at_all_begin_c(MPI_File fh, MPI_Offset offset, void *buf,
                                 MPI_Count count, MPI_Datatype datatype)
{
  return VT()->MPI_File_read_at_all_begin_c(fh, offset, buf, count, datatype);
}

int MPI_File_read_at_all_end(MPI_File fh, void *buf, MPI_Status *status)
{
  return VT()->MPI_File_read_at_all_end(fh, buf, status);
}

int MPI_File_read_ordered(MPI_File fh, void *buf, int count,
                          MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->MPI_File_read_ordered(fh, buf, count, datatype, status);
}

int MPI_File_read_ordered_c(MPI_File fh, void *buf, MPI_Count count,
                            MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->MPI_File_read_ordered_c(fh, buf, count, datatype, status);
}

int MPI_File_read_ordered_begin(MPI_File fh, void *buf, int count,
                                MPI_Datatype datatype)
{
  return VT()->MPI_File_read_ordered_begin(fh, buf, count, datatype);
}

int MPI_File_read_ordered_begin_c(MPI_File fh, void *buf, MPI_Count count,
                                  MPI_Datatype datatype)
{
  return VT()->MPI_File_read_ordered_begin_c(fh, buf, count, datatype);
}

int MPI_File_read_ordered_end(MPI_File fh, void *buf, MPI_Status *status)
{
  return VT()->MPI_File_read_ordered_end(fh, buf, status);
}

int MPI_File_read_shared(MPI_File fh, void *buf, int count,
                         MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->MPI_File_read_shared(fh, buf, count, datatype, status);
}

int MPI_File_read_shared_c(MPI_File fh, void *buf, MPI_Count count,
                           MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->MPI_File_read_shared_c(fh, buf, count, datatype, status);
}

int MPI_File_seek(MPI_File fh, MPI_Offset offset, int whence)
{
  return VT()->MPI_File_seek(fh, offset, whence);
}

int MPI_File_seek_shared(MPI_File fh, MPI_Offset offset, int whence)
{
  return VT()->MPI_File_seek_shared(fh, offset, whence);
}

int MPI_File_set_atomicity(MPI_File fh, int flag)
{
  return VT()->MPI_File_set_atomicity(fh, flag);
}

int MPI_File_set_errhandler(MPI_File file, MPI_Errhandler errhandler)
{
  return VT()->MPI_File_set_errhandler(file, errhandler);
}

int MPI_File_set_info(MPI_File fh, MPI_Info info)
{
  return VT()->MPI_File_set_info(fh, info);
}

int MPI_File_set_size(MPI_File fh, MPI_Offset size)
{
  return VT()->MPI_File_set_size(fh, size);
}

int MPI_File_set_view(MPI_File fh, MPI_Offset disp, MPI_Datatype etype,
                      MPI_Datatype filetype, const char *datarep,
                      MPI_Info info)
{
  return VT()->MPI_File_set_view(fh, disp, etype, filetype, datarep, info);
}

int MPI_File_sync(MPI_File fh) { return VT()->MPI_File_sync(fh); }

int MPI_File_write(MPI_File fh, const void *buf, int count,
                   MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->MPI_File_write(fh, buf, count, datatype, status);
}

int MPI_File_write_c(MPI_File fh, const void *buf, MPI_Count count,
                     MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->MPI_File_write_c(fh, buf, count, datatype, status);
}

int MPI_File_write_all(MPI_File fh, const void *buf, int count,
                       MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->MPI_File_write_all(fh, buf, count, datatype, status);
}

int MPI_File_write_all_c(MPI_File fh, const void *buf, MPI_Count count,
                         MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->MPI_File_write_all_c(fh, buf, count, datatype, status);
}

int MPI_File_write_all_begin(MPI_File fh, const void *buf, int count,
                             MPI_Datatype datatype)
{
  return VT()->MPI_File_write_all_begin(fh, buf, count, datatype);
}

int MPI_File_write_all_begin_c(MPI_File fh, const void *buf, MPI_Count count,
                               MPI_Datatype datatype)
{
  return VT()->MPI_File_write_all_begin_c(fh, buf, count, datatype);
}

int MPI_File_write_all_end(MPI_File fh, const void *buf, MPI_Status *status)
{
  return VT()->MPI_File_write_all_end(fh, buf, status);
}

int MPI_File_write_at(MPI_File fh, MPI_Offset offset, const void *buf,
                      int count, MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->MPI_File_write_at(fh, offset, buf, count, datatype, status);
}

int MPI_File_write_at_c(MPI_File fh, MPI_Offset offset, const void *buf,
                        MPI_Count count, MPI_Datatype datatype,
                        MPI_Status *status)
{
  return VT()->MPI_File_write_at_c(fh, offset, buf, count, datatype, status);
}

int MPI_File_write_at_all(MPI_File fh, MPI_Offset offset, const void *buf,
                          int count, MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->MPI_File_write_at_all(fh, offset, buf, count, datatype, status);
}

int MPI_File_write_at_all_c(MPI_File fh, MPI_Offset offset, const void *buf,
                            MPI_Count count, MPI_Datatype datatype,
                            MPI_Status *status)
{
  return VT()->MPI_File_write_at_all_c(fh, offset, buf, count, datatype,
                                       status);
}

int MPI_File_write_at_all_begin(MPI_File fh, MPI_Offset offset,
                                const void *buf, int count,
                                MPI_Datatype datatype)
{
  return VT()->MPI_File_write_at_all_begin(fh, offset, buf, count, datatype);
}

int MPI_File_write_at_all_begin_c(MPI_File fh, MPI_Offset offset,
                                  const void *buf, MPI_Count count,
                                  MPI_Datatype datatype)
{
  return VT()->MPI_File_write_at_all_begin_c(fh, offset, buf, count, datatype);
}

int MPI_File_write_at_all_end(MPI_File fh, const void *buf, MPI_Status *status)
{
  return VT()->MPI_File_write_at_all_end(fh, buf, status);
}

int MPI_File_write_ordered(MPI_File fh, const void *buf, int count,
                           MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->MPI_File_write_ordered(fh, buf, count, datatype, status);
}

int MPI_File_write_ordered_c(MPI_File fh, const void *buf, MPI_Count count,
                             MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->MPI_File_write_ordered_c(fh, buf, count, datatype, status);
}

int MPI_File_write_ordered_begin(MPI_File fh, const void *buf, int count,
                                 MPI_Datatype datatype)
{
  return VT()->MPI_File_write_ordered_begin(fh, buf, count, datatype);
}

int MPI_File_write_ordered_begin_c(MPI_File fh, const void *buf,
                                   MPI_Count count, MPI_Datatype datatype)
{
  return VT()->MPI_File_write_ordered_begin_c(fh, buf, count, datatype);
}

int MPI_File_write_ordered_end(MPI_File fh, const void *buf,
                               MPI_Status *status)
{
  return VT()->MPI_File_write_ordered_end(fh, buf, status);
}

int MPI_File_write_shared(MPI_File fh, const void *buf, int count,
                          MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->MPI_File_write_shared(fh, buf, count, datatype, status);
}

int MPI_File_write_shared_c(MPI_File fh, const void *buf, MPI_Count count,
                            MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->MPI_File_write_shared_c(fh, buf, count, datatype, status);
}

int MPI_Finalize(void) { return VT()->MPI_Finalize(); }

int MPI_Finalized(int *flag) { return VT()->MPI_Finalized(flag); }

int MPI_Free_mem(void *base) { return VT()->MPI_Free_mem(base); }

int MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
               void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
               MPI_Comm comm)
{
  return VT()->MPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                          recvtype, root, comm);
}

int MPI_Gather_c(const void *sendbuf, MPI_Count sendcount,
                 MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount,
                 MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  return VT()->MPI_Gather_c(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                            recvtype, root, comm);
}

int MPI_Gather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                    void *recvbuf, int recvcount, MPI_Datatype recvtype,
                    int root, MPI_Comm comm, MPI_Info info,
                    MPI_Request *request)
{
  return VT()->MPI_Gather_init(sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, root, comm, info, request);
}

int MPI_Gather_init_c(const void *sendbuf, MPI_Count sendcount,
                      MPI_Datatype sendtype, void *recvbuf,
                      MPI_Count recvcount, MPI_Datatype recvtype, int root,
                      MPI_Comm comm, MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Gather_init_c(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcount, recvtype, root, comm, info,
                                 request);
}

int MPI_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, const int recvcounts[], const int displs[],
                MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  return VT()->MPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                           displs, recvtype, root, comm);
}

int MPI_Gatherv_c(const void *sendbuf, MPI_Count sendcount,
                  MPI_Datatype sendtype, void *recvbuf,
                  const MPI_Count recvcounts[], const MPI_Aint displs[],
                  MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  return VT()->MPI_Gatherv_c(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                             displs, recvtype, root, comm);
}

int MPI_Gatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                     void *recvbuf, const int recvcounts[], const int displs[],
                     MPI_Datatype recvtype, int root, MPI_Comm comm,
                     MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Gatherv_init(sendbuf, sendcount, sendtype, recvbuf,
                                recvcounts, displs, recvtype, root, comm, info,
                                request);
}

int MPI_Gatherv_init_c(const void *sendbuf, MPI_Count sendcount,
                       MPI_Datatype sendtype, void *recvbuf,
                       const MPI_Count recvcounts[], const MPI_Aint displs[],
                       MPI_Datatype recvtype, int root, MPI_Comm comm,
                       MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Gatherv_init_c(sendbuf, sendcount, sendtype, recvbuf,
                                  recvcounts, displs, recvtype, root, comm,
                                  info, request);
}

int MPI_Get(void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
            int target_rank, MPI_Aint target_disp, int target_count,
            MPI_Datatype target_datatype, MPI_Win win)
{
  return VT()->MPI_Get(origin_addr, origin_count, origin_datatype, target_rank,
                       target_disp, target_count, target_datatype, win);
}

int MPI_Get_c(void *origin_addr, MPI_Count origin_count,
              MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, MPI_Count target_count,
              MPI_Datatype target_datatype, MPI_Win win)
{
  return VT()->MPI_Get_c(origin_addr, origin_count, origin_datatype,
                         target_rank, target_disp, target_count,
                         target_datatype, win);
}

int MPI_Get_accumulate(const void *origin_addr, int origin_count,
                       MPI_Datatype origin_datatype, void *result_addr,
                       int result_count, MPI_Datatype result_datatype,
                       int target_rank, MPI_Aint target_disp, int target_count,
                       MPI_Datatype target_datatype, MPI_Op op, MPI_Win win)
{
  return VT()->MPI_Get_accumulate(origin_addr, origin_count, origin_datatype,
                                  result_addr, result_count, result_datatype,
                                  target_rank, target_disp, target_count,
                                  target_datatype, op, win);
}

int MPI_Get_accumulate_c(const void *origin_addr, MPI_Count origin_count,
                         MPI_Datatype origin_datatype, void *result_addr,
                         MPI_Count result_count, MPI_Datatype result_datatype,
                         int target_rank, MPI_Aint target_disp,
                         MPI_Count target_count, MPI_Datatype target_datatype,
                         MPI_Op op, MPI_Win win)
{
  return VT()->MPI_Get_accumulate_c(origin_addr, origin_count, origin_datatype,
                                    result_addr, result_count, result_datatype,
                                    target_rank, target_disp, target_count,
                                    target_datatype, op, win);
}

int MPI_Get_address(const void *location, MPI_Aint *address)
{
  return VT()->MPI_Get_address(location, address);
}

int MPI_Get_count(const MPI_Status *status, MPI_Datatype datatype, int *count)
{
  return VT()->MPI_Get_count(status, datatype, count);
}

int MPI_Get_count_c(const MPI_Status *status, MPI_Datatype datatype,
                    MPI_Count *count)
{
  return VT()->MPI_Get_count_c(status, datatype, count);
}

int MPI_Get_elements(const MPI_Status *status, MPI_Datatype datatype,
                     int *count)
{
  return VT()->MPI_Get_elements(status, datatype, count);
}

int MPI_Get_elements_c(const MPI_Status *status, MPI_Datatype datatype,
                       MPI_Count *count)
{
  return VT()->MPI_Get_elements_c(status, datatype, count);
}

int MPI_Get_elements_x(const MPI_Status *status, MPI_Datatype datatype,
                       MPI_Count *count)
{
  return VT()->MPI_Get_elements_x(status, datatype, count);
}

int MPI_Get_hw_resource_info(MPI_Info *hw_info)
{
  return VT()->MPI_Get_hw_resource_info(hw_info);
}

int MPI_Get_library_version(char *version, int *resultlen)
{
  return VT()->MPI_Get_library_version(version, resultlen);
}

int MPI_Get_processor_name(char *name, int *resultlen)
{
  return VT()->MPI_Get_processor_name(name, resultlen);
}

int MPI_Get_version(int *version, int *subversion)
{
  return VT()->MPI_Get_version(version, subversion);
}

int MPI_Graph_create(MPI_Comm comm_old, int nnodes, const int indx[],
                     const int edges[], int reorder, MPI_Comm *comm_graph)
{
  return VT()->MPI_Graph_create(comm_old, nnodes, indx, edges, reorder,
                                comm_graph);
}

int MPI_Graph_get(MPI_Comm comm, int maxindex, int maxedges, int indx[],
                  int edges[])
{
  return VT()->MPI_Graph_get(comm, maxindex, maxedges, indx, edges);
}

int MPI_Graph_map(MPI_Comm comm, int nnodes, const int indx[],
                  const int edges[], int *newrank)
{
  return VT()->MPI_Graph_map(comm, nnodes, indx, edges, newrank);
}

int MPI_Graph_neighbors(MPI_Comm comm, int rank, int maxneighbors,
                        int neighbors[])
{
  return VT()->MPI_Graph_neighbors(comm, rank, maxneighbors, neighbors);
}

int MPI_Graph_neighbors_count(MPI_Comm comm, int rank, int *nneighbors)
{
  return VT()->MPI_Graph_neighbors_count(comm, rank, nneighbors);
}

int MPI_Graphdims_get(MPI_Comm comm, int *nnodes, int *nedges)
{
  return VT()->MPI_Graphdims_get(comm, nnodes, nedges);
}

int MPI_Grequest_complete(MPI_Request request)
{
  return VT()->MPI_Grequest_complete(request);
}

int MPI_Grequest_start(MPI_Grequest_query_function *query_fn,
                       MPI_Grequest_free_function *free_fn,
                       MPI_Grequest_cancel_function *cancel_fn,
                       void *extra_state, MPI_Request *request)
{
  return VT()->MPI_Grequest_start(query_fn, free_fn, cancel_fn, extra_state,
                                  request);
}

int MPI_Group_compare(MPI_Group group1, MPI_Group group2, int *result)
{
  return VT()->MPI_Group_compare(group1, group2, result);
}

int MPI_Group_difference(MPI_Group group1, MPI_Group group2,
                         MPI_Group *newgroup)
{
  return VT()->MPI_Group_difference(group1, group2, newgroup);
}

int MPI_Group_excl(MPI_Group group, int n, const int ranks[],
                   MPI_Group *newgroup)
{
  return VT()->MPI_Group_excl(group, n, ranks, newgroup);
}

int MPI_Group_free(MPI_Group *group) { return VT()->MPI_Group_free(group); }

int MPI_Group_from_session_pset(MPI_Session session, const char *pset_name,
                                MPI_Group *newgroup)
{
  return VT()->MPI_Group_from_session_pset(session, pset_name, newgroup);
}

int MPI_Group_incl(MPI_Group group, int n, const int ranks[],
                   MPI_Group *newgroup)
{
  return VT()->MPI_Group_incl(group, n, ranks, newgroup);
}

int MPI_Group_intersection(MPI_Group group1, MPI_Group group2,
                           MPI_Group *newgroup)
{
  return VT()->MPI_Group_intersection(group1, group2, newgroup);
}

int MPI_Group_range_excl(MPI_Group group, int n, int ranges[][3],
                         MPI_Group *newgroup)
{
  return VT()->MPI_Group_range_excl(group, n, ranges, newgroup);
}

int MPI_Group_range_incl(MPI_Group group, int n, int ranges[][3],
                         MPI_Group *newgroup)
{
  return VT()->MPI_Group_range_incl(group, n, ranges, newgroup);
}

int MPI_Group_rank(MPI_Group group, int *rank)
{
  return VT()->MPI_Group_rank(group, rank);
}

int MPI_Group_size(MPI_Group group, int *size)
{
  return VT()->MPI_Group_size(group, size);
}

int MPI_Group_translate_ranks(MPI_Group group1, int n, const int ranks1[],
                              MPI_Group group2, int ranks2[])
{
  return VT()->MPI_Group_translate_ranks(group1, n, ranks1, group2, ranks2);
}

int MPI_Group_union(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup)
{
  return VT()->MPI_Group_union(group1, group2, newgroup);
}

int MPI_Iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                   MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                              recvtype, comm, request);
}

int MPI_Iallgather_c(const void *sendbuf, MPI_Count sendcount,
                     MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount,
                     MPI_Datatype recvtype, MPI_Comm comm,
                     MPI_Request *request)
{
  return VT()->MPI_Iallgather_c(sendbuf, sendcount, sendtype, recvbuf,
                                recvcount, recvtype, comm, request);
}

int MPI_Iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                    void *recvbuf, const int recvcounts[], const int displs[],
                    MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Iallgatherv(sendbuf, sendcount, sendtype, recvbuf,
                               recvcounts, displs, recvtype, comm, request);
}

int MPI_Iallgatherv_c(const void *sendbuf, MPI_Count sendcount,
                      MPI_Datatype sendtype, void *recvbuf,
                      const MPI_Count recvcounts[], const MPI_Aint displs[],
                      MPI_Datatype recvtype, MPI_Comm comm,
                      MPI_Request *request)
{
  return VT()->MPI_Iallgatherv_c(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcounts, displs, recvtype, comm, request);
}

int MPI_Iallreduce(const void *sendbuf, void *recvbuf, int count,
                   MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                   MPI_Request *request)
{
  return VT()->MPI_Iallreduce(sendbuf, recvbuf, count, datatype, op, comm,
                              request);
}

int MPI_Iallreduce_c(const void *sendbuf, void *recvbuf, MPI_Count count,
                     MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                     MPI_Request *request)
{
  return VT()->MPI_Iallreduce_c(sendbuf, recvbuf, count, datatype, op, comm,
                                request);
}

int MPI_Ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                             recvtype, comm, request);
}

int MPI_Ialltoall_c(const void *sendbuf, MPI_Count sendcount,
                    MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount,
                    MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Ialltoall_c(sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, comm, request);
}

int MPI_Ialltoallv(const void *sendbuf, const int sendcounts[],
                   const int sdispls[], MPI_Datatype sendtype, void *recvbuf,
                   const int recvcounts[], const int rdispls[],
                   MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                              recvcounts, rdispls, recvtype, comm, request);
}

int MPI_Ialltoallv_c(const void *sendbuf, const MPI_Count sendcounts[],
                     const MPI_Aint sdispls[], MPI_Datatype sendtype,
                     void *recvbuf, const MPI_Count recvcounts[],
                     const MPI_Aint rdispls[], MPI_Datatype recvtype,
                     MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Ialltoallv_c(sendbuf, sendcounts, sdispls, sendtype,
                                recvbuf, recvcounts, rdispls, recvtype, comm,
                                request);
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

int MPI_Ialltoallw_c(const void *sendbuf, const MPI_Count sendcounts[],
                     const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                     void *recvbuf, const MPI_Count recvcounts[],
                     const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                     MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Ialltoallw_c(sendbuf, sendcounts, sdispls, sendtypes,
                                recvbuf, recvcounts, rdispls, recvtypes, comm,
                                request);
}

int MPI_Ibarrier(MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Ibarrier(comm, request);
}

int MPI_Ibcast(void *buffer, int count, MPI_Datatype datatype, int root,
               MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Ibcast(buffer, count, datatype, root, comm, request);
}

int MPI_Ibcast_c(void *buffer, MPI_Count count, MPI_Datatype datatype,
                 int root, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Ibcast_c(buffer, count, datatype, root, comm, request);
}

int MPI_Ibsend(const void *buf, int count, MPI_Datatype datatype, int dest,
               int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Ibsend(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Ibsend_c(const void *buf, MPI_Count count, MPI_Datatype datatype,
                 int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Ibsend_c(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Iexscan(const void *sendbuf, void *recvbuf, int count,
                MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                MPI_Request *request)
{
  return VT()->MPI_Iexscan(sendbuf, recvbuf, count, datatype, op, comm,
                           request);
}

int MPI_Iexscan_c(const void *sendbuf, void *recvbuf, MPI_Count count,
                  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                  MPI_Request *request)
{
  return VT()->MPI_Iexscan_c(sendbuf, recvbuf, count, datatype, op, comm,
                             request);
}

int MPI_Igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Igather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                           recvtype, root, comm, request);
}

int MPI_Igather_c(const void *sendbuf, MPI_Count sendcount,
                  MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount,
                  MPI_Datatype recvtype, int root, MPI_Comm comm,
                  MPI_Request *request)
{
  return VT()->MPI_Igather_c(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                             recvtype, root, comm, request);
}

int MPI_Igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, const int recvcounts[], const int displs[],
                 MPI_Datatype recvtype, int root, MPI_Comm comm,
                 MPI_Request *request)
{
  return VT()->MPI_Igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                            displs, recvtype, root, comm, request);
}

int MPI_Igatherv_c(const void *sendbuf, MPI_Count sendcount,
                   MPI_Datatype sendtype, void *recvbuf,
                   const MPI_Count recvcounts[], const MPI_Aint displs[],
                   MPI_Datatype recvtype, int root, MPI_Comm comm,
                   MPI_Request *request)
{
  return VT()->MPI_Igatherv_c(sendbuf, sendcount, sendtype, recvbuf,
                              recvcounts, displs, recvtype, root, comm,
                              request);
}

int MPI_Improbe(int source, int tag, MPI_Comm comm, int *flag,
                MPI_Message *message, MPI_Status *status)
{
  return VT()->MPI_Improbe(source, tag, comm, flag, message, status);
}

int MPI_Imrecv(void *buf, int count, MPI_Datatype datatype,
               MPI_Message *message, MPI_Request *request)
{
  return VT()->MPI_Imrecv(buf, count, datatype, message, request);
}

int MPI_Imrecv_c(void *buf, MPI_Count count, MPI_Datatype datatype,
                 MPI_Message *message, MPI_Request *request)
{
  return VT()->MPI_Imrecv_c(buf, count, datatype, message, request);
}

int MPI_Ineighbor_allgather(const void *sendbuf, int sendcount,
                            MPI_Datatype sendtype, void *recvbuf,
                            int recvcount, MPI_Datatype recvtype,
                            MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Ineighbor_allgather(sendbuf, sendcount, sendtype, recvbuf,
                                       recvcount, recvtype, comm, request);
}

int MPI_Ineighbor_allgather_c(const void *sendbuf, MPI_Count sendcount,
                              MPI_Datatype sendtype, void *recvbuf,
                              MPI_Count recvcount, MPI_Datatype recvtype,
                              MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Ineighbor_allgather_c(sendbuf, sendcount, sendtype, recvbuf,
                                         recvcount, recvtype, comm, request);
}

int MPI_Ineighbor_allgatherv(const void *sendbuf, int sendcount,
                             MPI_Datatype sendtype, void *recvbuf,
                             const int recvcounts[], const int displs[],
                             MPI_Datatype recvtype, MPI_Comm comm,
                             MPI_Request *request)
{
  return VT()->MPI_Ineighbor_allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                        recvcounts, displs, recvtype, comm,
                                        request);
}

int MPI_Ineighbor_allgatherv_c(const void *sendbuf, MPI_Count sendcount,
                               MPI_Datatype sendtype, void *recvbuf,
                               const MPI_Count recvcounts[],
                               const MPI_Aint displs[], MPI_Datatype recvtype,
                               MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Ineighbor_allgatherv_c(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcounts, displs,
                                          recvtype, comm, request);
}

int MPI_Ineighbor_alltoall(const void *sendbuf, int sendcount,
                           MPI_Datatype sendtype, void *recvbuf, int recvcount,
                           MPI_Datatype recvtype, MPI_Comm comm,
                           MPI_Request *request)
{
  return VT()->MPI_Ineighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                      recvcount, recvtype, comm, request);
}

int MPI_Ineighbor_alltoall_c(const void *sendbuf, MPI_Count sendcount,
                             MPI_Datatype sendtype, void *recvbuf,
                             MPI_Count recvcount, MPI_Datatype recvtype,
                             MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Ineighbor_alltoall_c(sendbuf, sendcount, sendtype, recvbuf,
                                        recvcount, recvtype, comm, request);
}

int MPI_Ineighbor_alltoallv(const void *sendbuf, const int sendcounts[],
                            const int sdispls[], MPI_Datatype sendtype,
                            void *recvbuf, const int recvcounts[],
                            const int rdispls[], MPI_Datatype recvtype,
                            MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Ineighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                       recvbuf, recvcounts, rdispls, recvtype,
                                       comm, request);
}

int MPI_Ineighbor_alltoallv_c(const void *sendbuf,
                              const MPI_Count sendcounts[],
                              const MPI_Aint sdispls[], MPI_Datatype sendtype,
                              void *recvbuf, const MPI_Count recvcounts[],
                              const MPI_Aint rdispls[], MPI_Datatype recvtype,
                              MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Ineighbor_alltoallv_c(sendbuf, sendcounts, sdispls,
                                         sendtype, recvbuf, recvcounts,
                                         rdispls, recvtype, comm, request);
}

int MPI_Ineighbor_alltoallw(const void *sendbuf, const int sendcounts[],
                            const MPI_Aint sdispls[],
                            const MPI_Datatype sendtypes[], void *recvbuf,
                            const int recvcounts[], const MPI_Aint rdispls[],
                            const MPI_Datatype recvtypes[], MPI_Comm comm,
                            MPI_Request *request)
{
  return VT()->MPI_Ineighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                       recvbuf, recvcounts, rdispls, recvtypes,
                                       comm, request);
}

int MPI_Ineighbor_alltoallw_c(const void *sendbuf,
                              const MPI_Count sendcounts[],
                              const MPI_Aint sdispls[],
                              const MPI_Datatype sendtypes[], void *recvbuf,
                              const MPI_Count recvcounts[],
                              const MPI_Aint rdispls[],
                              const MPI_Datatype recvtypes[], MPI_Comm comm,
                              MPI_Request *request)
{
  return VT()->MPI_Ineighbor_alltoallw_c(sendbuf, sendcounts, sdispls,
                                         sendtypes, recvbuf, recvcounts,
                                         rdispls, recvtypes, comm, request);
}

int MPI_Info_create(MPI_Info *info) { return VT()->MPI_Info_create(info); }

int MPI_Info_create_env(int argc, char *argv[], MPI_Info *info)
{
  return VT()->MPI_Info_create_env(argc, argv, info);
}

int MPI_Info_delete(MPI_Info info, const char *key)
{
  return VT()->MPI_Info_delete(info, key);
}

int MPI_Info_dup(MPI_Info info, MPI_Info *newinfo)
{
  return VT()->MPI_Info_dup(info, newinfo);
}

int MPI_Info_free(MPI_Info *info) { return VT()->MPI_Info_free(info); }

int MPI_Info_get(MPI_Info info, const char *key, int valuelen, char *value,
                 int *flag)
{
  return VT()->MPI_Info_get(info, key, valuelen, value, flag);
}

int MPI_Info_get_nkeys(MPI_Info info, int *nkeys)
{
  return VT()->MPI_Info_get_nkeys(info, nkeys);
}

int MPI_Info_get_nthkey(MPI_Info info, int n, char *key)
{
  return VT()->MPI_Info_get_nthkey(info, n, key);
}

int MPI_Info_get_string(MPI_Info info, const char *key, int *buflen,
                        char *value, int *flag)
{
  return VT()->MPI_Info_get_string(info, key, buflen, value, flag);
}

int MPI_Info_get_valuelen(MPI_Info info, const char *key, int *valuelen,
                          int *flag)
{
  return VT()->MPI_Info_get_valuelen(info, key, valuelen, flag);
}

int MPI_Info_set(MPI_Info info, const char *key, const char *value)
{
  return VT()->MPI_Info_set(info, key, value);
}

int MPI_Init(int *argc, char ***argv) { return VT()->MPI_Init(argc, argv); }

int MPI_Init_thread(int *argc, char ***argv, int required, int *provided)
{
  return VT()->MPI_Init_thread(argc, argv, required, provided);
}

int MPI_Initialized(int *flag) { return VT()->MPI_Initialized(flag); }

int MPI_Intercomm_create(MPI_Comm local_comm, int local_leader,
                         MPI_Comm peer_comm, int remote_leader, int tag,
                         MPI_Comm *newintercomm)
{
  return VT()->MPI_Intercomm_create(local_comm, local_leader, peer_comm,
                                    remote_leader, tag, newintercomm);
}

int MPI_Intercomm_create_from_groups(MPI_Group local_group, int local_leader,
                                     MPI_Group remote_group, int remote_leader,
                                     const char *stringtag, MPI_Info info,
                                     MPI_Errhandler errhandler,
                                     MPI_Comm *newintercomm)
{
  return VT()->MPI_Intercomm_create_from_groups(local_group, local_leader,
                                                remote_group, remote_leader,
                                                stringtag, info, errhandler,
                                                newintercomm);
}

int MPI_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm *newintracomm)
{
  return VT()->MPI_Intercomm_merge(intercomm, high, newintracomm);
}

int MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag,
               MPI_Status *status)
{
  return VT()->MPI_Iprobe(source, tag, comm, flag, status);
}

int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Irecv(buf, count, datatype, source, tag, comm, request);
}

int MPI_Irecv_c(void *buf, MPI_Count count, MPI_Datatype datatype, int source,
                int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Irecv_c(buf, count, datatype, source, tag, comm, request);
}

int MPI_Ireduce(const void *sendbuf, void *recvbuf, int count,
                MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm,
                MPI_Request *request)
{
  return VT()->MPI_Ireduce(sendbuf, recvbuf, count, datatype, op, root, comm,
                           request);
}

int MPI_Ireduce_c(const void *sendbuf, void *recvbuf, MPI_Count count,
                  MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm,
                  MPI_Request *request)
{
  return VT()->MPI_Ireduce_c(sendbuf, recvbuf, count, datatype, op, root, comm,
                             request);
}

int MPI_Ireduce_scatter(const void *sendbuf, void *recvbuf,
                        const int recvcounts[], MPI_Datatype datatype,
                        MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op,
                                   comm, request);
}

int MPI_Ireduce_scatter_c(const void *sendbuf, void *recvbuf,
                          const MPI_Count recvcounts[], MPI_Datatype datatype,
                          MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Ireduce_scatter_c(sendbuf, recvbuf, recvcounts, datatype,
                                     op, comm, request);
}

int MPI_Ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                              int recvcount, MPI_Datatype datatype, MPI_Op op,
                              MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype,
                                         op, comm, request);
}

int MPI_Ireduce_scatter_block_c(const void *sendbuf, void *recvbuf,
                                MPI_Count recvcount, MPI_Datatype datatype,
                                MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Ireduce_scatter_block_c(sendbuf, recvbuf, recvcount,
                                           datatype, op, comm, request);
}

int MPI_Irsend(const void *buf, int count, MPI_Datatype datatype, int dest,
               int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Irsend(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Irsend_c(const void *buf, MPI_Count count, MPI_Datatype datatype,
                 int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Irsend_c(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Is_thread_main(int *flag) { return VT()->MPI_Is_thread_main(flag); }

int MPI_Iscan(const void *sendbuf, void *recvbuf, int count,
              MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
              MPI_Request *request)
{
  return VT()->MPI_Iscan(sendbuf, recvbuf, count, datatype, op, comm, request);
}

int MPI_Iscan_c(const void *sendbuf, void *recvbuf, MPI_Count count,
                MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                MPI_Request *request)
{
  return VT()->MPI_Iscan_c(sendbuf, recvbuf, count, datatype, op, comm,
                           request);
}

int MPI_Iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                 MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                            recvtype, root, comm, request);
}

int MPI_Iscatter_c(const void *sendbuf, MPI_Count sendcount,
                   MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount,
                   MPI_Datatype recvtype, int root, MPI_Comm comm,
                   MPI_Request *request)
{
  return VT()->MPI_Iscatter_c(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                              recvtype, root, comm, request);
}

int MPI_Iscatterv(const void *sendbuf, const int sendcounts[],
                  const int displs[], MPI_Datatype sendtype, void *recvbuf,
                  int recvcount, MPI_Datatype recvtype, int root,
                  MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                             recvcount, recvtype, root, comm, request);
}

int MPI_Iscatterv_c(const void *sendbuf, const MPI_Count sendcounts[],
                    const MPI_Aint displs[], MPI_Datatype sendtype,
                    void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype,
                    int root, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Iscatterv_c(sendbuf, sendcounts, displs, sendtype, recvbuf,
                               recvcount, recvtype, root, comm, request);
}

int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest,
              int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Isend(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Isend_c(const void *buf, MPI_Count count, MPI_Datatype datatype,
                int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Isend_c(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Isendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  int dest, int sendtag, void *recvbuf, int recvcount,
                  MPI_Datatype recvtype, int source, int recvtag,
                  MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Isendrecv(sendbuf, sendcount, sendtype, dest, sendtag,
                             recvbuf, recvcount, recvtype, source, recvtag,
                             comm, request);
}

int MPI_Isendrecv_c(const void *sendbuf, MPI_Count sendcount,
                    MPI_Datatype sendtype, int dest, int sendtag,
                    void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype,
                    int source, int recvtag, MPI_Comm comm,
                    MPI_Request *request)
{
  return VT()->MPI_Isendrecv_c(sendbuf, sendcount, sendtype, dest, sendtag,
                               recvbuf, recvcount, recvtype, source, recvtag,
                               comm, request);
}

int MPI_Isendrecv_replace(void *buf, int count, MPI_Datatype datatype,
                          int dest, int sendtag, int source, int recvtag,
                          MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Isendrecv_replace(buf, count, datatype, dest, sendtag,
                                     source, recvtag, comm, request);
}

int MPI_Isendrecv_replace_c(void *buf, MPI_Count count, MPI_Datatype datatype,
                            int dest, int sendtag, int source, int recvtag,
                            MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Isendrecv_replace_c(buf, count, datatype, dest, sendtag,
                                       source, recvtag, comm, request);
}

int MPI_Issend(const void *buf, int count, MPI_Datatype datatype, int dest,
               int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Issend(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Issend_c(const void *buf, MPI_Count count, MPI_Datatype datatype,
                 int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Issend_c(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Keyval_create(MPI_Copy_function *copy_fn,
                      MPI_Delete_function *delete_fn, int *keyval,
                      void *extra_state)
{
  return VT()->MPI_Comm_create_keyval(copy_fn, delete_fn, keyval, extra_state);
}

int MPI_Keyval_free(int *keyval) { return VT()->MPI_Comm_free_keyval(keyval); }

int MPI_Lookup_name(const char *service_name, MPI_Info info, char *port_name)
{
  return VT()->MPI_Lookup_name(service_name, info, port_name);
}

int MPI_Mprobe(int source, int tag, MPI_Comm comm, MPI_Message *message,
               MPI_Status *status)
{
  return VT()->MPI_Mprobe(source, tag, comm, message, status);
}

int MPI_Mrecv(void *buf, int count, MPI_Datatype datatype,
              MPI_Message *message, MPI_Status *status)
{
  return VT()->MPI_Mrecv(buf, count, datatype, message, status);
}

int MPI_Mrecv_c(void *buf, MPI_Count count, MPI_Datatype datatype,
                MPI_Message *message, MPI_Status *status)
{
  return VT()->MPI_Mrecv_c(buf, count, datatype, message, status);
}

int MPI_Neighbor_allgather(const void *sendbuf, int sendcount,
                           MPI_Datatype sendtype, void *recvbuf, int recvcount,
                           MPI_Datatype recvtype, MPI_Comm comm)
{
  return VT()->MPI_Neighbor_allgather(sendbuf, sendcount, sendtype, recvbuf,
                                      recvcount, recvtype, comm);
}

int MPI_Neighbor_allgather_c(const void *sendbuf, MPI_Count sendcount,
                             MPI_Datatype sendtype, void *recvbuf,
                             MPI_Count recvcount, MPI_Datatype recvtype,
                             MPI_Comm comm)
{
  return VT()->MPI_Neighbor_allgather_c(sendbuf, sendcount, sendtype, recvbuf,
                                        recvcount, recvtype, comm);
}

int MPI_Neighbor_allgather_init(const void *sendbuf, int sendcount,
                                MPI_Datatype sendtype, void *recvbuf,
                                int recvcount, MPI_Datatype recvtype,
                                MPI_Comm comm, MPI_Info info,
                                MPI_Request *request)
{
  return VT()->MPI_Neighbor_allgather_init(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype, comm,
                                           info, request);
}

int MPI_Neighbor_allgather_init_c(const void *sendbuf, MPI_Count sendcount,
                                  MPI_Datatype sendtype, void *recvbuf,
                                  MPI_Count recvcount, MPI_Datatype recvtype,
                                  MPI_Comm comm, MPI_Info info,
                                  MPI_Request *request)
{
  return VT()->MPI_Neighbor_allgather_init_c(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcount, recvtype,
                                             comm, info, request);
}

int MPI_Neighbor_allgatherv(const void *sendbuf, int sendcount,
                            MPI_Datatype sendtype, void *recvbuf,
                            const int recvcounts[], const int displs[],
                            MPI_Datatype recvtype, MPI_Comm comm)
{
  return VT()->MPI_Neighbor_allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                       recvcounts, displs, recvtype, comm);
}

int MPI_Neighbor_allgatherv_c(const void *sendbuf, MPI_Count sendcount,
                              MPI_Datatype sendtype, void *recvbuf,
                              const MPI_Count recvcounts[],
                              const MPI_Aint displs[], MPI_Datatype recvtype,
                              MPI_Comm comm)
{
  return VT()->MPI_Neighbor_allgatherv_c(sendbuf, sendcount, sendtype, recvbuf,
                                         recvcounts, displs, recvtype, comm);
}

int MPI_Neighbor_allgatherv_init(const void *sendbuf, int sendcount,
                                 MPI_Datatype sendtype, void *recvbuf,
                                 const int recvcounts[], const int displs[],
                                 MPI_Datatype recvtype, MPI_Comm comm,
                                 MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Neighbor_allgatherv_init(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcounts, displs,
                                            recvtype, comm, info, request);
}

int MPI_Neighbor_allgatherv_init_c(const void *sendbuf, MPI_Count sendcount,
                                   MPI_Datatype sendtype, void *recvbuf,
                                   const MPI_Count recvcounts[],
                                   const MPI_Aint displs[],
                                   MPI_Datatype recvtype, MPI_Comm comm,
                                   MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Neighbor_allgatherv_init_c(sendbuf, sendcount, sendtype,
                                              recvbuf, recvcounts, displs,
                                              recvtype, comm, info, request);
}

int MPI_Neighbor_alltoall(const void *sendbuf, int sendcount,
                          MPI_Datatype sendtype, void *recvbuf, int recvcount,
                          MPI_Datatype recvtype, MPI_Comm comm)
{
  return VT()->MPI_Neighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                     recvcount, recvtype, comm);
}

int MPI_Neighbor_alltoall_c(const void *sendbuf, MPI_Count sendcount,
                            MPI_Datatype sendtype, void *recvbuf,
                            MPI_Count recvcount, MPI_Datatype recvtype,
                            MPI_Comm comm)
{
  return VT()->MPI_Neighbor_alltoall_c(sendbuf, sendcount, sendtype, recvbuf,
                                       recvcount, recvtype, comm);
}

int MPI_Neighbor_alltoall_init(const void *sendbuf, int sendcount,
                               MPI_Datatype sendtype, void *recvbuf,
                               int recvcount, MPI_Datatype recvtype,
                               MPI_Comm comm, MPI_Info info,
                               MPI_Request *request)
{
  return VT()->MPI_Neighbor_alltoall_init(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcount, recvtype, comm,
                                          info, request);
}

int MPI_Neighbor_alltoall_init_c(const void *sendbuf, MPI_Count sendcount,
                                 MPI_Datatype sendtype, void *recvbuf,
                                 MPI_Count recvcount, MPI_Datatype recvtype,
                                 MPI_Comm comm, MPI_Info info,
                                 MPI_Request *request)
{
  return VT()->MPI_Neighbor_alltoall_init_c(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcount, recvtype, comm,
                                            info, request);
}

int MPI_Neighbor_alltoallv(const void *sendbuf, const int sendcounts[],
                           const int sdispls[], MPI_Datatype sendtype,
                           void *recvbuf, const int recvcounts[],
                           const int rdispls[], MPI_Datatype recvtype,
                           MPI_Comm comm)
{
  return VT()->MPI_Neighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                      recvbuf, recvcounts, rdispls, recvtype,
                                      comm);
}

int MPI_Neighbor_alltoallv_c(const void *sendbuf, const MPI_Count sendcounts[],
                             const MPI_Aint sdispls[], MPI_Datatype sendtype,
                             void *recvbuf, const MPI_Count recvcounts[],
                             const MPI_Aint rdispls[], MPI_Datatype recvtype,
                             MPI_Comm comm)
{
  return VT()->MPI_Neighbor_alltoallv_c(sendbuf, sendcounts, sdispls, sendtype,
                                        recvbuf, recvcounts, rdispls, recvtype,
                                        comm);
}

int MPI_Neighbor_alltoallv_init(const void *sendbuf, const int sendcounts[],
                                const int sdispls[], MPI_Datatype sendtype,
                                void *recvbuf, const int recvcounts[],
                                const int rdispls[], MPI_Datatype recvtype,
                                MPI_Comm comm, MPI_Info info,
                                MPI_Request *request)
{
  return VT()->MPI_Neighbor_alltoallv_init(sendbuf, sendcounts, sdispls,
                                           sendtype, recvbuf, recvcounts,
                                           rdispls, recvtype, comm, info,
                                           request);
}

int MPI_Neighbor_alltoallv_init_c(const void *sendbuf,
                                  const MPI_Count sendcounts[],
                                  const MPI_Aint sdispls[],
                                  MPI_Datatype sendtype, void *recvbuf,
                                  const MPI_Count recvcounts[],
                                  const MPI_Aint rdispls[],
                                  MPI_Datatype recvtype, MPI_Comm comm,
                                  MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Neighbor_alltoallv_init_c(sendbuf, sendcounts, sdispls,
                                             sendtype, recvbuf, recvcounts,
                                             rdispls, recvtype, comm, info,
                                             request);
}

int MPI_Neighbor_alltoallw(const void *sendbuf, const int sendcounts[],
                           const MPI_Aint sdispls[],
                           const MPI_Datatype sendtypes[], void *recvbuf,
                           const int recvcounts[], const MPI_Aint rdispls[],
                           const MPI_Datatype recvtypes[], MPI_Comm comm)
{
  return VT()->MPI_Neighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                      recvbuf, recvcounts, rdispls, recvtypes,
                                      comm);
}

int MPI_Neighbor_alltoallw_c(const void *sendbuf, const MPI_Count sendcounts[],
                             const MPI_Aint sdispls[],
                             const MPI_Datatype sendtypes[], void *recvbuf,
                             const MPI_Count recvcounts[],
                             const MPI_Aint rdispls[],
                             const MPI_Datatype recvtypes[], MPI_Comm comm)
{
  return VT()->MPI_Neighbor_alltoallw_c(sendbuf, sendcounts, sdispls,
                                        sendtypes, recvbuf, recvcounts,
                                        rdispls, recvtypes, comm);
}

int MPI_Neighbor_alltoallw_init(const void *sendbuf, const int sendcounts[],
                                const MPI_Aint sdispls[],
                                const MPI_Datatype sendtypes[], void *recvbuf,
                                const int recvcounts[],
                                const MPI_Aint rdispls[],
                                const MPI_Datatype recvtypes[], MPI_Comm comm,
                                MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Neighbor_alltoallw_init(sendbuf, sendcounts, sdispls,
                                           sendtypes, recvbuf, recvcounts,
                                           rdispls, recvtypes, comm, info,
                                           request);
}

int MPI_Neighbor_alltoallw_init_c(const void *sendbuf,
                                  const MPI_Count sendcounts[],
                                  const MPI_Aint sdispls[],
                                  const MPI_Datatype sendtypes[],
                                  void *recvbuf, const MPI_Count recvcounts[],
                                  const MPI_Aint rdispls[],
                                  const MPI_Datatype recvtypes[],
                                  MPI_Comm comm, MPI_Info info,
                                  MPI_Request *request)
{
  return VT()->MPI_Neighbor_alltoallw_init_c(sendbuf, sendcounts, sdispls,
                                             sendtypes, recvbuf, recvcounts,
                                             rdispls, recvtypes, comm, info,
                                             request);
}

int MPI_Op_commutative(MPI_Op op, int *commute)
{
  return VT()->MPI_Op_commutative(op, commute);
}

int MPI_Op_create(MPI_User_function *user_fn, int commute, MPI_Op *op)
{
  return VT()->MPI_Op_create(user_fn, commute, op);
}

int MPI_Op_create_c(MPI_User_function_c *user_fn, int commute, MPI_Op *op)
{
  return VT()->MPI_Op_create_c(user_fn, commute, op);
}

int MPI_Op_free(MPI_Op *op) { return VT()->MPI_Op_free(op); }

int MPI_Open_port(MPI_Info info, char *port_name)
{
  return VT()->MPI_Open_port(info, port_name);
}

int MPI_Pack(const void *inbuf, int incount, MPI_Datatype datatype,
             void *outbuf, int outsize, int *position, MPI_Comm comm)
{
  return VT()->MPI_Pack(inbuf, incount, datatype, outbuf, outsize, position,
                        comm);
}

int MPI_Pack_c(const void *inbuf, MPI_Count incount, MPI_Datatype datatype,
               void *outbuf, MPI_Count outsize, MPI_Count *position,
               MPI_Comm comm)
{
  return VT()->MPI_Pack_c(inbuf, incount, datatype, outbuf, outsize, position,
                          comm);
}

int MPI_Pack_external(const char *datarep, const void *inbuf, int incount,
                      MPI_Datatype datatype, void *outbuf, MPI_Aint outsize,
                      MPI_Aint *position)
{
  return VT()->MPI_Pack_external(datarep, inbuf, incount, datatype, outbuf,
                                 outsize, position);
}

int MPI_Pack_external_c(const char *datarep, const void *inbuf,
                        MPI_Count incount, MPI_Datatype datatype, void *outbuf,
                        MPI_Count outsize, MPI_Count *position)
{
  return VT()->MPI_Pack_external_c(datarep, inbuf, incount, datatype, outbuf,
                                   outsize, position);
}

int MPI_Pack_external_size(const char *datarep, int incount,
                           MPI_Datatype datatype, MPI_Aint *size)
{
  return VT()->MPI_Pack_external_size(datarep, incount, datatype, size);
}

int MPI_Pack_external_size_c(const char *datarep, MPI_Count incount,
                             MPI_Datatype datatype, MPI_Count *size)
{
  return VT()->MPI_Pack_external_size_c(datarep, incount, datatype, size);
}

int MPI_Pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, int *size)
{
  return VT()->MPI_Pack_size(incount, datatype, comm, size);
}

int MPI_Pack_size_c(MPI_Count incount, MPI_Datatype datatype, MPI_Comm comm,
                    MPI_Count *size)
{
  return VT()->MPI_Pack_size_c(incount, datatype, comm, size);
}

int MPI_Parrived(MPI_Request request, int partition, int *flag)
{
  return VT()->MPI_Parrived(request, partition, flag);
}

/* The extra arguments stop here: C cannot forward
 * `...`, and MPI-5.0 14.2 lets a profiling layer
 * ignore them.
 */
int MPI_Pcontrol(const int level, ...) { return VT()->MPI_Pcontrol(level); }

int MPI_Pready(int partition, MPI_Request request)
{
  return VT()->MPI_Pready(partition, request);
}

int MPI_Pready_list(int length, const int array_of_partitions[],
                    MPI_Request request)
{
  return VT()->MPI_Pready_list(length, array_of_partitions, request);
}

int MPI_Pready_range(int partition_low, int partition_high,
                     MPI_Request request)
{
  return VT()->MPI_Pready_range(partition_low, partition_high, request);
}

int MPI_Precv_init(void *buf, int partitions, MPI_Count count,
                   MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                   MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Precv_init(buf, partitions, count, datatype, dest, tag,
                              comm, info, request);
}

int MPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status)
{
  return VT()->MPI_Probe(source, tag, comm, status);
}

int MPI_Psend_init(const void *buf, int partitions, MPI_Count count,
                   MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                   MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Psend_init(buf, partitions, count, datatype, dest, tag,
                              comm, info, request);
}

int MPI_Publish_name(const char *service_name, MPI_Info info,
                     const char *port_name)
{
  return VT()->MPI_Publish_name(service_name, info, port_name);
}

int MPI_Put(const void *origin_addr, int origin_count,
            MPI_Datatype origin_datatype, int target_rank,
            MPI_Aint target_disp, int target_count,
            MPI_Datatype target_datatype, MPI_Win win)
{
  return VT()->MPI_Put(origin_addr, origin_count, origin_datatype, target_rank,
                       target_disp, target_count, target_datatype, win);
}

int MPI_Put_c(const void *origin_addr, MPI_Count origin_count,
              MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, MPI_Count target_count,
              MPI_Datatype target_datatype, MPI_Win win)
{
  return VT()->MPI_Put_c(origin_addr, origin_count, origin_datatype,
                         target_rank, target_disp, target_count,
                         target_datatype, win);
}

int MPI_Query_thread(int *provided)
{
  return VT()->MPI_Query_thread(provided);
}

int MPI_Raccumulate(const void *origin_addr, int origin_count,
                    MPI_Datatype origin_datatype, int target_rank,
                    MPI_Aint target_disp, int target_count,
                    MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
                    MPI_Request *request)
{
  return VT()->MPI_Raccumulate(origin_addr, origin_count, origin_datatype,
                               target_rank, target_disp, target_count,
                               target_datatype, op, win, request);
}

int MPI_Raccumulate_c(const void *origin_addr, MPI_Count origin_count,
                      MPI_Datatype origin_datatype, int target_rank,
                      MPI_Aint target_disp, MPI_Count target_count,
                      MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
                      MPI_Request *request)
{
  return VT()->MPI_Raccumulate_c(origin_addr, origin_count, origin_datatype,
                                 target_rank, target_disp, target_count,
                                 target_datatype, op, win, request);
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
             MPI_Comm comm, MPI_Status *status)
{
  return VT()->MPI_Recv(buf, count, datatype, source, tag, comm, status);
}

int MPI_Recv_c(void *buf, MPI_Count count, MPI_Datatype datatype, int source,
               int tag, MPI_Comm comm, MPI_Status *status)
{
  return VT()->MPI_Recv_c(buf, count, datatype, source, tag, comm, status);
}

int MPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int source,
                  int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Recv_init(buf, count, datatype, source, tag, comm, request);
}

int MPI_Recv_init_c(void *buf, MPI_Count count, MPI_Datatype datatype,
                    int source, int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Recv_init_c(buf, count, datatype, source, tag, comm,
                               request);
}

int MPI_Reduce(const void *sendbuf, void *recvbuf, int count,
               MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
  return VT()->MPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
}

int MPI_Reduce_c(const void *sendbuf, void *recvbuf, MPI_Count count,
                 MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
  return VT()->MPI_Reduce_c(sendbuf, recvbuf, count, datatype, op, root, comm);
}

int MPI_Reduce_init(const void *sendbuf, void *recvbuf, int count,
                    MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm,
                    MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Reduce_init(sendbuf, recvbuf, count, datatype, op, root,
                               comm, info, request);
}

int MPI_Reduce_init_c(const void *sendbuf, void *recvbuf, MPI_Count count,
                      MPI_Datatype datatype, MPI_Op op, int root,
                      MPI_Comm comm, MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Reduce_init_c(sendbuf, recvbuf, count, datatype, op, root,
                                 comm, info, request);
}

int MPI_Reduce_local(const void *inbuf, void *inoutbuf, int count,
                     MPI_Datatype datatype, MPI_Op op)
{
  return VT()->MPI_Reduce_local(inbuf, inoutbuf, count, datatype, op);
}

int MPI_Reduce_local_c(const void *inbuf, void *inoutbuf, MPI_Count count,
                       MPI_Datatype datatype, MPI_Op op)
{
  return VT()->MPI_Reduce_local_c(inbuf, inoutbuf, count, datatype, op);
}

int MPI_Reduce_scatter(const void *sendbuf, void *recvbuf,
                       const int recvcounts[], MPI_Datatype datatype,
                       MPI_Op op, MPI_Comm comm)
{
  return VT()->MPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op,
                                  comm);
}

int MPI_Reduce_scatter_c(const void *sendbuf, void *recvbuf,
                         const MPI_Count recvcounts[], MPI_Datatype datatype,
                         MPI_Op op, MPI_Comm comm)
{
  return VT()->MPI_Reduce_scatter_c(sendbuf, recvbuf, recvcounts, datatype, op,
                                    comm);
}

int MPI_Reduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount,
                             MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return VT()->MPI_Reduce_scatter_block(sendbuf, recvbuf, recvcount, datatype,
                                        op, comm);
}

int MPI_Reduce_scatter_block_c(const void *sendbuf, void *recvbuf,
                               MPI_Count recvcount, MPI_Datatype datatype,
                               MPI_Op op, MPI_Comm comm)
{
  return VT()->MPI_Reduce_scatter_block_c(sendbuf, recvbuf, recvcount,
                                          datatype, op, comm);
}

int MPI_Reduce_scatter_block_init(const void *sendbuf, void *recvbuf,
                                  int recvcount, MPI_Datatype datatype,
                                  MPI_Op op, MPI_Comm comm, MPI_Info info,
                                  MPI_Request *request)
{
  return VT()->MPI_Reduce_scatter_block_init(sendbuf, recvbuf, recvcount,
                                             datatype, op, comm, info,
                                             request);
}

int MPI_Reduce_scatter_block_init_c(const void *sendbuf, void *recvbuf,
                                    MPI_Count recvcount, MPI_Datatype datatype,
                                    MPI_Op op, MPI_Comm comm, MPI_Info info,
                                    MPI_Request *request)
{
  return VT()->MPI_Reduce_scatter_block_init_c(sendbuf, recvbuf, recvcount,
                                               datatype, op, comm, info,
                                               request);
}

int MPI_Reduce_scatter_init(const void *sendbuf, void *recvbuf,
                            const int recvcounts[], MPI_Datatype datatype,
                            MPI_Op op, MPI_Comm comm, MPI_Info info,
                            MPI_Request *request)
{
  return VT()->MPI_Reduce_scatter_init(sendbuf, recvbuf, recvcounts, datatype,
                                       op, comm, info, request);
}

int MPI_Reduce_scatter_init_c(const void *sendbuf, void *recvbuf,
                              const MPI_Count recvcounts[],
                              MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                              MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Reduce_scatter_init_c(sendbuf, recvbuf, recvcounts,
                                         datatype, op, comm, info, request);
}

int MPI_Register_datarep(const char *datarep,
                         MPI_Datarep_conversion_function *read_conversion_fn,
                         MPI_Datarep_conversion_function *write_conversion_fn,
                         MPI_Datarep_extent_function *dtype_file_extent_fn,
                         void *extra_state)
{
  return VT()->MPI_Register_datarep(datarep, read_conversion_fn,
                                    write_conversion_fn, dtype_file_extent_fn,
                                    extra_state);
}

int MPI_Register_datarep_c(const char *datarep,
    MPI_Datarep_conversion_function_c *read_conversion_fn,
    MPI_Datarep_conversion_function_c *write_conversion_fn,
    MPI_Datarep_extent_function *dtype_file_extent_fn, void *extra_state)
{
  return VT()->MPI_Register_datarep_c(datarep, read_conversion_fn,
                                      write_conversion_fn,
                                      dtype_file_extent_fn, extra_state);
}

int MPI_Remove_error_class(int errorclass)
{
  return VT()->MPI_Remove_error_class(errorclass);
}

int MPI_Remove_error_code(int errorcode)
{
  return VT()->MPI_Remove_error_code(errorcode);
}

int MPI_Remove_error_string(int errorcode)
{
  return VT()->MPI_Remove_error_string(errorcode);
}

int MPI_Request_free(MPI_Request *request)
{
  return VT()->MPI_Request_free(request);
}

int MPI_Request_get_status(MPI_Request request, int *flag, MPI_Status *status)
{
  return VT()->MPI_Request_get_status(request, flag, status);
}

int MPI_Request_get_status_all(int count,
                               const MPI_Request array_of_requests[],
                               int *flag, MPI_Status *array_of_statuses)
{
  return VT()->MPI_Request_get_status_all(count, array_of_requests, flag,
                                          array_of_statuses);
}

int MPI_Request_get_status_any(int count,
                               const MPI_Request array_of_requests[],
                               int *indx, int *flag, MPI_Status *status)
{
  return VT()->MPI_Request_get_status_any(count, array_of_requests, indx, flag,
                                          status);
}

int MPI_Request_get_status_some(int incount,
                                const MPI_Request array_of_requests[],
                                int *outcount, int array_of_indices[],
                                MPI_Status *array_of_statuses)
{
  return VT()->MPI_Request_get_status_some(incount, array_of_requests,
                                           outcount, array_of_indices,
                                           array_of_statuses);
}

int MPI_Rget(void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
             int target_rank, MPI_Aint target_disp, int target_count,
             MPI_Datatype target_datatype, MPI_Win win, MPI_Request *request)
{
  return VT()->MPI_Rget(origin_addr, origin_count, origin_datatype,
                        target_rank, target_disp, target_count,
                        target_datatype, win, request);
}

int MPI_Rget_c(void *origin_addr, MPI_Count origin_count,
               MPI_Datatype origin_datatype, int target_rank,
               MPI_Aint target_disp, MPI_Count target_count,
               MPI_Datatype target_datatype, MPI_Win win, MPI_Request *request)
{
  return VT()->MPI_Rget_c(origin_addr, origin_count, origin_datatype,
                          target_rank, target_disp, target_count,
                          target_datatype, win, request);
}

int MPI_Rget_accumulate(const void *origin_addr, int origin_count,
                        MPI_Datatype origin_datatype, void *result_addr,
                        int result_count, MPI_Datatype result_datatype,
                        int target_rank, MPI_Aint target_disp,
                        int target_count, MPI_Datatype target_datatype,
                        MPI_Op op, MPI_Win win, MPI_Request *request)
{
  return VT()->MPI_Rget_accumulate(origin_addr, origin_count, origin_datatype,
                                   result_addr, result_count, result_datatype,
                                   target_rank, target_disp, target_count,
                                   target_datatype, op, win, request);
}

int MPI_Rget_accumulate_c(const void *origin_addr, MPI_Count origin_count,
                          MPI_Datatype origin_datatype, void *result_addr,
                          MPI_Count result_count, MPI_Datatype result_datatype,
                          int target_rank, MPI_Aint target_disp,
                          MPI_Count target_count, MPI_Datatype target_datatype,
                          MPI_Op op, MPI_Win win, MPI_Request *request)
{
  return VT()->MPI_Rget_accumulate_c(origin_addr, origin_count,
                                     origin_datatype, result_addr,
                                     result_count, result_datatype,
                                     target_rank, target_disp, target_count,
                                     target_datatype, op, win, request);
}

int MPI_Rput(const void *origin_addr, int origin_count,
             MPI_Datatype origin_datatype, int target_rank,
             MPI_Aint target_disp, int target_count,
             MPI_Datatype target_datatype, MPI_Win win, MPI_Request *request)
{
  return VT()->MPI_Rput(origin_addr, origin_count, origin_datatype,
                        target_rank, target_disp, target_count,
                        target_datatype, win, request);
}

int MPI_Rput_c(const void *origin_addr, MPI_Count origin_count,
               MPI_Datatype origin_datatype, int target_rank,
               MPI_Aint target_disp, MPI_Count target_count,
               MPI_Datatype target_datatype, MPI_Win win, MPI_Request *request)
{
  return VT()->MPI_Rput_c(origin_addr, origin_count, origin_datatype,
                          target_rank, target_disp, target_count,
                          target_datatype, win, request);
}

int MPI_Rsend(const void *buf, int count, MPI_Datatype datatype, int dest,
              int tag, MPI_Comm comm)
{
  return VT()->MPI_Rsend(buf, count, datatype, dest, tag, comm);
}

int MPI_Rsend_c(const void *buf, MPI_Count count, MPI_Datatype datatype,
                int dest, int tag, MPI_Comm comm)
{
  return VT()->MPI_Rsend_c(buf, count, datatype, dest, tag, comm);
}

int MPI_Rsend_init(const void *buf, int count, MPI_Datatype datatype, int dest,
                   int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Rsend_init(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Rsend_init_c(const void *buf, MPI_Count count, MPI_Datatype datatype,
                     int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Rsend_init_c(buf, count, datatype, dest, tag, comm,
                                request);
}

int MPI_Scan(const void *sendbuf, void *recvbuf, int count,
             MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return VT()->MPI_Scan(sendbuf, recvbuf, count, datatype, op, comm);
}

int MPI_Scan_c(const void *sendbuf, void *recvbuf, MPI_Count count,
               MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return VT()->MPI_Scan_c(sendbuf, recvbuf, count, datatype, op, comm);
}

int MPI_Scan_init(const void *sendbuf, void *recvbuf, int count,
                  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                  MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Scan_init(sendbuf, recvbuf, count, datatype, op, comm, info,
                             request);
}

int MPI_Scan_init_c(const void *sendbuf, void *recvbuf, MPI_Count count,
                    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                    MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Scan_init_c(sendbuf, recvbuf, count, datatype, op, comm,
                               info, request);
}

int MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                MPI_Comm comm)
{
  return VT()->MPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                           recvtype, root, comm);
}

int MPI_Scatter_c(const void *sendbuf, MPI_Count sendcount,
                  MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount,
                  MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  return VT()->MPI_Scatter_c(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                             recvtype, root, comm);
}

int MPI_Scatter_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                     void *recvbuf, int recvcount, MPI_Datatype recvtype,
                     int root, MPI_Comm comm, MPI_Info info,
                     MPI_Request *request)
{
  return VT()->MPI_Scatter_init(sendbuf, sendcount, sendtype, recvbuf,
                                recvcount, recvtype, root, comm, info,
                                request);
}

int MPI_Scatter_init_c(const void *sendbuf, MPI_Count sendcount,
                       MPI_Datatype sendtype, void *recvbuf,
                       MPI_Count recvcount, MPI_Datatype recvtype, int root,
                       MPI_Comm comm, MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Scatter_init_c(sendbuf, sendcount, sendtype, recvbuf,
                                  recvcount, recvtype, root, comm, info,
                                  request);
}

int MPI_Scatterv(const void *sendbuf, const int sendcounts[],
                 const int displs[], MPI_Datatype sendtype, void *recvbuf,
                 int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  return VT()->MPI_Scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                            recvcount, recvtype, root, comm);
}

int MPI_Scatterv_c(const void *sendbuf, const MPI_Count sendcounts[],
                   const MPI_Aint displs[], MPI_Datatype sendtype,
                   void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype,
                   int root, MPI_Comm comm)
{
  return VT()->MPI_Scatterv_c(sendbuf, sendcounts, displs, sendtype, recvbuf,
                              recvcount, recvtype, root, comm);
}

int MPI_Scatterv_init(const void *sendbuf, const int sendcounts[],
                      const int displs[], MPI_Datatype sendtype, void *recvbuf,
                      int recvcount, MPI_Datatype recvtype, int root,
                      MPI_Comm comm, MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Scatterv_init(sendbuf, sendcounts, displs, sendtype,
                                 recvbuf, recvcount, recvtype, root, comm,
                                 info, request);
}

int MPI_Scatterv_init_c(const void *sendbuf, const MPI_Count sendcounts[],
                        const MPI_Aint displs[], MPI_Datatype sendtype,
                        void *recvbuf, MPI_Count recvcount,
                        MPI_Datatype recvtype, int root, MPI_Comm comm,
                        MPI_Info info, MPI_Request *request)
{
  return VT()->MPI_Scatterv_init_c(sendbuf, sendcounts, displs, sendtype,
                                   recvbuf, recvcount, recvtype, root, comm,
                                   info, request);
}

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest,
             int tag, MPI_Comm comm)
{
  return VT()->MPI_Send(buf, count, datatype, dest, tag, comm);
}

int MPI_Send_c(const void *buf, MPI_Count count, MPI_Datatype datatype,
               int dest, int tag, MPI_Comm comm)
{
  return VT()->MPI_Send_c(buf, count, datatype, dest, tag, comm);
}

int MPI_Send_init(const void *buf, int count, MPI_Datatype datatype, int dest,
                  int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Send_init(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Send_init_c(const void *buf, MPI_Count count, MPI_Datatype datatype,
                    int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Send_init_c(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 int dest, int sendtag, void *recvbuf, int recvcount,
                 MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm,
                 MPI_Status *status)
{
  return VT()->MPI_Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag,
                            recvbuf, recvcount, recvtype, source, recvtag,
                            comm, status);
}

int MPI_Sendrecv_c(const void *sendbuf, MPI_Count sendcount,
                   MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf,
                   MPI_Count recvcount, MPI_Datatype recvtype, int source,
                   int recvtag, MPI_Comm comm, MPI_Status *status)
{
  return VT()->MPI_Sendrecv_c(sendbuf, sendcount, sendtype, dest, sendtag,
                              recvbuf, recvcount, recvtype, source, recvtag,
                              comm, status);
}

int MPI_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype, int dest,
                         int sendtag, int source, int recvtag, MPI_Comm comm,
                         MPI_Status *status)
{
  return VT()->MPI_Sendrecv_replace(buf, count, datatype, dest, sendtag,
                                    source, recvtag, comm, status);
}

int MPI_Sendrecv_replace_c(void *buf, MPI_Count count, MPI_Datatype datatype,
                           int dest, int sendtag, int source, int recvtag,
                           MPI_Comm comm, MPI_Status *status)
{
  return VT()->MPI_Sendrecv_replace_c(buf, count, datatype, dest, sendtag,
                                      source, recvtag, comm, status);
}

int MPI_Session_attach_buffer(MPI_Session session, void *buffer, int size)
{
  return VT()->MPI_Session_attach_buffer(session, buffer, size);
}

int MPI_Session_attach_buffer_c(MPI_Session session, void *buffer,
                                MPI_Count size)
{
  return VT()->MPI_Session_attach_buffer_c(session, buffer, size);
}

int MPI_Session_call_errhandler(MPI_Session session, int errorcode)
{
  return VT()->MPI_Session_call_errhandler(session, errorcode);
}

int MPI_Session_create_errhandler(
    MPI_Session_errhandler_function *session_errhandler_fn,
    MPI_Errhandler *errhandler)
{
  return VT()->MPI_Session_create_errhandler(session_errhandler_fn,
                                             errhandler);
}

int MPI_Session_detach_buffer(MPI_Session session, void *buffer_addr,
                              int *size)
{
  return VT()->MPI_Session_detach_buffer(session, buffer_addr, size);
}

int MPI_Session_detach_buffer_c(MPI_Session session, void *buffer_addr,
                                MPI_Count *size)
{
  return VT()->MPI_Session_detach_buffer_c(session, buffer_addr, size);
}

int MPI_Session_finalize(MPI_Session *session)
{
  return VT()->MPI_Session_finalize(session);
}

int MPI_Session_flush_buffer(MPI_Session session)
{
  return VT()->MPI_Session_flush_buffer(session);
}

int MPI_Session_get_errhandler(MPI_Session session, MPI_Errhandler *errhandler)
{
  return VT()->MPI_Session_get_errhandler(session, errhandler);
}

int MPI_Session_get_info(MPI_Session session, MPI_Info *info_used)
{
  return VT()->MPI_Session_get_info(session, info_used);
}

int MPI_Session_get_nth_pset(MPI_Session session, MPI_Info info, int n,
                             int *pset_len, char *pset_name)
{
  return VT()->MPI_Session_get_nth_pset(session, info, n, pset_len, pset_name);
}

int MPI_Session_get_num_psets(MPI_Session session, MPI_Info info,
                              int *npset_names)
{
  return VT()->MPI_Session_get_num_psets(session, info, npset_names);
}

int MPI_Session_get_pset_info(MPI_Session session, const char *pset_name,
                              MPI_Info *info)
{
  return VT()->MPI_Session_get_pset_info(session, pset_name, info);
}

int MPI_Session_iflush_buffer(MPI_Session session, MPI_Request *request)
{
  return VT()->MPI_Session_iflush_buffer(session, request);
}

int MPI_Session_init(MPI_Info info, MPI_Errhandler errhandler,
                     MPI_Session *session)
{
  return VT()->MPI_Session_init(info, errhandler, session);
}

int MPI_Session_set_errhandler(MPI_Session session, MPI_Errhandler errhandler)
{
  return VT()->MPI_Session_set_errhandler(session, errhandler);
}

int MPI_Ssend(const void *buf, int count, MPI_Datatype datatype, int dest,
              int tag, MPI_Comm comm)
{
  return VT()->MPI_Ssend(buf, count, datatype, dest, tag, comm);
}

int MPI_Ssend_c(const void *buf, MPI_Count count, MPI_Datatype datatype,
                int dest, int tag, MPI_Comm comm)
{
  return VT()->MPI_Ssend_c(buf, count, datatype, dest, tag, comm);
}

int MPI_Ssend_init(const void *buf, int count, MPI_Datatype datatype, int dest,
                   int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Ssend_init(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Ssend_init_c(const void *buf, MPI_Count count, MPI_Datatype datatype,
                     int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->MPI_Ssend_init_c(buf, count, datatype, dest, tag, comm,
                                request);
}

int MPI_Start(MPI_Request *request) { return VT()->MPI_Start(request); }

int MPI_Startall(int count, MPI_Request array_of_requests[])
{
  return VT()->MPI_Startall(count, array_of_requests);
}

int MPI_Status_get_error(const MPI_Status *status, int *error)
{
  return VT()->MPI_Status_get_error(status, error);
}

int MPI_Status_get_source(const MPI_Status *status, int *source)
{
  return VT()->MPI_Status_get_source(status, source);
}

int MPI_Status_get_tag(const MPI_Status *status, int *tag)
{
  return VT()->MPI_Status_get_tag(status, tag);
}

int MPI_Status_set_cancelled(MPI_Status *status, int flag)
{
  return VT()->MPI_Status_set_cancelled(status, flag);
}

int MPI_Status_set_elements(MPI_Status *status, MPI_Datatype datatype,
                            int count)
{
  return VT()->MPI_Status_set_elements(status, datatype, count);
}

int MPI_Status_set_elements_c(MPI_Status *status, MPI_Datatype datatype,
                              MPI_Count count)
{
  return VT()->MPI_Status_set_elements_c(status, datatype, count);
}

int MPI_Status_set_elements_x(MPI_Status *status, MPI_Datatype datatype,
                              MPI_Count count)
{
  return VT()->MPI_Status_set_elements_x(status, datatype, count);
}

int MPI_Status_set_error(MPI_Status *status, int error)
{
  return VT()->MPI_Status_set_error(status, error);
}

int MPI_Status_set_source(MPI_Status *status, int source)
{
  return VT()->MPI_Status_set_source(status, source);
}

int MPI_Status_set_tag(MPI_Status *status, int tag)
{
  return VT()->MPI_Status_set_tag(status, tag);
}

int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status)
{
  return VT()->MPI_Test(request, flag, status);
}

int MPI_Test_cancelled(const MPI_Status *status, int *flag)
{
  return VT()->MPI_Test_cancelled(status, flag);
}

int MPI_Testall(int count, MPI_Request array_of_requests[], int *flag,
                MPI_Status *array_of_statuses)
{
  return VT()->MPI_Testall(count, array_of_requests, flag, array_of_statuses);
}

int MPI_Testany(int count, MPI_Request array_of_requests[], int *indx,
                int *flag, MPI_Status *status)
{
  return VT()->MPI_Testany(count, array_of_requests, indx, flag, status);
}

int MPI_Testsome(int incount, MPI_Request array_of_requests[], int *outcount,
                 int array_of_indices[], MPI_Status *array_of_statuses)
{
  return VT()->MPI_Testsome(incount, array_of_requests, outcount,
                            array_of_indices, array_of_statuses);
}

int MPI_Topo_test(MPI_Comm comm, int *status)
{
  return VT()->MPI_Topo_test(comm, status);
}

int MPI_Type_commit(MPI_Datatype *datatype)
{
  return VT()->MPI_Type_commit(datatype);
}

int MPI_Type_contiguous(int count, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  return VT()->MPI_Type_contiguous(count, oldtype, newtype);
}

int MPI_Type_contiguous_c(MPI_Count count, MPI_Datatype oldtype,
                          MPI_Datatype *newtype)
{
  return VT()->MPI_Type_contiguous_c(count, oldtype, newtype);
}

int MPI_Type_create_darray(int size, int rank, int ndims,
                           const int array_of_gsizes[],
                           const int array_of_distribs[],
                           const int array_of_dargs[],
                           const int array_of_psizes[], int order,
                           MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  return VT()->MPI_Type_create_darray(size, rank, ndims, array_of_gsizes,
                                      array_of_distribs, array_of_dargs,
                                      array_of_psizes, order, oldtype,
                                      newtype);
}

int MPI_Type_create_darray_c(int size, int rank, int ndims,
                             const MPI_Count array_of_gsizes[],
                             const int array_of_distribs[],
                             const int array_of_dargs[],
                             const int array_of_psizes[], int order,
                             MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  return VT()->MPI_Type_create_darray_c(size, rank, ndims, array_of_gsizes,
                                        array_of_distribs, array_of_dargs,
                                        array_of_psizes, order, oldtype,
                                        newtype);
}

int MPI_Type_create_f90_complex(int p, int r, MPI_Datatype *newtype)
{
  return VT()->MPI_Type_create_f90_complex(p, r, newtype);
}

int MPI_Type_create_f90_integer(int r, MPI_Datatype *newtype)
{
  return VT()->MPI_Type_create_f90_integer(r, newtype);
}

int MPI_Type_create_f90_real(int p, int r, MPI_Datatype *newtype)
{
  return VT()->MPI_Type_create_f90_real(p, r, newtype);
}

int MPI_Type_create_hindexed(int count, const int array_of_blocklengths[],
                             const MPI_Aint array_of_displacements[],
                             MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  return VT()->MPI_Type_create_hindexed(count, array_of_blocklengths,
                                        array_of_displacements, oldtype,
                                        newtype);
}

int MPI_Type_create_hindexed_c(MPI_Count count,
                               const MPI_Count array_of_blocklengths[],
                               const MPI_Count array_of_displacements[],
                               MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  return VT()->MPI_Type_create_hindexed_c(count, array_of_blocklengths,
                                          array_of_displacements, oldtype,
                                          newtype);
}

int MPI_Type_create_hindexed_block(int count, int blocklength,
                                   const MPI_Aint array_of_displacements[],
                                   MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  return VT()->MPI_Type_create_hindexed_block(count, blocklength,
                                              array_of_displacements, oldtype,
                                              newtype);
}

int MPI_Type_create_hindexed_block_c(MPI_Count count, MPI_Count blocklength,
                                     const MPI_Count array_of_displacements[],
                                     MPI_Datatype oldtype,
                                     MPI_Datatype *newtype)
{
  return VT()->MPI_Type_create_hindexed_block_c(count, blocklength,
                                                array_of_displacements,
                                                oldtype, newtype);
}

int MPI_Type_create_hvector(int count, int blocklength, MPI_Aint stride,
                            MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  return VT()->MPI_Type_create_hvector(count, blocklength, stride, oldtype,
                                       newtype);
}

int MPI_Type_create_hvector_c(MPI_Count count, MPI_Count blocklength,
                              MPI_Count stride, MPI_Datatype oldtype,
                              MPI_Datatype *newtype)
{
  return VT()->MPI_Type_create_hvector_c(count, blocklength, stride, oldtype,
                                         newtype);
}

int MPI_Type_create_indexed_block(int count, int blocklength,
                                  const int array_of_displacements[],
                                  MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  return VT()->MPI_Type_create_indexed_block(count, blocklength,
                                             array_of_displacements, oldtype,
                                             newtype);
}

int MPI_Type_create_indexed_block_c(MPI_Count count, MPI_Count blocklength,
                                    const MPI_Count array_of_displacements[],
                                    MPI_Datatype oldtype,
                                    MPI_Datatype *newtype)
{
  return VT()->MPI_Type_create_indexed_block_c(count, blocklength,
                                               array_of_displacements, oldtype,
                                               newtype);
}

int MPI_Type_create_keyval(MPI_Type_copy_attr_function *type_copy_attr_fn,
                           MPI_Type_delete_attr_function *type_delete_attr_fn,
                           int *type_keyval, void *extra_state)
{
  return VT()->MPI_Type_create_keyval(type_copy_attr_fn, type_delete_attr_fn,
                                      type_keyval, extra_state);
}

int MPI_Type_create_resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent,
                            MPI_Datatype *newtype)
{
  return VT()->MPI_Type_create_resized(oldtype, lb, extent, newtype);
}

int MPI_Type_create_resized_c(MPI_Datatype oldtype, MPI_Count lb,
                              MPI_Count extent, MPI_Datatype *newtype)
{
  return VT()->MPI_Type_create_resized_c(oldtype, lb, extent, newtype);
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

int MPI_Type_create_subarray(int ndims, const int array_of_sizes[],
                             const int array_of_subsizes[],
                             const int array_of_starts[], int order,
                             MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  return VT()->MPI_Type_create_subarray(ndims, array_of_sizes,
                                        array_of_subsizes, array_of_starts,
                                        order, oldtype, newtype);
}

int MPI_Type_create_subarray_c(int ndims, const MPI_Count array_of_sizes[],
                               const MPI_Count array_of_subsizes[],
                               const MPI_Count array_of_starts[], int order,
                               MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  return VT()->MPI_Type_create_subarray_c(ndims, array_of_sizes,
                                          array_of_subsizes, array_of_starts,
                                          order, oldtype, newtype);
}

int MPI_Type_delete_attr(MPI_Datatype datatype, int type_keyval)
{
  return VT()->MPI_Type_delete_attr(datatype, type_keyval);
}

int MPI_Type_dup(MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  return VT()->MPI_Type_dup(oldtype, newtype);
}

int MPI_Type_free(MPI_Datatype *datatype)
{
  return VT()->MPI_Type_free(datatype);
}

int MPI_Type_free_keyval(int *type_keyval)
{
  return VT()->MPI_Type_free_keyval(type_keyval);
}

int MPI_Type_get_attr(MPI_Datatype datatype, int type_keyval,
                      void *attribute_val, int *flag)
{
  return VT()->MPI_Type_get_attr(datatype, type_keyval, attribute_val, flag);
}

int MPI_Type_get_contents(MPI_Datatype datatype, int max_integers,
                          int max_addresses, int max_datatypes,
                          int array_of_integers[],
                          MPI_Aint array_of_addresses[],
                          MPI_Datatype array_of_datatypes[])
{
  return VT()->MPI_Type_get_contents(datatype, max_integers, max_addresses,
                                     max_datatypes, array_of_integers,
                                     array_of_addresses, array_of_datatypes);
}

int MPI_Type_get_contents_c(MPI_Datatype datatype, MPI_Count max_integers,
                            MPI_Count max_addresses,
                            MPI_Count max_large_counts,
                            MPI_Count max_datatypes, int array_of_integers[],
                            MPI_Aint array_of_addresses[],
                            MPI_Count array_of_large_counts[],
                            MPI_Datatype array_of_datatypes[])
{
  return VT()->MPI_Type_get_contents_c(datatype, max_integers, max_addresses,
                                       max_large_counts, max_datatypes,
                                       array_of_integers, array_of_addresses,
                                       array_of_large_counts,
                                       array_of_datatypes);
}

int MPI_Type_get_envelope(MPI_Datatype datatype, int *num_integers,
                          int *num_addresses, int *num_datatypes,
                          int *combiner)
{
  return VT()->MPI_Type_get_envelope(datatype, num_integers, num_addresses,
                                     num_datatypes, combiner);
}

int MPI_Type_get_envelope_c(MPI_Datatype datatype, MPI_Count *num_integers,
                            MPI_Count *num_addresses,
                            MPI_Count *num_large_counts,
                            MPI_Count *num_datatypes, int *combiner)
{
  return VT()->MPI_Type_get_envelope_c(datatype, num_integers, num_addresses,
                                       num_large_counts, num_datatypes,
                                       combiner);
}

int MPI_Type_get_extent(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent)
{
  return VT()->MPI_Type_get_extent(datatype, lb, extent);
}

int MPI_Type_get_extent_c(MPI_Datatype datatype, MPI_Count *lb,
                          MPI_Count *extent)
{
  return VT()->MPI_Type_get_extent_c(datatype, lb, extent);
}

int MPI_Type_get_extent_x(MPI_Datatype datatype, MPI_Count *lb,
                          MPI_Count *extent)
{
  return VT()->MPI_Type_get_extent_x(datatype, lb, extent);
}

int MPI_Type_get_name(MPI_Datatype datatype, char *type_name, int *resultlen)
{
  return VT()->MPI_Type_get_name(datatype, type_name, resultlen);
}

int MPI_Type_get_true_extent(MPI_Datatype datatype, MPI_Aint *true_lb,
                             MPI_Aint *true_extent)
{
  return VT()->MPI_Type_get_true_extent(datatype, true_lb, true_extent);
}

int MPI_Type_get_true_extent_c(MPI_Datatype datatype, MPI_Count *true_lb,
                               MPI_Count *true_extent)
{
  return VT()->MPI_Type_get_true_extent_c(datatype, true_lb, true_extent);
}

int MPI_Type_get_true_extent_x(MPI_Datatype datatype, MPI_Count *true_lb,
                               MPI_Count *true_extent)
{
  return VT()->MPI_Type_get_true_extent_x(datatype, true_lb, true_extent);
}

int MPI_Type_get_value_index(MPI_Datatype value_type, MPI_Datatype index_type,
                             MPI_Datatype *pair_type)
{
  return VT()->MPI_Type_get_value_index(value_type, index_type, pair_type);
}

int MPI_Type_indexed(int count, const int array_of_blocklengths[],
                     const int array_of_displacements[], MPI_Datatype oldtype,
                     MPI_Datatype *newtype)
{
  return VT()->MPI_Type_indexed(count, array_of_blocklengths,
                                array_of_displacements, oldtype, newtype);
}

int MPI_Type_indexed_c(MPI_Count count,
                       const MPI_Count array_of_blocklengths[],
                       const MPI_Count array_of_displacements[],
                       MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  return VT()->MPI_Type_indexed_c(count, array_of_blocklengths,
                                  array_of_displacements, oldtype, newtype);
}

int MPI_Type_match_size(int typeclass, int size, MPI_Datatype *datatype)
{
  return VT()->MPI_Type_match_size(typeclass, size, datatype);
}

int MPI_Type_set_attr(MPI_Datatype datatype, int type_keyval,
                      void *attribute_val)
{
  return VT()->MPI_Type_set_attr(datatype, type_keyval, attribute_val);
}

int MPI_Type_set_name(MPI_Datatype datatype, const char *type_name)
{
  return VT()->MPI_Type_set_name(datatype, type_name);
}

int MPI_Type_size(MPI_Datatype datatype, int *size)
{
  return VT()->MPI_Type_size(datatype, size);
}

int MPI_Type_size_c(MPI_Datatype datatype, MPI_Count *size)
{
  return VT()->MPI_Type_size_c(datatype, size);
}

int MPI_Type_size_x(MPI_Datatype datatype, MPI_Count *size)
{
  return VT()->MPI_Type_size_x(datatype, size);
}

int MPI_Type_vector(int count, int blocklength, int stride,
                    MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  return VT()->MPI_Type_vector(count, blocklength, stride, oldtype, newtype);
}

int MPI_Type_vector_c(MPI_Count count, MPI_Count blocklength, MPI_Count stride,
                      MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  return VT()->MPI_Type_vector_c(count, blocklength, stride, oldtype, newtype);
}

int MPI_Unpack(const void *inbuf, int insize, int *position, void *outbuf,
               int outcount, MPI_Datatype datatype, MPI_Comm comm)
{
  return VT()->MPI_Unpack(inbuf, insize, position, outbuf, outcount, datatype,
                          comm);
}

int MPI_Unpack_c(const void *inbuf, MPI_Count insize, MPI_Count *position,
                 void *outbuf, MPI_Count outcount, MPI_Datatype datatype,
                 MPI_Comm comm)
{
  return VT()->MPI_Unpack_c(inbuf, insize, position, outbuf, outcount,
                            datatype, comm);
}

int MPI_Unpack_external(const char datarep[], const void *inbuf,
                        MPI_Aint insize, MPI_Aint *position, void *outbuf,
                        int outcount, MPI_Datatype datatype)
{
  return VT()->MPI_Unpack_external(datarep, inbuf, insize, position, outbuf,
                                   outcount, datatype);
}

int MPI_Unpack_external_c(const char datarep[], const void *inbuf,
                          MPI_Count insize, MPI_Count *position, void *outbuf,
                          MPI_Count outcount, MPI_Datatype datatype)
{
  return VT()->MPI_Unpack_external_c(datarep, inbuf, insize, position, outbuf,
                                     outcount, datatype);
}

int MPI_Unpublish_name(const char *service_name, MPI_Info info,
                       const char *port_name)
{
  return VT()->MPI_Unpublish_name(service_name, info, port_name);
}

int MPI_Wait(MPI_Request *request, MPI_Status *status)
{
  return VT()->MPI_Wait(request, status);
}

int MPI_Waitall(int count, MPI_Request array_of_requests[],
                MPI_Status *array_of_statuses)
{
  return VT()->MPI_Waitall(count, array_of_requests, array_of_statuses);
}

int MPI_Waitany(int count, MPI_Request array_of_requests[], int *indx,
                MPI_Status *status)
{
  return VT()->MPI_Waitany(count, array_of_requests, indx, status);
}

int MPI_Waitsome(int incount, MPI_Request array_of_requests[], int *outcount,
                 int array_of_indices[], MPI_Status *array_of_statuses)
{
  return VT()->MPI_Waitsome(incount, array_of_requests, outcount,
                            array_of_indices, array_of_statuses);
}

int MPI_Win_allocate(MPI_Aint size, int disp_unit, MPI_Info info,
                     MPI_Comm comm, void *baseptr, MPI_Win *win)
{
  return VT()->MPI_Win_allocate(size, disp_unit, info, comm, baseptr, win);
}

int MPI_Win_allocate_c(MPI_Aint size, MPI_Aint disp_unit, MPI_Info info,
                       MPI_Comm comm, void *baseptr, MPI_Win *win)
{
  return VT()->MPI_Win_allocate_c(size, disp_unit, info, comm, baseptr, win);
}

int MPI_Win_allocate_shared(MPI_Aint size, int disp_unit, MPI_Info info,
                            MPI_Comm comm, void *baseptr, MPI_Win *win)
{
  return VT()->MPI_Win_allocate_shared(size, disp_unit, info, comm, baseptr,
                                       win);
}

int MPI_Win_allocate_shared_c(MPI_Aint size, MPI_Aint disp_unit, MPI_Info info,
                              MPI_Comm comm, void *baseptr, MPI_Win *win)
{
  return VT()->MPI_Win_allocate_shared_c(size, disp_unit, info, comm, baseptr,
                                         win);
}

int MPI_Win_attach(MPI_Win win, void *base, MPI_Aint size)
{
  return VT()->MPI_Win_attach(win, base, size);
}

int MPI_Win_call_errhandler(MPI_Win win, int errorcode)
{
  return VT()->MPI_Win_call_errhandler(win, errorcode);
}

int MPI_Win_complete(MPI_Win win) { return VT()->MPI_Win_complete(win); }

int MPI_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info,
                   MPI_Comm comm, MPI_Win *win)
{
  return VT()->MPI_Win_create(base, size, disp_unit, info, comm, win);
}

int MPI_Win_create_c(void *base, MPI_Aint size, MPI_Aint disp_unit,
                     MPI_Info info, MPI_Comm comm, MPI_Win *win)
{
  return VT()->MPI_Win_create_c(base, size, disp_unit, info, comm, win);
}

int MPI_Win_create_dynamic(MPI_Info info, MPI_Comm comm, MPI_Win *win)
{
  return VT()->MPI_Win_create_dynamic(info, comm, win);
}

int MPI_Win_create_errhandler(MPI_Win_errhandler_function *win_errhandler_fn,
                              MPI_Errhandler *errhandler)
{
  return VT()->MPI_Win_create_errhandler(win_errhandler_fn, errhandler);
}

int MPI_Win_create_keyval(MPI_Win_copy_attr_function *win_copy_attr_fn,
                          MPI_Win_delete_attr_function *win_delete_attr_fn,
                          int *win_keyval, void *extra_state)
{
  return VT()->MPI_Win_create_keyval(win_copy_attr_fn, win_delete_attr_fn,
                                     win_keyval, extra_state);
}

int MPI_Win_delete_attr(MPI_Win win, int win_keyval)
{
  return VT()->MPI_Win_delete_attr(win, win_keyval);
}

int MPI_Win_detach(MPI_Win win, const void *base)
{
  return VT()->MPI_Win_detach(win, base);
}

int MPI_Win_fence(int assert, MPI_Win win)
{
  return VT()->MPI_Win_fence(assert, win);
}

int MPI_Win_flush(int rank, MPI_Win win)
{
  return VT()->MPI_Win_flush(rank, win);
}

int MPI_Win_flush_all(MPI_Win win) { return VT()->MPI_Win_flush_all(win); }

int MPI_Win_flush_local(int rank, MPI_Win win)
{
  return VT()->MPI_Win_flush_local(rank, win);
}

int MPI_Win_flush_local_all(MPI_Win win)
{
  return VT()->MPI_Win_flush_local_all(win);
}

int MPI_Win_free(MPI_Win *win) { return VT()->MPI_Win_free(win); }

int MPI_Win_free_keyval(int *win_keyval)
{
  return VT()->MPI_Win_free_keyval(win_keyval);
}

int MPI_Win_get_attr(MPI_Win win, int win_keyval, void *attribute_val,
                     int *flag)
{
  return VT()->MPI_Win_get_attr(win, win_keyval, attribute_val, flag);
}

int MPI_Win_get_errhandler(MPI_Win win, MPI_Errhandler *errhandler)
{
  return VT()->MPI_Win_get_errhandler(win, errhandler);
}

int MPI_Win_get_group(MPI_Win win, MPI_Group *group)
{
  return VT()->MPI_Win_get_group(win, group);
}

int MPI_Win_get_info(MPI_Win win, MPI_Info *info_used)
{
  return VT()->MPI_Win_get_info(win, info_used);
}

int MPI_Win_get_name(MPI_Win win, char *win_name, int *resultlen)
{
  return VT()->MPI_Win_get_name(win, win_name, resultlen);
}

int MPI_Win_lock(int lock_type, int rank, int assert, MPI_Win win)
{
  return VT()->MPI_Win_lock(lock_type, rank, assert, win);
}

int MPI_Win_lock_all(int assert, MPI_Win win)
{
  return VT()->MPI_Win_lock_all(assert, win);
}

int MPI_Win_post(MPI_Group group, int assert, MPI_Win win)
{
  return VT()->MPI_Win_post(group, assert, win);
}

int MPI_Win_set_attr(MPI_Win win, int win_keyval, void *attribute_val)
{
  return VT()->MPI_Win_set_attr(win, win_keyval, attribute_val);
}

int MPI_Win_set_errhandler(MPI_Win win, MPI_Errhandler errhandler)
{
  return VT()->MPI_Win_set_errhandler(win, errhandler);
}

int MPI_Win_set_info(MPI_Win win, MPI_Info info)
{
  return VT()->MPI_Win_set_info(win, info);
}

int MPI_Win_set_name(MPI_Win win, const char *win_name)
{
  return VT()->MPI_Win_set_name(win, win_name);
}

int MPI_Win_shared_query(MPI_Win win, int rank, MPI_Aint *size, int *disp_unit,
                         void *baseptr)
{
  return VT()->MPI_Win_shared_query(win, rank, size, disp_unit, baseptr);
}

int MPI_Win_shared_query_c(MPI_Win win, int rank, MPI_Aint *size,
                           MPI_Aint *disp_unit, void *baseptr)
{
  return VT()->MPI_Win_shared_query_c(win, rank, size, disp_unit, baseptr);
}

int MPI_Win_start(MPI_Group group, int assert, MPI_Win win)
{
  return VT()->MPI_Win_start(group, assert, win);
}

int MPI_Win_sync(MPI_Win win) { return VT()->MPI_Win_sync(win); }

int MPI_Win_test(MPI_Win win, int *flag)
{
  return VT()->MPI_Win_test(win, flag);
}

int MPI_Win_unlock(int rank, MPI_Win win)
{
  return VT()->MPI_Win_unlock(rank, win);
}

int MPI_Win_unlock_all(MPI_Win win) { return VT()->MPI_Win_unlock_all(win); }

int MPI_Win_wait(MPI_Win win) { return VT()->MPI_Win_wait(win); }

MPI_Aint MPI_Aint_add(MPI_Aint base, MPI_Aint disp)
{
  return VT()->MPI_Aint_add(base, disp);
}

MPI_Aint MPI_Aint_diff(MPI_Aint addr1, MPI_Aint addr2)
{
  return VT()->MPI_Aint_diff(addr1, addr2);
}

double MPI_Wtick(void) { return VT()->MPI_Wtick(); }

double MPI_Wtime(void) { return VT()->MPI_Wtime(); }

MPI_Comm MPI_Comm_fromint(int comm) { return VT()->MPI_Comm_fromint(comm); }

int MPI_Comm_toint(MPI_Comm comm) { return VT()->MPI_Comm_toint(comm); }

MPI_Errhandler MPI_Errhandler_fromint(int errhandler)
{
  return VT()->MPI_Errhandler_fromint(errhandler);
}

int MPI_Errhandler_toint(MPI_Errhandler errhandler)
{
  return VT()->MPI_Errhandler_toint(errhandler);
}

MPI_File MPI_File_fromint(int file) { return VT()->MPI_File_fromint(file); }

int MPI_File_toint(MPI_File file) { return VT()->MPI_File_toint(file); }

MPI_Group MPI_Group_fromint(int group)
{
  return VT()->MPI_Group_fromint(group);
}

int MPI_Group_toint(MPI_Group group) { return VT()->MPI_Group_toint(group); }

MPI_Info MPI_Info_fromint(int info) { return VT()->MPI_Info_fromint(info); }

int MPI_Info_toint(MPI_Info info) { return VT()->MPI_Info_toint(info); }

MPI_Message MPI_Message_fromint(int message)
{
  return VT()->MPI_Message_fromint(message);
}

int MPI_Message_toint(MPI_Message message)
{
  return VT()->MPI_Message_toint(message);
}

MPI_Op MPI_Op_fromint(int op) { return VT()->MPI_Op_fromint(op); }

int MPI_Op_toint(MPI_Op op) { return VT()->MPI_Op_toint(op); }

MPI_Request MPI_Request_fromint(int request)
{
  return VT()->MPI_Request_fromint(request);
}

int MPI_Request_toint(MPI_Request request)
{
  return VT()->MPI_Request_toint(request);
}

MPI_Session MPI_Session_fromint(int session)
{
  return VT()->MPI_Session_fromint(session);
}

int MPI_Session_toint(MPI_Session session)
{
  return VT()->MPI_Session_toint(session);
}

MPI_Datatype MPI_Type_fromint(int datatype)
{
  return VT()->MPI_Type_fromint(datatype);
}

int MPI_Type_toint(MPI_Datatype datatype)
{
  return VT()->MPI_Type_toint(datatype);
}

MPI_Win MPI_Win_fromint(int win) { return VT()->MPI_Win_fromint(win); }

int MPI_Win_toint(MPI_Win win) { return VT()->MPI_Win_toint(win); }

int MPI_T_category_changed(int *update_number)
{
  return VT()->MPI_T_category_changed(update_number);
}

int MPI_T_category_get_categories(int cat_index, int len, int indices[])
{
  return VT()->MPI_T_category_get_categories(cat_index, len, indices);
}

int MPI_T_category_get_cvars(int cat_index, int len, int indices[])
{
  return VT()->MPI_T_category_get_cvars(cat_index, len, indices);
}

int MPI_T_category_get_events(int cat_index, int len, int indices[])
{
  return VT()->MPI_T_category_get_events(cat_index, len, indices);
}

int MPI_T_category_get_index(const char *name, int *cat_index)
{
  return VT()->MPI_T_category_get_index(name, cat_index);
}

int MPI_T_category_get_info(int cat_index, char *name, int *name_len,
                            char *desc, int *desc_len, int *num_cvars,
                            int *num_pvars, int *num_categories)
{
  return VT()->MPI_T_category_get_info(cat_index, name, name_len, desc,
                                       desc_len, num_cvars, num_pvars,
                                       num_categories);
}

int MPI_T_category_get_num(int *num_cat)
{
  return VT()->MPI_T_category_get_num(num_cat);
}

int MPI_T_category_get_num_events(int cat_index, int *num_events)
{
  return VT()->MPI_T_category_get_num_events(cat_index, num_events);
}

int MPI_T_category_get_pvars(int cat_index, int len, int indices[])
{
  return VT()->MPI_T_category_get_pvars(cat_index, len, indices);
}

int MPI_T_cvar_get_index(const char *name, int *cvar_index)
{
  return VT()->MPI_T_cvar_get_index(name, cvar_index);
}

int MPI_T_cvar_get_info(int cvar_index, char *name, int *name_len,
                        int *verbosity, MPI_Datatype *datatype,
                        MPI_T_enum *enumtype, char *desc, int *desc_len,
                        int *bind, int *scope)
{
  return VT()->MPI_T_cvar_get_info(cvar_index, name, name_len, verbosity,
                                   datatype, enumtype, desc, desc_len, bind,
                                   scope);
}

int MPI_T_cvar_get_num(int *num_cvar)
{
  return VT()->MPI_T_cvar_get_num(num_cvar);
}

int MPI_T_cvar_handle_alloc(int cvar_index, void *obj_handle,
                            MPI_T_cvar_handle *handle, int *count)
{
  return VT()->MPI_T_cvar_handle_alloc(cvar_index, obj_handle, handle, count);
}

int MPI_T_cvar_handle_free(MPI_T_cvar_handle *handle)
{
  return VT()->MPI_T_cvar_handle_free(handle);
}

int MPI_T_cvar_read(MPI_T_cvar_handle handle, void *buf)
{
  return VT()->MPI_T_cvar_read(handle, buf);
}

int MPI_T_cvar_write(MPI_T_cvar_handle handle, const void *buf)
{
  return VT()->MPI_T_cvar_write(handle, buf);
}

int MPI_T_enum_get_info(MPI_T_enum enumtype, int *num, char *name,
                        int *name_len)
{
  return VT()->MPI_T_enum_get_info(enumtype, num, name, name_len);
}

int MPI_T_enum_get_item(MPI_T_enum enumtype, int indx, int *value, char *name,
                        int *name_len)
{
  return VT()->MPI_T_enum_get_item(enumtype, indx, value, name, name_len);
}

int MPI_T_event_callback_get_info(MPI_T_event_registration event_registration,
                                  MPI_T_cb_safety cb_safety,
                                  MPI_Info *info_used)
{
  return VT()->MPI_T_event_callback_get_info(event_registration,
                                             (MPIABI_T_cb_safety)cb_safety,
                                             info_used);
}

int MPI_T_event_callback_set_info(MPI_T_event_registration event_registration,
                                  MPI_T_cb_safety cb_safety, MPI_Info info)
{
  return VT()->MPI_T_event_callback_set_info(event_registration,
                                             (MPIABI_T_cb_safety)cb_safety,
                                             info);
}

int MPI_T_event_copy(MPI_T_event_instance event_instance, void *buffer)
{
  return VT()->MPI_T_event_copy(event_instance, buffer);
}

int MPI_T_event_get_index(const char *name, int *event_index)
{
  return VT()->MPI_T_event_get_index(name, event_index);
}

int MPI_T_event_get_info(int event_index, char *name, int *name_len,
                         int *verbosity, MPI_Datatype array_of_datatypes[],
                         MPI_Aint array_of_displacements[], int *num_elements,
                         MPI_T_enum *enumtype, MPI_Info *info, char *desc,
                         int *desc_len, int *bind)
{
  return VT()->MPI_T_event_get_info(event_index, name, name_len, verbosity,
                                    array_of_datatypes, array_of_displacements,
                                    num_elements, enumtype, info, desc,
                                    desc_len, bind);
}

int MPI_T_event_get_num(int *num_events)
{
  return VT()->MPI_T_event_get_num(num_events);
}

int MPI_T_event_get_source(MPI_T_event_instance event_instance,
                           int *source_index)
{
  return VT()->MPI_T_event_get_source(event_instance, source_index);
}

int MPI_T_event_get_timestamp(MPI_T_event_instance event_instance,
                              MPI_Count *event_timestamp)
{
  return VT()->MPI_T_event_get_timestamp(event_instance, event_timestamp);
}

int MPI_T_event_handle_alloc(int event_index, void *obj_handle, MPI_Info info,
                             MPI_T_event_registration *event_registration)
{
  return VT()->MPI_T_event_handle_alloc(event_index, obj_handle, info,
                                        event_registration);
}

int MPI_T_event_handle_free(MPI_T_event_registration event_registration,
                            void *user_data,
                            MPI_T_event_free_cb_function free_cb_function)
{
  return VT()->MPI_T_event_handle_free(event_registration, user_data,
      (MPIABI_T_event_free_cb_function *)free_cb_function);
}

int MPI_T_event_handle_get_info(MPI_T_event_registration event_registration,
                                MPI_Info *info_used)
{
  return VT()->MPI_T_event_handle_get_info(event_registration, info_used);
}

int MPI_T_event_handle_set_info(MPI_T_event_registration event_registration,
                                MPI_Info info)
{
  return VT()->MPI_T_event_handle_set_info(event_registration, info);
}

int MPI_T_event_read(MPI_T_event_instance event_instance, int element_index,
                     void *buffer)
{
  return VT()->MPI_T_event_read(event_instance, element_index, buffer);
}

int MPI_T_event_register_callback(MPI_T_event_registration event_registration,
                                  MPI_T_cb_safety cb_safety, MPI_Info info,
                                  void *user_data,
                                  MPI_T_event_cb_function event_cb_function)
{
  return VT()->MPI_T_event_register_callback(event_registration,
      (MPIABI_T_cb_safety)cb_safety, info, user_data,
      (MPIABI_T_event_cb_function *)event_cb_function);
}

int MPI_T_event_set_dropped_handler(
    MPI_T_event_registration event_registration,
    MPI_T_event_dropped_cb_function dropped_cb_function)
{
  return VT()->MPI_T_event_set_dropped_handler(event_registration,
      (MPIABI_T_event_dropped_cb_function *)dropped_cb_function);
}

int MPI_T_finalize(void) { return VT()->MPI_T_finalize(); }

int MPI_T_init_thread(int required, int *provided)
{
  return VT()->MPI_T_init_thread(required, provided);
}

int MPI_T_pvar_get_index(const char *name, int var_class, int *pvar_index)
{
  return VT()->MPI_T_pvar_get_index(name, var_class, pvar_index);
}

int MPI_T_pvar_get_info(int pvar_index, char *name, int *name_len,
                        int *verbosity, int *var_class, MPI_Datatype *datatype,
                        MPI_T_enum *enumtype, char *desc, int *desc_len,
                        int *bind, int *readonly, int *continuous, int *atomic)
{
  return VT()->MPI_T_pvar_get_info(pvar_index, name, name_len, verbosity,
                                   var_class, datatype, enumtype, desc,
                                   desc_len, bind, readonly, continuous,
                                   atomic);
}

int MPI_T_pvar_get_num(int *num_pvar)
{
  return VT()->MPI_T_pvar_get_num(num_pvar);
}

int MPI_T_pvar_handle_alloc(MPI_T_pvar_session session, int pvar_index,
                            void *obj_handle, MPI_T_pvar_handle *handle,
                            int *count)
{
  return VT()->MPI_T_pvar_handle_alloc(session, pvar_index, obj_handle, handle,
                                       count);
}

int MPI_T_pvar_handle_free(MPI_T_pvar_session session,
                           MPI_T_pvar_handle *handle)
{
  return VT()->MPI_T_pvar_handle_free(session, handle);
}

int MPI_T_pvar_read(MPI_T_pvar_session session, MPI_T_pvar_handle handle,
                    void *buf)
{
  return VT()->MPI_T_pvar_read(session, handle, buf);
}

int MPI_T_pvar_readreset(MPI_T_pvar_session session, MPI_T_pvar_handle handle,
                         void *buf)
{
  return VT()->MPI_T_pvar_readreset(session, handle, buf);
}

int MPI_T_pvar_reset(MPI_T_pvar_session session, MPI_T_pvar_handle handle)
{
  return VT()->MPI_T_pvar_reset(session, handle);
}

int MPI_T_pvar_session_create(MPI_T_pvar_session *session)
{
  return VT()->MPI_T_pvar_session_create(session);
}

int MPI_T_pvar_session_free(MPI_T_pvar_session *session)
{
  return VT()->MPI_T_pvar_session_free(session);
}

int MPI_T_pvar_start(MPI_T_pvar_session session, MPI_T_pvar_handle handle)
{
  return VT()->MPI_T_pvar_start(session, handle);
}

int MPI_T_pvar_stop(MPI_T_pvar_session session, MPI_T_pvar_handle handle)
{
  return VT()->MPI_T_pvar_stop(session, handle);
}

int MPI_T_pvar_write(MPI_T_pvar_session session, MPI_T_pvar_handle handle,
                     const void *buf)
{
  return VT()->MPI_T_pvar_write(session, handle, buf);
}

int MPI_T_source_get_info(int source_index, char *name, int *name_len,
                          char *desc, int *desc_len,
                          MPI_T_source_order *ordering,
                          MPI_Count *ticks_per_second, MPI_Count *max_ticks,
                          MPI_Info *info)
{
  return VT()->MPI_T_source_get_info(source_index, name, name_len, desc,
                                     desc_len,
                                     (MPIABI_T_source_order *)ordering,
                                     ticks_per_second, max_ticks, info);
}

int MPI_T_source_get_num(int *num_sources)
{
  return VT()->MPI_T_source_get_num(num_sources);
}

int MPI_T_source_get_timestamp(int source_index, MPI_Count *timestamp)
{
  return VT()->MPI_T_source_get_timestamp(source_index, timestamp);
}

int PMPI_Abi_get_fortran_booleans(int logical_size, void *logical_true,
                                  void *logical_false, int *is_set)
{
  return VT()->PMPI_Abi_get_fortran_booleans(logical_size, logical_true,
                                             logical_false, is_set);
}

int PMPI_Abi_get_fortran_info(MPI_Info *info)
{
  return VT()->PMPI_Abi_get_fortran_info(info);
}

int PMPI_Abi_get_info(MPI_Info *info) { return VT()->PMPI_Abi_get_info(info); }

int PMPI_Abi_get_version(int *abi_major, int *abi_minor)
{
  return VT()->PMPI_Abi_get_version(abi_major, abi_minor);
}

int PMPI_Abi_set_fortran_booleans(int logical_size, void *logical_true,
                                  void *logical_false)
{
  return VT()->PMPI_Abi_set_fortran_booleans(logical_size, logical_true,
                                             logical_false);
}

int PMPI_Abi_set_fortran_info(MPI_Info info)
{
  return VT()->PMPI_Abi_set_fortran_info(info);
}

int PMPI_Abort(MPI_Comm comm, int errorcode)
{
  return VT()->PMPI_Abort(comm, errorcode);
}

int PMPI_Accumulate(const void *origin_addr, int origin_count,
                    MPI_Datatype origin_datatype, int target_rank,
                    MPI_Aint target_disp, int target_count,
                    MPI_Datatype target_datatype, MPI_Op op, MPI_Win win)
{
  return VT()->PMPI_Accumulate(origin_addr, origin_count, origin_datatype,
                               target_rank, target_disp, target_count,
                               target_datatype, op, win);
}

int PMPI_Accumulate_c(const void *origin_addr, MPI_Count origin_count,
                      MPI_Datatype origin_datatype, int target_rank,
                      MPI_Aint target_disp, MPI_Count target_count,
                      MPI_Datatype target_datatype, MPI_Op op, MPI_Win win)
{
  return VT()->PMPI_Accumulate_c(origin_addr, origin_count, origin_datatype,
                                 target_rank, target_disp, target_count,
                                 target_datatype, op, win);
}

int PMPI_Add_error_class(int *errorclass)
{
  return VT()->PMPI_Add_error_class(errorclass);
}

int PMPI_Add_error_code(int errorclass, int *errorcode)
{
  return VT()->PMPI_Add_error_code(errorclass, errorcode);
}

int PMPI_Add_error_string(int errorcode, const char *string)
{
  return VT()->PMPI_Add_error_string(errorcode, string);
}

int PMPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                   MPI_Comm comm)
{
  return VT()->PMPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                              recvtype, comm);
}

int PMPI_Allgather_c(const void *sendbuf, MPI_Count sendcount,
                     MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount,
                     MPI_Datatype recvtype, MPI_Comm comm)
{
  return VT()->PMPI_Allgather_c(sendbuf, sendcount, sendtype, recvbuf,
                                recvcount, recvtype, comm);
}

int PMPI_Allgather_init(const void *sendbuf, int sendcount,
                        MPI_Datatype sendtype, void *recvbuf, int recvcount,
                        MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info,
                        MPI_Request *request)
{
  return VT()->PMPI_Allgather_init(sendbuf, sendcount, sendtype, recvbuf,
                                   recvcount, recvtype, comm, info, request);
}

int PMPI_Allgather_init_c(const void *sendbuf, MPI_Count sendcount,
                          MPI_Datatype sendtype, void *recvbuf,
                          MPI_Count recvcount, MPI_Datatype recvtype,
                          MPI_Comm comm, MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Allgather_init_c(sendbuf, sendcount, sendtype, recvbuf,
                                     recvcount, recvtype, comm, info, request);
}

int PMPI_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                    void *recvbuf, const int recvcounts[], const int displs[],
                    MPI_Datatype recvtype, MPI_Comm comm)
{
  return VT()->PMPI_Allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                               recvcounts, displs, recvtype, comm);
}

int PMPI_Allgatherv_c(const void *sendbuf, MPI_Count sendcount,
                      MPI_Datatype sendtype, void *recvbuf,
                      const MPI_Count recvcounts[], const MPI_Aint displs[],
                      MPI_Datatype recvtype, MPI_Comm comm)
{
  return VT()->PMPI_Allgatherv_c(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcounts, displs, recvtype, comm);
}

int PMPI_Allgatherv_init(const void *sendbuf, int sendcount,
                         MPI_Datatype sendtype, void *recvbuf,
                         const int recvcounts[], const int displs[],
                         MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info,
                         MPI_Request *request)
{
  return VT()->PMPI_Allgatherv_init(sendbuf, sendcount, sendtype, recvbuf,
                                    recvcounts, displs, recvtype, comm, info,
                                    request);
}

int PMPI_Allgatherv_init_c(const void *sendbuf, MPI_Count sendcount,
                           MPI_Datatype sendtype, void *recvbuf,
                           const MPI_Count recvcounts[],
                           const MPI_Aint displs[], MPI_Datatype recvtype,
                           MPI_Comm comm, MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Allgatherv_init_c(sendbuf, sendcount, sendtype, recvbuf,
                                      recvcounts, displs, recvtype, comm, info,
                                      request);
}

int PMPI_Alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr)
{
  return VT()->PMPI_Alloc_mem(size, info, baseptr);
}

int PMPI_Allreduce(const void *sendbuf, void *recvbuf, int count,
                   MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return VT()->PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
}

int PMPI_Allreduce_c(const void *sendbuf, void *recvbuf, MPI_Count count,
                     MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return VT()->PMPI_Allreduce_c(sendbuf, recvbuf, count, datatype, op, comm);
}

int PMPI_Allreduce_init(const void *sendbuf, void *recvbuf, int count,
                        MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                        MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Allreduce_init(sendbuf, recvbuf, count, datatype, op, comm,
                                   info, request);
}

int PMPI_Allreduce_init_c(const void *sendbuf, void *recvbuf, MPI_Count count,
                          MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                          MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Allreduce_init_c(sendbuf, recvbuf, count, datatype, op,
                                     comm, info, request);
}

int PMPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  MPI_Comm comm)
{
  return VT()->PMPI_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                             recvtype, comm);
}

int PMPI_Alltoall_c(const void *sendbuf, MPI_Count sendcount,
                    MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount,
                    MPI_Datatype recvtype, MPI_Comm comm)
{
  return VT()->PMPI_Alltoall_c(sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, comm);
}

int PMPI_Alltoall_init(const void *sendbuf, int sendcount,
                       MPI_Datatype sendtype, void *recvbuf, int recvcount,
                       MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info,
                       MPI_Request *request)
{
  return VT()->PMPI_Alltoall_init(sendbuf, sendcount, sendtype, recvbuf,
                                  recvcount, recvtype, comm, info, request);
}

int PMPI_Alltoall_init_c(const void *sendbuf, MPI_Count sendcount,
                         MPI_Datatype sendtype, void *recvbuf,
                         MPI_Count recvcount, MPI_Datatype recvtype,
                         MPI_Comm comm, MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Alltoall_init_c(sendbuf, sendcount, sendtype, recvbuf,
                                    recvcount, recvtype, comm, info, request);
}

int PMPI_Alltoallv(const void *sendbuf, const int sendcounts[],
                   const int sdispls[], MPI_Datatype sendtype, void *recvbuf,
                   const int recvcounts[], const int rdispls[],
                   MPI_Datatype recvtype, MPI_Comm comm)
{
  return VT()->PMPI_Alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                              recvcounts, rdispls, recvtype, comm);
}

int PMPI_Alltoallv_c(const void *sendbuf, const MPI_Count sendcounts[],
                     const MPI_Aint sdispls[], MPI_Datatype sendtype,
                     void *recvbuf, const MPI_Count recvcounts[],
                     const MPI_Aint rdispls[], MPI_Datatype recvtype,
                     MPI_Comm comm)
{
  return VT()->PMPI_Alltoallv_c(sendbuf, sendcounts, sdispls, sendtype,
                                recvbuf, recvcounts, rdispls, recvtype, comm);
}

int PMPI_Alltoallv_init(const void *sendbuf, const int sendcounts[],
                        const int sdispls[], MPI_Datatype sendtype,
                        void *recvbuf, const int recvcounts[],
                        const int rdispls[], MPI_Datatype recvtype,
                        MPI_Comm comm, MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Alltoallv_init(sendbuf, sendcounts, sdispls, sendtype,
                                   recvbuf, recvcounts, rdispls, recvtype,
                                   comm, info, request);
}

int PMPI_Alltoallv_init_c(const void *sendbuf, const MPI_Count sendcounts[],
                          const MPI_Aint sdispls[], MPI_Datatype sendtype,
                          void *recvbuf, const MPI_Count recvcounts[],
                          const MPI_Aint rdispls[], MPI_Datatype recvtype,
                          MPI_Comm comm, MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Alltoallv_init_c(sendbuf, sendcounts, sdispls, sendtype,
                                     recvbuf, recvcounts, rdispls, recvtype,
                                     comm, info, request);
}

int PMPI_Alltoallw(const void *sendbuf, const int sendcounts[],
                   const int sdispls[], const MPI_Datatype sendtypes[],
                   void *recvbuf, const int recvcounts[], const int rdispls[],
                   const MPI_Datatype recvtypes[], MPI_Comm comm)
{
  return VT()->PMPI_Alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                              recvcounts, rdispls, recvtypes, comm);
}

int PMPI_Alltoallw_c(const void *sendbuf, const MPI_Count sendcounts[],
                     const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                     void *recvbuf, const MPI_Count recvcounts[],
                     const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                     MPI_Comm comm)
{
  return VT()->PMPI_Alltoallw_c(sendbuf, sendcounts, sdispls, sendtypes,
                                recvbuf, recvcounts, rdispls, recvtypes, comm);
}

int PMPI_Alltoallw_init(const void *sendbuf, const int sendcounts[],
                        const int sdispls[], const MPI_Datatype sendtypes[],
                        void *recvbuf, const int recvcounts[],
                        const int rdispls[], const MPI_Datatype recvtypes[],
                        MPI_Comm comm, MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Alltoallw_init(sendbuf, sendcounts, sdispls, sendtypes,
                                   recvbuf, recvcounts, rdispls, recvtypes,
                                   comm, info, request);
}

int PMPI_Alltoallw_init_c(const void *sendbuf, const MPI_Count sendcounts[],
                          const MPI_Aint sdispls[],
                          const MPI_Datatype sendtypes[], void *recvbuf,
                          const MPI_Count recvcounts[],
                          const MPI_Aint rdispls[],
                          const MPI_Datatype recvtypes[], MPI_Comm comm,
                          MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Alltoallw_init_c(sendbuf, sendcounts, sdispls, sendtypes,
                                     recvbuf, recvcounts, rdispls, recvtypes,
                                     comm, info, request);
}

int PMPI_Attr_delete(MPI_Comm comm, int keyval)
{
  return VT()->PMPI_Comm_delete_attr(comm, keyval);
}

int PMPI_Attr_get(MPI_Comm comm, int keyval, void *attribute_val, int *flag)
{
  return VT()->PMPI_Comm_get_attr(comm, keyval, attribute_val, flag);
}

int PMPI_Attr_put(MPI_Comm comm, int keyval, void *attribute_val)
{
  return VT()->PMPI_Comm_set_attr(comm, keyval, attribute_val);
}

int PMPI_Barrier(MPI_Comm comm) { return VT()->PMPI_Barrier(comm); }

int PMPI_Barrier_init(MPI_Comm comm, MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Barrier_init(comm, info, request);
}

int PMPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root,
               MPI_Comm comm)
{
  return VT()->PMPI_Bcast(buffer, count, datatype, root, comm);
}

int PMPI_Bcast_c(void *buffer, MPI_Count count, MPI_Datatype datatype,
                 int root, MPI_Comm comm)
{
  return VT()->PMPI_Bcast_c(buffer, count, datatype, root, comm);
}

int PMPI_Bcast_init(void *buffer, int count, MPI_Datatype datatype, int root,
                    MPI_Comm comm, MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Bcast_init(buffer, count, datatype, root, comm, info,
                               request);
}

int PMPI_Bcast_init_c(void *buffer, MPI_Count count, MPI_Datatype datatype,
                      int root, MPI_Comm comm, MPI_Info info,
                      MPI_Request *request)
{
  return VT()->PMPI_Bcast_init_c(buffer, count, datatype, root, comm, info,
                                 request);
}

int PMPI_Bsend(const void *buf, int count, MPI_Datatype datatype, int dest,
               int tag, MPI_Comm comm)
{
  return VT()->PMPI_Bsend(buf, count, datatype, dest, tag, comm);
}

int PMPI_Bsend_c(const void *buf, MPI_Count count, MPI_Datatype datatype,
                 int dest, int tag, MPI_Comm comm)
{
  return VT()->PMPI_Bsend_c(buf, count, datatype, dest, tag, comm);
}

int PMPI_Bsend_init(const void *buf, int count, MPI_Datatype datatype,
                    int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Bsend_init(buf, count, datatype, dest, tag, comm, request);
}

int PMPI_Bsend_init_c(const void *buf, MPI_Count count, MPI_Datatype datatype,
                      int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Bsend_init_c(buf, count, datatype, dest, tag, comm,
                                 request);
}

int PMPI_Buffer_attach(void *buffer, int size)
{
  return VT()->PMPI_Buffer_attach(buffer, size);
}

int PMPI_Buffer_attach_c(void *buffer, MPI_Count size)
{
  return VT()->PMPI_Buffer_attach_c(buffer, size);
}

int PMPI_Buffer_detach(void *buffer_addr, int *size)
{
  return VT()->PMPI_Buffer_detach(buffer_addr, size);
}

int PMPI_Buffer_detach_c(void *buffer_addr, MPI_Count *size)
{
  return VT()->PMPI_Buffer_detach_c(buffer_addr, size);
}

int PMPI_Buffer_flush(void) { return VT()->PMPI_Buffer_flush(); }

int PMPI_Buffer_iflush(MPI_Request *request)
{
  return VT()->PMPI_Buffer_iflush(request);
}

int PMPI_Cancel(MPI_Request *request) { return VT()->PMPI_Cancel(request); }

int PMPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int coords[])
{
  return VT()->PMPI_Cart_coords(comm, rank, maxdims, coords);
}

int PMPI_Cart_create(MPI_Comm comm_old, int ndims, const int dims[],
                     const int periods[], int reorder, MPI_Comm *comm_cart)
{
  return VT()->PMPI_Cart_create(comm_old, ndims, dims, periods, reorder,
                                comm_cart);
}

int PMPI_Cart_get(MPI_Comm comm, int maxdims, int dims[], int periods[],
                  int coords[])
{
  return VT()->PMPI_Cart_get(comm, maxdims, dims, periods, coords);
}

int PMPI_Cart_map(MPI_Comm comm, int ndims, const int dims[],
                  const int periods[], int *newrank)
{
  return VT()->PMPI_Cart_map(comm, ndims, dims, periods, newrank);
}

int PMPI_Cart_rank(MPI_Comm comm, const int coords[], int *rank)
{
  return VT()->PMPI_Cart_rank(comm, coords, rank);
}

int PMPI_Cart_shift(MPI_Comm comm, int direction, int disp, int *rank_source,
                    int *rank_dest)
{
  return VT()->PMPI_Cart_shift(comm, direction, disp, rank_source, rank_dest);
}

int PMPI_Cart_sub(MPI_Comm comm, const int remain_dims[], MPI_Comm *newcomm)
{
  return VT()->PMPI_Cart_sub(comm, remain_dims, newcomm);
}

int PMPI_Cartdim_get(MPI_Comm comm, int *ndims)
{
  return VT()->PMPI_Cartdim_get(comm, ndims);
}

int PMPI_Close_port(const char *port_name)
{
  return VT()->PMPI_Close_port(port_name);
}

int PMPI_Comm_accept(const char *port_name, MPI_Info info, int root,
                     MPI_Comm comm, MPI_Comm *newcomm)
{
  return VT()->PMPI_Comm_accept(port_name, info, root, comm, newcomm);
}

int PMPI_Comm_attach_buffer(MPI_Comm comm, void *buffer, int size)
{
  return VT()->PMPI_Comm_attach_buffer(comm, buffer, size);
}

int PMPI_Comm_attach_buffer_c(MPI_Comm comm, void *buffer, MPI_Count size)
{
  return VT()->PMPI_Comm_attach_buffer_c(comm, buffer, size);
}

int PMPI_Comm_call_errhandler(MPI_Comm comm, int errorcode)
{
  return VT()->PMPI_Comm_call_errhandler(comm, errorcode);
}

int PMPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result)
{
  return VT()->PMPI_Comm_compare(comm1, comm2, result);
}

int PMPI_Comm_connect(const char *port_name, MPI_Info info, int root,
                      MPI_Comm comm, MPI_Comm *newcomm)
{
  return VT()->PMPI_Comm_connect(port_name, info, root, comm, newcomm);
}

int PMPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm)
{
  return VT()->PMPI_Comm_create(comm, group, newcomm);
}

int PMPI_Comm_create_errhandler(
    MPI_Comm_errhandler_function *comm_errhandler_fn,
    MPI_Errhandler *errhandler)
{
  return VT()->PMPI_Comm_create_errhandler(comm_errhandler_fn, errhandler);
}

int PMPI_Comm_create_from_group(MPI_Group group, const char *stringtag,
                                MPI_Info info, MPI_Errhandler errhandler,
                                MPI_Comm *newcomm)
{
  return VT()->PMPI_Comm_create_from_group(group, stringtag, info, errhandler,
                                           newcomm);
}

int PMPI_Comm_create_group(MPI_Comm comm, MPI_Group group, int tag,
                           MPI_Comm *newcomm)
{
  return VT()->PMPI_Comm_create_group(comm, group, tag, newcomm);
}

int PMPI_Comm_create_keyval(MPI_Comm_copy_attr_function *comm_copy_attr_fn,
    MPI_Comm_delete_attr_function *comm_delete_attr_fn, int *comm_keyval,
    void *extra_state)
{
  return VT()->PMPI_Comm_create_keyval(comm_copy_attr_fn, comm_delete_attr_fn,
                                       comm_keyval, extra_state);
}

int PMPI_Comm_delete_attr(MPI_Comm comm, int comm_keyval)
{
  return VT()->PMPI_Comm_delete_attr(comm, comm_keyval);
}

int PMPI_Comm_detach_buffer(MPI_Comm comm, void *buffer_addr, int *size)
{
  return VT()->PMPI_Comm_detach_buffer(comm, buffer_addr, size);
}

int PMPI_Comm_detach_buffer_c(MPI_Comm comm, void *buffer_addr,
                              MPI_Count *size)
{
  return VT()->PMPI_Comm_detach_buffer_c(comm, buffer_addr, size);
}

int PMPI_Comm_disconnect(MPI_Comm *comm)
{
  return VT()->PMPI_Comm_disconnect(comm);
}

int PMPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm)
{
  return VT()->PMPI_Comm_dup(comm, newcomm);
}

int PMPI_Comm_dup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm *newcomm)
{
  return VT()->PMPI_Comm_dup_with_info(comm, info, newcomm);
}

int PMPI_Comm_flush_buffer(MPI_Comm comm)
{
  return VT()->PMPI_Comm_flush_buffer(comm);
}

int PMPI_Comm_free(MPI_Comm *comm) { return VT()->PMPI_Comm_free(comm); }

int PMPI_Comm_free_keyval(int *comm_keyval)
{
  return VT()->PMPI_Comm_free_keyval(comm_keyval);
}

int PMPI_Comm_get_attr(MPI_Comm comm, int comm_keyval, void *attribute_val,
                       int *flag)
{
  return VT()->PMPI_Comm_get_attr(comm, comm_keyval, attribute_val, flag);
}

int PMPI_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler *errhandler)
{
  return VT()->PMPI_Comm_get_errhandler(comm, errhandler);
}

int PMPI_Comm_get_info(MPI_Comm comm, MPI_Info *info_used)
{
  return VT()->PMPI_Comm_get_info(comm, info_used);
}

int PMPI_Comm_get_name(MPI_Comm comm, char *comm_name, int *resultlen)
{
  return VT()->PMPI_Comm_get_name(comm, comm_name, resultlen);
}

int PMPI_Comm_get_parent(MPI_Comm *parent)
{
  return VT()->PMPI_Comm_get_parent(parent);
}

int PMPI_Comm_group(MPI_Comm comm, MPI_Group *group)
{
  return VT()->PMPI_Comm_group(comm, group);
}

int PMPI_Comm_idup(MPI_Comm comm, MPI_Comm *newcomm, MPI_Request *request)
{
  return VT()->PMPI_Comm_idup(comm, newcomm, request);
}

int PMPI_Comm_idup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm *newcomm,
                             MPI_Request *request)
{
  return VT()->PMPI_Comm_idup_with_info(comm, info, newcomm, request);
}

int PMPI_Comm_iflush_buffer(MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Comm_iflush_buffer(comm, request);
}

int PMPI_Comm_join(int fd, MPI_Comm *intercomm)
{
  return VT()->PMPI_Comm_join(fd, intercomm);
}

int PMPI_Comm_rank(MPI_Comm comm, int *rank)
{
  return VT()->PMPI_Comm_rank(comm, rank);
}

int PMPI_Comm_remote_group(MPI_Comm comm, MPI_Group *group)
{
  return VT()->PMPI_Comm_remote_group(comm, group);
}

int PMPI_Comm_remote_size(MPI_Comm comm, int *size)
{
  return VT()->PMPI_Comm_remote_size(comm, size);
}

int PMPI_Comm_set_attr(MPI_Comm comm, int comm_keyval, void *attribute_val)
{
  return VT()->PMPI_Comm_set_attr(comm, comm_keyval, attribute_val);
}

int PMPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler)
{
  return VT()->PMPI_Comm_set_errhandler(comm, errhandler);
}

int PMPI_Comm_set_info(MPI_Comm comm, MPI_Info info)
{
  return VT()->PMPI_Comm_set_info(comm, info);
}

int PMPI_Comm_set_name(MPI_Comm comm, const char *comm_name)
{
  return VT()->PMPI_Comm_set_name(comm, comm_name);
}

int PMPI_Comm_size(MPI_Comm comm, int *size)
{
  return VT()->PMPI_Comm_size(comm, size);
}

int PMPI_Comm_spawn(const char *command, char *argv[], int maxprocs,
                    MPI_Info info, int root, MPI_Comm comm,
                    MPI_Comm *intercomm, int array_of_errcodes[])
{
  return VT()->PMPI_Comm_spawn(command, argv, maxprocs, info, root, comm,
                               intercomm, array_of_errcodes);
}

int PMPI_Comm_spawn_multiple(int count, char *array_of_commands[],
                             char **array_of_argv[],
                             const int array_of_maxprocs[],
                             const MPI_Info array_of_info[], int root,
                             MPI_Comm comm, MPI_Comm *intercomm,
                             int array_of_errcodes[])
{
  return VT()->PMPI_Comm_spawn_multiple(count, array_of_commands,
                                        array_of_argv, array_of_maxprocs,
                                        array_of_info, root, comm, intercomm,
                                        array_of_errcodes);
}

int PMPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm)
{
  return VT()->PMPI_Comm_split(comm, color, key, newcomm);
}

int PMPI_Comm_split_type(MPI_Comm comm, int split_type, int key, MPI_Info info,
                         MPI_Comm *newcomm)
{
  return VT()->PMPI_Comm_split_type(comm, split_type, key, info, newcomm);
}

int PMPI_Comm_test_inter(MPI_Comm comm, int *flag)
{
  return VT()->PMPI_Comm_test_inter(comm, flag);
}

int PMPI_Compare_and_swap(const void *origin_addr, const void *compare_addr,
                          void *result_addr, MPI_Datatype datatype,
                          int target_rank, MPI_Aint target_disp, MPI_Win win)
{
  return VT()->PMPI_Compare_and_swap(origin_addr, compare_addr, result_addr,
                                     datatype, target_rank, target_disp, win);
}

int PMPI_Dims_create(int nnodes, int ndims, int dims[])
{
  return VT()->PMPI_Dims_create(nnodes, ndims, dims);
}

int PMPI_Dist_graph_create(MPI_Comm comm_old, int n, const int sources[],
                           const int degrees[], const int destinations[],
                           const int weights[], MPI_Info info, int reorder,
                           MPI_Comm *comm_dist_graph)
{
  return VT()->PMPI_Dist_graph_create(comm_old, n, sources, degrees,
                                      destinations, weights, info, reorder,
                                      comm_dist_graph);
}

int PMPI_Dist_graph_create_adjacent(MPI_Comm comm_old, int indegree,
                                    const int sources[],
                                    const int sourceweights[], int outdegree,
                                    const int destinations[],
                                    const int destweights[], MPI_Info info,
                                    int reorder, MPI_Comm *comm_dist_graph)
{
  return VT()->PMPI_Dist_graph_create_adjacent(comm_old, indegree, sources,
                                               sourceweights, outdegree,
                                               destinations, destweights, info,
                                               reorder, comm_dist_graph);
}

int PMPI_Dist_graph_neighbors(MPI_Comm comm, int maxindegree, int sources[],
                              int sourceweights[], int maxoutdegree,
                              int destinations[], int destweights[])
{
  return VT()->PMPI_Dist_graph_neighbors(comm, maxindegree, sources,
                                         sourceweights, maxoutdegree,
                                         destinations, destweights);
}

int PMPI_Dist_graph_neighbors_count(MPI_Comm comm, int *indegree,
                                    int *outdegree, int *weighted)
{
  return VT()->PMPI_Dist_graph_neighbors_count(comm, indegree, outdegree,
                                               weighted);
}

int PMPI_Errhandler_free(MPI_Errhandler *errhandler)
{
  return VT()->PMPI_Errhandler_free(errhandler);
}

int PMPI_Error_class(int errorcode, int *errorclass)
{
  return VT()->PMPI_Error_class(errorcode, errorclass);
}

int PMPI_Error_string(int errorcode, char *string, int *resultlen)
{
  return VT()->PMPI_Error_string(errorcode, string, resultlen);
}

int PMPI_Exscan(const void *sendbuf, void *recvbuf, int count,
                MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return VT()->PMPI_Exscan(sendbuf, recvbuf, count, datatype, op, comm);
}

int PMPI_Exscan_c(const void *sendbuf, void *recvbuf, MPI_Count count,
                  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return VT()->PMPI_Exscan_c(sendbuf, recvbuf, count, datatype, op, comm);
}

int PMPI_Exscan_init(const void *sendbuf, void *recvbuf, int count,
                     MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                     MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Exscan_init(sendbuf, recvbuf, count, datatype, op, comm,
                                info, request);
}

int PMPI_Exscan_init_c(const void *sendbuf, void *recvbuf, MPI_Count count,
                       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                       MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Exscan_init_c(sendbuf, recvbuf, count, datatype, op, comm,
                                  info, request);
}

int PMPI_Fetch_and_op(const void *origin_addr, void *result_addr,
                      MPI_Datatype datatype, int target_rank,
                      MPI_Aint target_disp, MPI_Op op, MPI_Win win)
{
  return VT()->PMPI_Fetch_and_op(origin_addr, result_addr, datatype,
                                 target_rank, target_disp, op, win);
}

int PMPI_File_call_errhandler(MPI_File fh, int errorcode)
{
  return VT()->PMPI_File_call_errhandler(fh, errorcode);
}

int PMPI_File_close(MPI_File *fh) { return VT()->PMPI_File_close(fh); }

int PMPI_File_create_errhandler(
    MPI_File_errhandler_function *file_errhandler_fn,
    MPI_Errhandler *errhandler)
{
  return VT()->PMPI_File_create_errhandler(file_errhandler_fn, errhandler);
}

int PMPI_File_delete(const char *filename, MPI_Info info)
{
  return VT()->PMPI_File_delete(filename, info);
}

int PMPI_File_get_amode(MPI_File fh, int *amode)
{
  return VT()->PMPI_File_get_amode(fh, amode);
}

int PMPI_File_get_atomicity(MPI_File fh, int *flag)
{
  return VT()->PMPI_File_get_atomicity(fh, flag);
}

int PMPI_File_get_byte_offset(MPI_File fh, MPI_Offset offset, MPI_Offset *disp)
{
  return VT()->PMPI_File_get_byte_offset(fh, offset, disp);
}

int PMPI_File_get_errhandler(MPI_File file, MPI_Errhandler *errhandler)
{
  return VT()->PMPI_File_get_errhandler(file, errhandler);
}

int PMPI_File_get_group(MPI_File fh, MPI_Group *group)
{
  return VT()->PMPI_File_get_group(fh, group);
}

int PMPI_File_get_info(MPI_File fh, MPI_Info *info_used)
{
  return VT()->PMPI_File_get_info(fh, info_used);
}

int PMPI_File_get_position(MPI_File fh, MPI_Offset *offset)
{
  return VT()->PMPI_File_get_position(fh, offset);
}

int PMPI_File_get_position_shared(MPI_File fh, MPI_Offset *offset)
{
  return VT()->PMPI_File_get_position_shared(fh, offset);
}

int PMPI_File_get_size(MPI_File fh, MPI_Offset *size)
{
  return VT()->PMPI_File_get_size(fh, size);
}

int PMPI_File_get_type_extent(MPI_File fh, MPI_Datatype datatype,
                              MPI_Aint *extent)
{
  return VT()->PMPI_File_get_type_extent(fh, datatype, extent);
}

int PMPI_File_get_type_extent_c(MPI_File fh, MPI_Datatype datatype,
                                MPI_Count *extent)
{
  return VT()->PMPI_File_get_type_extent_c(fh, datatype, extent);
}

int PMPI_File_get_view(MPI_File fh, MPI_Offset *disp, MPI_Datatype *etype,
                       MPI_Datatype *filetype, char *datarep)
{
  return VT()->PMPI_File_get_view(fh, disp, etype, filetype, datarep);
}

int PMPI_File_iread(MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                    MPI_Request *request)
{
  return VT()->PMPI_File_iread(fh, buf, count, datatype, request);
}

int PMPI_File_iread_c(MPI_File fh, void *buf, MPI_Count count,
                      MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->PMPI_File_iread_c(fh, buf, count, datatype, request);
}

int PMPI_File_iread_all(MPI_File fh, void *buf, int count,
                        MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->PMPI_File_iread_all(fh, buf, count, datatype, request);
}

int PMPI_File_iread_all_c(MPI_File fh, void *buf, MPI_Count count,
                          MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->PMPI_File_iread_all_c(fh, buf, count, datatype, request);
}

int PMPI_File_iread_at(MPI_File fh, MPI_Offset offset, void *buf, int count,
                       MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->PMPI_File_iread_at(fh, offset, buf, count, datatype, request);
}

int PMPI_File_iread_at_c(MPI_File fh, MPI_Offset offset, void *buf,
                         MPI_Count count, MPI_Datatype datatype,
                         MPI_Request *request)
{
  return VT()->PMPI_File_iread_at_c(fh, offset, buf, count, datatype, request);
}

int PMPI_File_iread_at_all(MPI_File fh, MPI_Offset offset, void *buf,
                           int count, MPI_Datatype datatype,
                           MPI_Request *request)
{
  return VT()->PMPI_File_iread_at_all(fh, offset, buf, count, datatype,
                                      request);
}

int PMPI_File_iread_at_all_c(MPI_File fh, MPI_Offset offset, void *buf,
                             MPI_Count count, MPI_Datatype datatype,
                             MPI_Request *request)
{
  return VT()->PMPI_File_iread_at_all_c(fh, offset, buf, count, datatype,
                                        request);
}

int PMPI_File_iread_shared(MPI_File fh, void *buf, int count,
                           MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->PMPI_File_iread_shared(fh, buf, count, datatype, request);
}

int PMPI_File_iread_shared_c(MPI_File fh, void *buf, MPI_Count count,
                             MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->PMPI_File_iread_shared_c(fh, buf, count, datatype, request);
}

int PMPI_File_iwrite(MPI_File fh, const void *buf, int count,
                     MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->PMPI_File_iwrite(fh, buf, count, datatype, request);
}

int PMPI_File_iwrite_c(MPI_File fh, const void *buf, MPI_Count count,
                       MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->PMPI_File_iwrite_c(fh, buf, count, datatype, request);
}

int PMPI_File_iwrite_all(MPI_File fh, const void *buf, int count,
                         MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->PMPI_File_iwrite_all(fh, buf, count, datatype, request);
}

int PMPI_File_iwrite_all_c(MPI_File fh, const void *buf, MPI_Count count,
                           MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->PMPI_File_iwrite_all_c(fh, buf, count, datatype, request);
}

int PMPI_File_iwrite_at(MPI_File fh, MPI_Offset offset, const void *buf,
                        int count, MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->PMPI_File_iwrite_at(fh, offset, buf, count, datatype, request);
}

int PMPI_File_iwrite_at_c(MPI_File fh, MPI_Offset offset, const void *buf,
                          MPI_Count count, MPI_Datatype datatype,
                          MPI_Request *request)
{
  return VT()->PMPI_File_iwrite_at_c(fh, offset, buf, count, datatype,
                                     request);
}

int PMPI_File_iwrite_at_all(MPI_File fh, MPI_Offset offset, const void *buf,
                            int count, MPI_Datatype datatype,
                            MPI_Request *request)
{
  return VT()->PMPI_File_iwrite_at_all(fh, offset, buf, count, datatype,
                                       request);
}

int PMPI_File_iwrite_at_all_c(MPI_File fh, MPI_Offset offset, const void *buf,
                              MPI_Count count, MPI_Datatype datatype,
                              MPI_Request *request)
{
  return VT()->PMPI_File_iwrite_at_all_c(fh, offset, buf, count, datatype,
                                         request);
}

int PMPI_File_iwrite_shared(MPI_File fh, const void *buf, int count,
                            MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->PMPI_File_iwrite_shared(fh, buf, count, datatype, request);
}

int PMPI_File_iwrite_shared_c(MPI_File fh, const void *buf, MPI_Count count,
                              MPI_Datatype datatype, MPI_Request *request)
{
  return VT()->PMPI_File_iwrite_shared_c(fh, buf, count, datatype, request);
}

int PMPI_File_open(MPI_Comm comm, const char *filename, int amode,
                   MPI_Info info, MPI_File *fh)
{
  return VT()->PMPI_File_open(comm, filename, amode, info, fh);
}

int PMPI_File_preallocate(MPI_File fh, MPI_Offset size)
{
  return VT()->PMPI_File_preallocate(fh, size);
}

int PMPI_File_read(MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                   MPI_Status *status)
{
  return VT()->PMPI_File_read(fh, buf, count, datatype, status);
}

int PMPI_File_read_c(MPI_File fh, void *buf, MPI_Count count,
                     MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->PMPI_File_read_c(fh, buf, count, datatype, status);
}

int PMPI_File_read_all(MPI_File fh, void *buf, int count,
                       MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->PMPI_File_read_all(fh, buf, count, datatype, status);
}

int PMPI_File_read_all_c(MPI_File fh, void *buf, MPI_Count count,
                         MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->PMPI_File_read_all_c(fh, buf, count, datatype, status);
}

int PMPI_File_read_all_begin(MPI_File fh, void *buf, int count,
                             MPI_Datatype datatype)
{
  return VT()->PMPI_File_read_all_begin(fh, buf, count, datatype);
}

int PMPI_File_read_all_begin_c(MPI_File fh, void *buf, MPI_Count count,
                               MPI_Datatype datatype)
{
  return VT()->PMPI_File_read_all_begin_c(fh, buf, count, datatype);
}

int PMPI_File_read_all_end(MPI_File fh, void *buf, MPI_Status *status)
{
  return VT()->PMPI_File_read_all_end(fh, buf, status);
}

int PMPI_File_read_at(MPI_File fh, MPI_Offset offset, void *buf, int count,
                      MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->PMPI_File_read_at(fh, offset, buf, count, datatype, status);
}

int PMPI_File_read_at_c(MPI_File fh, MPI_Offset offset, void *buf,
                        MPI_Count count, MPI_Datatype datatype,
                        MPI_Status *status)
{
  return VT()->PMPI_File_read_at_c(fh, offset, buf, count, datatype, status);
}

int PMPI_File_read_at_all(MPI_File fh, MPI_Offset offset, void *buf, int count,
                          MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->PMPI_File_read_at_all(fh, offset, buf, count, datatype, status);
}

int PMPI_File_read_at_all_c(MPI_File fh, MPI_Offset offset, void *buf,
                            MPI_Count count, MPI_Datatype datatype,
                            MPI_Status *status)
{
  return VT()->PMPI_File_read_at_all_c(fh, offset, buf, count, datatype,
                                       status);
}

int PMPI_File_read_at_all_begin(MPI_File fh, MPI_Offset offset, void *buf,
                                int count, MPI_Datatype datatype)
{
  return VT()->PMPI_File_read_at_all_begin(fh, offset, buf, count, datatype);
}

int PMPI_File_read_at_all_begin_c(MPI_File fh, MPI_Offset offset, void *buf,
                                  MPI_Count count, MPI_Datatype datatype)
{
  return VT()->PMPI_File_read_at_all_begin_c(fh, offset, buf, count, datatype);
}

int PMPI_File_read_at_all_end(MPI_File fh, void *buf, MPI_Status *status)
{
  return VT()->PMPI_File_read_at_all_end(fh, buf, status);
}

int PMPI_File_read_ordered(MPI_File fh, void *buf, int count,
                           MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->PMPI_File_read_ordered(fh, buf, count, datatype, status);
}

int PMPI_File_read_ordered_c(MPI_File fh, void *buf, MPI_Count count,
                             MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->PMPI_File_read_ordered_c(fh, buf, count, datatype, status);
}

int PMPI_File_read_ordered_begin(MPI_File fh, void *buf, int count,
                                 MPI_Datatype datatype)
{
  return VT()->PMPI_File_read_ordered_begin(fh, buf, count, datatype);
}

int PMPI_File_read_ordered_begin_c(MPI_File fh, void *buf, MPI_Count count,
                                   MPI_Datatype datatype)
{
  return VT()->PMPI_File_read_ordered_begin_c(fh, buf, count, datatype);
}

int PMPI_File_read_ordered_end(MPI_File fh, void *buf, MPI_Status *status)
{
  return VT()->PMPI_File_read_ordered_end(fh, buf, status);
}

int PMPI_File_read_shared(MPI_File fh, void *buf, int count,
                          MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->PMPI_File_read_shared(fh, buf, count, datatype, status);
}

int PMPI_File_read_shared_c(MPI_File fh, void *buf, MPI_Count count,
                            MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->PMPI_File_read_shared_c(fh, buf, count, datatype, status);
}

int PMPI_File_seek(MPI_File fh, MPI_Offset offset, int whence)
{
  return VT()->PMPI_File_seek(fh, offset, whence);
}

int PMPI_File_seek_shared(MPI_File fh, MPI_Offset offset, int whence)
{
  return VT()->PMPI_File_seek_shared(fh, offset, whence);
}

int PMPI_File_set_atomicity(MPI_File fh, int flag)
{
  return VT()->PMPI_File_set_atomicity(fh, flag);
}

int PMPI_File_set_errhandler(MPI_File file, MPI_Errhandler errhandler)
{
  return VT()->PMPI_File_set_errhandler(file, errhandler);
}

int PMPI_File_set_info(MPI_File fh, MPI_Info info)
{
  return VT()->PMPI_File_set_info(fh, info);
}

int PMPI_File_set_size(MPI_File fh, MPI_Offset size)
{
  return VT()->PMPI_File_set_size(fh, size);
}

int PMPI_File_set_view(MPI_File fh, MPI_Offset disp, MPI_Datatype etype,
                       MPI_Datatype filetype, const char *datarep,
                       MPI_Info info)
{
  return VT()->PMPI_File_set_view(fh, disp, etype, filetype, datarep, info);
}

int PMPI_File_sync(MPI_File fh) { return VT()->PMPI_File_sync(fh); }

int PMPI_File_write(MPI_File fh, const void *buf, int count,
                    MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->PMPI_File_write(fh, buf, count, datatype, status);
}

int PMPI_File_write_c(MPI_File fh, const void *buf, MPI_Count count,
                      MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->PMPI_File_write_c(fh, buf, count, datatype, status);
}

int PMPI_File_write_all(MPI_File fh, const void *buf, int count,
                        MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->PMPI_File_write_all(fh, buf, count, datatype, status);
}

int PMPI_File_write_all_c(MPI_File fh, const void *buf, MPI_Count count,
                          MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->PMPI_File_write_all_c(fh, buf, count, datatype, status);
}

int PMPI_File_write_all_begin(MPI_File fh, const void *buf, int count,
                              MPI_Datatype datatype)
{
  return VT()->PMPI_File_write_all_begin(fh, buf, count, datatype);
}

int PMPI_File_write_all_begin_c(MPI_File fh, const void *buf, MPI_Count count,
                                MPI_Datatype datatype)
{
  return VT()->PMPI_File_write_all_begin_c(fh, buf, count, datatype);
}

int PMPI_File_write_all_end(MPI_File fh, const void *buf, MPI_Status *status)
{
  return VT()->PMPI_File_write_all_end(fh, buf, status);
}

int PMPI_File_write_at(MPI_File fh, MPI_Offset offset, const void *buf,
                       int count, MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->PMPI_File_write_at(fh, offset, buf, count, datatype, status);
}

int PMPI_File_write_at_c(MPI_File fh, MPI_Offset offset, const void *buf,
                         MPI_Count count, MPI_Datatype datatype,
                         MPI_Status *status)
{
  return VT()->PMPI_File_write_at_c(fh, offset, buf, count, datatype, status);
}

int PMPI_File_write_at_all(MPI_File fh, MPI_Offset offset, const void *buf,
                           int count, MPI_Datatype datatype,
                           MPI_Status *status)
{
  return VT()->PMPI_File_write_at_all(fh, offset, buf, count, datatype,
                                      status);
}

int PMPI_File_write_at_all_c(MPI_File fh, MPI_Offset offset, const void *buf,
                             MPI_Count count, MPI_Datatype datatype,
                             MPI_Status *status)
{
  return VT()->PMPI_File_write_at_all_c(fh, offset, buf, count, datatype,
                                        status);
}

int PMPI_File_write_at_all_begin(MPI_File fh, MPI_Offset offset,
                                 const void *buf, int count,
                                 MPI_Datatype datatype)
{
  return VT()->PMPI_File_write_at_all_begin(fh, offset, buf, count, datatype);
}

int PMPI_File_write_at_all_begin_c(MPI_File fh, MPI_Offset offset,
                                   const void *buf, MPI_Count count,
                                   MPI_Datatype datatype)
{
  return VT()->PMPI_File_write_at_all_begin_c(fh, offset, buf, count,
                                              datatype);
}

int PMPI_File_write_at_all_end(MPI_File fh, const void *buf,
                               MPI_Status *status)
{
  return VT()->PMPI_File_write_at_all_end(fh, buf, status);
}

int PMPI_File_write_ordered(MPI_File fh, const void *buf, int count,
                            MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->PMPI_File_write_ordered(fh, buf, count, datatype, status);
}

int PMPI_File_write_ordered_c(MPI_File fh, const void *buf, MPI_Count count,
                              MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->PMPI_File_write_ordered_c(fh, buf, count, datatype, status);
}

int PMPI_File_write_ordered_begin(MPI_File fh, const void *buf, int count,
                                  MPI_Datatype datatype)
{
  return VT()->PMPI_File_write_ordered_begin(fh, buf, count, datatype);
}

int PMPI_File_write_ordered_begin_c(MPI_File fh, const void *buf,
                                    MPI_Count count, MPI_Datatype datatype)
{
  return VT()->PMPI_File_write_ordered_begin_c(fh, buf, count, datatype);
}

int PMPI_File_write_ordered_end(MPI_File fh, const void *buf,
                                MPI_Status *status)
{
  return VT()->PMPI_File_write_ordered_end(fh, buf, status);
}

int PMPI_File_write_shared(MPI_File fh, const void *buf, int count,
                           MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->PMPI_File_write_shared(fh, buf, count, datatype, status);
}

int PMPI_File_write_shared_c(MPI_File fh, const void *buf, MPI_Count count,
                             MPI_Datatype datatype, MPI_Status *status)
{
  return VT()->PMPI_File_write_shared_c(fh, buf, count, datatype, status);
}

int PMPI_Finalize(void) { return VT()->PMPI_Finalize(); }

int PMPI_Finalized(int *flag) { return VT()->PMPI_Finalized(flag); }

int PMPI_Free_mem(void *base) { return VT()->PMPI_Free_mem(base); }

int PMPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                MPI_Comm comm)
{
  return VT()->PMPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                           recvtype, root, comm);
}

int PMPI_Gather_c(const void *sendbuf, MPI_Count sendcount,
                  MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount,
                  MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  return VT()->PMPI_Gather_c(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                             recvtype, root, comm);
}

int PMPI_Gather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                     void *recvbuf, int recvcount, MPI_Datatype recvtype,
                     int root, MPI_Comm comm, MPI_Info info,
                     MPI_Request *request)
{
  return VT()->PMPI_Gather_init(sendbuf, sendcount, sendtype, recvbuf,
                                recvcount, recvtype, root, comm, info,
                                request);
}

int PMPI_Gather_init_c(const void *sendbuf, MPI_Count sendcount,
                       MPI_Datatype sendtype, void *recvbuf,
                       MPI_Count recvcount, MPI_Datatype recvtype, int root,
                       MPI_Comm comm, MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Gather_init_c(sendbuf, sendcount, sendtype, recvbuf,
                                  recvcount, recvtype, root, comm, info,
                                  request);
}

int PMPI_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, const int recvcounts[], const int displs[],
                 MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  return VT()->PMPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                            displs, recvtype, root, comm);
}

int PMPI_Gatherv_c(const void *sendbuf, MPI_Count sendcount,
                   MPI_Datatype sendtype, void *recvbuf,
                   const MPI_Count recvcounts[], const MPI_Aint displs[],
                   MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  return VT()->PMPI_Gatherv_c(sendbuf, sendcount, sendtype, recvbuf,
                              recvcounts, displs, recvtype, root, comm);
}

int PMPI_Gatherv_init(const void *sendbuf, int sendcount,
                      MPI_Datatype sendtype, void *recvbuf,
                      const int recvcounts[], const int displs[],
                      MPI_Datatype recvtype, int root, MPI_Comm comm,
                      MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Gatherv_init(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcounts, displs, recvtype, root, comm,
                                 info, request);
}

int PMPI_Gatherv_init_c(const void *sendbuf, MPI_Count sendcount,
                        MPI_Datatype sendtype, void *recvbuf,
                        const MPI_Count recvcounts[], const MPI_Aint displs[],
                        MPI_Datatype recvtype, int root, MPI_Comm comm,
                        MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Gatherv_init_c(sendbuf, sendcount, sendtype, recvbuf,
                                   recvcounts, displs, recvtype, root, comm,
                                   info, request);
}

int PMPI_Get(void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
             int target_rank, MPI_Aint target_disp, int target_count,
             MPI_Datatype target_datatype, MPI_Win win)
{
  return VT()->PMPI_Get(origin_addr, origin_count, origin_datatype,
                        target_rank, target_disp, target_count,
                        target_datatype, win);
}

int PMPI_Get_c(void *origin_addr, MPI_Count origin_count,
               MPI_Datatype origin_datatype, int target_rank,
               MPI_Aint target_disp, MPI_Count target_count,
               MPI_Datatype target_datatype, MPI_Win win)
{
  return VT()->PMPI_Get_c(origin_addr, origin_count, origin_datatype,
                          target_rank, target_disp, target_count,
                          target_datatype, win);
}

int PMPI_Get_accumulate(const void *origin_addr, int origin_count,
                        MPI_Datatype origin_datatype, void *result_addr,
                        int result_count, MPI_Datatype result_datatype,
                        int target_rank, MPI_Aint target_disp,
                        int target_count, MPI_Datatype target_datatype,
                        MPI_Op op, MPI_Win win)
{
  return VT()->PMPI_Get_accumulate(origin_addr, origin_count, origin_datatype,
                                   result_addr, result_count, result_datatype,
                                   target_rank, target_disp, target_count,
                                   target_datatype, op, win);
}

int PMPI_Get_accumulate_c(const void *origin_addr, MPI_Count origin_count,
                          MPI_Datatype origin_datatype, void *result_addr,
                          MPI_Count result_count, MPI_Datatype result_datatype,
                          int target_rank, MPI_Aint target_disp,
                          MPI_Count target_count, MPI_Datatype target_datatype,
                          MPI_Op op, MPI_Win win)
{
  return VT()->PMPI_Get_accumulate_c(origin_addr, origin_count,
                                     origin_datatype, result_addr,
                                     result_count, result_datatype,
                                     target_rank, target_disp, target_count,
                                     target_datatype, op, win);
}

int PMPI_Get_address(const void *location, MPI_Aint *address)
{
  return VT()->PMPI_Get_address(location, address);
}

int PMPI_Get_count(const MPI_Status *status, MPI_Datatype datatype, int *count)
{
  return VT()->PMPI_Get_count(status, datatype, count);
}

int PMPI_Get_count_c(const MPI_Status *status, MPI_Datatype datatype,
                     MPI_Count *count)
{
  return VT()->PMPI_Get_count_c(status, datatype, count);
}

int PMPI_Get_elements(const MPI_Status *status, MPI_Datatype datatype,
                      int *count)
{
  return VT()->PMPI_Get_elements(status, datatype, count);
}

int PMPI_Get_elements_c(const MPI_Status *status, MPI_Datatype datatype,
                        MPI_Count *count)
{
  return VT()->PMPI_Get_elements_c(status, datatype, count);
}

int PMPI_Get_elements_x(const MPI_Status *status, MPI_Datatype datatype,
                        MPI_Count *count)
{
  return VT()->PMPI_Get_elements_x(status, datatype, count);
}

int PMPI_Get_hw_resource_info(MPI_Info *hw_info)
{
  return VT()->PMPI_Get_hw_resource_info(hw_info);
}

int PMPI_Get_library_version(char *version, int *resultlen)
{
  return VT()->PMPI_Get_library_version(version, resultlen);
}

int PMPI_Get_processor_name(char *name, int *resultlen)
{
  return VT()->PMPI_Get_processor_name(name, resultlen);
}

int PMPI_Get_version(int *version, int *subversion)
{
  return VT()->PMPI_Get_version(version, subversion);
}

int PMPI_Graph_create(MPI_Comm comm_old, int nnodes, const int indx[],
                      const int edges[], int reorder, MPI_Comm *comm_graph)
{
  return VT()->PMPI_Graph_create(comm_old, nnodes, indx, edges, reorder,
                                 comm_graph);
}

int PMPI_Graph_get(MPI_Comm comm, int maxindex, int maxedges, int indx[],
                   int edges[])
{
  return VT()->PMPI_Graph_get(comm, maxindex, maxedges, indx, edges);
}

int PMPI_Graph_map(MPI_Comm comm, int nnodes, const int indx[],
                   const int edges[], int *newrank)
{
  return VT()->PMPI_Graph_map(comm, nnodes, indx, edges, newrank);
}

int PMPI_Graph_neighbors(MPI_Comm comm, int rank, int maxneighbors,
                         int neighbors[])
{
  return VT()->PMPI_Graph_neighbors(comm, rank, maxneighbors, neighbors);
}

int PMPI_Graph_neighbors_count(MPI_Comm comm, int rank, int *nneighbors)
{
  return VT()->PMPI_Graph_neighbors_count(comm, rank, nneighbors);
}

int PMPI_Graphdims_get(MPI_Comm comm, int *nnodes, int *nedges)
{
  return VT()->PMPI_Graphdims_get(comm, nnodes, nedges);
}

int PMPI_Grequest_complete(MPI_Request request)
{
  return VT()->PMPI_Grequest_complete(request);
}

int PMPI_Grequest_start(MPI_Grequest_query_function *query_fn,
                        MPI_Grequest_free_function *free_fn,
                        MPI_Grequest_cancel_function *cancel_fn,
                        void *extra_state, MPI_Request *request)
{
  return VT()->PMPI_Grequest_start(query_fn, free_fn, cancel_fn, extra_state,
                                   request);
}

int PMPI_Group_compare(MPI_Group group1, MPI_Group group2, int *result)
{
  return VT()->PMPI_Group_compare(group1, group2, result);
}

int PMPI_Group_difference(MPI_Group group1, MPI_Group group2,
                          MPI_Group *newgroup)
{
  return VT()->PMPI_Group_difference(group1, group2, newgroup);
}

int PMPI_Group_excl(MPI_Group group, int n, const int ranks[],
                    MPI_Group *newgroup)
{
  return VT()->PMPI_Group_excl(group, n, ranks, newgroup);
}

int PMPI_Group_free(MPI_Group *group) { return VT()->PMPI_Group_free(group); }

int PMPI_Group_from_session_pset(MPI_Session session, const char *pset_name,
                                 MPI_Group *newgroup)
{
  return VT()->PMPI_Group_from_session_pset(session, pset_name, newgroup);
}

int PMPI_Group_incl(MPI_Group group, int n, const int ranks[],
                    MPI_Group *newgroup)
{
  return VT()->PMPI_Group_incl(group, n, ranks, newgroup);
}

int PMPI_Group_intersection(MPI_Group group1, MPI_Group group2,
                            MPI_Group *newgroup)
{
  return VT()->PMPI_Group_intersection(group1, group2, newgroup);
}

int PMPI_Group_range_excl(MPI_Group group, int n, int ranges[][3],
                          MPI_Group *newgroup)
{
  return VT()->PMPI_Group_range_excl(group, n, ranges, newgroup);
}

int PMPI_Group_range_incl(MPI_Group group, int n, int ranges[][3],
                          MPI_Group *newgroup)
{
  return VT()->PMPI_Group_range_incl(group, n, ranges, newgroup);
}

int PMPI_Group_rank(MPI_Group group, int *rank)
{
  return VT()->PMPI_Group_rank(group, rank);
}

int PMPI_Group_size(MPI_Group group, int *size)
{
  return VT()->PMPI_Group_size(group, size);
}

int PMPI_Group_translate_ranks(MPI_Group group1, int n, const int ranks1[],
                               MPI_Group group2, int ranks2[])
{
  return VT()->PMPI_Group_translate_ranks(group1, n, ranks1, group2, ranks2);
}

int PMPI_Group_union(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup)
{
  return VT()->PMPI_Group_union(group1, group2, newgroup);
}

int PMPI_Iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                    void *recvbuf, int recvcount, MPI_Datatype recvtype,
                    MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Iallgather(sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, comm, request);
}

int PMPI_Iallgather_c(const void *sendbuf, MPI_Count sendcount,
                      MPI_Datatype sendtype, void *recvbuf,
                      MPI_Count recvcount, MPI_Datatype recvtype,
                      MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Iallgather_c(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcount, recvtype, comm, request);
}

int PMPI_Iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                     void *recvbuf, const int recvcounts[], const int displs[],
                     MPI_Datatype recvtype, MPI_Comm comm,
                     MPI_Request *request)
{
  return VT()->PMPI_Iallgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                recvcounts, displs, recvtype, comm, request);
}

int PMPI_Iallgatherv_c(const void *sendbuf, MPI_Count sendcount,
                       MPI_Datatype sendtype, void *recvbuf,
                       const MPI_Count recvcounts[], const MPI_Aint displs[],
                       MPI_Datatype recvtype, MPI_Comm comm,
                       MPI_Request *request)
{
  return VT()->PMPI_Iallgatherv_c(sendbuf, sendcount, sendtype, recvbuf,
                                  recvcounts, displs, recvtype, comm, request);
}

int PMPI_Iallreduce(const void *sendbuf, void *recvbuf, int count,
                    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                    MPI_Request *request)
{
  return VT()->PMPI_Iallreduce(sendbuf, recvbuf, count, datatype, op, comm,
                               request);
}

int PMPI_Iallreduce_c(const void *sendbuf, void *recvbuf, MPI_Count count,
                      MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                      MPI_Request *request)
{
  return VT()->PMPI_Iallreduce_c(sendbuf, recvbuf, count, datatype, op, comm,
                                 request);
}

int PMPI_Ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                   MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                              recvtype, comm, request);
}

int PMPI_Ialltoall_c(const void *sendbuf, MPI_Count sendcount,
                     MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount,
                     MPI_Datatype recvtype, MPI_Comm comm,
                     MPI_Request *request)
{
  return VT()->PMPI_Ialltoall_c(sendbuf, sendcount, sendtype, recvbuf,
                                recvcount, recvtype, comm, request);
}

int PMPI_Ialltoallv(const void *sendbuf, const int sendcounts[],
                    const int sdispls[], MPI_Datatype sendtype, void *recvbuf,
                    const int recvcounts[], const int rdispls[],
                    MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                               recvcounts, rdispls, recvtype, comm, request);
}

int PMPI_Ialltoallv_c(const void *sendbuf, const MPI_Count sendcounts[],
                      const MPI_Aint sdispls[], MPI_Datatype sendtype,
                      void *recvbuf, const MPI_Count recvcounts[],
                      const MPI_Aint rdispls[], MPI_Datatype recvtype,
                      MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Ialltoallv_c(sendbuf, sendcounts, sdispls, sendtype,
                                 recvbuf, recvcounts, rdispls, recvtype, comm,
                                 request);
}

int PMPI_Ialltoallw(const void *sendbuf, const int sendcounts[],
                    const int sdispls[], const MPI_Datatype sendtypes[],
                    void *recvbuf, const int recvcounts[], const int rdispls[],
                    const MPI_Datatype recvtypes[], MPI_Comm comm,
                    MPI_Request *request)
{
  return VT()->PMPI_Ialltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                               recvbuf, recvcounts, rdispls, recvtypes, comm,
                               request);
}

int PMPI_Ialltoallw_c(const void *sendbuf, const MPI_Count sendcounts[],
                      const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                      void *recvbuf, const MPI_Count recvcounts[],
                      const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                      MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Ialltoallw_c(sendbuf, sendcounts, sdispls, sendtypes,
                                 recvbuf, recvcounts, rdispls, recvtypes, comm,
                                 request);
}

int PMPI_Ibarrier(MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Ibarrier(comm, request);
}

int PMPI_Ibcast(void *buffer, int count, MPI_Datatype datatype, int root,
                MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Ibcast(buffer, count, datatype, root, comm, request);
}

int PMPI_Ibcast_c(void *buffer, MPI_Count count, MPI_Datatype datatype,
                  int root, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Ibcast_c(buffer, count, datatype, root, comm, request);
}

int PMPI_Ibsend(const void *buf, int count, MPI_Datatype datatype, int dest,
                int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Ibsend(buf, count, datatype, dest, tag, comm, request);
}

int PMPI_Ibsend_c(const void *buf, MPI_Count count, MPI_Datatype datatype,
                  int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Ibsend_c(buf, count, datatype, dest, tag, comm, request);
}

int PMPI_Iexscan(const void *sendbuf, void *recvbuf, int count,
                 MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                 MPI_Request *request)
{
  return VT()->PMPI_Iexscan(sendbuf, recvbuf, count, datatype, op, comm,
                            request);
}

int PMPI_Iexscan_c(const void *sendbuf, void *recvbuf, MPI_Count count,
                   MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                   MPI_Request *request)
{
  return VT()->PMPI_Iexscan_c(sendbuf, recvbuf, count, datatype, op, comm,
                              request);
}

int PMPI_Igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                 MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Igather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                            recvtype, root, comm, request);
}

int PMPI_Igather_c(const void *sendbuf, MPI_Count sendcount,
                   MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount,
                   MPI_Datatype recvtype, int root, MPI_Comm comm,
                   MPI_Request *request)
{
  return VT()->PMPI_Igather_c(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                              recvtype, root, comm, request);
}

int PMPI_Igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, const int recvcounts[], const int displs[],
                  MPI_Datatype recvtype, int root, MPI_Comm comm,
                  MPI_Request *request)
{
  return VT()->PMPI_Igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                             displs, recvtype, root, comm, request);
}

int PMPI_Igatherv_c(const void *sendbuf, MPI_Count sendcount,
                    MPI_Datatype sendtype, void *recvbuf,
                    const MPI_Count recvcounts[], const MPI_Aint displs[],
                    MPI_Datatype recvtype, int root, MPI_Comm comm,
                    MPI_Request *request)
{
  return VT()->PMPI_Igatherv_c(sendbuf, sendcount, sendtype, recvbuf,
                               recvcounts, displs, recvtype, root, comm,
                               request);
}

int PMPI_Improbe(int source, int tag, MPI_Comm comm, int *flag,
                 MPI_Message *message, MPI_Status *status)
{
  return VT()->PMPI_Improbe(source, tag, comm, flag, message, status);
}

int PMPI_Imrecv(void *buf, int count, MPI_Datatype datatype,
                MPI_Message *message, MPI_Request *request)
{
  return VT()->PMPI_Imrecv(buf, count, datatype, message, request);
}

int PMPI_Imrecv_c(void *buf, MPI_Count count, MPI_Datatype datatype,
                  MPI_Message *message, MPI_Request *request)
{
  return VT()->PMPI_Imrecv_c(buf, count, datatype, message, request);
}

int PMPI_Ineighbor_allgather(const void *sendbuf, int sendcount,
                             MPI_Datatype sendtype, void *recvbuf,
                             int recvcount, MPI_Datatype recvtype,
                             MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Ineighbor_allgather(sendbuf, sendcount, sendtype, recvbuf,
                                        recvcount, recvtype, comm, request);
}

int PMPI_Ineighbor_allgather_c(const void *sendbuf, MPI_Count sendcount,
                               MPI_Datatype sendtype, void *recvbuf,
                               MPI_Count recvcount, MPI_Datatype recvtype,
                               MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Ineighbor_allgather_c(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcount, recvtype, comm,
                                          request);
}

int PMPI_Ineighbor_allgatherv(const void *sendbuf, int sendcount,
                              MPI_Datatype sendtype, void *recvbuf,
                              const int recvcounts[], const int displs[],
                              MPI_Datatype recvtype, MPI_Comm comm,
                              MPI_Request *request)
{
  return VT()->PMPI_Ineighbor_allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                         recvcounts, displs, recvtype, comm,
                                         request);
}

int PMPI_Ineighbor_allgatherv_c(const void *sendbuf, MPI_Count sendcount,
                                MPI_Datatype sendtype, void *recvbuf,
                                const MPI_Count recvcounts[],
                                const MPI_Aint displs[], MPI_Datatype recvtype,
                                MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Ineighbor_allgatherv_c(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcounts, displs,
                                           recvtype, comm, request);
}

int PMPI_Ineighbor_alltoall(const void *sendbuf, int sendcount,
                            MPI_Datatype sendtype, void *recvbuf,
                            int recvcount, MPI_Datatype recvtype,
                            MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Ineighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                       recvcount, recvtype, comm, request);
}

int PMPI_Ineighbor_alltoall_c(const void *sendbuf, MPI_Count sendcount,
                              MPI_Datatype sendtype, void *recvbuf,
                              MPI_Count recvcount, MPI_Datatype recvtype,
                              MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Ineighbor_alltoall_c(sendbuf, sendcount, sendtype, recvbuf,
                                         recvcount, recvtype, comm, request);
}

int PMPI_Ineighbor_alltoallv(const void *sendbuf, const int sendcounts[],
                             const int sdispls[], MPI_Datatype sendtype,
                             void *recvbuf, const int recvcounts[],
                             const int rdispls[], MPI_Datatype recvtype,
                             MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Ineighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                        recvbuf, recvcounts, rdispls, recvtype,
                                        comm, request);
}

int PMPI_Ineighbor_alltoallv_c(const void *sendbuf,
                               const MPI_Count sendcounts[],
                               const MPI_Aint sdispls[], MPI_Datatype sendtype,
                               void *recvbuf, const MPI_Count recvcounts[],
                               const MPI_Aint rdispls[], MPI_Datatype recvtype,
                               MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Ineighbor_alltoallv_c(sendbuf, sendcounts, sdispls,
                                          sendtype, recvbuf, recvcounts,
                                          rdispls, recvtype, comm, request);
}

int PMPI_Ineighbor_alltoallw(const void *sendbuf, const int sendcounts[],
                             const MPI_Aint sdispls[],
                             const MPI_Datatype sendtypes[], void *recvbuf,
                             const int recvcounts[], const MPI_Aint rdispls[],
                             const MPI_Datatype recvtypes[], MPI_Comm comm,
                             MPI_Request *request)
{
  return VT()->PMPI_Ineighbor_alltoallw(sendbuf, sendcounts, sdispls,
                                        sendtypes, recvbuf, recvcounts,
                                        rdispls, recvtypes, comm, request);
}

int PMPI_Ineighbor_alltoallw_c(const void *sendbuf,
                               const MPI_Count sendcounts[],
                               const MPI_Aint sdispls[],
                               const MPI_Datatype sendtypes[], void *recvbuf,
                               const MPI_Count recvcounts[],
                               const MPI_Aint rdispls[],
                               const MPI_Datatype recvtypes[], MPI_Comm comm,
                               MPI_Request *request)
{
  return VT()->PMPI_Ineighbor_alltoallw_c(sendbuf, sendcounts, sdispls,
                                          sendtypes, recvbuf, recvcounts,
                                          rdispls, recvtypes, comm, request);
}

int PMPI_Info_create(MPI_Info *info) { return VT()->PMPI_Info_create(info); }

int PMPI_Info_create_env(int argc, char *argv[], MPI_Info *info)
{
  return VT()->PMPI_Info_create_env(argc, argv, info);
}

int PMPI_Info_delete(MPI_Info info, const char *key)
{
  return VT()->PMPI_Info_delete(info, key);
}

int PMPI_Info_dup(MPI_Info info, MPI_Info *newinfo)
{
  return VT()->PMPI_Info_dup(info, newinfo);
}

int PMPI_Info_free(MPI_Info *info) { return VT()->PMPI_Info_free(info); }

int PMPI_Info_get(MPI_Info info, const char *key, int valuelen, char *value,
                  int *flag)
{
  return VT()->PMPI_Info_get(info, key, valuelen, value, flag);
}

int PMPI_Info_get_nkeys(MPI_Info info, int *nkeys)
{
  return VT()->PMPI_Info_get_nkeys(info, nkeys);
}

int PMPI_Info_get_nthkey(MPI_Info info, int n, char *key)
{
  return VT()->PMPI_Info_get_nthkey(info, n, key);
}

int PMPI_Info_get_string(MPI_Info info, const char *key, int *buflen,
                         char *value, int *flag)
{
  return VT()->PMPI_Info_get_string(info, key, buflen, value, flag);
}

int PMPI_Info_get_valuelen(MPI_Info info, const char *key, int *valuelen,
                           int *flag)
{
  return VT()->PMPI_Info_get_valuelen(info, key, valuelen, flag);
}

int PMPI_Info_set(MPI_Info info, const char *key, const char *value)
{
  return VT()->PMPI_Info_set(info, key, value);
}

int PMPI_Init(int *argc, char ***argv) { return VT()->PMPI_Init(argc, argv); }

int PMPI_Init_thread(int *argc, char ***argv, int required, int *provided)
{
  return VT()->PMPI_Init_thread(argc, argv, required, provided);
}

int PMPI_Initialized(int *flag) { return VT()->PMPI_Initialized(flag); }

int PMPI_Intercomm_create(MPI_Comm local_comm, int local_leader,
                          MPI_Comm peer_comm, int remote_leader, int tag,
                          MPI_Comm *newintercomm)
{
  return VT()->PMPI_Intercomm_create(local_comm, local_leader, peer_comm,
                                     remote_leader, tag, newintercomm);
}

int PMPI_Intercomm_create_from_groups(MPI_Group local_group, int local_leader,
                                      MPI_Group remote_group,
                                      int remote_leader, const char *stringtag,
                                      MPI_Info info, MPI_Errhandler errhandler,
                                      MPI_Comm *newintercomm)
{
  return VT()->PMPI_Intercomm_create_from_groups(local_group, local_leader,
                                                 remote_group, remote_leader,
                                                 stringtag, info, errhandler,
                                                 newintercomm);
}

int PMPI_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm *newintracomm)
{
  return VT()->PMPI_Intercomm_merge(intercomm, high, newintracomm);
}

int PMPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag,
                MPI_Status *status)
{
  return VT()->PMPI_Iprobe(source, tag, comm, flag, status);
}

int PMPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source,
               int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Irecv(buf, count, datatype, source, tag, comm, request);
}

int PMPI_Irecv_c(void *buf, MPI_Count count, MPI_Datatype datatype, int source,
                 int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Irecv_c(buf, count, datatype, source, tag, comm, request);
}

int PMPI_Ireduce(const void *sendbuf, void *recvbuf, int count,
                 MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm,
                 MPI_Request *request)
{
  return VT()->PMPI_Ireduce(sendbuf, recvbuf, count, datatype, op, root, comm,
                            request);
}

int PMPI_Ireduce_c(const void *sendbuf, void *recvbuf, MPI_Count count,
                   MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm,
                   MPI_Request *request)
{
  return VT()->PMPI_Ireduce_c(sendbuf, recvbuf, count, datatype, op, root,
                              comm, request);
}

int PMPI_Ireduce_scatter(const void *sendbuf, void *recvbuf,
                         const int recvcounts[], MPI_Datatype datatype,
                         MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op,
                                    comm, request);
}

int PMPI_Ireduce_scatter_c(const void *sendbuf, void *recvbuf,
                           const MPI_Count recvcounts[], MPI_Datatype datatype,
                           MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Ireduce_scatter_c(sendbuf, recvbuf, recvcounts, datatype,
                                      op, comm, request);
}

int PMPI_Ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                               int recvcount, MPI_Datatype datatype, MPI_Op op,
                               MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Ireduce_scatter_block(sendbuf, recvbuf, recvcount,
                                          datatype, op, comm, request);
}

int PMPI_Ireduce_scatter_block_c(const void *sendbuf, void *recvbuf,
                                 MPI_Count recvcount, MPI_Datatype datatype,
                                 MPI_Op op, MPI_Comm comm,
                                 MPI_Request *request)
{
  return VT()->PMPI_Ireduce_scatter_block_c(sendbuf, recvbuf, recvcount,
                                            datatype, op, comm, request);
}

int PMPI_Irsend(const void *buf, int count, MPI_Datatype datatype, int dest,
                int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Irsend(buf, count, datatype, dest, tag, comm, request);
}

int PMPI_Irsend_c(const void *buf, MPI_Count count, MPI_Datatype datatype,
                  int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Irsend_c(buf, count, datatype, dest, tag, comm, request);
}

int PMPI_Is_thread_main(int *flag) { return VT()->PMPI_Is_thread_main(flag); }

int PMPI_Iscan(const void *sendbuf, void *recvbuf, int count,
               MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
               MPI_Request *request)
{
  return VT()->PMPI_Iscan(sendbuf, recvbuf, count, datatype, op, comm,
                          request);
}

int PMPI_Iscan_c(const void *sendbuf, void *recvbuf, MPI_Count count,
                 MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                 MPI_Request *request)
{
  return VT()->PMPI_Iscan_c(sendbuf, recvbuf, count, datatype, op, comm,
                            request);
}

int PMPI_Iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  int root, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                             recvtype, root, comm, request);
}

int PMPI_Iscatter_c(const void *sendbuf, MPI_Count sendcount,
                    MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount,
                    MPI_Datatype recvtype, int root, MPI_Comm comm,
                    MPI_Request *request)
{
  return VT()->PMPI_Iscatter_c(sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, root, comm, request);
}

int PMPI_Iscatterv(const void *sendbuf, const int sendcounts[],
                   const int displs[], MPI_Datatype sendtype, void *recvbuf,
                   int recvcount, MPI_Datatype recvtype, int root,
                   MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                              recvcount, recvtype, root, comm, request);
}

int PMPI_Iscatterv_c(const void *sendbuf, const MPI_Count sendcounts[],
                     const MPI_Aint displs[], MPI_Datatype sendtype,
                     void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype,
                     int root, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Iscatterv_c(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                recvcount, recvtype, root, comm, request);
}

int PMPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest,
               int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
}

int PMPI_Isend_c(const void *buf, MPI_Count count, MPI_Datatype datatype,
                 int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Isend_c(buf, count, datatype, dest, tag, comm, request);
}

int PMPI_Isendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   int dest, int sendtag, void *recvbuf, int recvcount,
                   MPI_Datatype recvtype, int source, int recvtag,
                   MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Isendrecv(sendbuf, sendcount, sendtype, dest, sendtag,
                              recvbuf, recvcount, recvtype, source, recvtag,
                              comm, request);
}

int PMPI_Isendrecv_c(const void *sendbuf, MPI_Count sendcount,
                     MPI_Datatype sendtype, int dest, int sendtag,
                     void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype,
                     int source, int recvtag, MPI_Comm comm,
                     MPI_Request *request)
{
  return VT()->PMPI_Isendrecv_c(sendbuf, sendcount, sendtype, dest, sendtag,
                                recvbuf, recvcount, recvtype, source, recvtag,
                                comm, request);
}

int PMPI_Isendrecv_replace(void *buf, int count, MPI_Datatype datatype,
                           int dest, int sendtag, int source, int recvtag,
                           MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Isendrecv_replace(buf, count, datatype, dest, sendtag,
                                      source, recvtag, comm, request);
}

int PMPI_Isendrecv_replace_c(void *buf, MPI_Count count, MPI_Datatype datatype,
                             int dest, int sendtag, int source, int recvtag,
                             MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Isendrecv_replace_c(buf, count, datatype, dest, sendtag,
                                        source, recvtag, comm, request);
}

int PMPI_Issend(const void *buf, int count, MPI_Datatype datatype, int dest,
                int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Issend(buf, count, datatype, dest, tag, comm, request);
}

int PMPI_Issend_c(const void *buf, MPI_Count count, MPI_Datatype datatype,
                  int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Issend_c(buf, count, datatype, dest, tag, comm, request);
}

int PMPI_Keyval_create(MPI_Copy_function *copy_fn,
                       MPI_Delete_function *delete_fn, int *keyval,
                       void *extra_state)
{
  return VT()->PMPI_Comm_create_keyval(copy_fn, delete_fn, keyval,
                                       extra_state);
}

int PMPI_Keyval_free(int *keyval)
{
  return VT()->PMPI_Comm_free_keyval(keyval);
}

int PMPI_Lookup_name(const char *service_name, MPI_Info info, char *port_name)
{
  return VT()->PMPI_Lookup_name(service_name, info, port_name);
}

int PMPI_Mprobe(int source, int tag, MPI_Comm comm, MPI_Message *message,
                MPI_Status *status)
{
  return VT()->PMPI_Mprobe(source, tag, comm, message, status);
}

int PMPI_Mrecv(void *buf, int count, MPI_Datatype datatype,
               MPI_Message *message, MPI_Status *status)
{
  return VT()->PMPI_Mrecv(buf, count, datatype, message, status);
}

int PMPI_Mrecv_c(void *buf, MPI_Count count, MPI_Datatype datatype,
                 MPI_Message *message, MPI_Status *status)
{
  return VT()->PMPI_Mrecv_c(buf, count, datatype, message, status);
}

int PMPI_Neighbor_allgather(const void *sendbuf, int sendcount,
                            MPI_Datatype sendtype, void *recvbuf,
                            int recvcount, MPI_Datatype recvtype,
                            MPI_Comm comm)
{
  return VT()->PMPI_Neighbor_allgather(sendbuf, sendcount, sendtype, recvbuf,
                                       recvcount, recvtype, comm);
}

int PMPI_Neighbor_allgather_c(const void *sendbuf, MPI_Count sendcount,
                              MPI_Datatype sendtype, void *recvbuf,
                              MPI_Count recvcount, MPI_Datatype recvtype,
                              MPI_Comm comm)
{
  return VT()->PMPI_Neighbor_allgather_c(sendbuf, sendcount, sendtype, recvbuf,
                                         recvcount, recvtype, comm);
}

int PMPI_Neighbor_allgather_init(const void *sendbuf, int sendcount,
                                 MPI_Datatype sendtype, void *recvbuf,
                                 int recvcount, MPI_Datatype recvtype,
                                 MPI_Comm comm, MPI_Info info,
                                 MPI_Request *request)
{
  return VT()->PMPI_Neighbor_allgather_init(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcount, recvtype, comm,
                                            info, request);
}

int PMPI_Neighbor_allgather_init_c(const void *sendbuf, MPI_Count sendcount,
                                   MPI_Datatype sendtype, void *recvbuf,
                                   MPI_Count recvcount, MPI_Datatype recvtype,
                                   MPI_Comm comm, MPI_Info info,
                                   MPI_Request *request)
{
  return VT()->PMPI_Neighbor_allgather_init_c(sendbuf, sendcount, sendtype,
                                              recvbuf, recvcount, recvtype,
                                              comm, info, request);
}

int PMPI_Neighbor_allgatherv(const void *sendbuf, int sendcount,
                             MPI_Datatype sendtype, void *recvbuf,
                             const int recvcounts[], const int displs[],
                             MPI_Datatype recvtype, MPI_Comm comm)
{
  return VT()->PMPI_Neighbor_allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                        recvcounts, displs, recvtype, comm);
}

int PMPI_Neighbor_allgatherv_c(const void *sendbuf, MPI_Count sendcount,
                               MPI_Datatype sendtype, void *recvbuf,
                               const MPI_Count recvcounts[],
                               const MPI_Aint displs[], MPI_Datatype recvtype,
                               MPI_Comm comm)
{
  return VT()->PMPI_Neighbor_allgatherv_c(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcounts, displs,
                                          recvtype, comm);
}

int PMPI_Neighbor_allgatherv_init(const void *sendbuf, int sendcount,
                                  MPI_Datatype sendtype, void *recvbuf,
                                  const int recvcounts[], const int displs[],
                                  MPI_Datatype recvtype, MPI_Comm comm,
                                  MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Neighbor_allgatherv_init(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcounts, displs,
                                             recvtype, comm, info, request);
}

int PMPI_Neighbor_allgatherv_init_c(const void *sendbuf, MPI_Count sendcount,
                                    MPI_Datatype sendtype, void *recvbuf,
                                    const MPI_Count recvcounts[],
                                    const MPI_Aint displs[],
                                    MPI_Datatype recvtype, MPI_Comm comm,
                                    MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Neighbor_allgatherv_init_c(sendbuf, sendcount, sendtype,
                                               recvbuf, recvcounts, displs,
                                               recvtype, comm, info, request);
}

int PMPI_Neighbor_alltoall(const void *sendbuf, int sendcount,
                           MPI_Datatype sendtype, void *recvbuf, int recvcount,
                           MPI_Datatype recvtype, MPI_Comm comm)
{
  return VT()->PMPI_Neighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                      recvcount, recvtype, comm);
}

int PMPI_Neighbor_alltoall_c(const void *sendbuf, MPI_Count sendcount,
                             MPI_Datatype sendtype, void *recvbuf,
                             MPI_Count recvcount, MPI_Datatype recvtype,
                             MPI_Comm comm)
{
  return VT()->PMPI_Neighbor_alltoall_c(sendbuf, sendcount, sendtype, recvbuf,
                                        recvcount, recvtype, comm);
}

int PMPI_Neighbor_alltoall_init(const void *sendbuf, int sendcount,
                                MPI_Datatype sendtype, void *recvbuf,
                                int recvcount, MPI_Datatype recvtype,
                                MPI_Comm comm, MPI_Info info,
                                MPI_Request *request)
{
  return VT()->PMPI_Neighbor_alltoall_init(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype, comm,
                                           info, request);
}

int PMPI_Neighbor_alltoall_init_c(const void *sendbuf, MPI_Count sendcount,
                                  MPI_Datatype sendtype, void *recvbuf,
                                  MPI_Count recvcount, MPI_Datatype recvtype,
                                  MPI_Comm comm, MPI_Info info,
                                  MPI_Request *request)
{
  return VT()->PMPI_Neighbor_alltoall_init_c(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcount, recvtype,
                                             comm, info, request);
}

int PMPI_Neighbor_alltoallv(const void *sendbuf, const int sendcounts[],
                            const int sdispls[], MPI_Datatype sendtype,
                            void *recvbuf, const int recvcounts[],
                            const int rdispls[], MPI_Datatype recvtype,
                            MPI_Comm comm)
{
  return VT()->PMPI_Neighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                       recvbuf, recvcounts, rdispls, recvtype,
                                       comm);
}

int PMPI_Neighbor_alltoallv_c(const void *sendbuf,
                              const MPI_Count sendcounts[],
                              const MPI_Aint sdispls[], MPI_Datatype sendtype,
                              void *recvbuf, const MPI_Count recvcounts[],
                              const MPI_Aint rdispls[], MPI_Datatype recvtype,
                              MPI_Comm comm)
{
  return VT()->PMPI_Neighbor_alltoallv_c(sendbuf, sendcounts, sdispls,
                                         sendtype, recvbuf, recvcounts,
                                         rdispls, recvtype, comm);
}

int PMPI_Neighbor_alltoallv_init(const void *sendbuf, const int sendcounts[],
                                 const int sdispls[], MPI_Datatype sendtype,
                                 void *recvbuf, const int recvcounts[],
                                 const int rdispls[], MPI_Datatype recvtype,
                                 MPI_Comm comm, MPI_Info info,
                                 MPI_Request *request)
{
  return VT()->PMPI_Neighbor_alltoallv_init(sendbuf, sendcounts, sdispls,
                                            sendtype, recvbuf, recvcounts,
                                            rdispls, recvtype, comm, info,
                                            request);
}

int PMPI_Neighbor_alltoallv_init_c(const void *sendbuf,
                                   const MPI_Count sendcounts[],
                                   const MPI_Aint sdispls[],
                                   MPI_Datatype sendtype, void *recvbuf,
                                   const MPI_Count recvcounts[],
                                   const MPI_Aint rdispls[],
                                   MPI_Datatype recvtype, MPI_Comm comm,
                                   MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Neighbor_alltoallv_init_c(sendbuf, sendcounts, sdispls,
                                              sendtype, recvbuf, recvcounts,
                                              rdispls, recvtype, comm, info,
                                              request);
}

int PMPI_Neighbor_alltoallw(const void *sendbuf, const int sendcounts[],
                            const MPI_Aint sdispls[],
                            const MPI_Datatype sendtypes[], void *recvbuf,
                            const int recvcounts[], const MPI_Aint rdispls[],
                            const MPI_Datatype recvtypes[], MPI_Comm comm)
{
  return VT()->PMPI_Neighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                       recvbuf, recvcounts, rdispls, recvtypes,
                                       comm);
}

int PMPI_Neighbor_alltoallw_c(const void *sendbuf,
                              const MPI_Count sendcounts[],
                              const MPI_Aint sdispls[],
                              const MPI_Datatype sendtypes[], void *recvbuf,
                              const MPI_Count recvcounts[],
                              const MPI_Aint rdispls[],
                              const MPI_Datatype recvtypes[], MPI_Comm comm)
{
  return VT()->PMPI_Neighbor_alltoallw_c(sendbuf, sendcounts, sdispls,
                                         sendtypes, recvbuf, recvcounts,
                                         rdispls, recvtypes, comm);
}

int PMPI_Neighbor_alltoallw_init(const void *sendbuf, const int sendcounts[],
                                 const MPI_Aint sdispls[],
                                 const MPI_Datatype sendtypes[], void *recvbuf,
                                 const int recvcounts[],
                                 const MPI_Aint rdispls[],
                                 const MPI_Datatype recvtypes[], MPI_Comm comm,
                                 MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Neighbor_alltoallw_init(sendbuf, sendcounts, sdispls,
                                            sendtypes, recvbuf, recvcounts,
                                            rdispls, recvtypes, comm, info,
                                            request);
}

int PMPI_Neighbor_alltoallw_init_c(const void *sendbuf,
                                   const MPI_Count sendcounts[],
                                   const MPI_Aint sdispls[],
                                   const MPI_Datatype sendtypes[],
                                   void *recvbuf, const MPI_Count recvcounts[],
                                   const MPI_Aint rdispls[],
                                   const MPI_Datatype recvtypes[],
                                   MPI_Comm comm, MPI_Info info,
                                   MPI_Request *request)
{
  return VT()->PMPI_Neighbor_alltoallw_init_c(sendbuf, sendcounts, sdispls,
                                              sendtypes, recvbuf, recvcounts,
                                              rdispls, recvtypes, comm, info,
                                              request);
}

int PMPI_Op_commutative(MPI_Op op, int *commute)
{
  return VT()->PMPI_Op_commutative(op, commute);
}

int PMPI_Op_create(MPI_User_function *user_fn, int commute, MPI_Op *op)
{
  return VT()->PMPI_Op_create(user_fn, commute, op);
}

int PMPI_Op_create_c(MPI_User_function_c *user_fn, int commute, MPI_Op *op)
{
  return VT()->PMPI_Op_create_c(user_fn, commute, op);
}

int PMPI_Op_free(MPI_Op *op) { return VT()->PMPI_Op_free(op); }

int PMPI_Open_port(MPI_Info info, char *port_name)
{
  return VT()->PMPI_Open_port(info, port_name);
}

int PMPI_Pack(const void *inbuf, int incount, MPI_Datatype datatype,
              void *outbuf, int outsize, int *position, MPI_Comm comm)
{
  return VT()->PMPI_Pack(inbuf, incount, datatype, outbuf, outsize, position,
                         comm);
}

int PMPI_Pack_c(const void *inbuf, MPI_Count incount, MPI_Datatype datatype,
                void *outbuf, MPI_Count outsize, MPI_Count *position,
                MPI_Comm comm)
{
  return VT()->PMPI_Pack_c(inbuf, incount, datatype, outbuf, outsize, position,
                           comm);
}

int PMPI_Pack_external(const char *datarep, const void *inbuf, int incount,
                       MPI_Datatype datatype, void *outbuf, MPI_Aint outsize,
                       MPI_Aint *position)
{
  return VT()->PMPI_Pack_external(datarep, inbuf, incount, datatype, outbuf,
                                  outsize, position);
}

int PMPI_Pack_external_c(const char *datarep, const void *inbuf,
                         MPI_Count incount, MPI_Datatype datatype,
                         void *outbuf, MPI_Count outsize, MPI_Count *position)
{
  return VT()->PMPI_Pack_external_c(datarep, inbuf, incount, datatype, outbuf,
                                    outsize, position);
}

int PMPI_Pack_external_size(const char *datarep, int incount,
                            MPI_Datatype datatype, MPI_Aint *size)
{
  return VT()->PMPI_Pack_external_size(datarep, incount, datatype, size);
}

int PMPI_Pack_external_size_c(const char *datarep, MPI_Count incount,
                              MPI_Datatype datatype, MPI_Count *size)
{
  return VT()->PMPI_Pack_external_size_c(datarep, incount, datatype, size);
}

int PMPI_Pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm,
                   int *size)
{
  return VT()->PMPI_Pack_size(incount, datatype, comm, size);
}

int PMPI_Pack_size_c(MPI_Count incount, MPI_Datatype datatype, MPI_Comm comm,
                     MPI_Count *size)
{
  return VT()->PMPI_Pack_size_c(incount, datatype, comm, size);
}

int PMPI_Parrived(MPI_Request request, int partition, int *flag)
{
  return VT()->PMPI_Parrived(request, partition, flag);
}

/* The extra arguments stop here: C cannot forward
 * `...`, and MPI-5.0 14.2 lets a profiling layer
 * ignore them.
 */
int PMPI_Pcontrol(const int level, ...) { return VT()->PMPI_Pcontrol(level); }

int PMPI_Pready(int partition, MPI_Request request)
{
  return VT()->PMPI_Pready(partition, request);
}

int PMPI_Pready_list(int length, const int array_of_partitions[],
                     MPI_Request request)
{
  return VT()->PMPI_Pready_list(length, array_of_partitions, request);
}

int PMPI_Pready_range(int partition_low, int partition_high,
                      MPI_Request request)
{
  return VT()->PMPI_Pready_range(partition_low, partition_high, request);
}

int PMPI_Precv_init(void *buf, int partitions, MPI_Count count,
                    MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                    MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Precv_init(buf, partitions, count, datatype, dest, tag,
                               comm, info, request);
}

int PMPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status)
{
  return VT()->PMPI_Probe(source, tag, comm, status);
}

int PMPI_Psend_init(const void *buf, int partitions, MPI_Count count,
                    MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                    MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Psend_init(buf, partitions, count, datatype, dest, tag,
                               comm, info, request);
}

int PMPI_Publish_name(const char *service_name, MPI_Info info,
                      const char *port_name)
{
  return VT()->PMPI_Publish_name(service_name, info, port_name);
}

int PMPI_Put(const void *origin_addr, int origin_count,
             MPI_Datatype origin_datatype, int target_rank,
             MPI_Aint target_disp, int target_count,
             MPI_Datatype target_datatype, MPI_Win win)
{
  return VT()->PMPI_Put(origin_addr, origin_count, origin_datatype,
                        target_rank, target_disp, target_count,
                        target_datatype, win);
}

int PMPI_Put_c(const void *origin_addr, MPI_Count origin_count,
               MPI_Datatype origin_datatype, int target_rank,
               MPI_Aint target_disp, MPI_Count target_count,
               MPI_Datatype target_datatype, MPI_Win win)
{
  return VT()->PMPI_Put_c(origin_addr, origin_count, origin_datatype,
                          target_rank, target_disp, target_count,
                          target_datatype, win);
}

int PMPI_Query_thread(int *provided)
{
  return VT()->PMPI_Query_thread(provided);
}

int PMPI_Raccumulate(const void *origin_addr, int origin_count,
                     MPI_Datatype origin_datatype, int target_rank,
                     MPI_Aint target_disp, int target_count,
                     MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
                     MPI_Request *request)
{
  return VT()->PMPI_Raccumulate(origin_addr, origin_count, origin_datatype,
                                target_rank, target_disp, target_count,
                                target_datatype, op, win, request);
}

int PMPI_Raccumulate_c(const void *origin_addr, MPI_Count origin_count,
                       MPI_Datatype origin_datatype, int target_rank,
                       MPI_Aint target_disp, MPI_Count target_count,
                       MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
                       MPI_Request *request)
{
  return VT()->PMPI_Raccumulate_c(origin_addr, origin_count, origin_datatype,
                                  target_rank, target_disp, target_count,
                                  target_datatype, op, win, request);
}

int PMPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Status *status)
{
  return VT()->PMPI_Recv(buf, count, datatype, source, tag, comm, status);
}

int PMPI_Recv_c(void *buf, MPI_Count count, MPI_Datatype datatype, int source,
                int tag, MPI_Comm comm, MPI_Status *status)
{
  return VT()->PMPI_Recv_c(buf, count, datatype, source, tag, comm, status);
}

int PMPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int source,
                   int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Recv_init(buf, count, datatype, source, tag, comm,
                              request);
}

int PMPI_Recv_init_c(void *buf, MPI_Count count, MPI_Datatype datatype,
                     int source, int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Recv_init_c(buf, count, datatype, source, tag, comm,
                                request);
}

int PMPI_Reduce(const void *sendbuf, void *recvbuf, int count,
                MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
  return VT()->PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
}

int PMPI_Reduce_c(const void *sendbuf, void *recvbuf, MPI_Count count,
                  MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
  return VT()->PMPI_Reduce_c(sendbuf, recvbuf, count, datatype, op, root,
                             comm);
}

int PMPI_Reduce_init(const void *sendbuf, void *recvbuf, int count,
                     MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm,
                     MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Reduce_init(sendbuf, recvbuf, count, datatype, op, root,
                                comm, info, request);
}

int PMPI_Reduce_init_c(const void *sendbuf, void *recvbuf, MPI_Count count,
                       MPI_Datatype datatype, MPI_Op op, int root,
                       MPI_Comm comm, MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Reduce_init_c(sendbuf, recvbuf, count, datatype, op, root,
                                  comm, info, request);
}

int PMPI_Reduce_local(const void *inbuf, void *inoutbuf, int count,
                      MPI_Datatype datatype, MPI_Op op)
{
  return VT()->PMPI_Reduce_local(inbuf, inoutbuf, count, datatype, op);
}

int PMPI_Reduce_local_c(const void *inbuf, void *inoutbuf, MPI_Count count,
                        MPI_Datatype datatype, MPI_Op op)
{
  return VT()->PMPI_Reduce_local_c(inbuf, inoutbuf, count, datatype, op);
}

int PMPI_Reduce_scatter(const void *sendbuf, void *recvbuf,
                        const int recvcounts[], MPI_Datatype datatype,
                        MPI_Op op, MPI_Comm comm)
{
  return VT()->PMPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op,
                                   comm);
}

int PMPI_Reduce_scatter_c(const void *sendbuf, void *recvbuf,
                          const MPI_Count recvcounts[], MPI_Datatype datatype,
                          MPI_Op op, MPI_Comm comm)
{
  return VT()->PMPI_Reduce_scatter_c(sendbuf, recvbuf, recvcounts, datatype,
                                     op, comm);
}

int PMPI_Reduce_scatter_block(const void *sendbuf, void *recvbuf,
                              int recvcount, MPI_Datatype datatype, MPI_Op op,
                              MPI_Comm comm)
{
  return VT()->PMPI_Reduce_scatter_block(sendbuf, recvbuf, recvcount, datatype,
                                         op, comm);
}

int PMPI_Reduce_scatter_block_c(const void *sendbuf, void *recvbuf,
                                MPI_Count recvcount, MPI_Datatype datatype,
                                MPI_Op op, MPI_Comm comm)
{
  return VT()->PMPI_Reduce_scatter_block_c(sendbuf, recvbuf, recvcount,
                                           datatype, op, comm);
}

int PMPI_Reduce_scatter_block_init(const void *sendbuf, void *recvbuf,
                                   int recvcount, MPI_Datatype datatype,
                                   MPI_Op op, MPI_Comm comm, MPI_Info info,
                                   MPI_Request *request)
{
  return VT()->PMPI_Reduce_scatter_block_init(sendbuf, recvbuf, recvcount,
                                              datatype, op, comm, info,
                                              request);
}

int PMPI_Reduce_scatter_block_init_c(const void *sendbuf, void *recvbuf,
                                     MPI_Count recvcount,
                                     MPI_Datatype datatype, MPI_Op op,
                                     MPI_Comm comm, MPI_Info info,
                                     MPI_Request *request)
{
  return VT()->PMPI_Reduce_scatter_block_init_c(sendbuf, recvbuf, recvcount,
                                                datatype, op, comm, info,
                                                request);
}

int PMPI_Reduce_scatter_init(const void *sendbuf, void *recvbuf,
                             const int recvcounts[], MPI_Datatype datatype,
                             MPI_Op op, MPI_Comm comm, MPI_Info info,
                             MPI_Request *request)
{
  return VT()->PMPI_Reduce_scatter_init(sendbuf, recvbuf, recvcounts, datatype,
                                        op, comm, info, request);
}

int PMPI_Reduce_scatter_init_c(const void *sendbuf, void *recvbuf,
                               const MPI_Count recvcounts[],
                               MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                               MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Reduce_scatter_init_c(sendbuf, recvbuf, recvcounts,
                                          datatype, op, comm, info, request);
}

int PMPI_Register_datarep(const char *datarep,
    MPI_Datarep_conversion_function *read_conversion_fn,
    MPI_Datarep_conversion_function *write_conversion_fn,
    MPI_Datarep_extent_function *dtype_file_extent_fn, void *extra_state)
{
  return VT()->PMPI_Register_datarep(datarep, read_conversion_fn,
                                     write_conversion_fn, dtype_file_extent_fn,
                                     extra_state);
}

int PMPI_Register_datarep_c(const char *datarep,
    MPI_Datarep_conversion_function_c *read_conversion_fn,
    MPI_Datarep_conversion_function_c *write_conversion_fn,
    MPI_Datarep_extent_function *dtype_file_extent_fn, void *extra_state)
{
  return VT()->PMPI_Register_datarep_c(datarep, read_conversion_fn,
                                       write_conversion_fn,
                                       dtype_file_extent_fn, extra_state);
}

int PMPI_Remove_error_class(int errorclass)
{
  return VT()->PMPI_Remove_error_class(errorclass);
}

int PMPI_Remove_error_code(int errorcode)
{
  return VT()->PMPI_Remove_error_code(errorcode);
}

int PMPI_Remove_error_string(int errorcode)
{
  return VT()->PMPI_Remove_error_string(errorcode);
}

int PMPI_Request_free(MPI_Request *request)
{
  return VT()->PMPI_Request_free(request);
}

int PMPI_Request_get_status(MPI_Request request, int *flag, MPI_Status *status)
{
  return VT()->PMPI_Request_get_status(request, flag, status);
}

int PMPI_Request_get_status_all(int count,
                                const MPI_Request array_of_requests[],
                                int *flag, MPI_Status *array_of_statuses)
{
  return VT()->PMPI_Request_get_status_all(count, array_of_requests, flag,
                                           array_of_statuses);
}

int PMPI_Request_get_status_any(int count,
                                const MPI_Request array_of_requests[],
                                int *indx, int *flag, MPI_Status *status)
{
  return VT()->PMPI_Request_get_status_any(count, array_of_requests, indx,
                                           flag, status);
}

int PMPI_Request_get_status_some(int incount,
                                 const MPI_Request array_of_requests[],
                                 int *outcount, int array_of_indices[],
                                 MPI_Status *array_of_statuses)
{
  return VT()->PMPI_Request_get_status_some(incount, array_of_requests,
                                            outcount, array_of_indices,
                                            array_of_statuses);
}

int PMPI_Rget(void *origin_addr, int origin_count,
              MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count,
              MPI_Datatype target_datatype, MPI_Win win, MPI_Request *request)
{
  return VT()->PMPI_Rget(origin_addr, origin_count, origin_datatype,
                         target_rank, target_disp, target_count,
                         target_datatype, win, request);
}

int PMPI_Rget_c(void *origin_addr, MPI_Count origin_count,
                MPI_Datatype origin_datatype, int target_rank,
                MPI_Aint target_disp, MPI_Count target_count,
                MPI_Datatype target_datatype, MPI_Win win,
                MPI_Request *request)
{
  return VT()->PMPI_Rget_c(origin_addr, origin_count, origin_datatype,
                           target_rank, target_disp, target_count,
                           target_datatype, win, request);
}

int PMPI_Rget_accumulate(const void *origin_addr, int origin_count,
                         MPI_Datatype origin_datatype, void *result_addr,
                         int result_count, MPI_Datatype result_datatype,
                         int target_rank, MPI_Aint target_disp,
                         int target_count, MPI_Datatype target_datatype,
                         MPI_Op op, MPI_Win win, MPI_Request *request)
{
  return VT()->PMPI_Rget_accumulate(origin_addr, origin_count, origin_datatype,
                                    result_addr, result_count, result_datatype,
                                    target_rank, target_disp, target_count,
                                    target_datatype, op, win, request);
}

int PMPI_Rget_accumulate_c(const void *origin_addr, MPI_Count origin_count,
                           MPI_Datatype origin_datatype, void *result_addr,
                           MPI_Count result_count,
                           MPI_Datatype result_datatype, int target_rank,
                           MPI_Aint target_disp, MPI_Count target_count,
                           MPI_Datatype target_datatype, MPI_Op op,
                           MPI_Win win, MPI_Request *request)
{
  return VT()->PMPI_Rget_accumulate_c(origin_addr, origin_count,
                                      origin_datatype, result_addr,
                                      result_count, result_datatype,
                                      target_rank, target_disp, target_count,
                                      target_datatype, op, win, request);
}

int PMPI_Rput(const void *origin_addr, int origin_count,
              MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count,
              MPI_Datatype target_datatype, MPI_Win win, MPI_Request *request)
{
  return VT()->PMPI_Rput(origin_addr, origin_count, origin_datatype,
                         target_rank, target_disp, target_count,
                         target_datatype, win, request);
}

int PMPI_Rput_c(const void *origin_addr, MPI_Count origin_count,
                MPI_Datatype origin_datatype, int target_rank,
                MPI_Aint target_disp, MPI_Count target_count,
                MPI_Datatype target_datatype, MPI_Win win,
                MPI_Request *request)
{
  return VT()->PMPI_Rput_c(origin_addr, origin_count, origin_datatype,
                           target_rank, target_disp, target_count,
                           target_datatype, win, request);
}

int PMPI_Rsend(const void *buf, int count, MPI_Datatype datatype, int dest,
               int tag, MPI_Comm comm)
{
  return VT()->PMPI_Rsend(buf, count, datatype, dest, tag, comm);
}

int PMPI_Rsend_c(const void *buf, MPI_Count count, MPI_Datatype datatype,
                 int dest, int tag, MPI_Comm comm)
{
  return VT()->PMPI_Rsend_c(buf, count, datatype, dest, tag, comm);
}

int PMPI_Rsend_init(const void *buf, int count, MPI_Datatype datatype,
                    int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Rsend_init(buf, count, datatype, dest, tag, comm, request);
}

int PMPI_Rsend_init_c(const void *buf, MPI_Count count, MPI_Datatype datatype,
                      int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Rsend_init_c(buf, count, datatype, dest, tag, comm,
                                 request);
}

int PMPI_Scan(const void *sendbuf, void *recvbuf, int count,
              MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return VT()->PMPI_Scan(sendbuf, recvbuf, count, datatype, op, comm);
}

int PMPI_Scan_c(const void *sendbuf, void *recvbuf, MPI_Count count,
                MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  return VT()->PMPI_Scan_c(sendbuf, recvbuf, count, datatype, op, comm);
}

int PMPI_Scan_init(const void *sendbuf, void *recvbuf, int count,
                   MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                   MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Scan_init(sendbuf, recvbuf, count, datatype, op, comm,
                              info, request);
}

int PMPI_Scan_init_c(const void *sendbuf, void *recvbuf, MPI_Count count,
                     MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                     MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Scan_init_c(sendbuf, recvbuf, count, datatype, op, comm,
                                info, request);
}

int PMPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                 MPI_Comm comm)
{
  return VT()->PMPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                            recvtype, root, comm);
}

int PMPI_Scatter_c(const void *sendbuf, MPI_Count sendcount,
                   MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount,
                   MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  return VT()->PMPI_Scatter_c(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                              recvtype, root, comm);
}

int PMPI_Scatter_init(const void *sendbuf, int sendcount,
                      MPI_Datatype sendtype, void *recvbuf, int recvcount,
                      MPI_Datatype recvtype, int root, MPI_Comm comm,
                      MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Scatter_init(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcount, recvtype, root, comm, info,
                                 request);
}

int PMPI_Scatter_init_c(const void *sendbuf, MPI_Count sendcount,
                        MPI_Datatype sendtype, void *recvbuf,
                        MPI_Count recvcount, MPI_Datatype recvtype, int root,
                        MPI_Comm comm, MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Scatter_init_c(sendbuf, sendcount, sendtype, recvbuf,
                                   recvcount, recvtype, root, comm, info,
                                   request);
}

int PMPI_Scatterv(const void *sendbuf, const int sendcounts[],
                  const int displs[], MPI_Datatype sendtype, void *recvbuf,
                  int recvcount, MPI_Datatype recvtype, int root,
                  MPI_Comm comm)
{
  return VT()->PMPI_Scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                             recvcount, recvtype, root, comm);
}

int PMPI_Scatterv_c(const void *sendbuf, const MPI_Count sendcounts[],
                    const MPI_Aint displs[], MPI_Datatype sendtype,
                    void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype,
                    int root, MPI_Comm comm)
{
  return VT()->PMPI_Scatterv_c(sendbuf, sendcounts, displs, sendtype, recvbuf,
                               recvcount, recvtype, root, comm);
}

int PMPI_Scatterv_init(const void *sendbuf, const int sendcounts[],
                       const int displs[], MPI_Datatype sendtype,
                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                       int root, MPI_Comm comm, MPI_Info info,
                       MPI_Request *request)
{
  return VT()->PMPI_Scatterv_init(sendbuf, sendcounts, displs, sendtype,
                                  recvbuf, recvcount, recvtype, root, comm,
                                  info, request);
}

int PMPI_Scatterv_init_c(const void *sendbuf, const MPI_Count sendcounts[],
                         const MPI_Aint displs[], MPI_Datatype sendtype,
                         void *recvbuf, MPI_Count recvcount,
                         MPI_Datatype recvtype, int root, MPI_Comm comm,
                         MPI_Info info, MPI_Request *request)
{
  return VT()->PMPI_Scatterv_init_c(sendbuf, sendcounts, displs, sendtype,
                                    recvbuf, recvcount, recvtype, root, comm,
                                    info, request);
}

int PMPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest,
              int tag, MPI_Comm comm)
{
  return VT()->PMPI_Send(buf, count, datatype, dest, tag, comm);
}

int PMPI_Send_c(const void *buf, MPI_Count count, MPI_Datatype datatype,
                int dest, int tag, MPI_Comm comm)
{
  return VT()->PMPI_Send_c(buf, count, datatype, dest, tag, comm);
}

int PMPI_Send_init(const void *buf, int count, MPI_Datatype datatype, int dest,
                   int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Send_init(buf, count, datatype, dest, tag, comm, request);
}

int PMPI_Send_init_c(const void *buf, MPI_Count count, MPI_Datatype datatype,
                     int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Send_init_c(buf, count, datatype, dest, tag, comm,
                                request);
}

int PMPI_Sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  int dest, int sendtag, void *recvbuf, int recvcount,
                  MPI_Datatype recvtype, int source, int recvtag,
                  MPI_Comm comm, MPI_Status *status)
{
  return VT()->PMPI_Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag,
                             recvbuf, recvcount, recvtype, source, recvtag,
                             comm, status);
}

int PMPI_Sendrecv_c(const void *sendbuf, MPI_Count sendcount,
                    MPI_Datatype sendtype, int dest, int sendtag,
                    void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype,
                    int source, int recvtag, MPI_Comm comm, MPI_Status *status)
{
  return VT()->PMPI_Sendrecv_c(sendbuf, sendcount, sendtype, dest, sendtag,
                               recvbuf, recvcount, recvtype, source, recvtag,
                               comm, status);
}

int PMPI_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype,
                          int dest, int sendtag, int source, int recvtag,
                          MPI_Comm comm, MPI_Status *status)
{
  return VT()->PMPI_Sendrecv_replace(buf, count, datatype, dest, sendtag,
                                     source, recvtag, comm, status);
}

int PMPI_Sendrecv_replace_c(void *buf, MPI_Count count, MPI_Datatype datatype,
                            int dest, int sendtag, int source, int recvtag,
                            MPI_Comm comm, MPI_Status *status)
{
  return VT()->PMPI_Sendrecv_replace_c(buf, count, datatype, dest, sendtag,
                                       source, recvtag, comm, status);
}

int PMPI_Session_attach_buffer(MPI_Session session, void *buffer, int size)
{
  return VT()->PMPI_Session_attach_buffer(session, buffer, size);
}

int PMPI_Session_attach_buffer_c(MPI_Session session, void *buffer,
                                 MPI_Count size)
{
  return VT()->PMPI_Session_attach_buffer_c(session, buffer, size);
}

int PMPI_Session_call_errhandler(MPI_Session session, int errorcode)
{
  return VT()->PMPI_Session_call_errhandler(session, errorcode);
}

int PMPI_Session_create_errhandler(
    MPI_Session_errhandler_function *session_errhandler_fn,
    MPI_Errhandler *errhandler)
{
  return VT()->PMPI_Session_create_errhandler(session_errhandler_fn,
                                              errhandler);
}

int PMPI_Session_detach_buffer(MPI_Session session, void *buffer_addr,
                               int *size)
{
  return VT()->PMPI_Session_detach_buffer(session, buffer_addr, size);
}

int PMPI_Session_detach_buffer_c(MPI_Session session, void *buffer_addr,
                                 MPI_Count *size)
{
  return VT()->PMPI_Session_detach_buffer_c(session, buffer_addr, size);
}

int PMPI_Session_finalize(MPI_Session *session)
{
  return VT()->PMPI_Session_finalize(session);
}

int PMPI_Session_flush_buffer(MPI_Session session)
{
  return VT()->PMPI_Session_flush_buffer(session);
}

int PMPI_Session_get_errhandler(MPI_Session session,
                                MPI_Errhandler *errhandler)
{
  return VT()->PMPI_Session_get_errhandler(session, errhandler);
}

int PMPI_Session_get_info(MPI_Session session, MPI_Info *info_used)
{
  return VT()->PMPI_Session_get_info(session, info_used);
}

int PMPI_Session_get_nth_pset(MPI_Session session, MPI_Info info, int n,
                              int *pset_len, char *pset_name)
{
  return VT()->PMPI_Session_get_nth_pset(session, info, n, pset_len,
                                         pset_name);
}

int PMPI_Session_get_num_psets(MPI_Session session, MPI_Info info,
                               int *npset_names)
{
  return VT()->PMPI_Session_get_num_psets(session, info, npset_names);
}

int PMPI_Session_get_pset_info(MPI_Session session, const char *pset_name,
                               MPI_Info *info)
{
  return VT()->PMPI_Session_get_pset_info(session, pset_name, info);
}

int PMPI_Session_iflush_buffer(MPI_Session session, MPI_Request *request)
{
  return VT()->PMPI_Session_iflush_buffer(session, request);
}

int PMPI_Session_init(MPI_Info info, MPI_Errhandler errhandler,
                      MPI_Session *session)
{
  return VT()->PMPI_Session_init(info, errhandler, session);
}

int PMPI_Session_set_errhandler(MPI_Session session, MPI_Errhandler errhandler)
{
  return VT()->PMPI_Session_set_errhandler(session, errhandler);
}

int PMPI_Ssend(const void *buf, int count, MPI_Datatype datatype, int dest,
               int tag, MPI_Comm comm)
{
  return VT()->PMPI_Ssend(buf, count, datatype, dest, tag, comm);
}

int PMPI_Ssend_c(const void *buf, MPI_Count count, MPI_Datatype datatype,
                 int dest, int tag, MPI_Comm comm)
{
  return VT()->PMPI_Ssend_c(buf, count, datatype, dest, tag, comm);
}

int PMPI_Ssend_init(const void *buf, int count, MPI_Datatype datatype,
                    int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Ssend_init(buf, count, datatype, dest, tag, comm, request);
}

int PMPI_Ssend_init_c(const void *buf, MPI_Count count, MPI_Datatype datatype,
                      int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  return VT()->PMPI_Ssend_init_c(buf, count, datatype, dest, tag, comm,
                                 request);
}

int PMPI_Start(MPI_Request *request) { return VT()->PMPI_Start(request); }

int PMPI_Startall(int count, MPI_Request array_of_requests[])
{
  return VT()->PMPI_Startall(count, array_of_requests);
}

int PMPI_Status_get_error(const MPI_Status *status, int *error)
{
  return VT()->PMPI_Status_get_error(status, error);
}

int PMPI_Status_get_source(const MPI_Status *status, int *source)
{
  return VT()->PMPI_Status_get_source(status, source);
}

int PMPI_Status_get_tag(const MPI_Status *status, int *tag)
{
  return VT()->PMPI_Status_get_tag(status, tag);
}

int PMPI_Status_set_cancelled(MPI_Status *status, int flag)
{
  return VT()->PMPI_Status_set_cancelled(status, flag);
}

int PMPI_Status_set_elements(MPI_Status *status, MPI_Datatype datatype,
                             int count)
{
  return VT()->PMPI_Status_set_elements(status, datatype, count);
}

int PMPI_Status_set_elements_c(MPI_Status *status, MPI_Datatype datatype,
                               MPI_Count count)
{
  return VT()->PMPI_Status_set_elements_c(status, datatype, count);
}

int PMPI_Status_set_elements_x(MPI_Status *status, MPI_Datatype datatype,
                               MPI_Count count)
{
  return VT()->PMPI_Status_set_elements_x(status, datatype, count);
}

int PMPI_Status_set_error(MPI_Status *status, int error)
{
  return VT()->PMPI_Status_set_error(status, error);
}

int PMPI_Status_set_source(MPI_Status *status, int source)
{
  return VT()->PMPI_Status_set_source(status, source);
}

int PMPI_Status_set_tag(MPI_Status *status, int tag)
{
  return VT()->PMPI_Status_set_tag(status, tag);
}

int PMPI_Test(MPI_Request *request, int *flag, MPI_Status *status)
{
  return VT()->PMPI_Test(request, flag, status);
}

int PMPI_Test_cancelled(const MPI_Status *status, int *flag)
{
  return VT()->PMPI_Test_cancelled(status, flag);
}

int PMPI_Testall(int count, MPI_Request array_of_requests[], int *flag,
                 MPI_Status *array_of_statuses)
{
  return VT()->PMPI_Testall(count, array_of_requests, flag, array_of_statuses);
}

int PMPI_Testany(int count, MPI_Request array_of_requests[], int *indx,
                 int *flag, MPI_Status *status)
{
  return VT()->PMPI_Testany(count, array_of_requests, indx, flag, status);
}

int PMPI_Testsome(int incount, MPI_Request array_of_requests[], int *outcount,
                  int array_of_indices[], MPI_Status *array_of_statuses)
{
  return VT()->PMPI_Testsome(incount, array_of_requests, outcount,
                             array_of_indices, array_of_statuses);
}

int PMPI_Topo_test(MPI_Comm comm, int *status)
{
  return VT()->PMPI_Topo_test(comm, status);
}

int PMPI_Type_commit(MPI_Datatype *datatype)
{
  return VT()->PMPI_Type_commit(datatype);
}

int PMPI_Type_contiguous(int count, MPI_Datatype oldtype,
                         MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_contiguous(count, oldtype, newtype);
}

int PMPI_Type_contiguous_c(MPI_Count count, MPI_Datatype oldtype,
                           MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_contiguous_c(count, oldtype, newtype);
}

int PMPI_Type_create_darray(int size, int rank, int ndims,
                            const int array_of_gsizes[],
                            const int array_of_distribs[],
                            const int array_of_dargs[],
                            const int array_of_psizes[], int order,
                            MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_create_darray(size, rank, ndims, array_of_gsizes,
                                       array_of_distribs, array_of_dargs,
                                       array_of_psizes, order, oldtype,
                                       newtype);
}

int PMPI_Type_create_darray_c(int size, int rank, int ndims,
                              const MPI_Count array_of_gsizes[],
                              const int array_of_distribs[],
                              const int array_of_dargs[],
                              const int array_of_psizes[], int order,
                              MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_create_darray_c(size, rank, ndims, array_of_gsizes,
                                         array_of_distribs, array_of_dargs,
                                         array_of_psizes, order, oldtype,
                                         newtype);
}

int PMPI_Type_create_f90_complex(int p, int r, MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_create_f90_complex(p, r, newtype);
}

int PMPI_Type_create_f90_integer(int r, MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_create_f90_integer(r, newtype);
}

int PMPI_Type_create_f90_real(int p, int r, MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_create_f90_real(p, r, newtype);
}

int PMPI_Type_create_hindexed(int count, const int array_of_blocklengths[],
                              const MPI_Aint array_of_displacements[],
                              MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_create_hindexed(count, array_of_blocklengths,
                                         array_of_displacements, oldtype,
                                         newtype);
}

int PMPI_Type_create_hindexed_c(MPI_Count count,
                                const MPI_Count array_of_blocklengths[],
                                const MPI_Count array_of_displacements[],
                                MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_create_hindexed_c(count, array_of_blocklengths,
                                           array_of_displacements, oldtype,
                                           newtype);
}

int PMPI_Type_create_hindexed_block(int count, int blocklength,
                                    const MPI_Aint array_of_displacements[],
                                    MPI_Datatype oldtype,
                                    MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_create_hindexed_block(count, blocklength,
                                               array_of_displacements, oldtype,
                                               newtype);
}

int PMPI_Type_create_hindexed_block_c(MPI_Count count, MPI_Count blocklength,
    const MPI_Count array_of_displacements[], MPI_Datatype oldtype,
    MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_create_hindexed_block_c(count, blocklength,
                                                 array_of_displacements,
                                                 oldtype, newtype);
}

int PMPI_Type_create_hvector(int count, int blocklength, MPI_Aint stride,
                             MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_create_hvector(count, blocklength, stride, oldtype,
                                        newtype);
}

int PMPI_Type_create_hvector_c(MPI_Count count, MPI_Count blocklength,
                               MPI_Count stride, MPI_Datatype oldtype,
                               MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_create_hvector_c(count, blocklength, stride, oldtype,
                                          newtype);
}

int PMPI_Type_create_indexed_block(int count, int blocklength,
                                   const int array_of_displacements[],
                                   MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_create_indexed_block(count, blocklength,
                                              array_of_displacements, oldtype,
                                              newtype);
}

int PMPI_Type_create_indexed_block_c(MPI_Count count, MPI_Count blocklength,
                                     const MPI_Count array_of_displacements[],
                                     MPI_Datatype oldtype,
                                     MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_create_indexed_block_c(count, blocklength,
                                                array_of_displacements,
                                                oldtype, newtype);
}

int PMPI_Type_create_keyval(MPI_Type_copy_attr_function *type_copy_attr_fn,
    MPI_Type_delete_attr_function *type_delete_attr_fn, int *type_keyval,
    void *extra_state)
{
  return VT()->PMPI_Type_create_keyval(type_copy_attr_fn, type_delete_attr_fn,
                                       type_keyval, extra_state);
}

int PMPI_Type_create_resized(MPI_Datatype oldtype, MPI_Aint lb,
                             MPI_Aint extent, MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_create_resized(oldtype, lb, extent, newtype);
}

int PMPI_Type_create_resized_c(MPI_Datatype oldtype, MPI_Count lb,
                               MPI_Count extent, MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_create_resized_c(oldtype, lb, extent, newtype);
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

int PMPI_Type_create_struct_c(MPI_Count count,
                              const MPI_Count array_of_blocklengths[],
                              const MPI_Count array_of_displacements[],
                              const MPI_Datatype array_of_types[],
                              MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_create_struct_c(count, array_of_blocklengths,
                                         array_of_displacements,
                                         array_of_types, newtype);
}

int PMPI_Type_create_subarray(int ndims, const int array_of_sizes[],
                              const int array_of_subsizes[],
                              const int array_of_starts[], int order,
                              MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_create_subarray(ndims, array_of_sizes,
                                         array_of_subsizes, array_of_starts,
                                         order, oldtype, newtype);
}

int PMPI_Type_create_subarray_c(int ndims, const MPI_Count array_of_sizes[],
                                const MPI_Count array_of_subsizes[],
                                const MPI_Count array_of_starts[], int order,
                                MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_create_subarray_c(ndims, array_of_sizes,
                                           array_of_subsizes, array_of_starts,
                                           order, oldtype, newtype);
}

int PMPI_Type_delete_attr(MPI_Datatype datatype, int type_keyval)
{
  return VT()->PMPI_Type_delete_attr(datatype, type_keyval);
}

int PMPI_Type_dup(MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_dup(oldtype, newtype);
}

int PMPI_Type_free(MPI_Datatype *datatype)
{
  return VT()->PMPI_Type_free(datatype);
}

int PMPI_Type_free_keyval(int *type_keyval)
{
  return VT()->PMPI_Type_free_keyval(type_keyval);
}

int PMPI_Type_get_attr(MPI_Datatype datatype, int type_keyval,
                       void *attribute_val, int *flag)
{
  return VT()->PMPI_Type_get_attr(datatype, type_keyval, attribute_val, flag);
}

int PMPI_Type_get_contents(MPI_Datatype datatype, int max_integers,
                           int max_addresses, int max_datatypes,
                           int array_of_integers[],
                           MPI_Aint array_of_addresses[],
                           MPI_Datatype array_of_datatypes[])
{
  return VT()->PMPI_Type_get_contents(datatype, max_integers, max_addresses,
                                      max_datatypes, array_of_integers,
                                      array_of_addresses, array_of_datatypes);
}

int PMPI_Type_get_contents_c(MPI_Datatype datatype, MPI_Count max_integers,
                             MPI_Count max_addresses,
                             MPI_Count max_large_counts,
                             MPI_Count max_datatypes, int array_of_integers[],
                             MPI_Aint array_of_addresses[],
                             MPI_Count array_of_large_counts[],
                             MPI_Datatype array_of_datatypes[])
{
  return VT()->PMPI_Type_get_contents_c(datatype, max_integers, max_addresses,
                                        max_large_counts, max_datatypes,
                                        array_of_integers, array_of_addresses,
                                        array_of_large_counts,
                                        array_of_datatypes);
}

int PMPI_Type_get_envelope(MPI_Datatype datatype, int *num_integers,
                           int *num_addresses, int *num_datatypes,
                           int *combiner)
{
  return VT()->PMPI_Type_get_envelope(datatype, num_integers, num_addresses,
                                      num_datatypes, combiner);
}

int PMPI_Type_get_envelope_c(MPI_Datatype datatype, MPI_Count *num_integers,
                             MPI_Count *num_addresses,
                             MPI_Count *num_large_counts,
                             MPI_Count *num_datatypes, int *combiner)
{
  return VT()->PMPI_Type_get_envelope_c(datatype, num_integers, num_addresses,
                                        num_large_counts, num_datatypes,
                                        combiner);
}

int PMPI_Type_get_extent(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent)
{
  return VT()->PMPI_Type_get_extent(datatype, lb, extent);
}

int PMPI_Type_get_extent_c(MPI_Datatype datatype, MPI_Count *lb,
                           MPI_Count *extent)
{
  return VT()->PMPI_Type_get_extent_c(datatype, lb, extent);
}

int PMPI_Type_get_extent_x(MPI_Datatype datatype, MPI_Count *lb,
                           MPI_Count *extent)
{
  return VT()->PMPI_Type_get_extent_x(datatype, lb, extent);
}

int PMPI_Type_get_name(MPI_Datatype datatype, char *type_name, int *resultlen)
{
  return VT()->PMPI_Type_get_name(datatype, type_name, resultlen);
}

int PMPI_Type_get_true_extent(MPI_Datatype datatype, MPI_Aint *true_lb,
                              MPI_Aint *true_extent)
{
  return VT()->PMPI_Type_get_true_extent(datatype, true_lb, true_extent);
}

int PMPI_Type_get_true_extent_c(MPI_Datatype datatype, MPI_Count *true_lb,
                                MPI_Count *true_extent)
{
  return VT()->PMPI_Type_get_true_extent_c(datatype, true_lb, true_extent);
}

int PMPI_Type_get_true_extent_x(MPI_Datatype datatype, MPI_Count *true_lb,
                                MPI_Count *true_extent)
{
  return VT()->PMPI_Type_get_true_extent_x(datatype, true_lb, true_extent);
}

int PMPI_Type_get_value_index(MPI_Datatype value_type, MPI_Datatype index_type,
                              MPI_Datatype *pair_type)
{
  return VT()->PMPI_Type_get_value_index(value_type, index_type, pair_type);
}

int PMPI_Type_indexed(int count, const int array_of_blocklengths[],
                      const int array_of_displacements[], MPI_Datatype oldtype,
                      MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_indexed(count, array_of_blocklengths,
                                 array_of_displacements, oldtype, newtype);
}

int PMPI_Type_indexed_c(MPI_Count count,
                        const MPI_Count array_of_blocklengths[],
                        const MPI_Count array_of_displacements[],
                        MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_indexed_c(count, array_of_blocklengths,
                                   array_of_displacements, oldtype, newtype);
}

int PMPI_Type_match_size(int typeclass, int size, MPI_Datatype *datatype)
{
  return VT()->PMPI_Type_match_size(typeclass, size, datatype);
}

int PMPI_Type_set_attr(MPI_Datatype datatype, int type_keyval,
                       void *attribute_val)
{
  return VT()->PMPI_Type_set_attr(datatype, type_keyval, attribute_val);
}

int PMPI_Type_set_name(MPI_Datatype datatype, const char *type_name)
{
  return VT()->PMPI_Type_set_name(datatype, type_name);
}

int PMPI_Type_size(MPI_Datatype datatype, int *size)
{
  return VT()->PMPI_Type_size(datatype, size);
}

int PMPI_Type_size_c(MPI_Datatype datatype, MPI_Count *size)
{
  return VT()->PMPI_Type_size_c(datatype, size);
}

int PMPI_Type_size_x(MPI_Datatype datatype, MPI_Count *size)
{
  return VT()->PMPI_Type_size_x(datatype, size);
}

int PMPI_Type_vector(int count, int blocklength, int stride,
                     MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_vector(count, blocklength, stride, oldtype, newtype);
}

int PMPI_Type_vector_c(MPI_Count count, MPI_Count blocklength,
                       MPI_Count stride, MPI_Datatype oldtype,
                       MPI_Datatype *newtype)
{
  return VT()->PMPI_Type_vector_c(count, blocklength, stride, oldtype,
                                  newtype);
}

int PMPI_Unpack(const void *inbuf, int insize, int *position, void *outbuf,
                int outcount, MPI_Datatype datatype, MPI_Comm comm)
{
  return VT()->PMPI_Unpack(inbuf, insize, position, outbuf, outcount, datatype,
                           comm);
}

int PMPI_Unpack_c(const void *inbuf, MPI_Count insize, MPI_Count *position,
                  void *outbuf, MPI_Count outcount, MPI_Datatype datatype,
                  MPI_Comm comm)
{
  return VT()->PMPI_Unpack_c(inbuf, insize, position, outbuf, outcount,
                             datatype, comm);
}

int PMPI_Unpack_external(const char datarep[], const void *inbuf,
                         MPI_Aint insize, MPI_Aint *position, void *outbuf,
                         int outcount, MPI_Datatype datatype)
{
  return VT()->PMPI_Unpack_external(datarep, inbuf, insize, position, outbuf,
                                    outcount, datatype);
}

int PMPI_Unpack_external_c(const char datarep[], const void *inbuf,
                           MPI_Count insize, MPI_Count *position, void *outbuf,
                           MPI_Count outcount, MPI_Datatype datatype)
{
  return VT()->PMPI_Unpack_external_c(datarep, inbuf, insize, position, outbuf,
                                      outcount, datatype);
}

int PMPI_Unpublish_name(const char *service_name, MPI_Info info,
                        const char *port_name)
{
  return VT()->PMPI_Unpublish_name(service_name, info, port_name);
}

int PMPI_Wait(MPI_Request *request, MPI_Status *status)
{
  return VT()->PMPI_Wait(request, status);
}

int PMPI_Waitall(int count, MPI_Request array_of_requests[],
                 MPI_Status *array_of_statuses)
{
  return VT()->PMPI_Waitall(count, array_of_requests, array_of_statuses);
}

int PMPI_Waitany(int count, MPI_Request array_of_requests[], int *indx,
                 MPI_Status *status)
{
  return VT()->PMPI_Waitany(count, array_of_requests, indx, status);
}

int PMPI_Waitsome(int incount, MPI_Request array_of_requests[], int *outcount,
                  int array_of_indices[], MPI_Status *array_of_statuses)
{
  return VT()->PMPI_Waitsome(incount, array_of_requests, outcount,
                             array_of_indices, array_of_statuses);
}

int PMPI_Win_allocate(MPI_Aint size, int disp_unit, MPI_Info info,
                      MPI_Comm comm, void *baseptr, MPI_Win *win)
{
  return VT()->PMPI_Win_allocate(size, disp_unit, info, comm, baseptr, win);
}

int PMPI_Win_allocate_c(MPI_Aint size, MPI_Aint disp_unit, MPI_Info info,
                        MPI_Comm comm, void *baseptr, MPI_Win *win)
{
  return VT()->PMPI_Win_allocate_c(size, disp_unit, info, comm, baseptr, win);
}

int PMPI_Win_allocate_shared(MPI_Aint size, int disp_unit, MPI_Info info,
                             MPI_Comm comm, void *baseptr, MPI_Win *win)
{
  return VT()->PMPI_Win_allocate_shared(size, disp_unit, info, comm, baseptr,
                                        win);
}

int PMPI_Win_allocate_shared_c(MPI_Aint size, MPI_Aint disp_unit,
                               MPI_Info info, MPI_Comm comm, void *baseptr,
                               MPI_Win *win)
{
  return VT()->PMPI_Win_allocate_shared_c(size, disp_unit, info, comm, baseptr,
                                          win);
}

int PMPI_Win_attach(MPI_Win win, void *base, MPI_Aint size)
{
  return VT()->PMPI_Win_attach(win, base, size);
}

int PMPI_Win_call_errhandler(MPI_Win win, int errorcode)
{
  return VT()->PMPI_Win_call_errhandler(win, errorcode);
}

int PMPI_Win_complete(MPI_Win win) { return VT()->PMPI_Win_complete(win); }

int PMPI_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info,
                    MPI_Comm comm, MPI_Win *win)
{
  return VT()->PMPI_Win_create(base, size, disp_unit, info, comm, win);
}

int PMPI_Win_create_c(void *base, MPI_Aint size, MPI_Aint disp_unit,
                      MPI_Info info, MPI_Comm comm, MPI_Win *win)
{
  return VT()->PMPI_Win_create_c(base, size, disp_unit, info, comm, win);
}

int PMPI_Win_create_dynamic(MPI_Info info, MPI_Comm comm, MPI_Win *win)
{
  return VT()->PMPI_Win_create_dynamic(info, comm, win);
}

int PMPI_Win_create_errhandler(MPI_Win_errhandler_function *win_errhandler_fn,
                               MPI_Errhandler *errhandler)
{
  return VT()->PMPI_Win_create_errhandler(win_errhandler_fn, errhandler);
}

int PMPI_Win_create_keyval(MPI_Win_copy_attr_function *win_copy_attr_fn,
                           MPI_Win_delete_attr_function *win_delete_attr_fn,
                           int *win_keyval, void *extra_state)
{
  return VT()->PMPI_Win_create_keyval(win_copy_attr_fn, win_delete_attr_fn,
                                      win_keyval, extra_state);
}

int PMPI_Win_delete_attr(MPI_Win win, int win_keyval)
{
  return VT()->PMPI_Win_delete_attr(win, win_keyval);
}

int PMPI_Win_detach(MPI_Win win, const void *base)
{
  return VT()->PMPI_Win_detach(win, base);
}

int PMPI_Win_fence(int assert, MPI_Win win)
{
  return VT()->PMPI_Win_fence(assert, win);
}

int PMPI_Win_flush(int rank, MPI_Win win)
{
  return VT()->PMPI_Win_flush(rank, win);
}

int PMPI_Win_flush_all(MPI_Win win) { return VT()->PMPI_Win_flush_all(win); }

int PMPI_Win_flush_local(int rank, MPI_Win win)
{
  return VT()->PMPI_Win_flush_local(rank, win);
}

int PMPI_Win_flush_local_all(MPI_Win win)
{
  return VT()->PMPI_Win_flush_local_all(win);
}

int PMPI_Win_free(MPI_Win *win) { return VT()->PMPI_Win_free(win); }

int PMPI_Win_free_keyval(int *win_keyval)
{
  return VT()->PMPI_Win_free_keyval(win_keyval);
}

int PMPI_Win_get_attr(MPI_Win win, int win_keyval, void *attribute_val,
                      int *flag)
{
  return VT()->PMPI_Win_get_attr(win, win_keyval, attribute_val, flag);
}

int PMPI_Win_get_errhandler(MPI_Win win, MPI_Errhandler *errhandler)
{
  return VT()->PMPI_Win_get_errhandler(win, errhandler);
}

int PMPI_Win_get_group(MPI_Win win, MPI_Group *group)
{
  return VT()->PMPI_Win_get_group(win, group);
}

int PMPI_Win_get_info(MPI_Win win, MPI_Info *info_used)
{
  return VT()->PMPI_Win_get_info(win, info_used);
}

int PMPI_Win_get_name(MPI_Win win, char *win_name, int *resultlen)
{
  return VT()->PMPI_Win_get_name(win, win_name, resultlen);
}

int PMPI_Win_lock(int lock_type, int rank, int assert, MPI_Win win)
{
  return VT()->PMPI_Win_lock(lock_type, rank, assert, win);
}

int PMPI_Win_lock_all(int assert, MPI_Win win)
{
  return VT()->PMPI_Win_lock_all(assert, win);
}

int PMPI_Win_post(MPI_Group group, int assert, MPI_Win win)
{
  return VT()->PMPI_Win_post(group, assert, win);
}

int PMPI_Win_set_attr(MPI_Win win, int win_keyval, void *attribute_val)
{
  return VT()->PMPI_Win_set_attr(win, win_keyval, attribute_val);
}

int PMPI_Win_set_errhandler(MPI_Win win, MPI_Errhandler errhandler)
{
  return VT()->PMPI_Win_set_errhandler(win, errhandler);
}

int PMPI_Win_set_info(MPI_Win win, MPI_Info info)
{
  return VT()->PMPI_Win_set_info(win, info);
}

int PMPI_Win_set_name(MPI_Win win, const char *win_name)
{
  return VT()->PMPI_Win_set_name(win, win_name);
}

int PMPI_Win_shared_query(MPI_Win win, int rank, MPI_Aint *size,
                          int *disp_unit, void *baseptr)
{
  return VT()->PMPI_Win_shared_query(win, rank, size, disp_unit, baseptr);
}

int PMPI_Win_shared_query_c(MPI_Win win, int rank, MPI_Aint *size,
                            MPI_Aint *disp_unit, void *baseptr)
{
  return VT()->PMPI_Win_shared_query_c(win, rank, size, disp_unit, baseptr);
}

int PMPI_Win_start(MPI_Group group, int assert, MPI_Win win)
{
  return VT()->PMPI_Win_start(group, assert, win);
}

int PMPI_Win_sync(MPI_Win win) { return VT()->PMPI_Win_sync(win); }

int PMPI_Win_test(MPI_Win win, int *flag)
{
  return VT()->PMPI_Win_test(win, flag);
}

int PMPI_Win_unlock(int rank, MPI_Win win)
{
  return VT()->PMPI_Win_unlock(rank, win);
}

int PMPI_Win_unlock_all(MPI_Win win) { return VT()->PMPI_Win_unlock_all(win); }

int PMPI_Win_wait(MPI_Win win) { return VT()->PMPI_Win_wait(win); }

MPI_Aint PMPI_Aint_add(MPI_Aint base, MPI_Aint disp)
{
  return VT()->PMPI_Aint_add(base, disp);
}

MPI_Aint PMPI_Aint_diff(MPI_Aint addr1, MPI_Aint addr2)
{
  return VT()->PMPI_Aint_diff(addr1, addr2);
}

double PMPI_Wtick(void) { return VT()->PMPI_Wtick(); }

double PMPI_Wtime(void) { return VT()->PMPI_Wtime(); }

MPI_Comm PMPI_Comm_fromint(int comm) { return VT()->PMPI_Comm_fromint(comm); }

int PMPI_Comm_toint(MPI_Comm comm) { return VT()->PMPI_Comm_toint(comm); }

MPI_Errhandler PMPI_Errhandler_fromint(int errhandler)
{
  return VT()->PMPI_Errhandler_fromint(errhandler);
}

int PMPI_Errhandler_toint(MPI_Errhandler errhandler)
{
  return VT()->PMPI_Errhandler_toint(errhandler);
}

MPI_File PMPI_File_fromint(int file) { return VT()->PMPI_File_fromint(file); }

int PMPI_File_toint(MPI_File file) { return VT()->PMPI_File_toint(file); }

MPI_Group PMPI_Group_fromint(int group)
{
  return VT()->PMPI_Group_fromint(group);
}

int PMPI_Group_toint(MPI_Group group) { return VT()->PMPI_Group_toint(group); }

MPI_Info PMPI_Info_fromint(int info) { return VT()->PMPI_Info_fromint(info); }

int PMPI_Info_toint(MPI_Info info) { return VT()->PMPI_Info_toint(info); }

MPI_Message PMPI_Message_fromint(int message)
{
  return VT()->PMPI_Message_fromint(message);
}

int PMPI_Message_toint(MPI_Message message)
{
  return VT()->PMPI_Message_toint(message);
}

MPI_Op PMPI_Op_fromint(int op) { return VT()->PMPI_Op_fromint(op); }

int PMPI_Op_toint(MPI_Op op) { return VT()->PMPI_Op_toint(op); }

MPI_Request PMPI_Request_fromint(int request)
{
  return VT()->PMPI_Request_fromint(request);
}

int PMPI_Request_toint(MPI_Request request)
{
  return VT()->PMPI_Request_toint(request);
}

MPI_Session PMPI_Session_fromint(int session)
{
  return VT()->PMPI_Session_fromint(session);
}

int PMPI_Session_toint(MPI_Session session)
{
  return VT()->PMPI_Session_toint(session);
}

MPI_Datatype PMPI_Type_fromint(int datatype)
{
  return VT()->PMPI_Type_fromint(datatype);
}

int PMPI_Type_toint(MPI_Datatype datatype)
{
  return VT()->PMPI_Type_toint(datatype);
}

MPI_Win PMPI_Win_fromint(int win) { return VT()->PMPI_Win_fromint(win); }

int PMPI_Win_toint(MPI_Win win) { return VT()->PMPI_Win_toint(win); }

int PMPI_T_category_changed(int *update_number)
{
  return VT()->PMPI_T_category_changed(update_number);
}

int PMPI_T_category_get_categories(int cat_index, int len, int indices[])
{
  return VT()->PMPI_T_category_get_categories(cat_index, len, indices);
}

int PMPI_T_category_get_cvars(int cat_index, int len, int indices[])
{
  return VT()->PMPI_T_category_get_cvars(cat_index, len, indices);
}

int PMPI_T_category_get_events(int cat_index, int len, int indices[])
{
  return VT()->PMPI_T_category_get_events(cat_index, len, indices);
}

int PMPI_T_category_get_index(const char *name, int *cat_index)
{
  return VT()->PMPI_T_category_get_index(name, cat_index);
}

int PMPI_T_category_get_info(int cat_index, char *name, int *name_len,
                             char *desc, int *desc_len, int *num_cvars,
                             int *num_pvars, int *num_categories)
{
  return VT()->PMPI_T_category_get_info(cat_index, name, name_len, desc,
                                        desc_len, num_cvars, num_pvars,
                                        num_categories);
}

int PMPI_T_category_get_num(int *num_cat)
{
  return VT()->PMPI_T_category_get_num(num_cat);
}

int PMPI_T_category_get_num_events(int cat_index, int *num_events)
{
  return VT()->PMPI_T_category_get_num_events(cat_index, num_events);
}

int PMPI_T_category_get_pvars(int cat_index, int len, int indices[])
{
  return VT()->PMPI_T_category_get_pvars(cat_index, len, indices);
}

int PMPI_T_cvar_get_index(const char *name, int *cvar_index)
{
  return VT()->PMPI_T_cvar_get_index(name, cvar_index);
}

int PMPI_T_cvar_get_info(int cvar_index, char *name, int *name_len,
                         int *verbosity, MPI_Datatype *datatype,
                         MPI_T_enum *enumtype, char *desc, int *desc_len,
                         int *bind, int *scope)
{
  return VT()->PMPI_T_cvar_get_info(cvar_index, name, name_len, verbosity,
                                    datatype, enumtype, desc, desc_len, bind,
                                    scope);
}

int PMPI_T_cvar_get_num(int *num_cvar)
{
  return VT()->PMPI_T_cvar_get_num(num_cvar);
}

int PMPI_T_cvar_handle_alloc(int cvar_index, void *obj_handle,
                             MPI_T_cvar_handle *handle, int *count)
{
  return VT()->PMPI_T_cvar_handle_alloc(cvar_index, obj_handle, handle, count);
}

int PMPI_T_cvar_handle_free(MPI_T_cvar_handle *handle)
{
  return VT()->PMPI_T_cvar_handle_free(handle);
}

int PMPI_T_cvar_read(MPI_T_cvar_handle handle, void *buf)
{
  return VT()->PMPI_T_cvar_read(handle, buf);
}

int PMPI_T_cvar_write(MPI_T_cvar_handle handle, const void *buf)
{
  return VT()->PMPI_T_cvar_write(handle, buf);
}

int PMPI_T_enum_get_info(MPI_T_enum enumtype, int *num, char *name,
                         int *name_len)
{
  return VT()->PMPI_T_enum_get_info(enumtype, num, name, name_len);
}

int PMPI_T_enum_get_item(MPI_T_enum enumtype, int indx, int *value, char *name,
                         int *name_len)
{
  return VT()->PMPI_T_enum_get_item(enumtype, indx, value, name, name_len);
}

int PMPI_T_event_callback_get_info(MPI_T_event_registration event_registration,
    MPI_T_cb_safety cb_safety, MPI_Info *info_used)
{
  return VT()->PMPI_T_event_callback_get_info(event_registration,
                                              (MPIABI_T_cb_safety)cb_safety,
                                              info_used);
}

int PMPI_T_event_callback_set_info(MPI_T_event_registration event_registration,
    MPI_T_cb_safety cb_safety, MPI_Info info)
{
  return VT()->PMPI_T_event_callback_set_info(event_registration,
                                              (MPIABI_T_cb_safety)cb_safety,
                                              info);
}

int PMPI_T_event_copy(MPI_T_event_instance event_instance, void *buffer)
{
  return VT()->PMPI_T_event_copy(event_instance, buffer);
}

int PMPI_T_event_get_index(const char *name, int *event_index)
{
  return VT()->PMPI_T_event_get_index(name, event_index);
}

int PMPI_T_event_get_info(int event_index, char *name, int *name_len,
                          int *verbosity, MPI_Datatype array_of_datatypes[],
                          MPI_Aint array_of_displacements[], int *num_elements,
                          MPI_T_enum *enumtype, MPI_Info *info, char *desc,
                          int *desc_len, int *bind)
{
  return VT()->PMPI_T_event_get_info(event_index, name, name_len, verbosity,
                                     array_of_datatypes,
                                     array_of_displacements, num_elements,
                                     enumtype, info, desc, desc_len, bind);
}

int PMPI_T_event_get_num(int *num_events)
{
  return VT()->PMPI_T_event_get_num(num_events);
}

int PMPI_T_event_get_source(MPI_T_event_instance event_instance,
                            int *source_index)
{
  return VT()->PMPI_T_event_get_source(event_instance, source_index);
}

int PMPI_T_event_get_timestamp(MPI_T_event_instance event_instance,
                               MPI_Count *event_timestamp)
{
  return VT()->PMPI_T_event_get_timestamp(event_instance, event_timestamp);
}

int PMPI_T_event_handle_alloc(int event_index, void *obj_handle, MPI_Info info,
                              MPI_T_event_registration *event_registration)
{
  return VT()->PMPI_T_event_handle_alloc(event_index, obj_handle, info,
                                         event_registration);
}

int PMPI_T_event_handle_free(MPI_T_event_registration event_registration,
                             void *user_data,
                             MPI_T_event_free_cb_function free_cb_function)
{
  return VT()->PMPI_T_event_handle_free(event_registration, user_data,
      (MPIABI_T_event_free_cb_function *)free_cb_function);
}

int PMPI_T_event_handle_get_info(MPI_T_event_registration event_registration,
                                 MPI_Info *info_used)
{
  return VT()->PMPI_T_event_handle_get_info(event_registration, info_used);
}

int PMPI_T_event_handle_set_info(MPI_T_event_registration event_registration,
                                 MPI_Info info)
{
  return VT()->PMPI_T_event_handle_set_info(event_registration, info);
}

int PMPI_T_event_read(MPI_T_event_instance event_instance, int element_index,
                      void *buffer)
{
  return VT()->PMPI_T_event_read(event_instance, element_index, buffer);
}

int PMPI_T_event_register_callback(MPI_T_event_registration event_registration,
    MPI_T_cb_safety cb_safety, MPI_Info info, void *user_data,
    MPI_T_event_cb_function event_cb_function)
{
  return VT()->PMPI_T_event_register_callback(event_registration,
      (MPIABI_T_cb_safety)cb_safety, info, user_data,
      (MPIABI_T_event_cb_function *)event_cb_function);
}

int PMPI_T_event_set_dropped_handler(
    MPI_T_event_registration event_registration,
    MPI_T_event_dropped_cb_function dropped_cb_function)
{
  return VT()->PMPI_T_event_set_dropped_handler(event_registration,
      (MPIABI_T_event_dropped_cb_function *)dropped_cb_function);
}

int PMPI_T_finalize(void) { return VT()->PMPI_T_finalize(); }

int PMPI_T_init_thread(int required, int *provided)
{
  return VT()->PMPI_T_init_thread(required, provided);
}

int PMPI_T_pvar_get_index(const char *name, int var_class, int *pvar_index)
{
  return VT()->PMPI_T_pvar_get_index(name, var_class, pvar_index);
}

int PMPI_T_pvar_get_info(int pvar_index, char *name, int *name_len,
                         int *verbosity, int *var_class,
                         MPI_Datatype *datatype, MPI_T_enum *enumtype,
                         char *desc, int *desc_len, int *bind, int *readonly,
                         int *continuous, int *atomic)
{
  return VT()->PMPI_T_pvar_get_info(pvar_index, name, name_len, verbosity,
                                    var_class, datatype, enumtype, desc,
                                    desc_len, bind, readonly, continuous,
                                    atomic);
}

int PMPI_T_pvar_get_num(int *num_pvar)
{
  return VT()->PMPI_T_pvar_get_num(num_pvar);
}

int PMPI_T_pvar_handle_alloc(MPI_T_pvar_session session, int pvar_index,
                             void *obj_handle, MPI_T_pvar_handle *handle,
                             int *count)
{
  return VT()->PMPI_T_pvar_handle_alloc(session, pvar_index, obj_handle,
                                        handle, count);
}

int PMPI_T_pvar_handle_free(MPI_T_pvar_session session,
                            MPI_T_pvar_handle *handle)
{
  return VT()->PMPI_T_pvar_handle_free(session, handle);
}

int PMPI_T_pvar_read(MPI_T_pvar_session session, MPI_T_pvar_handle handle,
                     void *buf)
{
  return VT()->PMPI_T_pvar_read(session, handle, buf);
}

int PMPI_T_pvar_readreset(MPI_T_pvar_session session, MPI_T_pvar_handle handle,
                          void *buf)
{
  return VT()->PMPI_T_pvar_readreset(session, handle, buf);
}

int PMPI_T_pvar_reset(MPI_T_pvar_session session, MPI_T_pvar_handle handle)
{
  return VT()->PMPI_T_pvar_reset(session, handle);
}

int PMPI_T_pvar_session_create(MPI_T_pvar_session *session)
{
  return VT()->PMPI_T_pvar_session_create(session);
}

int PMPI_T_pvar_session_free(MPI_T_pvar_session *session)
{
  return VT()->PMPI_T_pvar_session_free(session);
}

int PMPI_T_pvar_start(MPI_T_pvar_session session, MPI_T_pvar_handle handle)
{
  return VT()->PMPI_T_pvar_start(session, handle);
}

int PMPI_T_pvar_stop(MPI_T_pvar_session session, MPI_T_pvar_handle handle)
{
  return VT()->PMPI_T_pvar_stop(session, handle);
}

int PMPI_T_pvar_write(MPI_T_pvar_session session, MPI_T_pvar_handle handle,
                      const void *buf)
{
  return VT()->PMPI_T_pvar_write(session, handle, buf);
}

int PMPI_T_source_get_info(int source_index, char *name, int *name_len,
                           char *desc, int *desc_len,
                           MPI_T_source_order *ordering,
                           MPI_Count *ticks_per_second, MPI_Count *max_ticks,
                           MPI_Info *info)
{
  return VT()->PMPI_T_source_get_info(source_index, name, name_len, desc,
                                      desc_len,
                                      (MPIABI_T_source_order *)ordering,
                                      ticks_per_second, max_ticks, info);
}

int PMPI_T_source_get_num(int *num_sources)
{
  return VT()->PMPI_T_source_get_num(num_sources);
}

int PMPI_T_source_get_timestamp(int source_index, MPI_Count *timestamp)
{
  return VT()->PMPI_T_source_get_timestamp(source_index, timestamp);
}

int PMPI_Status_f2c(const MPI_Fint *f_status, MPI_Status *c_status)
{
  return VT()->PMPI_Status_f2c(f_status, c_status);
}

int PMPI_Status_c2f(const MPI_Status *c_status, MPI_Fint *f_status)
{
  return VT()->PMPI_Status_c2f(c_status, f_status);
}

int PMPI_Status_f082c(const MPI_F08_Status *f08_status, MPI_Status *c_status)
{
  return VT()->PMPI_Status_f082c(f08_status, c_status);
}

int PMPI_Status_c2f08(const MPI_Status *c_status, MPI_F08_Status *f08_status)
{
  return VT()->PMPI_Status_c2f08(c_status, f08_status);
}

int MPI_Status_f2c(const MPI_Fint *f_status, MPI_Status *c_status)
{
  return VT()->MPI_Status_f2c(f_status, c_status);
}

int MPI_Status_c2f(const MPI_Status *c_status, MPI_Fint *f_status)
{
  return VT()->MPI_Status_c2f(c_status, f_status);
}

int MPI_Status_f082c(const MPI_F08_Status *f08_status, MPI_Status *c_status)
{
  return VT()->MPI_Status_f082c(f08_status, c_status);
}

int MPI_Status_c2f08(const MPI_Status *c_status, MPI_F08_Status *f08_status)
{
  return VT()->MPI_Status_c2f08(c_status, f08_status);
}

MPI_Comm PMPI_Comm_f2c(MPI_Fint comm) { return VT()->PMPI_Comm_f2c(comm); }

MPI_Fint PMPI_Comm_c2f(MPI_Comm comm) { return VT()->PMPI_Comm_c2f(comm); }

MPI_Errhandler PMPI_Errhandler_f2c(MPI_Fint errhandler)
{
  return VT()->PMPI_Errhandler_f2c(errhandler);
}

MPI_Fint PMPI_Errhandler_c2f(MPI_Errhandler errhandler)
{
  return VT()->PMPI_Errhandler_c2f(errhandler);
}

MPI_File PMPI_File_f2c(MPI_Fint file) { return VT()->PMPI_File_f2c(file); }

MPI_Fint PMPI_File_c2f(MPI_File file) { return VT()->PMPI_File_c2f(file); }

MPI_Group PMPI_Group_f2c(MPI_Fint group)
{
  return VT()->PMPI_Group_f2c(group);
}

MPI_Fint PMPI_Group_c2f(MPI_Group group)
{
  return VT()->PMPI_Group_c2f(group);
}

MPI_Info PMPI_Info_f2c(MPI_Fint info) { return VT()->PMPI_Info_f2c(info); }

MPI_Fint PMPI_Info_c2f(MPI_Info info) { return VT()->PMPI_Info_c2f(info); }

MPI_Message PMPI_Message_f2c(MPI_Fint message)
{
  return VT()->PMPI_Message_f2c(message);
}

MPI_Fint PMPI_Message_c2f(MPI_Message message)
{
  return VT()->PMPI_Message_c2f(message);
}

MPI_Op PMPI_Op_f2c(MPI_Fint op) { return VT()->PMPI_Op_f2c(op); }

MPI_Fint PMPI_Op_c2f(MPI_Op op) { return VT()->PMPI_Op_c2f(op); }

MPI_Request PMPI_Request_f2c(MPI_Fint request)
{
  return VT()->PMPI_Request_f2c(request);
}

MPI_Fint PMPI_Request_c2f(MPI_Request request)
{
  return VT()->PMPI_Request_c2f(request);
}

MPI_Session PMPI_Session_f2c(MPI_Fint session)
{
  return VT()->PMPI_Session_f2c(session);
}

MPI_Fint PMPI_Session_c2f(MPI_Session session)
{
  return VT()->PMPI_Session_c2f(session);
}

MPI_Datatype PMPI_Type_f2c(MPI_Fint datatype)
{
  return VT()->PMPI_Type_f2c(datatype);
}

MPI_Fint PMPI_Type_c2f(MPI_Datatype datatype)
{
  return VT()->PMPI_Type_c2f(datatype);
}

MPI_Win PMPI_Win_f2c(MPI_Fint win) { return VT()->PMPI_Win_f2c(win); }

MPI_Fint PMPI_Win_c2f(MPI_Win win) { return VT()->PMPI_Win_c2f(win); }

MPI_Comm MPI_Comm_f2c(MPI_Fint comm) { return VT()->MPI_Comm_f2c(comm); }

MPI_Fint MPI_Comm_c2f(MPI_Comm comm) { return VT()->MPI_Comm_c2f(comm); }

MPI_Errhandler MPI_Errhandler_f2c(MPI_Fint errhandler)
{
  return VT()->MPI_Errhandler_f2c(errhandler);
}

MPI_Fint MPI_Errhandler_c2f(MPI_Errhandler errhandler)
{
  return VT()->MPI_Errhandler_c2f(errhandler);
}

MPI_File MPI_File_f2c(MPI_Fint file) { return VT()->MPI_File_f2c(file); }

MPI_Fint MPI_File_c2f(MPI_File file) { return VT()->MPI_File_c2f(file); }

MPI_Group MPI_Group_f2c(MPI_Fint group) { return VT()->MPI_Group_f2c(group); }

MPI_Fint MPI_Group_c2f(MPI_Group group) { return VT()->MPI_Group_c2f(group); }

MPI_Info MPI_Info_f2c(MPI_Fint info) { return VT()->MPI_Info_f2c(info); }

MPI_Fint MPI_Info_c2f(MPI_Info info) { return VT()->MPI_Info_c2f(info); }

MPI_Message MPI_Message_f2c(MPI_Fint message)
{
  return VT()->MPI_Message_f2c(message);
}

MPI_Fint MPI_Message_c2f(MPI_Message message)
{
  return VT()->MPI_Message_c2f(message);
}

MPI_Op MPI_Op_f2c(MPI_Fint op) { return VT()->MPI_Op_f2c(op); }

MPI_Fint MPI_Op_c2f(MPI_Op op) { return VT()->MPI_Op_c2f(op); }

MPI_Request MPI_Request_f2c(MPI_Fint request)
{
  return VT()->MPI_Request_f2c(request);
}

MPI_Fint MPI_Request_c2f(MPI_Request request)
{
  return VT()->MPI_Request_c2f(request);
}

MPI_Session MPI_Session_f2c(MPI_Fint session)
{
  return VT()->MPI_Session_f2c(session);
}

MPI_Fint MPI_Session_c2f(MPI_Session session)
{
  return VT()->MPI_Session_c2f(session);
}

MPI_Datatype MPI_Type_f2c(MPI_Fint datatype)
{
  return VT()->MPI_Type_f2c(datatype);
}

MPI_Fint MPI_Type_c2f(MPI_Datatype datatype)
{
  return VT()->MPI_Type_c2f(datatype);
}

MPI_Win MPI_Win_f2c(MPI_Fint win) { return VT()->MPI_Win_f2c(win); }

MPI_Fint MPI_Win_c2f(MPI_Win win) { return VT()->MPI_Win_c2f(win); }
