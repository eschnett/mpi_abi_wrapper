/* GENERATED FILE -- do not edit by hand.
 *
 * Produced by dev/generate.py. The vtable: the only thing libmpi_abi and
 * libmpiwrapper share.
 *
 * Both sides are generated from the same slot list, so they cannot disagree
 * about the layout by accident -- but they are built at different times
 * against different MPIs, so they can disagree by *version*, and
 * MPIWRAPPER_LAYOUT_HASH is what turns that into a clean failure instead of a
 * call through a shifted slot.
 */

#ifndef MPIWRAPPER_VTABLE_H
#define MPIWRAPPER_VTABLE_H

#include <stddef.h>
#include <stdint.h>

/* The MPIABI_ view of the ABI: every typedef, macro and enumerator renamed, no
 * prototypes.
 *
 * The renaming touches typedef names and macro/enumerator names only. Struct
 * *tags* are left alone, so `MPIABI_Comm` and the ABI header's own `MPI_Comm`
 * are both `struct MPI_ABI_Comm *` -- the same type, not two incompatible
 * ones. That is what lets the ABI side forward its arguments into a slot
 * without a single cast (see gen/mpi_abi/entrypoints.c). Struct *members* are
 * left alone too: MPIABI_Status has fields MPI_SOURCE, MPI_TAG, MPI_ERROR,
 * MPI_internal, exactly as the ABI header does.
 */
#include "mpiabi.h"

/* Computed by dev/layout_hash.py's definition over the slot list below:
 * comments removed, whitespace collapsed, FNV-1a/32 of the result. Any edit
 * that changes the struct changes this value, and `ctest -R layout-hash` fails
 * until a regeneration updates it.
 */
#define MPIWRAPPER_LAYOUT_HASH 0x2bef627fu

/* One slot per *forwarded* ABI entry point, so 1366 of them: MPI_X and
 * PMPI_X get their own, and each leads to a wrapper body that calls the
 * implementation's correspondingly-shifted name. That is fewer than the 1376
 * names libmpi_abi exports, because the entry points MPI-3.0 deleted are
 * answered on the ABI side in terms of their replacements and reach no slot at
 * all (NOTES.md #3); gen/report.txt freezes both counts.
 *
 * Routing MPI_X and PMPI_X to a single slot would be cheaper, but then an
 * application calling PMPI_Send to bypass profiling would still be seen by a
 * tool interposed between the wrapper and the implementation -- it would have
 * bypassed the ABI-level profiling layer only. Keeping them distinct also
 * makes the ledger 1:1 rather than 2:1, so "each forwarded entry point has
 * exactly one slot and one body" is a uniform invariant with no special case.
 *
 * Both names are always available to link against, though not in the shape
 * NOTES.md #2 originally recorded from Linux: on macOS the conda-forge MPICH
 * 4.3.1 build keeps every PMPI_* in a separate libpmpi.dylib (libmpi.dylib has
 * a strong MPI_Send and no PMPI_ symbols at all), and Open MPI 5.0.10 has
 * MPI_Send and PMPI_Send as two distinct definitions rather than an alias
 * pair. Either way both names resolve as long as the wrapper links what mpicc
 * links, which is what src/mpiwrapper/ does.
 */
struct mpiwrapper_vtable {
  int (*MPI_Abi_get_fortran_booleans)(int, void *, void *, int *);
  int (*MPI_Abi_get_fortran_info)(MPIABI_Info *);
  int (*MPI_Abi_get_info)(MPIABI_Info *);
  int (*MPI_Abi_get_version)(int *, int *);
  int (*MPI_Abi_set_fortran_booleans)(int, void *, void *);
  int (*MPI_Abi_set_fortran_info)(MPIABI_Info);
  int (*MPI_Abort)(MPIABI_Comm, int);
  int (*MPI_Accumulate)(const void *, int, MPIABI_Datatype, int, MPIABI_Aint,
                        int, MPIABI_Datatype, MPIABI_Op, MPIABI_Win);
  int (*MPI_Accumulate_c)(const void *, MPIABI_Count, MPIABI_Datatype, int,
                          MPIABI_Aint, MPIABI_Count, MPIABI_Datatype,
                          MPIABI_Op, MPIABI_Win);
  int (*MPI_Add_error_class)(int *);
  int (*MPI_Add_error_code)(int, int *);
  int (*MPI_Add_error_string)(int, const char *);
  int (*MPI_Allgather)(const void *, int, MPIABI_Datatype, void *, int,
                       MPIABI_Datatype, MPIABI_Comm);
  int (*MPI_Allgather_c)(const void *, MPIABI_Count, MPIABI_Datatype, void *,
                         MPIABI_Count, MPIABI_Datatype, MPIABI_Comm);
  int (*MPI_Allgather_init)(const void *, int, MPIABI_Datatype, void *, int,
                            MPIABI_Datatype, MPIABI_Comm, MPIABI_Info,
                            MPIABI_Request *);
  int (*MPI_Allgather_init_c)(const void *, MPIABI_Count, MPIABI_Datatype,
                              void *, MPIABI_Count, MPIABI_Datatype,
                              MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*MPI_Allgatherv)(const void *, int, MPIABI_Datatype, void *,
                        const int[], const int[], MPIABI_Datatype,
                        MPIABI_Comm);
  int (*MPI_Allgatherv_c)(const void *, MPIABI_Count, MPIABI_Datatype, void *,
                          const MPIABI_Count[], const MPIABI_Aint[],
                          MPIABI_Datatype, MPIABI_Comm);
  int (*MPI_Allgatherv_init)(const void *, int, MPIABI_Datatype, void *,
                             const int[], const int[], MPIABI_Datatype,
                             MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*MPI_Allgatherv_init_c)(const void *, MPIABI_Count, MPIABI_Datatype,
                               void *, const MPIABI_Count[],
                               const MPIABI_Aint[], MPIABI_Datatype,
                               MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*MPI_Alloc_mem)(MPIABI_Aint, MPIABI_Info, void *);
  int (*MPI_Allreduce)(const void *, void *, int, MPIABI_Datatype, MPIABI_Op,
                       MPIABI_Comm);
  int (*MPI_Allreduce_c)(const void *, void *, MPIABI_Count, MPIABI_Datatype,
                         MPIABI_Op, MPIABI_Comm);
  int (*MPI_Allreduce_init)(const void *, void *, int, MPIABI_Datatype,
                            MPIABI_Op, MPIABI_Comm, MPIABI_Info,
                            MPIABI_Request *);
  int (*MPI_Allreduce_init_c)(const void *, void *, MPIABI_Count,
                              MPIABI_Datatype, MPIABI_Op, MPIABI_Comm,
                              MPIABI_Info, MPIABI_Request *);
  int (*MPI_Alltoall)(const void *, int, MPIABI_Datatype, void *, int,
                      MPIABI_Datatype, MPIABI_Comm);
  int (*MPI_Alltoall_c)(const void *, MPIABI_Count, MPIABI_Datatype, void *,
                        MPIABI_Count, MPIABI_Datatype, MPIABI_Comm);
  int (*MPI_Alltoall_init)(const void *, int, MPIABI_Datatype, void *, int,
                           MPIABI_Datatype, MPIABI_Comm, MPIABI_Info,
                           MPIABI_Request *);
  int (*MPI_Alltoall_init_c)(const void *, MPIABI_Count, MPIABI_Datatype,
                             void *, MPIABI_Count, MPIABI_Datatype,
                             MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*MPI_Alltoallv)(const void *, const int[], const int[], MPIABI_Datatype,
                       void *, const int[], const int[], MPIABI_Datatype,
                       MPIABI_Comm);
  int (*MPI_Alltoallv_c)(const void *, const MPIABI_Count[],
                         const MPIABI_Aint[], MPIABI_Datatype, void *,
                         const MPIABI_Count[], const MPIABI_Aint[],
                         MPIABI_Datatype, MPIABI_Comm);
  int (*MPI_Alltoallv_init)(const void *, const int[], const int[],
                            MPIABI_Datatype, void *, const int[], const int[],
                            MPIABI_Datatype, MPIABI_Comm, MPIABI_Info,
                            MPIABI_Request *);
  int (*MPI_Alltoallv_init_c)(const void *, const MPIABI_Count[],
                              const MPIABI_Aint[], MPIABI_Datatype, void *,
                              const MPIABI_Count[], const MPIABI_Aint[],
                              MPIABI_Datatype, MPIABI_Comm, MPIABI_Info,
                              MPIABI_Request *);
  int (*MPI_Alltoallw)(const void *, const int[], const int[],
                       const MPIABI_Datatype[], void *, const int[],
                       const int[], const MPIABI_Datatype[], MPIABI_Comm);
  int (*MPI_Alltoallw_c)(const void *, const MPIABI_Count[],
                         const MPIABI_Aint[], const MPIABI_Datatype[], void *,
                         const MPIABI_Count[], const MPIABI_Aint[],
                         const MPIABI_Datatype[], MPIABI_Comm);
  int (*MPI_Alltoallw_init)(const void *, const int[], const int[],
                            const MPIABI_Datatype[], void *, const int[],
                            const int[], const MPIABI_Datatype[], MPIABI_Comm,
                            MPIABI_Info, MPIABI_Request *);
  int (*MPI_Alltoallw_init_c)(const void *, const MPIABI_Count[],
                              const MPIABI_Aint[], const MPIABI_Datatype[],
                              void *, const MPIABI_Count[],
                              const MPIABI_Aint[], const MPIABI_Datatype[],
                              MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*MPI_Barrier)(MPIABI_Comm);
  int (*MPI_Barrier_init)(MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*MPI_Bcast)(void *, int, MPIABI_Datatype, int, MPIABI_Comm);
  int (*MPI_Bcast_c)(void *, MPIABI_Count, MPIABI_Datatype, int, MPIABI_Comm);
  int (*MPI_Bcast_init)(void *, int, MPIABI_Datatype, int, MPIABI_Comm,
                        MPIABI_Info, MPIABI_Request *);
  int (*MPI_Bcast_init_c)(void *, MPIABI_Count, MPIABI_Datatype, int,
                          MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*MPI_Bsend)(const void *, int, MPIABI_Datatype, int, int, MPIABI_Comm);
  int (*MPI_Bsend_c)(const void *, MPIABI_Count, MPIABI_Datatype, int, int,
                     MPIABI_Comm);
  int (*MPI_Bsend_init)(const void *, int, MPIABI_Datatype, int, int,
                        MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Bsend_init_c)(const void *, MPIABI_Count, MPIABI_Datatype, int,
                          int, MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Buffer_attach)(void *, int);
  int (*MPI_Buffer_attach_c)(void *, MPIABI_Count);
  int (*MPI_Buffer_detach)(void *, int *);
  int (*MPI_Buffer_detach_c)(void *, MPIABI_Count *);
  int (*MPI_Buffer_flush)(void);
  int (*MPI_Buffer_iflush)(MPIABI_Request *);
  int (*MPI_Cancel)(MPIABI_Request *);
  int (*MPI_Cart_coords)(MPIABI_Comm, int, int, int[]);
  int (*MPI_Cart_create)(MPIABI_Comm, int, const int[], const int[], int,
                         MPIABI_Comm *);
  int (*MPI_Cart_get)(MPIABI_Comm, int, int[], int[], int[]);
  int (*MPI_Cart_map)(MPIABI_Comm, int, const int[], const int[], int *);
  int (*MPI_Cart_rank)(MPIABI_Comm, const int[], int *);
  int (*MPI_Cart_shift)(MPIABI_Comm, int, int, int *, int *);
  int (*MPI_Cart_sub)(MPIABI_Comm, const int[], MPIABI_Comm *);
  int (*MPI_Cartdim_get)(MPIABI_Comm, int *);
  int (*MPI_Close_port)(const char *);
  int (*MPI_Comm_accept)(const char *, MPIABI_Info, int, MPIABI_Comm,
                         MPIABI_Comm *);
  int (*MPI_Comm_attach_buffer)(MPIABI_Comm, void *, int);
  int (*MPI_Comm_attach_buffer_c)(MPIABI_Comm, void *, MPIABI_Count);
  int (*MPI_Comm_call_errhandler)(MPIABI_Comm, int);
  int (*MPI_Comm_compare)(MPIABI_Comm, MPIABI_Comm, int *);
  int (*MPI_Comm_connect)(const char *, MPIABI_Info, int, MPIABI_Comm,
                          MPIABI_Comm *);
  int (*MPI_Comm_create)(MPIABI_Comm, MPIABI_Group, MPIABI_Comm *);
  int (*MPI_Comm_create_errhandler)(MPIABI_Comm_errhandler_function *,
                                    MPIABI_Errhandler *);
  int (*MPI_Comm_create_from_group)(MPIABI_Group, const char *, MPIABI_Info,
                                    MPIABI_Errhandler, MPIABI_Comm *);
  int (*MPI_Comm_create_group)(MPIABI_Comm, MPIABI_Group, int, MPIABI_Comm *);
  int (*MPI_Comm_create_keyval)(MPIABI_Comm_copy_attr_function *,
                                MPIABI_Comm_delete_attr_function *, int *,
                                void *);
  int (*MPI_Comm_delete_attr)(MPIABI_Comm, int);
  int (*MPI_Comm_detach_buffer)(MPIABI_Comm, void *, int *);
  int (*MPI_Comm_detach_buffer_c)(MPIABI_Comm, void *, MPIABI_Count *);
  int (*MPI_Comm_disconnect)(MPIABI_Comm *);
  int (*MPI_Comm_dup)(MPIABI_Comm, MPIABI_Comm *);
  int (*MPI_Comm_dup_with_info)(MPIABI_Comm, MPIABI_Info, MPIABI_Comm *);
  int (*MPI_Comm_flush_buffer)(MPIABI_Comm);
  int (*MPI_Comm_free)(MPIABI_Comm *);
  int (*MPI_Comm_free_keyval)(int *);
  int (*MPI_Comm_get_attr)(MPIABI_Comm, int, void *, int *);
  int (*MPI_Comm_get_errhandler)(MPIABI_Comm, MPIABI_Errhandler *);
  int (*MPI_Comm_get_info)(MPIABI_Comm, MPIABI_Info *);
  int (*MPI_Comm_get_name)(MPIABI_Comm, char *, int *);
  int (*MPI_Comm_get_parent)(MPIABI_Comm *);
  int (*MPI_Comm_group)(MPIABI_Comm, MPIABI_Group *);
  int (*MPI_Comm_idup)(MPIABI_Comm, MPIABI_Comm *, MPIABI_Request *);
  int (*MPI_Comm_idup_with_info)(MPIABI_Comm, MPIABI_Info, MPIABI_Comm *,
                                 MPIABI_Request *);
  int (*MPI_Comm_iflush_buffer)(MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Comm_join)(int, MPIABI_Comm *);
  int (*MPI_Comm_rank)(MPIABI_Comm, int *);
  int (*MPI_Comm_remote_group)(MPIABI_Comm, MPIABI_Group *);
  int (*MPI_Comm_remote_size)(MPIABI_Comm, int *);
  int (*MPI_Comm_set_attr)(MPIABI_Comm, int, void *);
  int (*MPI_Comm_set_errhandler)(MPIABI_Comm, MPIABI_Errhandler);
  int (*MPI_Comm_set_info)(MPIABI_Comm, MPIABI_Info);
  int (*MPI_Comm_set_name)(MPIABI_Comm, const char *);
  int (*MPI_Comm_size)(MPIABI_Comm, int *);
  int (*MPI_Comm_spawn)(const char *, char *[], int, MPIABI_Info, int,
                        MPIABI_Comm, MPIABI_Comm *, int[]);
  int (*MPI_Comm_spawn_multiple)(int, char *[], char **[], const int[],
                                 const MPIABI_Info[], int, MPIABI_Comm,
                                 MPIABI_Comm *, int[]);
  int (*MPI_Comm_split)(MPIABI_Comm, int, int, MPIABI_Comm *);
  int (*MPI_Comm_split_type)(MPIABI_Comm, int, int, MPIABI_Info,
                             MPIABI_Comm *);
  int (*MPI_Comm_test_inter)(MPIABI_Comm, int *);
  int (*MPI_Compare_and_swap)(const void *, const void *, void *,
                              MPIABI_Datatype, int, MPIABI_Aint, MPIABI_Win);
  int (*MPI_Dims_create)(int, int, int[]);
  int (*MPI_Dist_graph_create)(MPIABI_Comm, int, const int[], const int[],
                               const int[], const int[], MPIABI_Info, int,
                               MPIABI_Comm *);
  int (*MPI_Dist_graph_create_adjacent)(MPIABI_Comm, int, const int[],
                                        const int[], int, const int[],
                                        const int[], MPIABI_Info, int,
                                        MPIABI_Comm *);
  int (*MPI_Dist_graph_neighbors)(MPIABI_Comm, int, int[], int[], int, int[],
                                  int[]);
  int (*MPI_Dist_graph_neighbors_count)(MPIABI_Comm, int *, int *, int *);
  int (*MPI_Errhandler_free)(MPIABI_Errhandler *);
  int (*MPI_Error_class)(int, int *);
  int (*MPI_Error_string)(int, char *, int *);
  int (*MPI_Exscan)(const void *, void *, int, MPIABI_Datatype, MPIABI_Op,
                    MPIABI_Comm);
  int (*MPI_Exscan_c)(const void *, void *, MPIABI_Count, MPIABI_Datatype,
                      MPIABI_Op, MPIABI_Comm);
  int (*MPI_Exscan_init)(const void *, void *, int, MPIABI_Datatype, MPIABI_Op,
                         MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*MPI_Exscan_init_c)(const void *, void *, MPIABI_Count, MPIABI_Datatype,
                           MPIABI_Op, MPIABI_Comm, MPIABI_Info,
                           MPIABI_Request *);
  int (*MPI_Fetch_and_op)(const void *, void *, MPIABI_Datatype, int,
                          MPIABI_Aint, MPIABI_Op, MPIABI_Win);
  int (*MPI_File_call_errhandler)(MPIABI_File, int);
  int (*MPI_File_close)(MPIABI_File *);
  int (*MPI_File_create_errhandler)(MPIABI_File_errhandler_function *,
                                    MPIABI_Errhandler *);
  int (*MPI_File_delete)(const char *, MPIABI_Info);
  int (*MPI_File_get_amode)(MPIABI_File, int *);
  int (*MPI_File_get_atomicity)(MPIABI_File, int *);
  int (*MPI_File_get_byte_offset)(MPIABI_File, MPIABI_Offset, MPIABI_Offset *);
  int (*MPI_File_get_errhandler)(MPIABI_File, MPIABI_Errhandler *);
  int (*MPI_File_get_group)(MPIABI_File, MPIABI_Group *);
  int (*MPI_File_get_info)(MPIABI_File, MPIABI_Info *);
  int (*MPI_File_get_position)(MPIABI_File, MPIABI_Offset *);
  int (*MPI_File_get_position_shared)(MPIABI_File, MPIABI_Offset *);
  int (*MPI_File_get_size)(MPIABI_File, MPIABI_Offset *);
  int (*MPI_File_get_type_extent)(MPIABI_File, MPIABI_Datatype, MPIABI_Aint *);
  int (*MPI_File_get_type_extent_c)(MPIABI_File, MPIABI_Datatype,
                                    MPIABI_Count *);
  int (*MPI_File_get_view)(MPIABI_File, MPIABI_Offset *, MPIABI_Datatype *,
                           MPIABI_Datatype *, char *);
  int (*MPI_File_iread)(MPIABI_File, void *, int, MPIABI_Datatype,
                        MPIABI_Request *);
  int (*MPI_File_iread_c)(MPIABI_File, void *, MPIABI_Count, MPIABI_Datatype,
                          MPIABI_Request *);
  int (*MPI_File_iread_all)(MPIABI_File, void *, int, MPIABI_Datatype,
                            MPIABI_Request *);
  int (*MPI_File_iread_all_c)(MPIABI_File, void *, MPIABI_Count,
                              MPIABI_Datatype, MPIABI_Request *);
  int (*MPI_File_iread_at)(MPIABI_File, MPIABI_Offset, void *, int,
                           MPIABI_Datatype, MPIABI_Request *);
  int (*MPI_File_iread_at_c)(MPIABI_File, MPIABI_Offset, void *, MPIABI_Count,
                             MPIABI_Datatype, MPIABI_Request *);
  int (*MPI_File_iread_at_all)(MPIABI_File, MPIABI_Offset, void *, int,
                               MPIABI_Datatype, MPIABI_Request *);
  int (*MPI_File_iread_at_all_c)(MPIABI_File, MPIABI_Offset, void *,
                                 MPIABI_Count, MPIABI_Datatype,
                                 MPIABI_Request *);
  int (*MPI_File_iread_shared)(MPIABI_File, void *, int, MPIABI_Datatype,
                               MPIABI_Request *);
  int (*MPI_File_iread_shared_c)(MPIABI_File, void *, MPIABI_Count,
                                 MPIABI_Datatype, MPIABI_Request *);
  int (*MPI_File_iwrite)(MPIABI_File, const void *, int, MPIABI_Datatype,
                         MPIABI_Request *);
  int (*MPI_File_iwrite_c)(MPIABI_File, const void *, MPIABI_Count,
                           MPIABI_Datatype, MPIABI_Request *);
  int (*MPI_File_iwrite_all)(MPIABI_File, const void *, int, MPIABI_Datatype,
                             MPIABI_Request *);
  int (*MPI_File_iwrite_all_c)(MPIABI_File, const void *, MPIABI_Count,
                               MPIABI_Datatype, MPIABI_Request *);
  int (*MPI_File_iwrite_at)(MPIABI_File, MPIABI_Offset, const void *, int,
                            MPIABI_Datatype, MPIABI_Request *);
  int (*MPI_File_iwrite_at_c)(MPIABI_File, MPIABI_Offset, const void *,
                              MPIABI_Count, MPIABI_Datatype, MPIABI_Request *);
  int (*MPI_File_iwrite_at_all)(MPIABI_File, MPIABI_Offset, const void *, int,
                                MPIABI_Datatype, MPIABI_Request *);
  int (*MPI_File_iwrite_at_all_c)(MPIABI_File, MPIABI_Offset, const void *,
                                  MPIABI_Count, MPIABI_Datatype,
                                  MPIABI_Request *);
  int (*MPI_File_iwrite_shared)(MPIABI_File, const void *, int,
                                MPIABI_Datatype, MPIABI_Request *);
  int (*MPI_File_iwrite_shared_c)(MPIABI_File, const void *, MPIABI_Count,
                                  MPIABI_Datatype, MPIABI_Request *);
  int (*MPI_File_open)(MPIABI_Comm, const char *, int, MPIABI_Info,
                       MPIABI_File *);
  int (*MPI_File_preallocate)(MPIABI_File, MPIABI_Offset);
  int (*MPI_File_read)(MPIABI_File, void *, int, MPIABI_Datatype,
                       MPIABI_Status *);
  int (*MPI_File_read_c)(MPIABI_File, void *, MPIABI_Count, MPIABI_Datatype,
                         MPIABI_Status *);
  int (*MPI_File_read_all)(MPIABI_File, void *, int, MPIABI_Datatype,
                           MPIABI_Status *);
  int (*MPI_File_read_all_c)(MPIABI_File, void *, MPIABI_Count,
                             MPIABI_Datatype, MPIABI_Status *);
  int (*MPI_File_read_all_begin)(MPIABI_File, void *, int, MPIABI_Datatype);
  int (*MPI_File_read_all_begin_c)(MPIABI_File, void *, MPIABI_Count,
                                   MPIABI_Datatype);
  int (*MPI_File_read_all_end)(MPIABI_File, void *, MPIABI_Status *);
  int (*MPI_File_read_at)(MPIABI_File, MPIABI_Offset, void *, int,
                          MPIABI_Datatype, MPIABI_Status *);
  int (*MPI_File_read_at_c)(MPIABI_File, MPIABI_Offset, void *, MPIABI_Count,
                            MPIABI_Datatype, MPIABI_Status *);
  int (*MPI_File_read_at_all)(MPIABI_File, MPIABI_Offset, void *, int,
                              MPIABI_Datatype, MPIABI_Status *);
  int (*MPI_File_read_at_all_c)(MPIABI_File, MPIABI_Offset, void *,
                                MPIABI_Count, MPIABI_Datatype,
                                MPIABI_Status *);
  int (*MPI_File_read_at_all_begin)(MPIABI_File, MPIABI_Offset, void *, int,
                                    MPIABI_Datatype);
  int (*MPI_File_read_at_all_begin_c)(MPIABI_File, MPIABI_Offset, void *,
                                      MPIABI_Count, MPIABI_Datatype);
  int (*MPI_File_read_at_all_end)(MPIABI_File, void *, MPIABI_Status *);
  int (*MPI_File_read_ordered)(MPIABI_File, void *, int, MPIABI_Datatype,
                               MPIABI_Status *);
  int (*MPI_File_read_ordered_c)(MPIABI_File, void *, MPIABI_Count,
                                 MPIABI_Datatype, MPIABI_Status *);
  int (*MPI_File_read_ordered_begin)(MPIABI_File, void *, int,
                                     MPIABI_Datatype);
  int (*MPI_File_read_ordered_begin_c)(MPIABI_File, void *, MPIABI_Count,
                                       MPIABI_Datatype);
  int (*MPI_File_read_ordered_end)(MPIABI_File, void *, MPIABI_Status *);
  int (*MPI_File_read_shared)(MPIABI_File, void *, int, MPIABI_Datatype,
                              MPIABI_Status *);
  int (*MPI_File_read_shared_c)(MPIABI_File, void *, MPIABI_Count,
                                MPIABI_Datatype, MPIABI_Status *);
  int (*MPI_File_seek)(MPIABI_File, MPIABI_Offset, int);
  int (*MPI_File_seek_shared)(MPIABI_File, MPIABI_Offset, int);
  int (*MPI_File_set_atomicity)(MPIABI_File, int);
  int (*MPI_File_set_errhandler)(MPIABI_File, MPIABI_Errhandler);
  int (*MPI_File_set_info)(MPIABI_File, MPIABI_Info);
  int (*MPI_File_set_size)(MPIABI_File, MPIABI_Offset);
  int (*MPI_File_set_view)(MPIABI_File, MPIABI_Offset, MPIABI_Datatype,
                           MPIABI_Datatype, const char *, MPIABI_Info);
  int (*MPI_File_sync)(MPIABI_File);
  int (*MPI_File_write)(MPIABI_File, const void *, int, MPIABI_Datatype,
                        MPIABI_Status *);
  int (*MPI_File_write_c)(MPIABI_File, const void *, MPIABI_Count,
                          MPIABI_Datatype, MPIABI_Status *);
  int (*MPI_File_write_all)(MPIABI_File, const void *, int, MPIABI_Datatype,
                            MPIABI_Status *);
  int (*MPI_File_write_all_c)(MPIABI_File, const void *, MPIABI_Count,
                              MPIABI_Datatype, MPIABI_Status *);
  int (*MPI_File_write_all_begin)(MPIABI_File, const void *, int,
                                  MPIABI_Datatype);
  int (*MPI_File_write_all_begin_c)(MPIABI_File, const void *, MPIABI_Count,
                                    MPIABI_Datatype);
  int (*MPI_File_write_all_end)(MPIABI_File, const void *, MPIABI_Status *);
  int (*MPI_File_write_at)(MPIABI_File, MPIABI_Offset, const void *, int,
                           MPIABI_Datatype, MPIABI_Status *);
  int (*MPI_File_write_at_c)(MPIABI_File, MPIABI_Offset, const void *,
                             MPIABI_Count, MPIABI_Datatype, MPIABI_Status *);
  int (*MPI_File_write_at_all)(MPIABI_File, MPIABI_Offset, const void *, int,
                               MPIABI_Datatype, MPIABI_Status *);
  int (*MPI_File_write_at_all_c)(MPIABI_File, MPIABI_Offset, const void *,
                                 MPIABI_Count, MPIABI_Datatype,
                                 MPIABI_Status *);
  int (*MPI_File_write_at_all_begin)(MPIABI_File, MPIABI_Offset, const void *,
                                     int, MPIABI_Datatype);
  int (*MPI_File_write_at_all_begin_c)(MPIABI_File, MPIABI_Offset,
                                       const void *, MPIABI_Count,
                                       MPIABI_Datatype);
  int (*MPI_File_write_at_all_end)(MPIABI_File, const void *, MPIABI_Status *);
  int (*MPI_File_write_ordered)(MPIABI_File, const void *, int,
                                MPIABI_Datatype, MPIABI_Status *);
  int (*MPI_File_write_ordered_c)(MPIABI_File, const void *, MPIABI_Count,
                                  MPIABI_Datatype, MPIABI_Status *);
  int (*MPI_File_write_ordered_begin)(MPIABI_File, const void *, int,
                                      MPIABI_Datatype);
  int (*MPI_File_write_ordered_begin_c)(MPIABI_File, const void *,
                                        MPIABI_Count, MPIABI_Datatype);
  int (*MPI_File_write_ordered_end)(MPIABI_File, const void *,
                                    MPIABI_Status *);
  int (*MPI_File_write_shared)(MPIABI_File, const void *, int, MPIABI_Datatype,
                               MPIABI_Status *);
  int (*MPI_File_write_shared_c)(MPIABI_File, const void *, MPIABI_Count,
                                 MPIABI_Datatype, MPIABI_Status *);
  int (*MPI_Finalize)(void);
  int (*MPI_Finalized)(int *);
  int (*MPI_Free_mem)(void *);
  int (*MPI_Gather)(const void *, int, MPIABI_Datatype, void *, int,
                    MPIABI_Datatype, int, MPIABI_Comm);
  int (*MPI_Gather_c)(const void *, MPIABI_Count, MPIABI_Datatype, void *,
                      MPIABI_Count, MPIABI_Datatype, int, MPIABI_Comm);
  int (*MPI_Gather_init)(const void *, int, MPIABI_Datatype, void *, int,
                         MPIABI_Datatype, int, MPIABI_Comm, MPIABI_Info,
                         MPIABI_Request *);
  int (*MPI_Gather_init_c)(const void *, MPIABI_Count, MPIABI_Datatype, void *,
                           MPIABI_Count, MPIABI_Datatype, int, MPIABI_Comm,
                           MPIABI_Info, MPIABI_Request *);
  int (*MPI_Gatherv)(const void *, int, MPIABI_Datatype, void *, const int[],
                     const int[], MPIABI_Datatype, int, MPIABI_Comm);
  int (*MPI_Gatherv_c)(const void *, MPIABI_Count, MPIABI_Datatype, void *,
                       const MPIABI_Count[], const MPIABI_Aint[],
                       MPIABI_Datatype, int, MPIABI_Comm);
  int (*MPI_Gatherv_init)(const void *, int, MPIABI_Datatype, void *,
                          const int[], const int[], MPIABI_Datatype, int,
                          MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*MPI_Gatherv_init_c)(const void *, MPIABI_Count, MPIABI_Datatype,
                            void *, const MPIABI_Count[], const MPIABI_Aint[],
                            MPIABI_Datatype, int, MPIABI_Comm, MPIABI_Info,
                            MPIABI_Request *);
  int (*MPI_Get)(void *, int, MPIABI_Datatype, int, MPIABI_Aint, int,
                 MPIABI_Datatype, MPIABI_Win);
  int (*MPI_Get_c)(void *, MPIABI_Count, MPIABI_Datatype, int, MPIABI_Aint,
                   MPIABI_Count, MPIABI_Datatype, MPIABI_Win);
  int (*MPI_Get_accumulate)(const void *, int, MPIABI_Datatype, void *, int,
                            MPIABI_Datatype, int, MPIABI_Aint, int,
                            MPIABI_Datatype, MPIABI_Op, MPIABI_Win);
  int (*MPI_Get_accumulate_c)(const void *, MPIABI_Count, MPIABI_Datatype,
                              void *, MPIABI_Count, MPIABI_Datatype, int,
                              MPIABI_Aint, MPIABI_Count, MPIABI_Datatype,
                              MPIABI_Op, MPIABI_Win);
  int (*MPI_Get_address)(const void *, MPIABI_Aint *);
  int (*MPI_Get_count)(const MPIABI_Status *, MPIABI_Datatype, int *);
  int (*MPI_Get_count_c)(const MPIABI_Status *, MPIABI_Datatype,
                         MPIABI_Count *);
  int (*MPI_Get_elements)(const MPIABI_Status *, MPIABI_Datatype, int *);
  int (*MPI_Get_elements_c)(const MPIABI_Status *, MPIABI_Datatype,
                            MPIABI_Count *);
  int (*MPI_Get_elements_x)(const MPIABI_Status *, MPIABI_Datatype,
                            MPIABI_Count *);
  int (*MPI_Get_hw_resource_info)(MPIABI_Info *);
  int (*MPI_Get_library_version)(char *, int *);
  int (*MPI_Get_processor_name)(char *, int *);
  int (*MPI_Get_version)(int *, int *);
  int (*MPI_Graph_create)(MPIABI_Comm, int, const int[], const int[], int,
                          MPIABI_Comm *);
  int (*MPI_Graph_get)(MPIABI_Comm, int, int, int[], int[]);
  int (*MPI_Graph_map)(MPIABI_Comm, int, const int[], const int[], int *);
  int (*MPI_Graph_neighbors)(MPIABI_Comm, int, int, int[]);
  int (*MPI_Graph_neighbors_count)(MPIABI_Comm, int, int *);
  int (*MPI_Graphdims_get)(MPIABI_Comm, int *, int *);
  int (*MPI_Grequest_complete)(MPIABI_Request);
  int (*MPI_Grequest_start)(MPIABI_Grequest_query_function *,
                            MPIABI_Grequest_free_function *,
                            MPIABI_Grequest_cancel_function *, void *,
                            MPIABI_Request *);
  int (*MPI_Group_compare)(MPIABI_Group, MPIABI_Group, int *);
  int (*MPI_Group_difference)(MPIABI_Group, MPIABI_Group, MPIABI_Group *);
  int (*MPI_Group_excl)(MPIABI_Group, int, const int[], MPIABI_Group *);
  int (*MPI_Group_free)(MPIABI_Group *);
  int (*MPI_Group_from_session_pset)(MPIABI_Session, const char *,
                                     MPIABI_Group *);
  int (*MPI_Group_incl)(MPIABI_Group, int, const int[], MPIABI_Group *);
  int (*MPI_Group_intersection)(MPIABI_Group, MPIABI_Group, MPIABI_Group *);
  int (*MPI_Group_range_excl)(MPIABI_Group, int, int[][3], MPIABI_Group *);
  int (*MPI_Group_range_incl)(MPIABI_Group, int, int[][3], MPIABI_Group *);
  int (*MPI_Group_rank)(MPIABI_Group, int *);
  int (*MPI_Group_size)(MPIABI_Group, int *);
  int (*MPI_Group_translate_ranks)(MPIABI_Group, int, const int[],
                                   MPIABI_Group, int[]);
  int (*MPI_Group_union)(MPIABI_Group, MPIABI_Group, MPIABI_Group *);
  int (*MPI_Iallgather)(const void *, int, MPIABI_Datatype, void *, int,
                        MPIABI_Datatype, MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Iallgather_c)(const void *, MPIABI_Count, MPIABI_Datatype, void *,
                          MPIABI_Count, MPIABI_Datatype, MPIABI_Comm,
                          MPIABI_Request *);
  int (*MPI_Iallgatherv)(const void *, int, MPIABI_Datatype, void *,
                         const int[], const int[], MPIABI_Datatype,
                         MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Iallgatherv_c)(const void *, MPIABI_Count, MPIABI_Datatype, void *,
                           const MPIABI_Count[], const MPIABI_Aint[],
                           MPIABI_Datatype, MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Iallreduce)(const void *, void *, int, MPIABI_Datatype, MPIABI_Op,
                        MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Iallreduce_c)(const void *, void *, MPIABI_Count, MPIABI_Datatype,
                          MPIABI_Op, MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Ialltoall)(const void *, int, MPIABI_Datatype, void *, int,
                       MPIABI_Datatype, MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Ialltoall_c)(const void *, MPIABI_Count, MPIABI_Datatype, void *,
                         MPIABI_Count, MPIABI_Datatype, MPIABI_Comm,
                         MPIABI_Request *);
  int (*MPI_Ialltoallv)(const void *, const int[], const int[],
                        MPIABI_Datatype, void *, const int[], const int[],
                        MPIABI_Datatype, MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Ialltoallv_c)(const void *, const MPIABI_Count[],
                          const MPIABI_Aint[], MPIABI_Datatype, void *,
                          const MPIABI_Count[], const MPIABI_Aint[],
                          MPIABI_Datatype, MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Ialltoallw)(const void *, const int[], const int[],
                        const MPIABI_Datatype[], void *, const int[],
                        const int[], const MPIABI_Datatype[], MPIABI_Comm,
                        MPIABI_Request *);
  int (*MPI_Ialltoallw_c)(const void *, const MPIABI_Count[],
                          const MPIABI_Aint[], const MPIABI_Datatype[], void *,
                          const MPIABI_Count[], const MPIABI_Aint[],
                          const MPIABI_Datatype[], MPIABI_Comm,
                          MPIABI_Request *);
  int (*MPI_Ibarrier)(MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Ibcast)(void *, int, MPIABI_Datatype, int, MPIABI_Comm,
                    MPIABI_Request *);
  int (*MPI_Ibcast_c)(void *, MPIABI_Count, MPIABI_Datatype, int, MPIABI_Comm,
                      MPIABI_Request *);
  int (*MPI_Ibsend)(const void *, int, MPIABI_Datatype, int, int, MPIABI_Comm,
                    MPIABI_Request *);
  int (*MPI_Ibsend_c)(const void *, MPIABI_Count, MPIABI_Datatype, int, int,
                      MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Iexscan)(const void *, void *, int, MPIABI_Datatype, MPIABI_Op,
                     MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Iexscan_c)(const void *, void *, MPIABI_Count, MPIABI_Datatype,
                       MPIABI_Op, MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Igather)(const void *, int, MPIABI_Datatype, void *, int,
                     MPIABI_Datatype, int, MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Igather_c)(const void *, MPIABI_Count, MPIABI_Datatype, void *,
                       MPIABI_Count, MPIABI_Datatype, int, MPIABI_Comm,
                       MPIABI_Request *);
  int (*MPI_Igatherv)(const void *, int, MPIABI_Datatype, void *, const int[],
                      const int[], MPIABI_Datatype, int, MPIABI_Comm,
                      MPIABI_Request *);
  int (*MPI_Igatherv_c)(const void *, MPIABI_Count, MPIABI_Datatype, void *,
                        const MPIABI_Count[], const MPIABI_Aint[],
                        MPIABI_Datatype, int, MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Improbe)(int, int, MPIABI_Comm, int *, MPIABI_Message *,
                     MPIABI_Status *);
  int (*MPI_Imrecv)(void *, int, MPIABI_Datatype, MPIABI_Message *,
                    MPIABI_Request *);
  int (*MPI_Imrecv_c)(void *, MPIABI_Count, MPIABI_Datatype, MPIABI_Message *,
                      MPIABI_Request *);
  int (*MPI_Ineighbor_allgather)(const void *, int, MPIABI_Datatype, void *,
                                 int, MPIABI_Datatype, MPIABI_Comm,
                                 MPIABI_Request *);
  int (*MPI_Ineighbor_allgather_c)(const void *, MPIABI_Count, MPIABI_Datatype,
                                   void *, MPIABI_Count, MPIABI_Datatype,
                                   MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Ineighbor_allgatherv)(const void *, int, MPIABI_Datatype, void *,
                                  const int[], const int[], MPIABI_Datatype,
                                  MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Ineighbor_allgatherv_c)(const void *, MPIABI_Count,
                                    MPIABI_Datatype, void *,
                                    const MPIABI_Count[], const MPIABI_Aint[],
                                    MPIABI_Datatype, MPIABI_Comm,
                                    MPIABI_Request *);
  int (*MPI_Ineighbor_alltoall)(const void *, int, MPIABI_Datatype, void *,
                                int, MPIABI_Datatype, MPIABI_Comm,
                                MPIABI_Request *);
  int (*MPI_Ineighbor_alltoall_c)(const void *, MPIABI_Count, MPIABI_Datatype,
                                  void *, MPIABI_Count, MPIABI_Datatype,
                                  MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Ineighbor_alltoallv)(const void *, const int[], const int[],
                                 MPIABI_Datatype, void *, const int[],
                                 const int[], MPIABI_Datatype, MPIABI_Comm,
                                 MPIABI_Request *);
  int (*MPI_Ineighbor_alltoallv_c)(const void *, const MPIABI_Count[],
                                   const MPIABI_Aint[], MPIABI_Datatype,
                                   void *, const MPIABI_Count[],
                                   const MPIABI_Aint[], MPIABI_Datatype,
                                   MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Ineighbor_alltoallw)(const void *, const int[],
                                 const MPIABI_Aint[], const MPIABI_Datatype[],
                                 void *, const int[], const MPIABI_Aint[],
                                 const MPIABI_Datatype[], MPIABI_Comm,
                                 MPIABI_Request *);
  int (*MPI_Ineighbor_alltoallw_c)(const void *, const MPIABI_Count[],
                                   const MPIABI_Aint[],
                                   const MPIABI_Datatype[], void *,
                                   const MPIABI_Count[], const MPIABI_Aint[],
                                   const MPIABI_Datatype[], MPIABI_Comm,
                                   MPIABI_Request *);
  int (*MPI_Info_create)(MPIABI_Info *);
  int (*MPI_Info_create_env)(int, char *[], MPIABI_Info *);
  int (*MPI_Info_delete)(MPIABI_Info, const char *);
  int (*MPI_Info_dup)(MPIABI_Info, MPIABI_Info *);
  int (*MPI_Info_free)(MPIABI_Info *);
  int (*MPI_Info_get)(MPIABI_Info, const char *, int, char *, int *);
  int (*MPI_Info_get_nkeys)(MPIABI_Info, int *);
  int (*MPI_Info_get_nthkey)(MPIABI_Info, int, char *);
  int (*MPI_Info_get_string)(MPIABI_Info, const char *, int *, char *, int *);
  int (*MPI_Info_get_valuelen)(MPIABI_Info, const char *, int *, int *);
  int (*MPI_Info_set)(MPIABI_Info, const char *, const char *);
  int (*MPI_Init)(int *, char ***);
  int (*MPI_Init_thread)(int *, char ***, int, int *);
  int (*MPI_Initialized)(int *);
  int (*MPI_Intercomm_create)(MPIABI_Comm, int, MPIABI_Comm, int, int,
                              MPIABI_Comm *);
  int (*MPI_Intercomm_create_from_groups)(MPIABI_Group, int, MPIABI_Group, int,
                                          const char *, MPIABI_Info,
                                          MPIABI_Errhandler, MPIABI_Comm *);
  int (*MPI_Intercomm_merge)(MPIABI_Comm, int, MPIABI_Comm *);
  int (*MPI_Iprobe)(int, int, MPIABI_Comm, int *, MPIABI_Status *);
  int (*MPI_Irecv)(void *, int, MPIABI_Datatype, int, int, MPIABI_Comm,
                   MPIABI_Request *);
  int (*MPI_Irecv_c)(void *, MPIABI_Count, MPIABI_Datatype, int, int,
                     MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Ireduce)(const void *, void *, int, MPIABI_Datatype, MPIABI_Op,
                     int, MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Ireduce_c)(const void *, void *, MPIABI_Count, MPIABI_Datatype,
                       MPIABI_Op, int, MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Ireduce_scatter)(const void *, void *, const int[],
                             MPIABI_Datatype, MPIABI_Op, MPIABI_Comm,
                             MPIABI_Request *);
  int (*MPI_Ireduce_scatter_c)(const void *, void *, const MPIABI_Count[],
                               MPIABI_Datatype, MPIABI_Op, MPIABI_Comm,
                               MPIABI_Request *);
  int (*MPI_Ireduce_scatter_block)(const void *, void *, int, MPIABI_Datatype,
                                   MPIABI_Op, MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Ireduce_scatter_block_c)(const void *, void *, MPIABI_Count,
                                     MPIABI_Datatype, MPIABI_Op, MPIABI_Comm,
                                     MPIABI_Request *);
  int (*MPI_Irsend)(const void *, int, MPIABI_Datatype, int, int, MPIABI_Comm,
                    MPIABI_Request *);
  int (*MPI_Irsend_c)(const void *, MPIABI_Count, MPIABI_Datatype, int, int,
                      MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Is_thread_main)(int *);
  int (*MPI_Iscan)(const void *, void *, int, MPIABI_Datatype, MPIABI_Op,
                   MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Iscan_c)(const void *, void *, MPIABI_Count, MPIABI_Datatype,
                     MPIABI_Op, MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Iscatter)(const void *, int, MPIABI_Datatype, void *, int,
                      MPIABI_Datatype, int, MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Iscatter_c)(const void *, MPIABI_Count, MPIABI_Datatype, void *,
                        MPIABI_Count, MPIABI_Datatype, int, MPIABI_Comm,
                        MPIABI_Request *);
  int (*MPI_Iscatterv)(const void *, const int[], const int[], MPIABI_Datatype,
                       void *, int, MPIABI_Datatype, int, MPIABI_Comm,
                       MPIABI_Request *);
  int (*MPI_Iscatterv_c)(const void *, const MPIABI_Count[],
                         const MPIABI_Aint[], MPIABI_Datatype, void *,
                         MPIABI_Count, MPIABI_Datatype, int, MPIABI_Comm,
                         MPIABI_Request *);
  int (*MPI_Isend)(const void *, int, MPIABI_Datatype, int, int, MPIABI_Comm,
                   MPIABI_Request *);
  int (*MPI_Isend_c)(const void *, MPIABI_Count, MPIABI_Datatype, int, int,
                     MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Isendrecv)(const void *, int, MPIABI_Datatype, int, int, void *,
                       int, MPIABI_Datatype, int, int, MPIABI_Comm,
                       MPIABI_Request *);
  int (*MPI_Isendrecv_c)(const void *, MPIABI_Count, MPIABI_Datatype, int, int,
                         void *, MPIABI_Count, MPIABI_Datatype, int, int,
                         MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Isendrecv_replace)(void *, int, MPIABI_Datatype, int, int, int,
                               int, MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Isendrecv_replace_c)(void *, MPIABI_Count, MPIABI_Datatype, int,
                                 int, int, int, MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Issend)(const void *, int, MPIABI_Datatype, int, int, MPIABI_Comm,
                    MPIABI_Request *);
  int (*MPI_Issend_c)(const void *, MPIABI_Count, MPIABI_Datatype, int, int,
                      MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Lookup_name)(const char *, MPIABI_Info, char *);
  int (*MPI_Mprobe)(int, int, MPIABI_Comm, MPIABI_Message *, MPIABI_Status *);
  int (*MPI_Mrecv)(void *, int, MPIABI_Datatype, MPIABI_Message *,
                   MPIABI_Status *);
  int (*MPI_Mrecv_c)(void *, MPIABI_Count, MPIABI_Datatype, MPIABI_Message *,
                     MPIABI_Status *);
  int (*MPI_Neighbor_allgather)(const void *, int, MPIABI_Datatype, void *,
                                int, MPIABI_Datatype, MPIABI_Comm);
  int (*MPI_Neighbor_allgather_c)(const void *, MPIABI_Count, MPIABI_Datatype,
                                  void *, MPIABI_Count, MPIABI_Datatype,
                                  MPIABI_Comm);
  int (*MPI_Neighbor_allgather_init)(const void *, int, MPIABI_Datatype,
                                     void *, int, MPIABI_Datatype, MPIABI_Comm,
                                     MPIABI_Info, MPIABI_Request *);
  int (*MPI_Neighbor_allgather_init_c)(const void *, MPIABI_Count,
                                       MPIABI_Datatype, void *, MPIABI_Count,
                                       MPIABI_Datatype, MPIABI_Comm,
                                       MPIABI_Info, MPIABI_Request *);
  int (*MPI_Neighbor_allgatherv)(const void *, int, MPIABI_Datatype, void *,
                                 const int[], const int[], MPIABI_Datatype,
                                 MPIABI_Comm);
  int (*MPI_Neighbor_allgatherv_c)(const void *, MPIABI_Count, MPIABI_Datatype,
                                   void *, const MPIABI_Count[],
                                   const MPIABI_Aint[], MPIABI_Datatype,
                                   MPIABI_Comm);
  int (*MPI_Neighbor_allgatherv_init)(const void *, int, MPIABI_Datatype,
                                      void *, const int[], const int[],
                                      MPIABI_Datatype, MPIABI_Comm,
                                      MPIABI_Info, MPIABI_Request *);
  int (*MPI_Neighbor_allgatherv_init_c)(const void *, MPIABI_Count,
                                        MPIABI_Datatype, void *,
                                        const MPIABI_Count[],
                                        const MPIABI_Aint[], MPIABI_Datatype,
                                        MPIABI_Comm, MPIABI_Info,
                                        MPIABI_Request *);
  int (*MPI_Neighbor_alltoall)(const void *, int, MPIABI_Datatype, void *, int,
                               MPIABI_Datatype, MPIABI_Comm);
  int (*MPI_Neighbor_alltoall_c)(const void *, MPIABI_Count, MPIABI_Datatype,
                                 void *, MPIABI_Count, MPIABI_Datatype,
                                 MPIABI_Comm);
  int (*MPI_Neighbor_alltoall_init)(const void *, int, MPIABI_Datatype, void *,
                                    int, MPIABI_Datatype, MPIABI_Comm,
                                    MPIABI_Info, MPIABI_Request *);
  int (*MPI_Neighbor_alltoall_init_c)(const void *, MPIABI_Count,
                                      MPIABI_Datatype, void *, MPIABI_Count,
                                      MPIABI_Datatype, MPIABI_Comm,
                                      MPIABI_Info, MPIABI_Request *);
  int (*MPI_Neighbor_alltoallv)(const void *, const int[], const int[],
                                MPIABI_Datatype, void *, const int[],
                                const int[], MPIABI_Datatype, MPIABI_Comm);
  int (*MPI_Neighbor_alltoallv_c)(const void *, const MPIABI_Count[],
                                  const MPIABI_Aint[], MPIABI_Datatype, void *,
                                  const MPIABI_Count[], const MPIABI_Aint[],
                                  MPIABI_Datatype, MPIABI_Comm);
  int (*MPI_Neighbor_alltoallv_init)(const void *, const int[], const int[],
                                     MPIABI_Datatype, void *, const int[],
                                     const int[], MPIABI_Datatype, MPIABI_Comm,
                                     MPIABI_Info, MPIABI_Request *);
  int (*MPI_Neighbor_alltoallv_init_c)(const void *, const MPIABI_Count[],
                                       const MPIABI_Aint[], MPIABI_Datatype,
                                       void *, const MPIABI_Count[],
                                       const MPIABI_Aint[], MPIABI_Datatype,
                                       MPIABI_Comm, MPIABI_Info,
                                       MPIABI_Request *);
  int (*MPI_Neighbor_alltoallw)(const void *, const int[], const MPIABI_Aint[],
                                const MPIABI_Datatype[], void *, const int[],
                                const MPIABI_Aint[], const MPIABI_Datatype[],
                                MPIABI_Comm);
  int (*MPI_Neighbor_alltoallw_c)(const void *, const MPIABI_Count[],
                                  const MPIABI_Aint[], const MPIABI_Datatype[],
                                  void *, const MPIABI_Count[],
                                  const MPIABI_Aint[], const MPIABI_Datatype[],
                                  MPIABI_Comm);
  int (*MPI_Neighbor_alltoallw_init)(const void *, const int[],
                                     const MPIABI_Aint[],
                                     const MPIABI_Datatype[], void *,
                                     const int[], const MPIABI_Aint[],
                                     const MPIABI_Datatype[], MPIABI_Comm,
                                     MPIABI_Info, MPIABI_Request *);
  int (*MPI_Neighbor_alltoallw_init_c)(const void *, const MPIABI_Count[],
                                       const MPIABI_Aint[],
                                       const MPIABI_Datatype[], void *,
                                       const MPIABI_Count[],
                                       const MPIABI_Aint[],
                                       const MPIABI_Datatype[], MPIABI_Comm,
                                       MPIABI_Info, MPIABI_Request *);
  int (*MPI_Op_commutative)(MPIABI_Op, int *);
  int (*MPI_Op_create)(MPIABI_User_function *, int, MPIABI_Op *);
  int (*MPI_Op_create_c)(MPIABI_User_function_c *, int, MPIABI_Op *);
  int (*MPI_Op_free)(MPIABI_Op *);
  int (*MPI_Open_port)(MPIABI_Info, char *);
  int (*MPI_Pack)(const void *, int, MPIABI_Datatype, void *, int, int *,
                  MPIABI_Comm);
  int (*MPI_Pack_c)(const void *, MPIABI_Count, MPIABI_Datatype, void *,
                    MPIABI_Count, MPIABI_Count *, MPIABI_Comm);
  int (*MPI_Pack_external)(const char *, const void *, int, MPIABI_Datatype,
                           void *, MPIABI_Aint, MPIABI_Aint *);
  int (*MPI_Pack_external_c)(const char *, const void *, MPIABI_Count,
                             MPIABI_Datatype, void *, MPIABI_Count,
                             MPIABI_Count *);
  int (*MPI_Pack_external_size)(const char *, int, MPIABI_Datatype,
                                MPIABI_Aint *);
  int (*MPI_Pack_external_size_c)(const char *, MPIABI_Count, MPIABI_Datatype,
                                  MPIABI_Count *);
  int (*MPI_Pack_size)(int, MPIABI_Datatype, MPIABI_Comm, int *);
  int (*MPI_Pack_size_c)(MPIABI_Count, MPIABI_Datatype, MPIABI_Comm,
                         MPIABI_Count *);
  int (*MPI_Parrived)(MPIABI_Request, int, int *);
  int (*MPI_Pcontrol)(const int, ...);
  int (*MPI_Pready)(int, MPIABI_Request);
  int (*MPI_Pready_list)(int, const int[], MPIABI_Request);
  int (*MPI_Pready_range)(int, int, MPIABI_Request);
  int (*MPI_Precv_init)(void *, int, MPIABI_Count, MPIABI_Datatype, int, int,
                        MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*MPI_Probe)(int, int, MPIABI_Comm, MPIABI_Status *);
  int (*MPI_Psend_init)(const void *, int, MPIABI_Count, MPIABI_Datatype, int,
                        int, MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*MPI_Publish_name)(const char *, MPIABI_Info, const char *);
  int (*MPI_Put)(const void *, int, MPIABI_Datatype, int, MPIABI_Aint, int,
                 MPIABI_Datatype, MPIABI_Win);
  int (*MPI_Put_c)(const void *, MPIABI_Count, MPIABI_Datatype, int,
                   MPIABI_Aint, MPIABI_Count, MPIABI_Datatype, MPIABI_Win);
  int (*MPI_Query_thread)(int *);
  int (*MPI_Raccumulate)(const void *, int, MPIABI_Datatype, int, MPIABI_Aint,
                         int, MPIABI_Datatype, MPIABI_Op, MPIABI_Win,
                         MPIABI_Request *);
  int (*MPI_Raccumulate_c)(const void *, MPIABI_Count, MPIABI_Datatype, int,
                           MPIABI_Aint, MPIABI_Count, MPIABI_Datatype,
                           MPIABI_Op, MPIABI_Win, MPIABI_Request *);
  int (*MPI_Recv)(void *, int, MPIABI_Datatype, int, int, MPIABI_Comm,
                  MPIABI_Status *);
  int (*MPI_Recv_c)(void *, MPIABI_Count, MPIABI_Datatype, int, int,
                    MPIABI_Comm, MPIABI_Status *);
  int (*MPI_Recv_init)(void *, int, MPIABI_Datatype, int, int, MPIABI_Comm,
                       MPIABI_Request *);
  int (*MPI_Recv_init_c)(void *, MPIABI_Count, MPIABI_Datatype, int, int,
                         MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Reduce)(const void *, void *, int, MPIABI_Datatype, MPIABI_Op, int,
                    MPIABI_Comm);
  int (*MPI_Reduce_c)(const void *, void *, MPIABI_Count, MPIABI_Datatype,
                      MPIABI_Op, int, MPIABI_Comm);
  int (*MPI_Reduce_init)(const void *, void *, int, MPIABI_Datatype, MPIABI_Op,
                         int, MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*MPI_Reduce_init_c)(const void *, void *, MPIABI_Count, MPIABI_Datatype,
                           MPIABI_Op, int, MPIABI_Comm, MPIABI_Info,
                           MPIABI_Request *);
  int (*MPI_Reduce_local)(const void *, void *, int, MPIABI_Datatype,
                          MPIABI_Op);
  int (*MPI_Reduce_local_c)(const void *, void *, MPIABI_Count,
                            MPIABI_Datatype, MPIABI_Op);
  int (*MPI_Reduce_scatter)(const void *, void *, const int[], MPIABI_Datatype,
                            MPIABI_Op, MPIABI_Comm);
  int (*MPI_Reduce_scatter_c)(const void *, void *, const MPIABI_Count[],
                              MPIABI_Datatype, MPIABI_Op, MPIABI_Comm);
  int (*MPI_Reduce_scatter_block)(const void *, void *, int, MPIABI_Datatype,
                                  MPIABI_Op, MPIABI_Comm);
  int (*MPI_Reduce_scatter_block_c)(const void *, void *, MPIABI_Count,
                                    MPIABI_Datatype, MPIABI_Op, MPIABI_Comm);
  int (*MPI_Reduce_scatter_block_init)(const void *, void *, int,
                                       MPIABI_Datatype, MPIABI_Op, MPIABI_Comm,
                                       MPIABI_Info, MPIABI_Request *);
  int (*MPI_Reduce_scatter_block_init_c)(const void *, void *, MPIABI_Count,
                                         MPIABI_Datatype, MPIABI_Op,
                                         MPIABI_Comm, MPIABI_Info,
                                         MPIABI_Request *);
  int (*MPI_Reduce_scatter_init)(const void *, void *, const int[],
                                 MPIABI_Datatype, MPIABI_Op, MPIABI_Comm,
                                 MPIABI_Info, MPIABI_Request *);
  int (*MPI_Reduce_scatter_init_c)(const void *, void *, const MPIABI_Count[],
                                   MPIABI_Datatype, MPIABI_Op, MPIABI_Comm,
                                   MPIABI_Info, MPIABI_Request *);
  int (*MPI_Register_datarep)(const char *,
                              MPIABI_Datarep_conversion_function *,
                              MPIABI_Datarep_conversion_function *,
                              MPIABI_Datarep_extent_function *, void *);
  int (*MPI_Register_datarep_c)(const char *,
                                MPIABI_Datarep_conversion_function_c *,
                                MPIABI_Datarep_conversion_function_c *,
                                MPIABI_Datarep_extent_function *, void *);
  int (*MPI_Remove_error_class)(int);
  int (*MPI_Remove_error_code)(int);
  int (*MPI_Remove_error_string)(int);
  int (*MPI_Request_free)(MPIABI_Request *);
  int (*MPI_Request_get_status)(MPIABI_Request, int *, MPIABI_Status *);
  int (*MPI_Request_get_status_all)(int, const MPIABI_Request[], int *,
                                    MPIABI_Status *);
  int (*MPI_Request_get_status_any)(int, const MPIABI_Request[], int *, int *,
                                    MPIABI_Status *);
  int (*MPI_Request_get_status_some)(int, const MPIABI_Request[], int *, int[],
                                     MPIABI_Status *);
  int (*MPI_Rget)(void *, int, MPIABI_Datatype, int, MPIABI_Aint, int,
                  MPIABI_Datatype, MPIABI_Win, MPIABI_Request *);
  int (*MPI_Rget_c)(void *, MPIABI_Count, MPIABI_Datatype, int, MPIABI_Aint,
                    MPIABI_Count, MPIABI_Datatype, MPIABI_Win,
                    MPIABI_Request *);
  int (*MPI_Rget_accumulate)(const void *, int, MPIABI_Datatype, void *, int,
                             MPIABI_Datatype, int, MPIABI_Aint, int,
                             MPIABI_Datatype, MPIABI_Op, MPIABI_Win,
                             MPIABI_Request *);
  int (*MPI_Rget_accumulate_c)(const void *, MPIABI_Count, MPIABI_Datatype,
                               void *, MPIABI_Count, MPIABI_Datatype, int,
                               MPIABI_Aint, MPIABI_Count, MPIABI_Datatype,
                               MPIABI_Op, MPIABI_Win, MPIABI_Request *);
  int (*MPI_Rput)(const void *, int, MPIABI_Datatype, int, MPIABI_Aint, int,
                  MPIABI_Datatype, MPIABI_Win, MPIABI_Request *);
  int (*MPI_Rput_c)(const void *, MPIABI_Count, MPIABI_Datatype, int,
                    MPIABI_Aint, MPIABI_Count, MPIABI_Datatype, MPIABI_Win,
                    MPIABI_Request *);
  int (*MPI_Rsend)(const void *, int, MPIABI_Datatype, int, int, MPIABI_Comm);
  int (*MPI_Rsend_c)(const void *, MPIABI_Count, MPIABI_Datatype, int, int,
                     MPIABI_Comm);
  int (*MPI_Rsend_init)(const void *, int, MPIABI_Datatype, int, int,
                        MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Rsend_init_c)(const void *, MPIABI_Count, MPIABI_Datatype, int,
                          int, MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Scan)(const void *, void *, int, MPIABI_Datatype, MPIABI_Op,
                  MPIABI_Comm);
  int (*MPI_Scan_c)(const void *, void *, MPIABI_Count, MPIABI_Datatype,
                    MPIABI_Op, MPIABI_Comm);
  int (*MPI_Scan_init)(const void *, void *, int, MPIABI_Datatype, MPIABI_Op,
                       MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*MPI_Scan_init_c)(const void *, void *, MPIABI_Count, MPIABI_Datatype,
                         MPIABI_Op, MPIABI_Comm, MPIABI_Info,
                         MPIABI_Request *);
  int (*MPI_Scatter)(const void *, int, MPIABI_Datatype, void *, int,
                     MPIABI_Datatype, int, MPIABI_Comm);
  int (*MPI_Scatter_c)(const void *, MPIABI_Count, MPIABI_Datatype, void *,
                       MPIABI_Count, MPIABI_Datatype, int, MPIABI_Comm);
  int (*MPI_Scatter_init)(const void *, int, MPIABI_Datatype, void *, int,
                          MPIABI_Datatype, int, MPIABI_Comm, MPIABI_Info,
                          MPIABI_Request *);
  int (*MPI_Scatter_init_c)(const void *, MPIABI_Count, MPIABI_Datatype,
                            void *, MPIABI_Count, MPIABI_Datatype, int,
                            MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*MPI_Scatterv)(const void *, const int[], const int[], MPIABI_Datatype,
                      void *, int, MPIABI_Datatype, int, MPIABI_Comm);
  int (*MPI_Scatterv_c)(const void *, const MPIABI_Count[],
                        const MPIABI_Aint[], MPIABI_Datatype, void *,
                        MPIABI_Count, MPIABI_Datatype, int, MPIABI_Comm);
  int (*MPI_Scatterv_init)(const void *, const int[], const int[],
                           MPIABI_Datatype, void *, int, MPIABI_Datatype, int,
                           MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*MPI_Scatterv_init_c)(const void *, const MPIABI_Count[],
                             const MPIABI_Aint[], MPIABI_Datatype, void *,
                             MPIABI_Count, MPIABI_Datatype, int, MPIABI_Comm,
                             MPIABI_Info, MPIABI_Request *);
  int (*MPI_Send)(const void *, int, MPIABI_Datatype, int, int, MPIABI_Comm);
  int (*MPI_Send_c)(const void *, MPIABI_Count, MPIABI_Datatype, int, int,
                    MPIABI_Comm);
  int (*MPI_Send_init)(const void *, int, MPIABI_Datatype, int, int,
                       MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Send_init_c)(const void *, MPIABI_Count, MPIABI_Datatype, int, int,
                         MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Sendrecv)(const void *, int, MPIABI_Datatype, int, int, void *,
                      int, MPIABI_Datatype, int, int, MPIABI_Comm,
                      MPIABI_Status *);
  int (*MPI_Sendrecv_c)(const void *, MPIABI_Count, MPIABI_Datatype, int, int,
                        void *, MPIABI_Count, MPIABI_Datatype, int, int,
                        MPIABI_Comm, MPIABI_Status *);
  int (*MPI_Sendrecv_replace)(void *, int, MPIABI_Datatype, int, int, int, int,
                              MPIABI_Comm, MPIABI_Status *);
  int (*MPI_Sendrecv_replace_c)(void *, MPIABI_Count, MPIABI_Datatype, int,
                                int, int, int, MPIABI_Comm, MPIABI_Status *);
  int (*MPI_Session_attach_buffer)(MPIABI_Session, void *, int);
  int (*MPI_Session_attach_buffer_c)(MPIABI_Session, void *, MPIABI_Count);
  int (*MPI_Session_call_errhandler)(MPIABI_Session, int);
  int (*MPI_Session_create_errhandler)(MPIABI_Session_errhandler_function *,
                                       MPIABI_Errhandler *);
  int (*MPI_Session_detach_buffer)(MPIABI_Session, void *, int *);
  int (*MPI_Session_detach_buffer_c)(MPIABI_Session, void *, MPIABI_Count *);
  int (*MPI_Session_finalize)(MPIABI_Session *);
  int (*MPI_Session_flush_buffer)(MPIABI_Session);
  int (*MPI_Session_get_errhandler)(MPIABI_Session, MPIABI_Errhandler *);
  int (*MPI_Session_get_info)(MPIABI_Session, MPIABI_Info *);
  int (*MPI_Session_get_nth_pset)(MPIABI_Session, MPIABI_Info, int, int *,
                                  char *);
  int (*MPI_Session_get_num_psets)(MPIABI_Session, MPIABI_Info, int *);
  int (*MPI_Session_get_pset_info)(MPIABI_Session, const char *,
                                   MPIABI_Info *);
  int (*MPI_Session_iflush_buffer)(MPIABI_Session, MPIABI_Request *);
  int (*MPI_Session_init)(MPIABI_Info, MPIABI_Errhandler, MPIABI_Session *);
  int (*MPI_Session_set_errhandler)(MPIABI_Session, MPIABI_Errhandler);
  int (*MPI_Ssend)(const void *, int, MPIABI_Datatype, int, int, MPIABI_Comm);
  int (*MPI_Ssend_c)(const void *, MPIABI_Count, MPIABI_Datatype, int, int,
                     MPIABI_Comm);
  int (*MPI_Ssend_init)(const void *, int, MPIABI_Datatype, int, int,
                        MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Ssend_init_c)(const void *, MPIABI_Count, MPIABI_Datatype, int,
                          int, MPIABI_Comm, MPIABI_Request *);
  int (*MPI_Start)(MPIABI_Request *);
  int (*MPI_Startall)(int, MPIABI_Request[]);
  int (*MPI_Status_get_error)(const MPIABI_Status *, int *);
  int (*MPI_Status_get_source)(const MPIABI_Status *, int *);
  int (*MPI_Status_get_tag)(const MPIABI_Status *, int *);
  int (*MPI_Status_set_cancelled)(MPIABI_Status *, int);
  int (*MPI_Status_set_elements)(MPIABI_Status *, MPIABI_Datatype, int);
  int (*MPI_Status_set_elements_c)(MPIABI_Status *, MPIABI_Datatype,
                                   MPIABI_Count);
  int (*MPI_Status_set_elements_x)(MPIABI_Status *, MPIABI_Datatype,
                                   MPIABI_Count);
  int (*MPI_Status_set_error)(MPIABI_Status *, int);
  int (*MPI_Status_set_source)(MPIABI_Status *, int);
  int (*MPI_Status_set_tag)(MPIABI_Status *, int);
  int (*MPI_Test)(MPIABI_Request *, int *, MPIABI_Status *);
  int (*MPI_Test_cancelled)(const MPIABI_Status *, int *);
  int (*MPI_Testall)(int, MPIABI_Request[], int *, MPIABI_Status *);
  int (*MPI_Testany)(int, MPIABI_Request[], int *, int *, MPIABI_Status *);
  int (*MPI_Testsome)(int, MPIABI_Request[], int *, int[], MPIABI_Status *);
  int (*MPI_Topo_test)(MPIABI_Comm, int *);
  int (*MPI_Type_commit)(MPIABI_Datatype *);
  int (*MPI_Type_contiguous)(int, MPIABI_Datatype, MPIABI_Datatype *);
  int (*MPI_Type_contiguous_c)(MPIABI_Count, MPIABI_Datatype,
                               MPIABI_Datatype *);
  int (*MPI_Type_create_darray)(int, int, int, const int[], const int[],
                                const int[], const int[], int, MPIABI_Datatype,
                                MPIABI_Datatype *);
  int (*MPI_Type_create_darray_c)(int, int, int, const MPIABI_Count[],
                                  const int[], const int[], const int[], int,
                                  MPIABI_Datatype, MPIABI_Datatype *);
  int (*MPI_Type_create_f90_complex)(int, int, MPIABI_Datatype *);
  int (*MPI_Type_create_f90_integer)(int, MPIABI_Datatype *);
  int (*MPI_Type_create_f90_real)(int, int, MPIABI_Datatype *);
  int (*MPI_Type_create_hindexed)(int, const int[], const MPIABI_Aint[],
                                  MPIABI_Datatype, MPIABI_Datatype *);
  int (*MPI_Type_create_hindexed_c)(MPIABI_Count, const MPIABI_Count[],
                                    const MPIABI_Count[], MPIABI_Datatype,
                                    MPIABI_Datatype *);
  int (*MPI_Type_create_hindexed_block)(int, int, const MPIABI_Aint[],
                                        MPIABI_Datatype, MPIABI_Datatype *);
  int (*MPI_Type_create_hindexed_block_c)(MPIABI_Count, MPIABI_Count,
                                          const MPIABI_Count[],
                                          MPIABI_Datatype, MPIABI_Datatype *);
  int (*MPI_Type_create_hvector)(int, int, MPIABI_Aint, MPIABI_Datatype,
                                 MPIABI_Datatype *);
  int (*MPI_Type_create_hvector_c)(MPIABI_Count, MPIABI_Count, MPIABI_Count,
                                   MPIABI_Datatype, MPIABI_Datatype *);
  int (*MPI_Type_create_indexed_block)(int, int, const int[], MPIABI_Datatype,
                                       MPIABI_Datatype *);
  int (*MPI_Type_create_indexed_block_c)(MPIABI_Count, MPIABI_Count,
                                         const MPIABI_Count[], MPIABI_Datatype,
                                         MPIABI_Datatype *);
  int (*MPI_Type_create_keyval)(MPIABI_Type_copy_attr_function *,
                                MPIABI_Type_delete_attr_function *, int *,
                                void *);
  int (*MPI_Type_create_resized)(MPIABI_Datatype, MPIABI_Aint, MPIABI_Aint,
                                 MPIABI_Datatype *);
  int (*MPI_Type_create_resized_c)(MPIABI_Datatype, MPIABI_Count, MPIABI_Count,
                                   MPIABI_Datatype *);
  int (*MPI_Type_create_struct)(int, const int[], const MPIABI_Aint[],
                                const MPIABI_Datatype[], MPIABI_Datatype *);
  int (*MPI_Type_create_struct_c)(MPIABI_Count, const MPIABI_Count[],
                                  const MPIABI_Count[],
                                  const MPIABI_Datatype[], MPIABI_Datatype *);
  int (*MPI_Type_create_subarray)(int, const int[], const int[], const int[],
                                  int, MPIABI_Datatype, MPIABI_Datatype *);
  int (*MPI_Type_create_subarray_c)(int, const MPIABI_Count[],
                                    const MPIABI_Count[], const MPIABI_Count[],
                                    int, MPIABI_Datatype, MPIABI_Datatype *);
  int (*MPI_Type_delete_attr)(MPIABI_Datatype, int);
  int (*MPI_Type_dup)(MPIABI_Datatype, MPIABI_Datatype *);
  int (*MPI_Type_free)(MPIABI_Datatype *);
  int (*MPI_Type_free_keyval)(int *);
  int (*MPI_Type_get_attr)(MPIABI_Datatype, int, void *, int *);
  int (*MPI_Type_get_contents)(MPIABI_Datatype, int, int, int, int[],
                               MPIABI_Aint[], MPIABI_Datatype[]);
  int (*MPI_Type_get_contents_c)(MPIABI_Datatype, MPIABI_Count, MPIABI_Count,
                                 MPIABI_Count, MPIABI_Count, int[],
                                 MPIABI_Aint[], MPIABI_Count[],
                                 MPIABI_Datatype[]);
  int (*MPI_Type_get_envelope)(MPIABI_Datatype, int *, int *, int *, int *);
  int (*MPI_Type_get_envelope_c)(MPIABI_Datatype, MPIABI_Count *,
                                 MPIABI_Count *, MPIABI_Count *,
                                 MPIABI_Count *, int *);
  int (*MPI_Type_get_extent)(MPIABI_Datatype, MPIABI_Aint *, MPIABI_Aint *);
  int (*MPI_Type_get_extent_c)(MPIABI_Datatype, MPIABI_Count *,
                               MPIABI_Count *);
  int (*MPI_Type_get_extent_x)(MPIABI_Datatype, MPIABI_Count *,
                               MPIABI_Count *);
  int (*MPI_Type_get_name)(MPIABI_Datatype, char *, int *);
  int (*MPI_Type_get_true_extent)(MPIABI_Datatype, MPIABI_Aint *,
                                  MPIABI_Aint *);
  int (*MPI_Type_get_true_extent_c)(MPIABI_Datatype, MPIABI_Count *,
                                    MPIABI_Count *);
  int (*MPI_Type_get_true_extent_x)(MPIABI_Datatype, MPIABI_Count *,
                                    MPIABI_Count *);
  int (*MPI_Type_get_value_index)(MPIABI_Datatype, MPIABI_Datatype,
                                  MPIABI_Datatype *);
  int (*MPI_Type_indexed)(int, const int[], const int[], MPIABI_Datatype,
                          MPIABI_Datatype *);
  int (*MPI_Type_indexed_c)(MPIABI_Count, const MPIABI_Count[],
                            const MPIABI_Count[], MPIABI_Datatype,
                            MPIABI_Datatype *);
  int (*MPI_Type_match_size)(int, int, MPIABI_Datatype *);
  int (*MPI_Type_set_attr)(MPIABI_Datatype, int, void *);
  int (*MPI_Type_set_name)(MPIABI_Datatype, const char *);
  int (*MPI_Type_size)(MPIABI_Datatype, int *);
  int (*MPI_Type_size_c)(MPIABI_Datatype, MPIABI_Count *);
  int (*MPI_Type_size_x)(MPIABI_Datatype, MPIABI_Count *);
  int (*MPI_Type_vector)(int, int, int, MPIABI_Datatype, MPIABI_Datatype *);
  int (*MPI_Type_vector_c)(MPIABI_Count, MPIABI_Count, MPIABI_Count,
                           MPIABI_Datatype, MPIABI_Datatype *);
  int (*MPI_Unpack)(const void *, int, int *, void *, int, MPIABI_Datatype,
                    MPIABI_Comm);
  int (*MPI_Unpack_c)(const void *, MPIABI_Count, MPIABI_Count *, void *,
                      MPIABI_Count, MPIABI_Datatype, MPIABI_Comm);
  int (*MPI_Unpack_external)(const char[], const void *, MPIABI_Aint,
                             MPIABI_Aint *, void *, int, MPIABI_Datatype);
  int (*MPI_Unpack_external_c)(const char[], const void *, MPIABI_Count,
                               MPIABI_Count *, void *, MPIABI_Count,
                               MPIABI_Datatype);
  int (*MPI_Unpublish_name)(const char *, MPIABI_Info, const char *);
  int (*MPI_Wait)(MPIABI_Request *, MPIABI_Status *);
  int (*MPI_Waitall)(int, MPIABI_Request[], MPIABI_Status *);
  int (*MPI_Waitany)(int, MPIABI_Request[], int *, MPIABI_Status *);
  int (*MPI_Waitsome)(int, MPIABI_Request[], int *, int[], MPIABI_Status *);
  int (*MPI_Win_allocate)(MPIABI_Aint, int, MPIABI_Info, MPIABI_Comm, void *,
                          MPIABI_Win *);
  int (*MPI_Win_allocate_c)(MPIABI_Aint, MPIABI_Aint, MPIABI_Info, MPIABI_Comm,
                            void *, MPIABI_Win *);
  int (*MPI_Win_allocate_shared)(MPIABI_Aint, int, MPIABI_Info, MPIABI_Comm,
                                 void *, MPIABI_Win *);
  int (*MPI_Win_allocate_shared_c)(MPIABI_Aint, MPIABI_Aint, MPIABI_Info,
                                   MPIABI_Comm, void *, MPIABI_Win *);
  int (*MPI_Win_attach)(MPIABI_Win, void *, MPIABI_Aint);
  int (*MPI_Win_call_errhandler)(MPIABI_Win, int);
  int (*MPI_Win_complete)(MPIABI_Win);
  int (*MPI_Win_create)(void *, MPIABI_Aint, int, MPIABI_Info, MPIABI_Comm,
                        MPIABI_Win *);
  int (*MPI_Win_create_c)(void *, MPIABI_Aint, MPIABI_Aint, MPIABI_Info,
                          MPIABI_Comm, MPIABI_Win *);
  int (*MPI_Win_create_dynamic)(MPIABI_Info, MPIABI_Comm, MPIABI_Win *);
  int (*MPI_Win_create_errhandler)(MPIABI_Win_errhandler_function *,
                                   MPIABI_Errhandler *);
  int (*MPI_Win_create_keyval)(MPIABI_Win_copy_attr_function *,
                               MPIABI_Win_delete_attr_function *, int *,
                               void *);
  int (*MPI_Win_delete_attr)(MPIABI_Win, int);
  int (*MPI_Win_detach)(MPIABI_Win, const void *);
  int (*MPI_Win_fence)(int, MPIABI_Win);
  int (*MPI_Win_flush)(int, MPIABI_Win);
  int (*MPI_Win_flush_all)(MPIABI_Win);
  int (*MPI_Win_flush_local)(int, MPIABI_Win);
  int (*MPI_Win_flush_local_all)(MPIABI_Win);
  int (*MPI_Win_free)(MPIABI_Win *);
  int (*MPI_Win_free_keyval)(int *);
  int (*MPI_Win_get_attr)(MPIABI_Win, int, void *, int *);
  int (*MPI_Win_get_errhandler)(MPIABI_Win, MPIABI_Errhandler *);
  int (*MPI_Win_get_group)(MPIABI_Win, MPIABI_Group *);
  int (*MPI_Win_get_info)(MPIABI_Win, MPIABI_Info *);
  int (*MPI_Win_get_name)(MPIABI_Win, char *, int *);
  int (*MPI_Win_lock)(int, int, int, MPIABI_Win);
  int (*MPI_Win_lock_all)(int, MPIABI_Win);
  int (*MPI_Win_post)(MPIABI_Group, int, MPIABI_Win);
  int (*MPI_Win_set_attr)(MPIABI_Win, int, void *);
  int (*MPI_Win_set_errhandler)(MPIABI_Win, MPIABI_Errhandler);
  int (*MPI_Win_set_info)(MPIABI_Win, MPIABI_Info);
  int (*MPI_Win_set_name)(MPIABI_Win, const char *);
  int (*MPI_Win_shared_query)(MPIABI_Win, int, MPIABI_Aint *, int *, void *);
  int (*MPI_Win_shared_query_c)(MPIABI_Win, int, MPIABI_Aint *, MPIABI_Aint *,
                                void *);
  int (*MPI_Win_start)(MPIABI_Group, int, MPIABI_Win);
  int (*MPI_Win_sync)(MPIABI_Win);
  int (*MPI_Win_test)(MPIABI_Win, int *);
  int (*MPI_Win_unlock)(int, MPIABI_Win);
  int (*MPI_Win_unlock_all)(MPIABI_Win);
  int (*MPI_Win_wait)(MPIABI_Win);
  MPIABI_Aint (*MPI_Aint_add)(MPIABI_Aint, MPIABI_Aint);
  MPIABI_Aint (*MPI_Aint_diff)(MPIABI_Aint, MPIABI_Aint);
  double (*MPI_Wtick)(void);
  double (*MPI_Wtime)(void);
  MPIABI_Comm (*MPI_Comm_fromint)(int);
  int (*MPI_Comm_toint)(MPIABI_Comm);
  MPIABI_Errhandler (*MPI_Errhandler_fromint)(int);
  int (*MPI_Errhandler_toint)(MPIABI_Errhandler);
  MPIABI_File (*MPI_File_fromint)(int);
  int (*MPI_File_toint)(MPIABI_File);
  MPIABI_Group (*MPI_Group_fromint)(int);
  int (*MPI_Group_toint)(MPIABI_Group);
  MPIABI_Info (*MPI_Info_fromint)(int);
  int (*MPI_Info_toint)(MPIABI_Info);
  MPIABI_Message (*MPI_Message_fromint)(int);
  int (*MPI_Message_toint)(MPIABI_Message);
  MPIABI_Op (*MPI_Op_fromint)(int);
  int (*MPI_Op_toint)(MPIABI_Op);
  MPIABI_Request (*MPI_Request_fromint)(int);
  int (*MPI_Request_toint)(MPIABI_Request);
  MPIABI_Session (*MPI_Session_fromint)(int);
  int (*MPI_Session_toint)(MPIABI_Session);
  MPIABI_Datatype (*MPI_Type_fromint)(int);
  int (*MPI_Type_toint)(MPIABI_Datatype);
  MPIABI_Win (*MPI_Win_fromint)(int);
  int (*MPI_Win_toint)(MPIABI_Win);
  int (*MPI_T_category_changed)(int *);
  int (*MPI_T_category_get_categories)(int, int, int[]);
  int (*MPI_T_category_get_cvars)(int, int, int[]);
  int (*MPI_T_category_get_events)(int, int, int[]);
  int (*MPI_T_category_get_index)(const char *, int *);
  int (*MPI_T_category_get_info)(int, char *, int *, char *, int *, int *,
                                 int *, int *);
  int (*MPI_T_category_get_num)(int *);
  int (*MPI_T_category_get_num_events)(int, int *);
  int (*MPI_T_category_get_pvars)(int, int, int[]);
  int (*MPI_T_cvar_get_index)(const char *, int *);
  int (*MPI_T_cvar_get_info)(int, char *, int *, int *, MPIABI_Datatype *,
                             MPIABI_T_enum *, char *, int *, int *, int *);
  int (*MPI_T_cvar_get_num)(int *);
  int (*MPI_T_cvar_handle_alloc)(int, void *, MPIABI_T_cvar_handle *, int *);
  int (*MPI_T_cvar_handle_free)(MPIABI_T_cvar_handle *);
  int (*MPI_T_cvar_read)(MPIABI_T_cvar_handle, void *);
  int (*MPI_T_cvar_write)(MPIABI_T_cvar_handle, const void *);
  int (*MPI_T_enum_get_info)(MPIABI_T_enum, int *, char *, int *);
  int (*MPI_T_enum_get_item)(MPIABI_T_enum, int, int *, char *, int *);
  int (*MPI_T_event_callback_get_info)(MPIABI_T_event_registration,
                                       MPIABI_T_cb_safety, MPIABI_Info *);
  int (*MPI_T_event_callback_set_info)(MPIABI_T_event_registration,
                                       MPIABI_T_cb_safety, MPIABI_Info);
  int (*MPI_T_event_copy)(MPIABI_T_event_instance, void *);
  int (*MPI_T_event_get_index)(const char *, int *);
  int (*MPI_T_event_get_info)(int, char *, int *, int *, MPIABI_Datatype[],
                              MPIABI_Aint[], int *, MPIABI_T_enum *,
                              MPIABI_Info *, char *, int *, int *);
  int (*MPI_T_event_get_num)(int *);
  int (*MPI_T_event_get_source)(MPIABI_T_event_instance, int *);
  int (*MPI_T_event_get_timestamp)(MPIABI_T_event_instance, MPIABI_Count *);
  int (*MPI_T_event_handle_alloc)(int, void *, MPIABI_Info,
                                  MPIABI_T_event_registration *);
  int (*MPI_T_event_handle_free)(MPIABI_T_event_registration, void *,
                                 MPIABI_T_event_free_cb_function);
  int (*MPI_T_event_handle_get_info)(MPIABI_T_event_registration,
                                     MPIABI_Info *);
  int (*MPI_T_event_handle_set_info)(MPIABI_T_event_registration, MPIABI_Info);
  int (*MPI_T_event_read)(MPIABI_T_event_instance, int, void *);
  int (*MPI_T_event_register_callback)(MPIABI_T_event_registration,
                                       MPIABI_T_cb_safety, MPIABI_Info, void *,
                                       MPIABI_T_event_cb_function);
  int (*MPI_T_event_set_dropped_handler)(MPIABI_T_event_registration,
                                         MPIABI_T_event_dropped_cb_function);
  int (*MPI_T_finalize)(void);
  int (*MPI_T_init_thread)(int, int *);
  int (*MPI_T_pvar_get_index)(const char *, int, int *);
  int (*MPI_T_pvar_get_info)(int, char *, int *, int *, int *,
                             MPIABI_Datatype *, MPIABI_T_enum *, char *, int *,
                             int *, int *, int *, int *);
  int (*MPI_T_pvar_get_num)(int *);
  int (*MPI_T_pvar_handle_alloc)(MPIABI_T_pvar_session, int, void *,
                                 MPIABI_T_pvar_handle *, int *);
  int (*MPI_T_pvar_handle_free)(MPIABI_T_pvar_session, MPIABI_T_pvar_handle *);
  int (*MPI_T_pvar_read)(MPIABI_T_pvar_session, MPIABI_T_pvar_handle, void *);
  int (*MPI_T_pvar_readreset)(MPIABI_T_pvar_session, MPIABI_T_pvar_handle,
                              void *);
  int (*MPI_T_pvar_reset)(MPIABI_T_pvar_session, MPIABI_T_pvar_handle);
  int (*MPI_T_pvar_session_create)(MPIABI_T_pvar_session *);
  int (*MPI_T_pvar_session_free)(MPIABI_T_pvar_session *);
  int (*MPI_T_pvar_start)(MPIABI_T_pvar_session, MPIABI_T_pvar_handle);
  int (*MPI_T_pvar_stop)(MPIABI_T_pvar_session, MPIABI_T_pvar_handle);
  int (*MPI_T_pvar_write)(MPIABI_T_pvar_session, MPIABI_T_pvar_handle,
                          const void *);
  int (*MPI_T_source_get_info)(int, char *, int *, char *, int *,
                               MPIABI_T_source_order *, MPIABI_Count *,
                               MPIABI_Count *, MPIABI_Info *);
  int (*MPI_T_source_get_num)(int *);
  int (*MPI_T_source_get_timestamp)(int, MPIABI_Count *);
  int (*PMPI_Abi_get_fortran_booleans)(int, void *, void *, int *);
  int (*PMPI_Abi_get_fortran_info)(MPIABI_Info *);
  int (*PMPI_Abi_get_info)(MPIABI_Info *);
  int (*PMPI_Abi_get_version)(int *, int *);
  int (*PMPI_Abi_set_fortran_booleans)(int, void *, void *);
  int (*PMPI_Abi_set_fortran_info)(MPIABI_Info);
  int (*PMPI_Abort)(MPIABI_Comm, int);
  int (*PMPI_Accumulate)(const void *, int, MPIABI_Datatype, int, MPIABI_Aint,
                         int, MPIABI_Datatype, MPIABI_Op, MPIABI_Win);
  int (*PMPI_Accumulate_c)(const void *, MPIABI_Count, MPIABI_Datatype, int,
                           MPIABI_Aint, MPIABI_Count, MPIABI_Datatype,
                           MPIABI_Op, MPIABI_Win);
  int (*PMPI_Add_error_class)(int *);
  int (*PMPI_Add_error_code)(int, int *);
  int (*PMPI_Add_error_string)(int, const char *);
  int (*PMPI_Allgather)(const void *, int, MPIABI_Datatype, void *, int,
                        MPIABI_Datatype, MPIABI_Comm);
  int (*PMPI_Allgather_c)(const void *, MPIABI_Count, MPIABI_Datatype, void *,
                          MPIABI_Count, MPIABI_Datatype, MPIABI_Comm);
  int (*PMPI_Allgather_init)(const void *, int, MPIABI_Datatype, void *, int,
                             MPIABI_Datatype, MPIABI_Comm, MPIABI_Info,
                             MPIABI_Request *);
  int (*PMPI_Allgather_init_c)(const void *, MPIABI_Count, MPIABI_Datatype,
                               void *, MPIABI_Count, MPIABI_Datatype,
                               MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Allgatherv)(const void *, int, MPIABI_Datatype, void *,
                         const int[], const int[], MPIABI_Datatype,
                         MPIABI_Comm);
  int (*PMPI_Allgatherv_c)(const void *, MPIABI_Count, MPIABI_Datatype, void *,
                           const MPIABI_Count[], const MPIABI_Aint[],
                           MPIABI_Datatype, MPIABI_Comm);
  int (*PMPI_Allgatherv_init)(const void *, int, MPIABI_Datatype, void *,
                              const int[], const int[], MPIABI_Datatype,
                              MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Allgatherv_init_c)(const void *, MPIABI_Count, MPIABI_Datatype,
                                void *, const MPIABI_Count[],
                                const MPIABI_Aint[], MPIABI_Datatype,
                                MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Alloc_mem)(MPIABI_Aint, MPIABI_Info, void *);
  int (*PMPI_Allreduce)(const void *, void *, int, MPIABI_Datatype, MPIABI_Op,
                        MPIABI_Comm);
  int (*PMPI_Allreduce_c)(const void *, void *, MPIABI_Count, MPIABI_Datatype,
                          MPIABI_Op, MPIABI_Comm);
  int (*PMPI_Allreduce_init)(const void *, void *, int, MPIABI_Datatype,
                             MPIABI_Op, MPIABI_Comm, MPIABI_Info,
                             MPIABI_Request *);
  int (*PMPI_Allreduce_init_c)(const void *, void *, MPIABI_Count,
                               MPIABI_Datatype, MPIABI_Op, MPIABI_Comm,
                               MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Alltoall)(const void *, int, MPIABI_Datatype, void *, int,
                       MPIABI_Datatype, MPIABI_Comm);
  int (*PMPI_Alltoall_c)(const void *, MPIABI_Count, MPIABI_Datatype, void *,
                         MPIABI_Count, MPIABI_Datatype, MPIABI_Comm);
  int (*PMPI_Alltoall_init)(const void *, int, MPIABI_Datatype, void *, int,
                            MPIABI_Datatype, MPIABI_Comm, MPIABI_Info,
                            MPIABI_Request *);
  int (*PMPI_Alltoall_init_c)(const void *, MPIABI_Count, MPIABI_Datatype,
                              void *, MPIABI_Count, MPIABI_Datatype,
                              MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Alltoallv)(const void *, const int[], const int[],
                        MPIABI_Datatype, void *, const int[], const int[],
                        MPIABI_Datatype, MPIABI_Comm);
  int (*PMPI_Alltoallv_c)(const void *, const MPIABI_Count[],
                          const MPIABI_Aint[], MPIABI_Datatype, void *,
                          const MPIABI_Count[], const MPIABI_Aint[],
                          MPIABI_Datatype, MPIABI_Comm);
  int (*PMPI_Alltoallv_init)(const void *, const int[], const int[],
                             MPIABI_Datatype, void *, const int[], const int[],
                             MPIABI_Datatype, MPIABI_Comm, MPIABI_Info,
                             MPIABI_Request *);
  int (*PMPI_Alltoallv_init_c)(const void *, const MPIABI_Count[],
                               const MPIABI_Aint[], MPIABI_Datatype, void *,
                               const MPIABI_Count[], const MPIABI_Aint[],
                               MPIABI_Datatype, MPIABI_Comm, MPIABI_Info,
                               MPIABI_Request *);
  int (*PMPI_Alltoallw)(const void *, const int[], const int[],
                        const MPIABI_Datatype[], void *, const int[],
                        const int[], const MPIABI_Datatype[], MPIABI_Comm);
  int (*PMPI_Alltoallw_c)(const void *, const MPIABI_Count[],
                          const MPIABI_Aint[], const MPIABI_Datatype[], void *,
                          const MPIABI_Count[], const MPIABI_Aint[],
                          const MPIABI_Datatype[], MPIABI_Comm);
  int (*PMPI_Alltoallw_init)(const void *, const int[], const int[],
                             const MPIABI_Datatype[], void *, const int[],
                             const int[], const MPIABI_Datatype[], MPIABI_Comm,
                             MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Alltoallw_init_c)(const void *, const MPIABI_Count[],
                               const MPIABI_Aint[], const MPIABI_Datatype[],
                               void *, const MPIABI_Count[],
                               const MPIABI_Aint[], const MPIABI_Datatype[],
                               MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Barrier)(MPIABI_Comm);
  int (*PMPI_Barrier_init)(MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Bcast)(void *, int, MPIABI_Datatype, int, MPIABI_Comm);
  int (*PMPI_Bcast_c)(void *, MPIABI_Count, MPIABI_Datatype, int, MPIABI_Comm);
  int (*PMPI_Bcast_init)(void *, int, MPIABI_Datatype, int, MPIABI_Comm,
                         MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Bcast_init_c)(void *, MPIABI_Count, MPIABI_Datatype, int,
                           MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Bsend)(const void *, int, MPIABI_Datatype, int, int, MPIABI_Comm);
  int (*PMPI_Bsend_c)(const void *, MPIABI_Count, MPIABI_Datatype, int, int,
                      MPIABI_Comm);
  int (*PMPI_Bsend_init)(const void *, int, MPIABI_Datatype, int, int,
                         MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Bsend_init_c)(const void *, MPIABI_Count, MPIABI_Datatype, int,
                           int, MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Buffer_attach)(void *, int);
  int (*PMPI_Buffer_attach_c)(void *, MPIABI_Count);
  int (*PMPI_Buffer_detach)(void *, int *);
  int (*PMPI_Buffer_detach_c)(void *, MPIABI_Count *);
  int (*PMPI_Buffer_flush)(void);
  int (*PMPI_Buffer_iflush)(MPIABI_Request *);
  int (*PMPI_Cancel)(MPIABI_Request *);
  int (*PMPI_Cart_coords)(MPIABI_Comm, int, int, int[]);
  int (*PMPI_Cart_create)(MPIABI_Comm, int, const int[], const int[], int,
                          MPIABI_Comm *);
  int (*PMPI_Cart_get)(MPIABI_Comm, int, int[], int[], int[]);
  int (*PMPI_Cart_map)(MPIABI_Comm, int, const int[], const int[], int *);
  int (*PMPI_Cart_rank)(MPIABI_Comm, const int[], int *);
  int (*PMPI_Cart_shift)(MPIABI_Comm, int, int, int *, int *);
  int (*PMPI_Cart_sub)(MPIABI_Comm, const int[], MPIABI_Comm *);
  int (*PMPI_Cartdim_get)(MPIABI_Comm, int *);
  int (*PMPI_Close_port)(const char *);
  int (*PMPI_Comm_accept)(const char *, MPIABI_Info, int, MPIABI_Comm,
                          MPIABI_Comm *);
  int (*PMPI_Comm_attach_buffer)(MPIABI_Comm, void *, int);
  int (*PMPI_Comm_attach_buffer_c)(MPIABI_Comm, void *, MPIABI_Count);
  int (*PMPI_Comm_call_errhandler)(MPIABI_Comm, int);
  int (*PMPI_Comm_compare)(MPIABI_Comm, MPIABI_Comm, int *);
  int (*PMPI_Comm_connect)(const char *, MPIABI_Info, int, MPIABI_Comm,
                           MPIABI_Comm *);
  int (*PMPI_Comm_create)(MPIABI_Comm, MPIABI_Group, MPIABI_Comm *);
  int (*PMPI_Comm_create_errhandler)(MPIABI_Comm_errhandler_function *,
                                     MPIABI_Errhandler *);
  int (*PMPI_Comm_create_from_group)(MPIABI_Group, const char *, MPIABI_Info,
                                     MPIABI_Errhandler, MPIABI_Comm *);
  int (*PMPI_Comm_create_group)(MPIABI_Comm, MPIABI_Group, int, MPIABI_Comm *);
  int (*PMPI_Comm_create_keyval)(MPIABI_Comm_copy_attr_function *,
                                 MPIABI_Comm_delete_attr_function *, int *,
                                 void *);
  int (*PMPI_Comm_delete_attr)(MPIABI_Comm, int);
  int (*PMPI_Comm_detach_buffer)(MPIABI_Comm, void *, int *);
  int (*PMPI_Comm_detach_buffer_c)(MPIABI_Comm, void *, MPIABI_Count *);
  int (*PMPI_Comm_disconnect)(MPIABI_Comm *);
  int (*PMPI_Comm_dup)(MPIABI_Comm, MPIABI_Comm *);
  int (*PMPI_Comm_dup_with_info)(MPIABI_Comm, MPIABI_Info, MPIABI_Comm *);
  int (*PMPI_Comm_flush_buffer)(MPIABI_Comm);
  int (*PMPI_Comm_free)(MPIABI_Comm *);
  int (*PMPI_Comm_free_keyval)(int *);
  int (*PMPI_Comm_get_attr)(MPIABI_Comm, int, void *, int *);
  int (*PMPI_Comm_get_errhandler)(MPIABI_Comm, MPIABI_Errhandler *);
  int (*PMPI_Comm_get_info)(MPIABI_Comm, MPIABI_Info *);
  int (*PMPI_Comm_get_name)(MPIABI_Comm, char *, int *);
  int (*PMPI_Comm_get_parent)(MPIABI_Comm *);
  int (*PMPI_Comm_group)(MPIABI_Comm, MPIABI_Group *);
  int (*PMPI_Comm_idup)(MPIABI_Comm, MPIABI_Comm *, MPIABI_Request *);
  int (*PMPI_Comm_idup_with_info)(MPIABI_Comm, MPIABI_Info, MPIABI_Comm *,
                                  MPIABI_Request *);
  int (*PMPI_Comm_iflush_buffer)(MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Comm_join)(int, MPIABI_Comm *);
  int (*PMPI_Comm_rank)(MPIABI_Comm, int *);
  int (*PMPI_Comm_remote_group)(MPIABI_Comm, MPIABI_Group *);
  int (*PMPI_Comm_remote_size)(MPIABI_Comm, int *);
  int (*PMPI_Comm_set_attr)(MPIABI_Comm, int, void *);
  int (*PMPI_Comm_set_errhandler)(MPIABI_Comm, MPIABI_Errhandler);
  int (*PMPI_Comm_set_info)(MPIABI_Comm, MPIABI_Info);
  int (*PMPI_Comm_set_name)(MPIABI_Comm, const char *);
  int (*PMPI_Comm_size)(MPIABI_Comm, int *);
  int (*PMPI_Comm_spawn)(const char *, char *[], int, MPIABI_Info, int,
                         MPIABI_Comm, MPIABI_Comm *, int[]);
  int (*PMPI_Comm_spawn_multiple)(int, char *[], char **[], const int[],
                                  const MPIABI_Info[], int, MPIABI_Comm,
                                  MPIABI_Comm *, int[]);
  int (*PMPI_Comm_split)(MPIABI_Comm, int, int, MPIABI_Comm *);
  int (*PMPI_Comm_split_type)(MPIABI_Comm, int, int, MPIABI_Info,
                              MPIABI_Comm *);
  int (*PMPI_Comm_test_inter)(MPIABI_Comm, int *);
  int (*PMPI_Compare_and_swap)(const void *, const void *, void *,
                               MPIABI_Datatype, int, MPIABI_Aint, MPIABI_Win);
  int (*PMPI_Dims_create)(int, int, int[]);
  int (*PMPI_Dist_graph_create)(MPIABI_Comm, int, const int[], const int[],
                                const int[], const int[], MPIABI_Info, int,
                                MPIABI_Comm *);
  int (*PMPI_Dist_graph_create_adjacent)(MPIABI_Comm, int, const int[],
                                         const int[], int, const int[],
                                         const int[], MPIABI_Info, int,
                                         MPIABI_Comm *);
  int (*PMPI_Dist_graph_neighbors)(MPIABI_Comm, int, int[], int[], int, int[],
                                   int[]);
  int (*PMPI_Dist_graph_neighbors_count)(MPIABI_Comm, int *, int *, int *);
  int (*PMPI_Errhandler_free)(MPIABI_Errhandler *);
  int (*PMPI_Error_class)(int, int *);
  int (*PMPI_Error_string)(int, char *, int *);
  int (*PMPI_Exscan)(const void *, void *, int, MPIABI_Datatype, MPIABI_Op,
                     MPIABI_Comm);
  int (*PMPI_Exscan_c)(const void *, void *, MPIABI_Count, MPIABI_Datatype,
                       MPIABI_Op, MPIABI_Comm);
  int (*PMPI_Exscan_init)(const void *, void *, int, MPIABI_Datatype,
                          MPIABI_Op, MPIABI_Comm, MPIABI_Info,
                          MPIABI_Request *);
  int (*PMPI_Exscan_init_c)(const void *, void *, MPIABI_Count,
                            MPIABI_Datatype, MPIABI_Op, MPIABI_Comm,
                            MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Fetch_and_op)(const void *, void *, MPIABI_Datatype, int,
                           MPIABI_Aint, MPIABI_Op, MPIABI_Win);
  int (*PMPI_File_call_errhandler)(MPIABI_File, int);
  int (*PMPI_File_close)(MPIABI_File *);
  int (*PMPI_File_create_errhandler)(MPIABI_File_errhandler_function *,
                                     MPIABI_Errhandler *);
  int (*PMPI_File_delete)(const char *, MPIABI_Info);
  int (*PMPI_File_get_amode)(MPIABI_File, int *);
  int (*PMPI_File_get_atomicity)(MPIABI_File, int *);
  int (*PMPI_File_get_byte_offset)(MPIABI_File, MPIABI_Offset,
                                   MPIABI_Offset *);
  int (*PMPI_File_get_errhandler)(MPIABI_File, MPIABI_Errhandler *);
  int (*PMPI_File_get_group)(MPIABI_File, MPIABI_Group *);
  int (*PMPI_File_get_info)(MPIABI_File, MPIABI_Info *);
  int (*PMPI_File_get_position)(MPIABI_File, MPIABI_Offset *);
  int (*PMPI_File_get_position_shared)(MPIABI_File, MPIABI_Offset *);
  int (*PMPI_File_get_size)(MPIABI_File, MPIABI_Offset *);
  int (*PMPI_File_get_type_extent)(MPIABI_File, MPIABI_Datatype,
                                   MPIABI_Aint *);
  int (*PMPI_File_get_type_extent_c)(MPIABI_File, MPIABI_Datatype,
                                     MPIABI_Count *);
  int (*PMPI_File_get_view)(MPIABI_File, MPIABI_Offset *, MPIABI_Datatype *,
                            MPIABI_Datatype *, char *);
  int (*PMPI_File_iread)(MPIABI_File, void *, int, MPIABI_Datatype,
                         MPIABI_Request *);
  int (*PMPI_File_iread_c)(MPIABI_File, void *, MPIABI_Count, MPIABI_Datatype,
                           MPIABI_Request *);
  int (*PMPI_File_iread_all)(MPIABI_File, void *, int, MPIABI_Datatype,
                             MPIABI_Request *);
  int (*PMPI_File_iread_all_c)(MPIABI_File, void *, MPIABI_Count,
                               MPIABI_Datatype, MPIABI_Request *);
  int (*PMPI_File_iread_at)(MPIABI_File, MPIABI_Offset, void *, int,
                            MPIABI_Datatype, MPIABI_Request *);
  int (*PMPI_File_iread_at_c)(MPIABI_File, MPIABI_Offset, void *, MPIABI_Count,
                              MPIABI_Datatype, MPIABI_Request *);
  int (*PMPI_File_iread_at_all)(MPIABI_File, MPIABI_Offset, void *, int,
                                MPIABI_Datatype, MPIABI_Request *);
  int (*PMPI_File_iread_at_all_c)(MPIABI_File, MPIABI_Offset, void *,
                                  MPIABI_Count, MPIABI_Datatype,
                                  MPIABI_Request *);
  int (*PMPI_File_iread_shared)(MPIABI_File, void *, int, MPIABI_Datatype,
                                MPIABI_Request *);
  int (*PMPI_File_iread_shared_c)(MPIABI_File, void *, MPIABI_Count,
                                  MPIABI_Datatype, MPIABI_Request *);
  int (*PMPI_File_iwrite)(MPIABI_File, const void *, int, MPIABI_Datatype,
                          MPIABI_Request *);
  int (*PMPI_File_iwrite_c)(MPIABI_File, const void *, MPIABI_Count,
                            MPIABI_Datatype, MPIABI_Request *);
  int (*PMPI_File_iwrite_all)(MPIABI_File, const void *, int, MPIABI_Datatype,
                              MPIABI_Request *);
  int (*PMPI_File_iwrite_all_c)(MPIABI_File, const void *, MPIABI_Count,
                                MPIABI_Datatype, MPIABI_Request *);
  int (*PMPI_File_iwrite_at)(MPIABI_File, MPIABI_Offset, const void *, int,
                             MPIABI_Datatype, MPIABI_Request *);
  int (*PMPI_File_iwrite_at_c)(MPIABI_File, MPIABI_Offset, const void *,
                               MPIABI_Count, MPIABI_Datatype,
                               MPIABI_Request *);
  int (*PMPI_File_iwrite_at_all)(MPIABI_File, MPIABI_Offset, const void *, int,
                                 MPIABI_Datatype, MPIABI_Request *);
  int (*PMPI_File_iwrite_at_all_c)(MPIABI_File, MPIABI_Offset, const void *,
                                   MPIABI_Count, MPIABI_Datatype,
                                   MPIABI_Request *);
  int (*PMPI_File_iwrite_shared)(MPIABI_File, const void *, int,
                                 MPIABI_Datatype, MPIABI_Request *);
  int (*PMPI_File_iwrite_shared_c)(MPIABI_File, const void *, MPIABI_Count,
                                   MPIABI_Datatype, MPIABI_Request *);
  int (*PMPI_File_open)(MPIABI_Comm, const char *, int, MPIABI_Info,
                        MPIABI_File *);
  int (*PMPI_File_preallocate)(MPIABI_File, MPIABI_Offset);
  int (*PMPI_File_read)(MPIABI_File, void *, int, MPIABI_Datatype,
                        MPIABI_Status *);
  int (*PMPI_File_read_c)(MPIABI_File, void *, MPIABI_Count, MPIABI_Datatype,
                          MPIABI_Status *);
  int (*PMPI_File_read_all)(MPIABI_File, void *, int, MPIABI_Datatype,
                            MPIABI_Status *);
  int (*PMPI_File_read_all_c)(MPIABI_File, void *, MPIABI_Count,
                              MPIABI_Datatype, MPIABI_Status *);
  int (*PMPI_File_read_all_begin)(MPIABI_File, void *, int, MPIABI_Datatype);
  int (*PMPI_File_read_all_begin_c)(MPIABI_File, void *, MPIABI_Count,
                                    MPIABI_Datatype);
  int (*PMPI_File_read_all_end)(MPIABI_File, void *, MPIABI_Status *);
  int (*PMPI_File_read_at)(MPIABI_File, MPIABI_Offset, void *, int,
                           MPIABI_Datatype, MPIABI_Status *);
  int (*PMPI_File_read_at_c)(MPIABI_File, MPIABI_Offset, void *, MPIABI_Count,
                             MPIABI_Datatype, MPIABI_Status *);
  int (*PMPI_File_read_at_all)(MPIABI_File, MPIABI_Offset, void *, int,
                               MPIABI_Datatype, MPIABI_Status *);
  int (*PMPI_File_read_at_all_c)(MPIABI_File, MPIABI_Offset, void *,
                                 MPIABI_Count, MPIABI_Datatype,
                                 MPIABI_Status *);
  int (*PMPI_File_read_at_all_begin)(MPIABI_File, MPIABI_Offset, void *, int,
                                     MPIABI_Datatype);
  int (*PMPI_File_read_at_all_begin_c)(MPIABI_File, MPIABI_Offset, void *,
                                       MPIABI_Count, MPIABI_Datatype);
  int (*PMPI_File_read_at_all_end)(MPIABI_File, void *, MPIABI_Status *);
  int (*PMPI_File_read_ordered)(MPIABI_File, void *, int, MPIABI_Datatype,
                                MPIABI_Status *);
  int (*PMPI_File_read_ordered_c)(MPIABI_File, void *, MPIABI_Count,
                                  MPIABI_Datatype, MPIABI_Status *);
  int (*PMPI_File_read_ordered_begin)(MPIABI_File, void *, int,
                                      MPIABI_Datatype);
  int (*PMPI_File_read_ordered_begin_c)(MPIABI_File, void *, MPIABI_Count,
                                        MPIABI_Datatype);
  int (*PMPI_File_read_ordered_end)(MPIABI_File, void *, MPIABI_Status *);
  int (*PMPI_File_read_shared)(MPIABI_File, void *, int, MPIABI_Datatype,
                               MPIABI_Status *);
  int (*PMPI_File_read_shared_c)(MPIABI_File, void *, MPIABI_Count,
                                 MPIABI_Datatype, MPIABI_Status *);
  int (*PMPI_File_seek)(MPIABI_File, MPIABI_Offset, int);
  int (*PMPI_File_seek_shared)(MPIABI_File, MPIABI_Offset, int);
  int (*PMPI_File_set_atomicity)(MPIABI_File, int);
  int (*PMPI_File_set_errhandler)(MPIABI_File, MPIABI_Errhandler);
  int (*PMPI_File_set_info)(MPIABI_File, MPIABI_Info);
  int (*PMPI_File_set_size)(MPIABI_File, MPIABI_Offset);
  int (*PMPI_File_set_view)(MPIABI_File, MPIABI_Offset, MPIABI_Datatype,
                            MPIABI_Datatype, const char *, MPIABI_Info);
  int (*PMPI_File_sync)(MPIABI_File);
  int (*PMPI_File_write)(MPIABI_File, const void *, int, MPIABI_Datatype,
                         MPIABI_Status *);
  int (*PMPI_File_write_c)(MPIABI_File, const void *, MPIABI_Count,
                           MPIABI_Datatype, MPIABI_Status *);
  int (*PMPI_File_write_all)(MPIABI_File, const void *, int, MPIABI_Datatype,
                             MPIABI_Status *);
  int (*PMPI_File_write_all_c)(MPIABI_File, const void *, MPIABI_Count,
                               MPIABI_Datatype, MPIABI_Status *);
  int (*PMPI_File_write_all_begin)(MPIABI_File, const void *, int,
                                   MPIABI_Datatype);
  int (*PMPI_File_write_all_begin_c)(MPIABI_File, const void *, MPIABI_Count,
                                     MPIABI_Datatype);
  int (*PMPI_File_write_all_end)(MPIABI_File, const void *, MPIABI_Status *);
  int (*PMPI_File_write_at)(MPIABI_File, MPIABI_Offset, const void *, int,
                            MPIABI_Datatype, MPIABI_Status *);
  int (*PMPI_File_write_at_c)(MPIABI_File, MPIABI_Offset, const void *,
                              MPIABI_Count, MPIABI_Datatype, MPIABI_Status *);
  int (*PMPI_File_write_at_all)(MPIABI_File, MPIABI_Offset, const void *, int,
                                MPIABI_Datatype, MPIABI_Status *);
  int (*PMPI_File_write_at_all_c)(MPIABI_File, MPIABI_Offset, const void *,
                                  MPIABI_Count, MPIABI_Datatype,
                                  MPIABI_Status *);
  int (*PMPI_File_write_at_all_begin)(MPIABI_File, MPIABI_Offset, const void *,
                                      int, MPIABI_Datatype);
  int (*PMPI_File_write_at_all_begin_c)(MPIABI_File, MPIABI_Offset,
                                        const void *, MPIABI_Count,
                                        MPIABI_Datatype);
  int (*PMPI_File_write_at_all_end)(MPIABI_File, const void *,
                                    MPIABI_Status *);
  int (*PMPI_File_write_ordered)(MPIABI_File, const void *, int,
                                 MPIABI_Datatype, MPIABI_Status *);
  int (*PMPI_File_write_ordered_c)(MPIABI_File, const void *, MPIABI_Count,
                                   MPIABI_Datatype, MPIABI_Status *);
  int (*PMPI_File_write_ordered_begin)(MPIABI_File, const void *, int,
                                       MPIABI_Datatype);
  int (*PMPI_File_write_ordered_begin_c)(MPIABI_File, const void *,
                                         MPIABI_Count, MPIABI_Datatype);
  int (*PMPI_File_write_ordered_end)(MPIABI_File, const void *,
                                     MPIABI_Status *);
  int (*PMPI_File_write_shared)(MPIABI_File, const void *, int,
                                MPIABI_Datatype, MPIABI_Status *);
  int (*PMPI_File_write_shared_c)(MPIABI_File, const void *, MPIABI_Count,
                                  MPIABI_Datatype, MPIABI_Status *);
  int (*PMPI_Finalize)(void);
  int (*PMPI_Finalized)(int *);
  int (*PMPI_Free_mem)(void *);
  int (*PMPI_Gather)(const void *, int, MPIABI_Datatype, void *, int,
                     MPIABI_Datatype, int, MPIABI_Comm);
  int (*PMPI_Gather_c)(const void *, MPIABI_Count, MPIABI_Datatype, void *,
                       MPIABI_Count, MPIABI_Datatype, int, MPIABI_Comm);
  int (*PMPI_Gather_init)(const void *, int, MPIABI_Datatype, void *, int,
                          MPIABI_Datatype, int, MPIABI_Comm, MPIABI_Info,
                          MPIABI_Request *);
  int (*PMPI_Gather_init_c)(const void *, MPIABI_Count, MPIABI_Datatype,
                            void *, MPIABI_Count, MPIABI_Datatype, int,
                            MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Gatherv)(const void *, int, MPIABI_Datatype, void *, const int[],
                      const int[], MPIABI_Datatype, int, MPIABI_Comm);
  int (*PMPI_Gatherv_c)(const void *, MPIABI_Count, MPIABI_Datatype, void *,
                        const MPIABI_Count[], const MPIABI_Aint[],
                        MPIABI_Datatype, int, MPIABI_Comm);
  int (*PMPI_Gatherv_init)(const void *, int, MPIABI_Datatype, void *,
                           const int[], const int[], MPIABI_Datatype, int,
                           MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Gatherv_init_c)(const void *, MPIABI_Count, MPIABI_Datatype,
                             void *, const MPIABI_Count[], const MPIABI_Aint[],
                             MPIABI_Datatype, int, MPIABI_Comm, MPIABI_Info,
                             MPIABI_Request *);
  int (*PMPI_Get)(void *, int, MPIABI_Datatype, int, MPIABI_Aint, int,
                  MPIABI_Datatype, MPIABI_Win);
  int (*PMPI_Get_c)(void *, MPIABI_Count, MPIABI_Datatype, int, MPIABI_Aint,
                    MPIABI_Count, MPIABI_Datatype, MPIABI_Win);
  int (*PMPI_Get_accumulate)(const void *, int, MPIABI_Datatype, void *, int,
                             MPIABI_Datatype, int, MPIABI_Aint, int,
                             MPIABI_Datatype, MPIABI_Op, MPIABI_Win);
  int (*PMPI_Get_accumulate_c)(const void *, MPIABI_Count, MPIABI_Datatype,
                               void *, MPIABI_Count, MPIABI_Datatype, int,
                               MPIABI_Aint, MPIABI_Count, MPIABI_Datatype,
                               MPIABI_Op, MPIABI_Win);
  int (*PMPI_Get_address)(const void *, MPIABI_Aint *);
  int (*PMPI_Get_count)(const MPIABI_Status *, MPIABI_Datatype, int *);
  int (*PMPI_Get_count_c)(const MPIABI_Status *, MPIABI_Datatype,
                          MPIABI_Count *);
  int (*PMPI_Get_elements)(const MPIABI_Status *, MPIABI_Datatype, int *);
  int (*PMPI_Get_elements_c)(const MPIABI_Status *, MPIABI_Datatype,
                             MPIABI_Count *);
  int (*PMPI_Get_elements_x)(const MPIABI_Status *, MPIABI_Datatype,
                             MPIABI_Count *);
  int (*PMPI_Get_hw_resource_info)(MPIABI_Info *);
  int (*PMPI_Get_library_version)(char *, int *);
  int (*PMPI_Get_processor_name)(char *, int *);
  int (*PMPI_Get_version)(int *, int *);
  int (*PMPI_Graph_create)(MPIABI_Comm, int, const int[], const int[], int,
                           MPIABI_Comm *);
  int (*PMPI_Graph_get)(MPIABI_Comm, int, int, int[], int[]);
  int (*PMPI_Graph_map)(MPIABI_Comm, int, const int[], const int[], int *);
  int (*PMPI_Graph_neighbors)(MPIABI_Comm, int, int, int[]);
  int (*PMPI_Graph_neighbors_count)(MPIABI_Comm, int, int *);
  int (*PMPI_Graphdims_get)(MPIABI_Comm, int *, int *);
  int (*PMPI_Grequest_complete)(MPIABI_Request);
  int (*PMPI_Grequest_start)(MPIABI_Grequest_query_function *,
                             MPIABI_Grequest_free_function *,
                             MPIABI_Grequest_cancel_function *, void *,
                             MPIABI_Request *);
  int (*PMPI_Group_compare)(MPIABI_Group, MPIABI_Group, int *);
  int (*PMPI_Group_difference)(MPIABI_Group, MPIABI_Group, MPIABI_Group *);
  int (*PMPI_Group_excl)(MPIABI_Group, int, const int[], MPIABI_Group *);
  int (*PMPI_Group_free)(MPIABI_Group *);
  int (*PMPI_Group_from_session_pset)(MPIABI_Session, const char *,
                                      MPIABI_Group *);
  int (*PMPI_Group_incl)(MPIABI_Group, int, const int[], MPIABI_Group *);
  int (*PMPI_Group_intersection)(MPIABI_Group, MPIABI_Group, MPIABI_Group *);
  int (*PMPI_Group_range_excl)(MPIABI_Group, int, int[][3], MPIABI_Group *);
  int (*PMPI_Group_range_incl)(MPIABI_Group, int, int[][3], MPIABI_Group *);
  int (*PMPI_Group_rank)(MPIABI_Group, int *);
  int (*PMPI_Group_size)(MPIABI_Group, int *);
  int (*PMPI_Group_translate_ranks)(MPIABI_Group, int, const int[],
                                    MPIABI_Group, int[]);
  int (*PMPI_Group_union)(MPIABI_Group, MPIABI_Group, MPIABI_Group *);
  int (*PMPI_Iallgather)(const void *, int, MPIABI_Datatype, void *, int,
                         MPIABI_Datatype, MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Iallgather_c)(const void *, MPIABI_Count, MPIABI_Datatype, void *,
                           MPIABI_Count, MPIABI_Datatype, MPIABI_Comm,
                           MPIABI_Request *);
  int (*PMPI_Iallgatherv)(const void *, int, MPIABI_Datatype, void *,
                          const int[], const int[], MPIABI_Datatype,
                          MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Iallgatherv_c)(const void *, MPIABI_Count, MPIABI_Datatype,
                            void *, const MPIABI_Count[], const MPIABI_Aint[],
                            MPIABI_Datatype, MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Iallreduce)(const void *, void *, int, MPIABI_Datatype, MPIABI_Op,
                         MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Iallreduce_c)(const void *, void *, MPIABI_Count, MPIABI_Datatype,
                           MPIABI_Op, MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Ialltoall)(const void *, int, MPIABI_Datatype, void *, int,
                        MPIABI_Datatype, MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Ialltoall_c)(const void *, MPIABI_Count, MPIABI_Datatype, void *,
                          MPIABI_Count, MPIABI_Datatype, MPIABI_Comm,
                          MPIABI_Request *);
  int (*PMPI_Ialltoallv)(const void *, const int[], const int[],
                         MPIABI_Datatype, void *, const int[], const int[],
                         MPIABI_Datatype, MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Ialltoallv_c)(const void *, const MPIABI_Count[],
                           const MPIABI_Aint[], MPIABI_Datatype, void *,
                           const MPIABI_Count[], const MPIABI_Aint[],
                           MPIABI_Datatype, MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Ialltoallw)(const void *, const int[], const int[],
                         const MPIABI_Datatype[], void *, const int[],
                         const int[], const MPIABI_Datatype[], MPIABI_Comm,
                         MPIABI_Request *);
  int (*PMPI_Ialltoallw_c)(const void *, const MPIABI_Count[],
                           const MPIABI_Aint[], const MPIABI_Datatype[],
                           void *, const MPIABI_Count[], const MPIABI_Aint[],
                           const MPIABI_Datatype[], MPIABI_Comm,
                           MPIABI_Request *);
  int (*PMPI_Ibarrier)(MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Ibcast)(void *, int, MPIABI_Datatype, int, MPIABI_Comm,
                     MPIABI_Request *);
  int (*PMPI_Ibcast_c)(void *, MPIABI_Count, MPIABI_Datatype, int, MPIABI_Comm,
                       MPIABI_Request *);
  int (*PMPI_Ibsend)(const void *, int, MPIABI_Datatype, int, int, MPIABI_Comm,
                     MPIABI_Request *);
  int (*PMPI_Ibsend_c)(const void *, MPIABI_Count, MPIABI_Datatype, int, int,
                       MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Iexscan)(const void *, void *, int, MPIABI_Datatype, MPIABI_Op,
                      MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Iexscan_c)(const void *, void *, MPIABI_Count, MPIABI_Datatype,
                        MPIABI_Op, MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Igather)(const void *, int, MPIABI_Datatype, void *, int,
                      MPIABI_Datatype, int, MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Igather_c)(const void *, MPIABI_Count, MPIABI_Datatype, void *,
                        MPIABI_Count, MPIABI_Datatype, int, MPIABI_Comm,
                        MPIABI_Request *);
  int (*PMPI_Igatherv)(const void *, int, MPIABI_Datatype, void *, const int[],
                       const int[], MPIABI_Datatype, int, MPIABI_Comm,
                       MPIABI_Request *);
  int (*PMPI_Igatherv_c)(const void *, MPIABI_Count, MPIABI_Datatype, void *,
                         const MPIABI_Count[], const MPIABI_Aint[],
                         MPIABI_Datatype, int, MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Improbe)(int, int, MPIABI_Comm, int *, MPIABI_Message *,
                      MPIABI_Status *);
  int (*PMPI_Imrecv)(void *, int, MPIABI_Datatype, MPIABI_Message *,
                     MPIABI_Request *);
  int (*PMPI_Imrecv_c)(void *, MPIABI_Count, MPIABI_Datatype, MPIABI_Message *,
                       MPIABI_Request *);
  int (*PMPI_Ineighbor_allgather)(const void *, int, MPIABI_Datatype, void *,
                                  int, MPIABI_Datatype, MPIABI_Comm,
                                  MPIABI_Request *);
  int (*PMPI_Ineighbor_allgather_c)(const void *, MPIABI_Count,
                                    MPIABI_Datatype, void *, MPIABI_Count,
                                    MPIABI_Datatype, MPIABI_Comm,
                                    MPIABI_Request *);
  int (*PMPI_Ineighbor_allgatherv)(const void *, int, MPIABI_Datatype, void *,
                                   const int[], const int[], MPIABI_Datatype,
                                   MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Ineighbor_allgatherv_c)(const void *, MPIABI_Count,
                                     MPIABI_Datatype, void *,
                                     const MPIABI_Count[], const MPIABI_Aint[],
                                     MPIABI_Datatype, MPIABI_Comm,
                                     MPIABI_Request *);
  int (*PMPI_Ineighbor_alltoall)(const void *, int, MPIABI_Datatype, void *,
                                 int, MPIABI_Datatype, MPIABI_Comm,
                                 MPIABI_Request *);
  int (*PMPI_Ineighbor_alltoall_c)(const void *, MPIABI_Count, MPIABI_Datatype,
                                   void *, MPIABI_Count, MPIABI_Datatype,
                                   MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Ineighbor_alltoallv)(const void *, const int[], const int[],
                                  MPIABI_Datatype, void *, const int[],
                                  const int[], MPIABI_Datatype, MPIABI_Comm,
                                  MPIABI_Request *);
  int (*PMPI_Ineighbor_alltoallv_c)(const void *, const MPIABI_Count[],
                                    const MPIABI_Aint[], MPIABI_Datatype,
                                    void *, const MPIABI_Count[],
                                    const MPIABI_Aint[], MPIABI_Datatype,
                                    MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Ineighbor_alltoallw)(const void *, const int[],
                                  const MPIABI_Aint[], const MPIABI_Datatype[],
                                  void *, const int[], const MPIABI_Aint[],
                                  const MPIABI_Datatype[], MPIABI_Comm,
                                  MPIABI_Request *);
  int (*PMPI_Ineighbor_alltoallw_c)(const void *, const MPIABI_Count[],
                                    const MPIABI_Aint[],
                                    const MPIABI_Datatype[], void *,
                                    const MPIABI_Count[], const MPIABI_Aint[],
                                    const MPIABI_Datatype[], MPIABI_Comm,
                                    MPIABI_Request *);
  int (*PMPI_Info_create)(MPIABI_Info *);
  int (*PMPI_Info_create_env)(int, char *[], MPIABI_Info *);
  int (*PMPI_Info_delete)(MPIABI_Info, const char *);
  int (*PMPI_Info_dup)(MPIABI_Info, MPIABI_Info *);
  int (*PMPI_Info_free)(MPIABI_Info *);
  int (*PMPI_Info_get)(MPIABI_Info, const char *, int, char *, int *);
  int (*PMPI_Info_get_nkeys)(MPIABI_Info, int *);
  int (*PMPI_Info_get_nthkey)(MPIABI_Info, int, char *);
  int (*PMPI_Info_get_string)(MPIABI_Info, const char *, int *, char *, int *);
  int (*PMPI_Info_get_valuelen)(MPIABI_Info, const char *, int *, int *);
  int (*PMPI_Info_set)(MPIABI_Info, const char *, const char *);
  int (*PMPI_Init)(int *, char ***);
  int (*PMPI_Init_thread)(int *, char ***, int, int *);
  int (*PMPI_Initialized)(int *);
  int (*PMPI_Intercomm_create)(MPIABI_Comm, int, MPIABI_Comm, int, int,
                               MPIABI_Comm *);
  int (*PMPI_Intercomm_create_from_groups)(MPIABI_Group, int, MPIABI_Group,
                                           int, const char *, MPIABI_Info,
                                           MPIABI_Errhandler, MPIABI_Comm *);
  int (*PMPI_Intercomm_merge)(MPIABI_Comm, int, MPIABI_Comm *);
  int (*PMPI_Iprobe)(int, int, MPIABI_Comm, int *, MPIABI_Status *);
  int (*PMPI_Irecv)(void *, int, MPIABI_Datatype, int, int, MPIABI_Comm,
                    MPIABI_Request *);
  int (*PMPI_Irecv_c)(void *, MPIABI_Count, MPIABI_Datatype, int, int,
                      MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Ireduce)(const void *, void *, int, MPIABI_Datatype, MPIABI_Op,
                      int, MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Ireduce_c)(const void *, void *, MPIABI_Count, MPIABI_Datatype,
                        MPIABI_Op, int, MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Ireduce_scatter)(const void *, void *, const int[],
                              MPIABI_Datatype, MPIABI_Op, MPIABI_Comm,
                              MPIABI_Request *);
  int (*PMPI_Ireduce_scatter_c)(const void *, void *, const MPIABI_Count[],
                                MPIABI_Datatype, MPIABI_Op, MPIABI_Comm,
                                MPIABI_Request *);
  int (*PMPI_Ireduce_scatter_block)(const void *, void *, int, MPIABI_Datatype,
                                    MPIABI_Op, MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Ireduce_scatter_block_c)(const void *, void *, MPIABI_Count,
                                      MPIABI_Datatype, MPIABI_Op, MPIABI_Comm,
                                      MPIABI_Request *);
  int (*PMPI_Irsend)(const void *, int, MPIABI_Datatype, int, int, MPIABI_Comm,
                     MPIABI_Request *);
  int (*PMPI_Irsend_c)(const void *, MPIABI_Count, MPIABI_Datatype, int, int,
                       MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Is_thread_main)(int *);
  int (*PMPI_Iscan)(const void *, void *, int, MPIABI_Datatype, MPIABI_Op,
                    MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Iscan_c)(const void *, void *, MPIABI_Count, MPIABI_Datatype,
                      MPIABI_Op, MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Iscatter)(const void *, int, MPIABI_Datatype, void *, int,
                       MPIABI_Datatype, int, MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Iscatter_c)(const void *, MPIABI_Count, MPIABI_Datatype, void *,
                         MPIABI_Count, MPIABI_Datatype, int, MPIABI_Comm,
                         MPIABI_Request *);
  int (*PMPI_Iscatterv)(const void *, const int[], const int[],
                        MPIABI_Datatype, void *, int, MPIABI_Datatype, int,
                        MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Iscatterv_c)(const void *, const MPIABI_Count[],
                          const MPIABI_Aint[], MPIABI_Datatype, void *,
                          MPIABI_Count, MPIABI_Datatype, int, MPIABI_Comm,
                          MPIABI_Request *);
  int (*PMPI_Isend)(const void *, int, MPIABI_Datatype, int, int, MPIABI_Comm,
                    MPIABI_Request *);
  int (*PMPI_Isend_c)(const void *, MPIABI_Count, MPIABI_Datatype, int, int,
                      MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Isendrecv)(const void *, int, MPIABI_Datatype, int, int, void *,
                        int, MPIABI_Datatype, int, int, MPIABI_Comm,
                        MPIABI_Request *);
  int (*PMPI_Isendrecv_c)(const void *, MPIABI_Count, MPIABI_Datatype, int,
                          int, void *, MPIABI_Count, MPIABI_Datatype, int, int,
                          MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Isendrecv_replace)(void *, int, MPIABI_Datatype, int, int, int,
                                int, MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Isendrecv_replace_c)(void *, MPIABI_Count, MPIABI_Datatype, int,
                                  int, int, int, MPIABI_Comm,
                                  MPIABI_Request *);
  int (*PMPI_Issend)(const void *, int, MPIABI_Datatype, int, int, MPIABI_Comm,
                     MPIABI_Request *);
  int (*PMPI_Issend_c)(const void *, MPIABI_Count, MPIABI_Datatype, int, int,
                       MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Lookup_name)(const char *, MPIABI_Info, char *);
  int (*PMPI_Mprobe)(int, int, MPIABI_Comm, MPIABI_Message *, MPIABI_Status *);
  int (*PMPI_Mrecv)(void *, int, MPIABI_Datatype, MPIABI_Message *,
                    MPIABI_Status *);
  int (*PMPI_Mrecv_c)(void *, MPIABI_Count, MPIABI_Datatype, MPIABI_Message *,
                      MPIABI_Status *);
  int (*PMPI_Neighbor_allgather)(const void *, int, MPIABI_Datatype, void *,
                                 int, MPIABI_Datatype, MPIABI_Comm);
  int (*PMPI_Neighbor_allgather_c)(const void *, MPIABI_Count, MPIABI_Datatype,
                                   void *, MPIABI_Count, MPIABI_Datatype,
                                   MPIABI_Comm);
  int (*PMPI_Neighbor_allgather_init)(const void *, int, MPIABI_Datatype,
                                      void *, int, MPIABI_Datatype,
                                      MPIABI_Comm, MPIABI_Info,
                                      MPIABI_Request *);
  int (*PMPI_Neighbor_allgather_init_c)(const void *, MPIABI_Count,
                                        MPIABI_Datatype, void *, MPIABI_Count,
                                        MPIABI_Datatype, MPIABI_Comm,
                                        MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Neighbor_allgatherv)(const void *, int, MPIABI_Datatype, void *,
                                  const int[], const int[], MPIABI_Datatype,
                                  MPIABI_Comm);
  int (*PMPI_Neighbor_allgatherv_c)(const void *, MPIABI_Count,
                                    MPIABI_Datatype, void *,
                                    const MPIABI_Count[], const MPIABI_Aint[],
                                    MPIABI_Datatype, MPIABI_Comm);
  int (*PMPI_Neighbor_allgatherv_init)(const void *, int, MPIABI_Datatype,
                                       void *, const int[], const int[],
                                       MPIABI_Datatype, MPIABI_Comm,
                                       MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Neighbor_allgatherv_init_c)(const void *, MPIABI_Count,
                                         MPIABI_Datatype, void *,
                                         const MPIABI_Count[],
                                         const MPIABI_Aint[], MPIABI_Datatype,
                                         MPIABI_Comm, MPIABI_Info,
                                         MPIABI_Request *);
  int (*PMPI_Neighbor_alltoall)(const void *, int, MPIABI_Datatype, void *,
                                int, MPIABI_Datatype, MPIABI_Comm);
  int (*PMPI_Neighbor_alltoall_c)(const void *, MPIABI_Count, MPIABI_Datatype,
                                  void *, MPIABI_Count, MPIABI_Datatype,
                                  MPIABI_Comm);
  int (*PMPI_Neighbor_alltoall_init)(const void *, int, MPIABI_Datatype,
                                     void *, int, MPIABI_Datatype, MPIABI_Comm,
                                     MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Neighbor_alltoall_init_c)(const void *, MPIABI_Count,
                                       MPIABI_Datatype, void *, MPIABI_Count,
                                       MPIABI_Datatype, MPIABI_Comm,
                                       MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Neighbor_alltoallv)(const void *, const int[], const int[],
                                 MPIABI_Datatype, void *, const int[],
                                 const int[], MPIABI_Datatype, MPIABI_Comm);
  int (*PMPI_Neighbor_alltoallv_c)(const void *, const MPIABI_Count[],
                                   const MPIABI_Aint[], MPIABI_Datatype,
                                   void *, const MPIABI_Count[],
                                   const MPIABI_Aint[], MPIABI_Datatype,
                                   MPIABI_Comm);
  int (*PMPI_Neighbor_alltoallv_init)(const void *, const int[], const int[],
                                      MPIABI_Datatype, void *, const int[],
                                      const int[], MPIABI_Datatype,
                                      MPIABI_Comm, MPIABI_Info,
                                      MPIABI_Request *);
  int (*PMPI_Neighbor_alltoallv_init_c)(const void *, const MPIABI_Count[],
                                        const MPIABI_Aint[], MPIABI_Datatype,
                                        void *, const MPIABI_Count[],
                                        const MPIABI_Aint[], MPIABI_Datatype,
                                        MPIABI_Comm, MPIABI_Info,
                                        MPIABI_Request *);
  int (*PMPI_Neighbor_alltoallw)(const void *, const int[],
                                 const MPIABI_Aint[], const MPIABI_Datatype[],
                                 void *, const int[], const MPIABI_Aint[],
                                 const MPIABI_Datatype[], MPIABI_Comm);
  int (*PMPI_Neighbor_alltoallw_c)(const void *, const MPIABI_Count[],
                                   const MPIABI_Aint[],
                                   const MPIABI_Datatype[], void *,
                                   const MPIABI_Count[], const MPIABI_Aint[],
                                   const MPIABI_Datatype[], MPIABI_Comm);
  int (*PMPI_Neighbor_alltoallw_init)(const void *, const int[],
                                      const MPIABI_Aint[],
                                      const MPIABI_Datatype[], void *,
                                      const int[], const MPIABI_Aint[],
                                      const MPIABI_Datatype[], MPIABI_Comm,
                                      MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Neighbor_alltoallw_init_c)(const void *, const MPIABI_Count[],
                                        const MPIABI_Aint[],
                                        const MPIABI_Datatype[], void *,
                                        const MPIABI_Count[],
                                        const MPIABI_Aint[],
                                        const MPIABI_Datatype[], MPIABI_Comm,
                                        MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Op_commutative)(MPIABI_Op, int *);
  int (*PMPI_Op_create)(MPIABI_User_function *, int, MPIABI_Op *);
  int (*PMPI_Op_create_c)(MPIABI_User_function_c *, int, MPIABI_Op *);
  int (*PMPI_Op_free)(MPIABI_Op *);
  int (*PMPI_Open_port)(MPIABI_Info, char *);
  int (*PMPI_Pack)(const void *, int, MPIABI_Datatype, void *, int, int *,
                   MPIABI_Comm);
  int (*PMPI_Pack_c)(const void *, MPIABI_Count, MPIABI_Datatype, void *,
                     MPIABI_Count, MPIABI_Count *, MPIABI_Comm);
  int (*PMPI_Pack_external)(const char *, const void *, int, MPIABI_Datatype,
                            void *, MPIABI_Aint, MPIABI_Aint *);
  int (*PMPI_Pack_external_c)(const char *, const void *, MPIABI_Count,
                              MPIABI_Datatype, void *, MPIABI_Count,
                              MPIABI_Count *);
  int (*PMPI_Pack_external_size)(const char *, int, MPIABI_Datatype,
                                 MPIABI_Aint *);
  int (*PMPI_Pack_external_size_c)(const char *, MPIABI_Count, MPIABI_Datatype,
                                   MPIABI_Count *);
  int (*PMPI_Pack_size)(int, MPIABI_Datatype, MPIABI_Comm, int *);
  int (*PMPI_Pack_size_c)(MPIABI_Count, MPIABI_Datatype, MPIABI_Comm,
                          MPIABI_Count *);
  int (*PMPI_Parrived)(MPIABI_Request, int, int *);
  int (*PMPI_Pcontrol)(const int, ...);
  int (*PMPI_Pready)(int, MPIABI_Request);
  int (*PMPI_Pready_list)(int, const int[], MPIABI_Request);
  int (*PMPI_Pready_range)(int, int, MPIABI_Request);
  int (*PMPI_Precv_init)(void *, int, MPIABI_Count, MPIABI_Datatype, int, int,
                         MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Probe)(int, int, MPIABI_Comm, MPIABI_Status *);
  int (*PMPI_Psend_init)(const void *, int, MPIABI_Count, MPIABI_Datatype, int,
                         int, MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Publish_name)(const char *, MPIABI_Info, const char *);
  int (*PMPI_Put)(const void *, int, MPIABI_Datatype, int, MPIABI_Aint, int,
                  MPIABI_Datatype, MPIABI_Win);
  int (*PMPI_Put_c)(const void *, MPIABI_Count, MPIABI_Datatype, int,
                    MPIABI_Aint, MPIABI_Count, MPIABI_Datatype, MPIABI_Win);
  int (*PMPI_Query_thread)(int *);
  int (*PMPI_Raccumulate)(const void *, int, MPIABI_Datatype, int, MPIABI_Aint,
                          int, MPIABI_Datatype, MPIABI_Op, MPIABI_Win,
                          MPIABI_Request *);
  int (*PMPI_Raccumulate_c)(const void *, MPIABI_Count, MPIABI_Datatype, int,
                            MPIABI_Aint, MPIABI_Count, MPIABI_Datatype,
                            MPIABI_Op, MPIABI_Win, MPIABI_Request *);
  int (*PMPI_Recv)(void *, int, MPIABI_Datatype, int, int, MPIABI_Comm,
                   MPIABI_Status *);
  int (*PMPI_Recv_c)(void *, MPIABI_Count, MPIABI_Datatype, int, int,
                     MPIABI_Comm, MPIABI_Status *);
  int (*PMPI_Recv_init)(void *, int, MPIABI_Datatype, int, int, MPIABI_Comm,
                        MPIABI_Request *);
  int (*PMPI_Recv_init_c)(void *, MPIABI_Count, MPIABI_Datatype, int, int,
                          MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Reduce)(const void *, void *, int, MPIABI_Datatype, MPIABI_Op,
                     int, MPIABI_Comm);
  int (*PMPI_Reduce_c)(const void *, void *, MPIABI_Count, MPIABI_Datatype,
                       MPIABI_Op, int, MPIABI_Comm);
  int (*PMPI_Reduce_init)(const void *, void *, int, MPIABI_Datatype,
                          MPIABI_Op, int, MPIABI_Comm, MPIABI_Info,
                          MPIABI_Request *);
  int (*PMPI_Reduce_init_c)(const void *, void *, MPIABI_Count,
                            MPIABI_Datatype, MPIABI_Op, int, MPIABI_Comm,
                            MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Reduce_local)(const void *, void *, int, MPIABI_Datatype,
                           MPIABI_Op);
  int (*PMPI_Reduce_local_c)(const void *, void *, MPIABI_Count,
                             MPIABI_Datatype, MPIABI_Op);
  int (*PMPI_Reduce_scatter)(const void *, void *, const int[],
                             MPIABI_Datatype, MPIABI_Op, MPIABI_Comm);
  int (*PMPI_Reduce_scatter_c)(const void *, void *, const MPIABI_Count[],
                               MPIABI_Datatype, MPIABI_Op, MPIABI_Comm);
  int (*PMPI_Reduce_scatter_block)(const void *, void *, int, MPIABI_Datatype,
                                   MPIABI_Op, MPIABI_Comm);
  int (*PMPI_Reduce_scatter_block_c)(const void *, void *, MPIABI_Count,
                                     MPIABI_Datatype, MPIABI_Op, MPIABI_Comm);
  int (*PMPI_Reduce_scatter_block_init)(const void *, void *, int,
                                        MPIABI_Datatype, MPIABI_Op,
                                        MPIABI_Comm, MPIABI_Info,
                                        MPIABI_Request *);
  int (*PMPI_Reduce_scatter_block_init_c)(const void *, void *, MPIABI_Count,
                                          MPIABI_Datatype, MPIABI_Op,
                                          MPIABI_Comm, MPIABI_Info,
                                          MPIABI_Request *);
  int (*PMPI_Reduce_scatter_init)(const void *, void *, const int[],
                                  MPIABI_Datatype, MPIABI_Op, MPIABI_Comm,
                                  MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Reduce_scatter_init_c)(const void *, void *, const MPIABI_Count[],
                                    MPIABI_Datatype, MPIABI_Op, MPIABI_Comm,
                                    MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Register_datarep)(const char *,
                               MPIABI_Datarep_conversion_function *,
                               MPIABI_Datarep_conversion_function *,
                               MPIABI_Datarep_extent_function *, void *);
  int (*PMPI_Register_datarep_c)(const char *,
                                 MPIABI_Datarep_conversion_function_c *,
                                 MPIABI_Datarep_conversion_function_c *,
                                 MPIABI_Datarep_extent_function *, void *);
  int (*PMPI_Remove_error_class)(int);
  int (*PMPI_Remove_error_code)(int);
  int (*PMPI_Remove_error_string)(int);
  int (*PMPI_Request_free)(MPIABI_Request *);
  int (*PMPI_Request_get_status)(MPIABI_Request, int *, MPIABI_Status *);
  int (*PMPI_Request_get_status_all)(int, const MPIABI_Request[], int *,
                                     MPIABI_Status *);
  int (*PMPI_Request_get_status_any)(int, const MPIABI_Request[], int *, int *,
                                     MPIABI_Status *);
  int (*PMPI_Request_get_status_some)(int, const MPIABI_Request[], int *,
                                      int[], MPIABI_Status *);
  int (*PMPI_Rget)(void *, int, MPIABI_Datatype, int, MPIABI_Aint, int,
                   MPIABI_Datatype, MPIABI_Win, MPIABI_Request *);
  int (*PMPI_Rget_c)(void *, MPIABI_Count, MPIABI_Datatype, int, MPIABI_Aint,
                     MPIABI_Count, MPIABI_Datatype, MPIABI_Win,
                     MPIABI_Request *);
  int (*PMPI_Rget_accumulate)(const void *, int, MPIABI_Datatype, void *, int,
                              MPIABI_Datatype, int, MPIABI_Aint, int,
                              MPIABI_Datatype, MPIABI_Op, MPIABI_Win,
                              MPIABI_Request *);
  int (*PMPI_Rget_accumulate_c)(const void *, MPIABI_Count, MPIABI_Datatype,
                                void *, MPIABI_Count, MPIABI_Datatype, int,
                                MPIABI_Aint, MPIABI_Count, MPIABI_Datatype,
                                MPIABI_Op, MPIABI_Win, MPIABI_Request *);
  int (*PMPI_Rput)(const void *, int, MPIABI_Datatype, int, MPIABI_Aint, int,
                   MPIABI_Datatype, MPIABI_Win, MPIABI_Request *);
  int (*PMPI_Rput_c)(const void *, MPIABI_Count, MPIABI_Datatype, int,
                     MPIABI_Aint, MPIABI_Count, MPIABI_Datatype, MPIABI_Win,
                     MPIABI_Request *);
  int (*PMPI_Rsend)(const void *, int, MPIABI_Datatype, int, int, MPIABI_Comm);
  int (*PMPI_Rsend_c)(const void *, MPIABI_Count, MPIABI_Datatype, int, int,
                      MPIABI_Comm);
  int (*PMPI_Rsend_init)(const void *, int, MPIABI_Datatype, int, int,
                         MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Rsend_init_c)(const void *, MPIABI_Count, MPIABI_Datatype, int,
                           int, MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Scan)(const void *, void *, int, MPIABI_Datatype, MPIABI_Op,
                   MPIABI_Comm);
  int (*PMPI_Scan_c)(const void *, void *, MPIABI_Count, MPIABI_Datatype,
                     MPIABI_Op, MPIABI_Comm);
  int (*PMPI_Scan_init)(const void *, void *, int, MPIABI_Datatype, MPIABI_Op,
                        MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Scan_init_c)(const void *, void *, MPIABI_Count, MPIABI_Datatype,
                          MPIABI_Op, MPIABI_Comm, MPIABI_Info,
                          MPIABI_Request *);
  int (*PMPI_Scatter)(const void *, int, MPIABI_Datatype, void *, int,
                      MPIABI_Datatype, int, MPIABI_Comm);
  int (*PMPI_Scatter_c)(const void *, MPIABI_Count, MPIABI_Datatype, void *,
                        MPIABI_Count, MPIABI_Datatype, int, MPIABI_Comm);
  int (*PMPI_Scatter_init)(const void *, int, MPIABI_Datatype, void *, int,
                           MPIABI_Datatype, int, MPIABI_Comm, MPIABI_Info,
                           MPIABI_Request *);
  int (*PMPI_Scatter_init_c)(const void *, MPIABI_Count, MPIABI_Datatype,
                             void *, MPIABI_Count, MPIABI_Datatype, int,
                             MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Scatterv)(const void *, const int[], const int[], MPIABI_Datatype,
                       void *, int, MPIABI_Datatype, int, MPIABI_Comm);
  int (*PMPI_Scatterv_c)(const void *, const MPIABI_Count[],
                         const MPIABI_Aint[], MPIABI_Datatype, void *,
                         MPIABI_Count, MPIABI_Datatype, int, MPIABI_Comm);
  int (*PMPI_Scatterv_init)(const void *, const int[], const int[],
                            MPIABI_Datatype, void *, int, MPIABI_Datatype, int,
                            MPIABI_Comm, MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Scatterv_init_c)(const void *, const MPIABI_Count[],
                              const MPIABI_Aint[], MPIABI_Datatype, void *,
                              MPIABI_Count, MPIABI_Datatype, int, MPIABI_Comm,
                              MPIABI_Info, MPIABI_Request *);
  int (*PMPI_Send)(const void *, int, MPIABI_Datatype, int, int, MPIABI_Comm);
  int (*PMPI_Send_c)(const void *, MPIABI_Count, MPIABI_Datatype, int, int,
                     MPIABI_Comm);
  int (*PMPI_Send_init)(const void *, int, MPIABI_Datatype, int, int,
                        MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Send_init_c)(const void *, MPIABI_Count, MPIABI_Datatype, int,
                          int, MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Sendrecv)(const void *, int, MPIABI_Datatype, int, int, void *,
                       int, MPIABI_Datatype, int, int, MPIABI_Comm,
                       MPIABI_Status *);
  int (*PMPI_Sendrecv_c)(const void *, MPIABI_Count, MPIABI_Datatype, int, int,
                         void *, MPIABI_Count, MPIABI_Datatype, int, int,
                         MPIABI_Comm, MPIABI_Status *);
  int (*PMPI_Sendrecv_replace)(void *, int, MPIABI_Datatype, int, int, int,
                               int, MPIABI_Comm, MPIABI_Status *);
  int (*PMPI_Sendrecv_replace_c)(void *, MPIABI_Count, MPIABI_Datatype, int,
                                 int, int, int, MPIABI_Comm, MPIABI_Status *);
  int (*PMPI_Session_attach_buffer)(MPIABI_Session, void *, int);
  int (*PMPI_Session_attach_buffer_c)(MPIABI_Session, void *, MPIABI_Count);
  int (*PMPI_Session_call_errhandler)(MPIABI_Session, int);
  int (*PMPI_Session_create_errhandler)(MPIABI_Session_errhandler_function *,
                                        MPIABI_Errhandler *);
  int (*PMPI_Session_detach_buffer)(MPIABI_Session, void *, int *);
  int (*PMPI_Session_detach_buffer_c)(MPIABI_Session, void *, MPIABI_Count *);
  int (*PMPI_Session_finalize)(MPIABI_Session *);
  int (*PMPI_Session_flush_buffer)(MPIABI_Session);
  int (*PMPI_Session_get_errhandler)(MPIABI_Session, MPIABI_Errhandler *);
  int (*PMPI_Session_get_info)(MPIABI_Session, MPIABI_Info *);
  int (*PMPI_Session_get_nth_pset)(MPIABI_Session, MPIABI_Info, int, int *,
                                   char *);
  int (*PMPI_Session_get_num_psets)(MPIABI_Session, MPIABI_Info, int *);
  int (*PMPI_Session_get_pset_info)(MPIABI_Session, const char *,
                                    MPIABI_Info *);
  int (*PMPI_Session_iflush_buffer)(MPIABI_Session, MPIABI_Request *);
  int (*PMPI_Session_init)(MPIABI_Info, MPIABI_Errhandler, MPIABI_Session *);
  int (*PMPI_Session_set_errhandler)(MPIABI_Session, MPIABI_Errhandler);
  int (*PMPI_Ssend)(const void *, int, MPIABI_Datatype, int, int, MPIABI_Comm);
  int (*PMPI_Ssend_c)(const void *, MPIABI_Count, MPIABI_Datatype, int, int,
                      MPIABI_Comm);
  int (*PMPI_Ssend_init)(const void *, int, MPIABI_Datatype, int, int,
                         MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Ssend_init_c)(const void *, MPIABI_Count, MPIABI_Datatype, int,
                           int, MPIABI_Comm, MPIABI_Request *);
  int (*PMPI_Start)(MPIABI_Request *);
  int (*PMPI_Startall)(int, MPIABI_Request[]);
  int (*PMPI_Status_get_error)(const MPIABI_Status *, int *);
  int (*PMPI_Status_get_source)(const MPIABI_Status *, int *);
  int (*PMPI_Status_get_tag)(const MPIABI_Status *, int *);
  int (*PMPI_Status_set_cancelled)(MPIABI_Status *, int);
  int (*PMPI_Status_set_elements)(MPIABI_Status *, MPIABI_Datatype, int);
  int (*PMPI_Status_set_elements_c)(MPIABI_Status *, MPIABI_Datatype,
                                    MPIABI_Count);
  int (*PMPI_Status_set_elements_x)(MPIABI_Status *, MPIABI_Datatype,
                                    MPIABI_Count);
  int (*PMPI_Status_set_error)(MPIABI_Status *, int);
  int (*PMPI_Status_set_source)(MPIABI_Status *, int);
  int (*PMPI_Status_set_tag)(MPIABI_Status *, int);
  int (*PMPI_Test)(MPIABI_Request *, int *, MPIABI_Status *);
  int (*PMPI_Test_cancelled)(const MPIABI_Status *, int *);
  int (*PMPI_Testall)(int, MPIABI_Request[], int *, MPIABI_Status *);
  int (*PMPI_Testany)(int, MPIABI_Request[], int *, int *, MPIABI_Status *);
  int (*PMPI_Testsome)(int, MPIABI_Request[], int *, int[], MPIABI_Status *);
  int (*PMPI_Topo_test)(MPIABI_Comm, int *);
  int (*PMPI_Type_commit)(MPIABI_Datatype *);
  int (*PMPI_Type_contiguous)(int, MPIABI_Datatype, MPIABI_Datatype *);
  int (*PMPI_Type_contiguous_c)(MPIABI_Count, MPIABI_Datatype,
                                MPIABI_Datatype *);
  int (*PMPI_Type_create_darray)(int, int, int, const int[], const int[],
                                 const int[], const int[], int,
                                 MPIABI_Datatype, MPIABI_Datatype *);
  int (*PMPI_Type_create_darray_c)(int, int, int, const MPIABI_Count[],
                                   const int[], const int[], const int[], int,
                                   MPIABI_Datatype, MPIABI_Datatype *);
  int (*PMPI_Type_create_f90_complex)(int, int, MPIABI_Datatype *);
  int (*PMPI_Type_create_f90_integer)(int, MPIABI_Datatype *);
  int (*PMPI_Type_create_f90_real)(int, int, MPIABI_Datatype *);
  int (*PMPI_Type_create_hindexed)(int, const int[], const MPIABI_Aint[],
                                   MPIABI_Datatype, MPIABI_Datatype *);
  int (*PMPI_Type_create_hindexed_c)(MPIABI_Count, const MPIABI_Count[],
                                     const MPIABI_Count[], MPIABI_Datatype,
                                     MPIABI_Datatype *);
  int (*PMPI_Type_create_hindexed_block)(int, int, const MPIABI_Aint[],
                                         MPIABI_Datatype, MPIABI_Datatype *);
  int (*PMPI_Type_create_hindexed_block_c)(MPIABI_Count, MPIABI_Count,
                                           const MPIABI_Count[],
                                           MPIABI_Datatype, MPIABI_Datatype *);
  int (*PMPI_Type_create_hvector)(int, int, MPIABI_Aint, MPIABI_Datatype,
                                  MPIABI_Datatype *);
  int (*PMPI_Type_create_hvector_c)(MPIABI_Count, MPIABI_Count, MPIABI_Count,
                                    MPIABI_Datatype, MPIABI_Datatype *);
  int (*PMPI_Type_create_indexed_block)(int, int, const int[], MPIABI_Datatype,
                                        MPIABI_Datatype *);
  int (*PMPI_Type_create_indexed_block_c)(MPIABI_Count, MPIABI_Count,
                                          const MPIABI_Count[],
                                          MPIABI_Datatype, MPIABI_Datatype *);
  int (*PMPI_Type_create_keyval)(MPIABI_Type_copy_attr_function *,
                                 MPIABI_Type_delete_attr_function *, int *,
                                 void *);
  int (*PMPI_Type_create_resized)(MPIABI_Datatype, MPIABI_Aint, MPIABI_Aint,
                                  MPIABI_Datatype *);
  int (*PMPI_Type_create_resized_c)(MPIABI_Datatype, MPIABI_Count,
                                    MPIABI_Count, MPIABI_Datatype *);
  int (*PMPI_Type_create_struct)(int, const int[], const MPIABI_Aint[],
                                 const MPIABI_Datatype[], MPIABI_Datatype *);
  int (*PMPI_Type_create_struct_c)(MPIABI_Count, const MPIABI_Count[],
                                   const MPIABI_Count[],
                                   const MPIABI_Datatype[], MPIABI_Datatype *);
  int (*PMPI_Type_create_subarray)(int, const int[], const int[], const int[],
                                   int, MPIABI_Datatype, MPIABI_Datatype *);
  int (*PMPI_Type_create_subarray_c)(int, const MPIABI_Count[],
                                     const MPIABI_Count[],
                                     const MPIABI_Count[], int,
                                     MPIABI_Datatype, MPIABI_Datatype *);
  int (*PMPI_Type_delete_attr)(MPIABI_Datatype, int);
  int (*PMPI_Type_dup)(MPIABI_Datatype, MPIABI_Datatype *);
  int (*PMPI_Type_free)(MPIABI_Datatype *);
  int (*PMPI_Type_free_keyval)(int *);
  int (*PMPI_Type_get_attr)(MPIABI_Datatype, int, void *, int *);
  int (*PMPI_Type_get_contents)(MPIABI_Datatype, int, int, int, int[],
                                MPIABI_Aint[], MPIABI_Datatype[]);
  int (*PMPI_Type_get_contents_c)(MPIABI_Datatype, MPIABI_Count, MPIABI_Count,
                                  MPIABI_Count, MPIABI_Count, int[],
                                  MPIABI_Aint[], MPIABI_Count[],
                                  MPIABI_Datatype[]);
  int (*PMPI_Type_get_envelope)(MPIABI_Datatype, int *, int *, int *, int *);
  int (*PMPI_Type_get_envelope_c)(MPIABI_Datatype, MPIABI_Count *,
                                  MPIABI_Count *, MPIABI_Count *,
                                  MPIABI_Count *, int *);
  int (*PMPI_Type_get_extent)(MPIABI_Datatype, MPIABI_Aint *, MPIABI_Aint *);
  int (*PMPI_Type_get_extent_c)(MPIABI_Datatype, MPIABI_Count *,
                                MPIABI_Count *);
  int (*PMPI_Type_get_extent_x)(MPIABI_Datatype, MPIABI_Count *,
                                MPIABI_Count *);
  int (*PMPI_Type_get_name)(MPIABI_Datatype, char *, int *);
  int (*PMPI_Type_get_true_extent)(MPIABI_Datatype, MPIABI_Aint *,
                                   MPIABI_Aint *);
  int (*PMPI_Type_get_true_extent_c)(MPIABI_Datatype, MPIABI_Count *,
                                     MPIABI_Count *);
  int (*PMPI_Type_get_true_extent_x)(MPIABI_Datatype, MPIABI_Count *,
                                     MPIABI_Count *);
  int (*PMPI_Type_get_value_index)(MPIABI_Datatype, MPIABI_Datatype,
                                   MPIABI_Datatype *);
  int (*PMPI_Type_indexed)(int, const int[], const int[], MPIABI_Datatype,
                           MPIABI_Datatype *);
  int (*PMPI_Type_indexed_c)(MPIABI_Count, const MPIABI_Count[],
                             const MPIABI_Count[], MPIABI_Datatype,
                             MPIABI_Datatype *);
  int (*PMPI_Type_match_size)(int, int, MPIABI_Datatype *);
  int (*PMPI_Type_set_attr)(MPIABI_Datatype, int, void *);
  int (*PMPI_Type_set_name)(MPIABI_Datatype, const char *);
  int (*PMPI_Type_size)(MPIABI_Datatype, int *);
  int (*PMPI_Type_size_c)(MPIABI_Datatype, MPIABI_Count *);
  int (*PMPI_Type_size_x)(MPIABI_Datatype, MPIABI_Count *);
  int (*PMPI_Type_vector)(int, int, int, MPIABI_Datatype, MPIABI_Datatype *);
  int (*PMPI_Type_vector_c)(MPIABI_Count, MPIABI_Count, MPIABI_Count,
                            MPIABI_Datatype, MPIABI_Datatype *);
  int (*PMPI_Unpack)(const void *, int, int *, void *, int, MPIABI_Datatype,
                     MPIABI_Comm);
  int (*PMPI_Unpack_c)(const void *, MPIABI_Count, MPIABI_Count *, void *,
                       MPIABI_Count, MPIABI_Datatype, MPIABI_Comm);
  int (*PMPI_Unpack_external)(const char[], const void *, MPIABI_Aint,
                              MPIABI_Aint *, void *, int, MPIABI_Datatype);
  int (*PMPI_Unpack_external_c)(const char[], const void *, MPIABI_Count,
                                MPIABI_Count *, void *, MPIABI_Count,
                                MPIABI_Datatype);
  int (*PMPI_Unpublish_name)(const char *, MPIABI_Info, const char *);
  int (*PMPI_Wait)(MPIABI_Request *, MPIABI_Status *);
  int (*PMPI_Waitall)(int, MPIABI_Request[], MPIABI_Status *);
  int (*PMPI_Waitany)(int, MPIABI_Request[], int *, MPIABI_Status *);
  int (*PMPI_Waitsome)(int, MPIABI_Request[], int *, int[], MPIABI_Status *);
  int (*PMPI_Win_allocate)(MPIABI_Aint, int, MPIABI_Info, MPIABI_Comm, void *,
                           MPIABI_Win *);
  int (*PMPI_Win_allocate_c)(MPIABI_Aint, MPIABI_Aint, MPIABI_Info,
                             MPIABI_Comm, void *, MPIABI_Win *);
  int (*PMPI_Win_allocate_shared)(MPIABI_Aint, int, MPIABI_Info, MPIABI_Comm,
                                  void *, MPIABI_Win *);
  int (*PMPI_Win_allocate_shared_c)(MPIABI_Aint, MPIABI_Aint, MPIABI_Info,
                                    MPIABI_Comm, void *, MPIABI_Win *);
  int (*PMPI_Win_attach)(MPIABI_Win, void *, MPIABI_Aint);
  int (*PMPI_Win_call_errhandler)(MPIABI_Win, int);
  int (*PMPI_Win_complete)(MPIABI_Win);
  int (*PMPI_Win_create)(void *, MPIABI_Aint, int, MPIABI_Info, MPIABI_Comm,
                         MPIABI_Win *);
  int (*PMPI_Win_create_c)(void *, MPIABI_Aint, MPIABI_Aint, MPIABI_Info,
                           MPIABI_Comm, MPIABI_Win *);
  int (*PMPI_Win_create_dynamic)(MPIABI_Info, MPIABI_Comm, MPIABI_Win *);
  int (*PMPI_Win_create_errhandler)(MPIABI_Win_errhandler_function *,
                                    MPIABI_Errhandler *);
  int (*PMPI_Win_create_keyval)(MPIABI_Win_copy_attr_function *,
                                MPIABI_Win_delete_attr_function *, int *,
                                void *);
  int (*PMPI_Win_delete_attr)(MPIABI_Win, int);
  int (*PMPI_Win_detach)(MPIABI_Win, const void *);
  int (*PMPI_Win_fence)(int, MPIABI_Win);
  int (*PMPI_Win_flush)(int, MPIABI_Win);
  int (*PMPI_Win_flush_all)(MPIABI_Win);
  int (*PMPI_Win_flush_local)(int, MPIABI_Win);
  int (*PMPI_Win_flush_local_all)(MPIABI_Win);
  int (*PMPI_Win_free)(MPIABI_Win *);
  int (*PMPI_Win_free_keyval)(int *);
  int (*PMPI_Win_get_attr)(MPIABI_Win, int, void *, int *);
  int (*PMPI_Win_get_errhandler)(MPIABI_Win, MPIABI_Errhandler *);
  int (*PMPI_Win_get_group)(MPIABI_Win, MPIABI_Group *);
  int (*PMPI_Win_get_info)(MPIABI_Win, MPIABI_Info *);
  int (*PMPI_Win_get_name)(MPIABI_Win, char *, int *);
  int (*PMPI_Win_lock)(int, int, int, MPIABI_Win);
  int (*PMPI_Win_lock_all)(int, MPIABI_Win);
  int (*PMPI_Win_post)(MPIABI_Group, int, MPIABI_Win);
  int (*PMPI_Win_set_attr)(MPIABI_Win, int, void *);
  int (*PMPI_Win_set_errhandler)(MPIABI_Win, MPIABI_Errhandler);
  int (*PMPI_Win_set_info)(MPIABI_Win, MPIABI_Info);
  int (*PMPI_Win_set_name)(MPIABI_Win, const char *);
  int (*PMPI_Win_shared_query)(MPIABI_Win, int, MPIABI_Aint *, int *, void *);
  int (*PMPI_Win_shared_query_c)(MPIABI_Win, int, MPIABI_Aint *, MPIABI_Aint *,
                                 void *);
  int (*PMPI_Win_start)(MPIABI_Group, int, MPIABI_Win);
  int (*PMPI_Win_sync)(MPIABI_Win);
  int (*PMPI_Win_test)(MPIABI_Win, int *);
  int (*PMPI_Win_unlock)(int, MPIABI_Win);
  int (*PMPI_Win_unlock_all)(MPIABI_Win);
  int (*PMPI_Win_wait)(MPIABI_Win);
  MPIABI_Aint (*PMPI_Aint_add)(MPIABI_Aint, MPIABI_Aint);
  MPIABI_Aint (*PMPI_Aint_diff)(MPIABI_Aint, MPIABI_Aint);
  double (*PMPI_Wtick)(void);
  double (*PMPI_Wtime)(void);
  MPIABI_Comm (*PMPI_Comm_fromint)(int);
  int (*PMPI_Comm_toint)(MPIABI_Comm);
  MPIABI_Errhandler (*PMPI_Errhandler_fromint)(int);
  int (*PMPI_Errhandler_toint)(MPIABI_Errhandler);
  MPIABI_File (*PMPI_File_fromint)(int);
  int (*PMPI_File_toint)(MPIABI_File);
  MPIABI_Group (*PMPI_Group_fromint)(int);
  int (*PMPI_Group_toint)(MPIABI_Group);
  MPIABI_Info (*PMPI_Info_fromint)(int);
  int (*PMPI_Info_toint)(MPIABI_Info);
  MPIABI_Message (*PMPI_Message_fromint)(int);
  int (*PMPI_Message_toint)(MPIABI_Message);
  MPIABI_Op (*PMPI_Op_fromint)(int);
  int (*PMPI_Op_toint)(MPIABI_Op);
  MPIABI_Request (*PMPI_Request_fromint)(int);
  int (*PMPI_Request_toint)(MPIABI_Request);
  MPIABI_Session (*PMPI_Session_fromint)(int);
  int (*PMPI_Session_toint)(MPIABI_Session);
  MPIABI_Datatype (*PMPI_Type_fromint)(int);
  int (*PMPI_Type_toint)(MPIABI_Datatype);
  MPIABI_Win (*PMPI_Win_fromint)(int);
  int (*PMPI_Win_toint)(MPIABI_Win);
  int (*PMPI_T_category_changed)(int *);
  int (*PMPI_T_category_get_categories)(int, int, int[]);
  int (*PMPI_T_category_get_cvars)(int, int, int[]);
  int (*PMPI_T_category_get_events)(int, int, int[]);
  int (*PMPI_T_category_get_index)(const char *, int *);
  int (*PMPI_T_category_get_info)(int, char *, int *, char *, int *, int *,
                                  int *, int *);
  int (*PMPI_T_category_get_num)(int *);
  int (*PMPI_T_category_get_num_events)(int, int *);
  int (*PMPI_T_category_get_pvars)(int, int, int[]);
  int (*PMPI_T_cvar_get_index)(const char *, int *);
  int (*PMPI_T_cvar_get_info)(int, char *, int *, int *, MPIABI_Datatype *,
                              MPIABI_T_enum *, char *, int *, int *, int *);
  int (*PMPI_T_cvar_get_num)(int *);
  int (*PMPI_T_cvar_handle_alloc)(int, void *, MPIABI_T_cvar_handle *, int *);
  int (*PMPI_T_cvar_handle_free)(MPIABI_T_cvar_handle *);
  int (*PMPI_T_cvar_read)(MPIABI_T_cvar_handle, void *);
  int (*PMPI_T_cvar_write)(MPIABI_T_cvar_handle, const void *);
  int (*PMPI_T_enum_get_info)(MPIABI_T_enum, int *, char *, int *);
  int (*PMPI_T_enum_get_item)(MPIABI_T_enum, int, int *, char *, int *);
  int (*PMPI_T_event_callback_get_info)(MPIABI_T_event_registration,
                                        MPIABI_T_cb_safety, MPIABI_Info *);
  int (*PMPI_T_event_callback_set_info)(MPIABI_T_event_registration,
                                        MPIABI_T_cb_safety, MPIABI_Info);
  int (*PMPI_T_event_copy)(MPIABI_T_event_instance, void *);
  int (*PMPI_T_event_get_index)(const char *, int *);
  int (*PMPI_T_event_get_info)(int, char *, int *, int *, MPIABI_Datatype[],
                               MPIABI_Aint[], int *, MPIABI_T_enum *,
                               MPIABI_Info *, char *, int *, int *);
  int (*PMPI_T_event_get_num)(int *);
  int (*PMPI_T_event_get_source)(MPIABI_T_event_instance, int *);
  int (*PMPI_T_event_get_timestamp)(MPIABI_T_event_instance, MPIABI_Count *);
  int (*PMPI_T_event_handle_alloc)(int, void *, MPIABI_Info,
                                   MPIABI_T_event_registration *);
  int (*PMPI_T_event_handle_free)(MPIABI_T_event_registration, void *,
                                  MPIABI_T_event_free_cb_function);
  int (*PMPI_T_event_handle_get_info)(MPIABI_T_event_registration,
                                      MPIABI_Info *);
  int (*PMPI_T_event_handle_set_info)(MPIABI_T_event_registration,
                                      MPIABI_Info);
  int (*PMPI_T_event_read)(MPIABI_T_event_instance, int, void *);
  int (*PMPI_T_event_register_callback)(MPIABI_T_event_registration,
                                        MPIABI_T_cb_safety, MPIABI_Info,
                                        void *, MPIABI_T_event_cb_function);
  int (*PMPI_T_event_set_dropped_handler)(MPIABI_T_event_registration,
                                          MPIABI_T_event_dropped_cb_function);
  int (*PMPI_T_finalize)(void);
  int (*PMPI_T_init_thread)(int, int *);
  int (*PMPI_T_pvar_get_index)(const char *, int, int *);
  int (*PMPI_T_pvar_get_info)(int, char *, int *, int *, int *,
                              MPIABI_Datatype *, MPIABI_T_enum *, char *,
                              int *, int *, int *, int *, int *);
  int (*PMPI_T_pvar_get_num)(int *);
  int (*PMPI_T_pvar_handle_alloc)(MPIABI_T_pvar_session, int, void *,
                                  MPIABI_T_pvar_handle *, int *);
  int (*PMPI_T_pvar_handle_free)(MPIABI_T_pvar_session,
                                 MPIABI_T_pvar_handle *);
  int (*PMPI_T_pvar_read)(MPIABI_T_pvar_session, MPIABI_T_pvar_handle, void *);
  int (*PMPI_T_pvar_readreset)(MPIABI_T_pvar_session, MPIABI_T_pvar_handle,
                               void *);
  int (*PMPI_T_pvar_reset)(MPIABI_T_pvar_session, MPIABI_T_pvar_handle);
  int (*PMPI_T_pvar_session_create)(MPIABI_T_pvar_session *);
  int (*PMPI_T_pvar_session_free)(MPIABI_T_pvar_session *);
  int (*PMPI_T_pvar_start)(MPIABI_T_pvar_session, MPIABI_T_pvar_handle);
  int (*PMPI_T_pvar_stop)(MPIABI_T_pvar_session, MPIABI_T_pvar_handle);
  int (*PMPI_T_pvar_write)(MPIABI_T_pvar_session, MPIABI_T_pvar_handle,
                           const void *);
  int (*PMPI_T_source_get_info)(int, char *, int *, char *, int *,
                                MPIABI_T_source_order *, MPIABI_Count *,
                                MPIABI_Count *, MPIABI_Info *);
  int (*PMPI_T_source_get_num)(int *);
  int (*PMPI_T_source_get_timestamp)(int, MPIABI_Count *);
  int (*PMPI_Status_f2c)(const MPIABI_Fint *, MPIABI_Status *);
  int (*PMPI_Status_c2f)(const MPIABI_Status *, MPIABI_Fint *);
  int (*PMPI_Status_f082c)(const MPIABI_F08_Status *, MPIABI_Status *);
  int (*PMPI_Status_c2f08)(const MPIABI_Status *, MPIABI_F08_Status *);
  int (*MPI_Status_f2c)(const MPIABI_Fint *, MPIABI_Status *);
  int (*MPI_Status_c2f)(const MPIABI_Status *, MPIABI_Fint *);
  int (*MPI_Status_f082c)(const MPIABI_F08_Status *, MPIABI_Status *);
  int (*MPI_Status_c2f08)(const MPIABI_Status *, MPIABI_F08_Status *);
  MPIABI_Comm (*PMPI_Comm_f2c)(MPIABI_Fint);
  MPIABI_Fint (*PMPI_Comm_c2f)(MPIABI_Comm);
  MPIABI_Errhandler (*PMPI_Errhandler_f2c)(MPIABI_Fint);
  MPIABI_Fint (*PMPI_Errhandler_c2f)(MPIABI_Errhandler);
  MPIABI_File (*PMPI_File_f2c)(MPIABI_Fint);
  MPIABI_Fint (*PMPI_File_c2f)(MPIABI_File);
  MPIABI_Group (*PMPI_Group_f2c)(MPIABI_Fint);
  MPIABI_Fint (*PMPI_Group_c2f)(MPIABI_Group);
  MPIABI_Info (*PMPI_Info_f2c)(MPIABI_Fint);
  MPIABI_Fint (*PMPI_Info_c2f)(MPIABI_Info);
  MPIABI_Message (*PMPI_Message_f2c)(MPIABI_Fint);
  MPIABI_Fint (*PMPI_Message_c2f)(MPIABI_Message);
  MPIABI_Op (*PMPI_Op_f2c)(MPIABI_Fint);
  MPIABI_Fint (*PMPI_Op_c2f)(MPIABI_Op);
  MPIABI_Request (*PMPI_Request_f2c)(MPIABI_Fint);
  MPIABI_Fint (*PMPI_Request_c2f)(MPIABI_Request);
  MPIABI_Session (*PMPI_Session_f2c)(MPIABI_Fint);
  MPIABI_Fint (*PMPI_Session_c2f)(MPIABI_Session);
  MPIABI_Datatype (*PMPI_Type_f2c)(MPIABI_Fint);
  MPIABI_Fint (*PMPI_Type_c2f)(MPIABI_Datatype);
  MPIABI_Win (*PMPI_Win_f2c)(MPIABI_Fint);
  MPIABI_Fint (*PMPI_Win_c2f)(MPIABI_Win);
  MPIABI_Comm (*MPI_Comm_f2c)(MPIABI_Fint);
  MPIABI_Fint (*MPI_Comm_c2f)(MPIABI_Comm);
  MPIABI_Errhandler (*MPI_Errhandler_f2c)(MPIABI_Fint);
  MPIABI_Fint (*MPI_Errhandler_c2f)(MPIABI_Errhandler);
  MPIABI_File (*MPI_File_f2c)(MPIABI_Fint);
  MPIABI_Fint (*MPI_File_c2f)(MPIABI_File);
  MPIABI_Group (*MPI_Group_f2c)(MPIABI_Fint);
  MPIABI_Fint (*MPI_Group_c2f)(MPIABI_Group);
  MPIABI_Info (*MPI_Info_f2c)(MPIABI_Fint);
  MPIABI_Fint (*MPI_Info_c2f)(MPIABI_Info);
  MPIABI_Message (*MPI_Message_f2c)(MPIABI_Fint);
  MPIABI_Fint (*MPI_Message_c2f)(MPIABI_Message);
  MPIABI_Op (*MPI_Op_f2c)(MPIABI_Fint);
  MPIABI_Fint (*MPI_Op_c2f)(MPIABI_Op);
  MPIABI_Request (*MPI_Request_f2c)(MPIABI_Fint);
  MPIABI_Fint (*MPI_Request_c2f)(MPIABI_Request);
  MPIABI_Session (*MPI_Session_f2c)(MPIABI_Fint);
  MPIABI_Fint (*MPI_Session_c2f)(MPIABI_Session);
  MPIABI_Datatype (*MPI_Type_f2c)(MPIABI_Fint);
  MPIABI_Fint (*MPI_Type_c2f)(MPIABI_Datatype);
  MPIABI_Win (*MPI_Win_f2c)(MPIABI_Fint);
  MPIABI_Fint (*MPI_Win_c2f)(MPIABI_Win);
};

/* The only symbol libmpiwrapper exports.
 *
 * Returns NULL and sets *diagnostic on any mismatch. A getter rather than an
 * exported struct, because reading a version field out of a struct means
 * trusting the layout you are trying to validate -- and because this is the
 * natural place for the wrapper to build its reverse handle map, and to check
 * its own symbol resolution, before anyone can call a slot.
 *
 * `size` is sizeof(struct mpiwrapper_vtable) as the *caller* understands it,
 * and it must match exactly. What the size catches is the one mismatch the
 * hash cannot: the hash is taken over the *text* of the slot list, so two
 * halves built for different targets (32- against 64-bit) or with
 * incompatible struct-layout settings hash identically and disagree about
 * sizeof.
 *
 * Both `abi_version` and `abi_subversion` are checked, against the header's
 * MPI_ABI_VERSION and MPI_ABI_SUBVERSION. The layout hash alone is not enough:
 * a subversion that added no slot would leave it unchanged.
 *
 * `abi_probe` is the address of any function in libmpi_abi. The wrapper
 * dladdr()s it together with the MPI_Send it actually resolved and refuses if
 * the two share a base object, which would mean the loader bound the wrapper's
 * calls back into libmpi_abi instead of out to the implementation -- infinite
 * recursion, and on ELF the default outcome unless the wrapper is loaded into
 * its own namespace or with RTLD_DEEPBIND. See the long comment in
 * src/mpi_abi/bootstrap.c.
 */
/* The wrapper is compiled -fvisibility=hidden, so this attribute is what makes
 * the single exported symbol single rather than merely intended: everything
 * else in libmpiwrapper is unexported by the language, not by a linker script
 * that has to be kept in step. test/check_exports.cmake confirms it with nm.
 */
#if defined(__GNUC__)
#  define MPIWRAPPER_EXPORT __attribute__((visibility("default")))
#else
#  define MPIWRAPPER_EXPORT
#endif

MPIWRAPPER_EXPORT const struct mpiwrapper_vtable *
mpiwrapper_get_vtable(uint32_t abi_version, uint32_t abi_subversion,
                      uint32_t layout_hash, size_t size, const void *abi_probe,
                      const char **diagnostic);

#endif /* MPIWRAPPER_VTABLE_H */
