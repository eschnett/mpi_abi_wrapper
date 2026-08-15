/* The vtable: the only thing libmpi_abi and libmpiwrapper share.
 *
 * S1 STATUS -- this file is HAND-WRITTEN, and it is the one file in this stage
 * that will not survive S2. The real one is gen/include/mpiwrapper_vtable.h,
 * emitted by the generator from the 1376-slot list; this is the 58-slot
 * prototype of it (29 entry points x {MPI_, PMPI_}), written first so that the
 * generator is designed against a known output shape rather than the other way
 * round (NOTES.md #11, decision 17). S2 replaces it; nothing else in
 * src/mpi_abi/ or src/mpiwrapper/ has to change when it does, which is part of
 * what this stage is checking.
 *
 * Both sides are generated from the same slot list, so they cannot disagree
 * about the layout by accident -- but they are built at different times against
 * different MPIs, so they can disagree by *version*, and MPIWRAPPER_LAYOUT_HASH
 * is what turns that into a clean failure instead of a call through a shifted
 * slot.
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
 * are both `struct MPI_ABI_Comm *` -- the same type, not two incompatible ones.
 * That is what lets the ABI side forward its arguments into a slot without a
 * single cast (see src/mpi_abi/entrypoints.c). Struct *members* are left alone
 * too: MPIABI_Status has fields MPI_SOURCE, MPI_TAG, MPI_ERROR, MPI_internal,
 * exactly as the ABI header does.
 */
#include "mpiabi.h"

/* Generated from the slot list below by dev/layout_hash.py, which normalizes
 * the declarations (comments and whitespace removed) and takes FNV-1a/32 over
 * the result. Any edit that changes the struct changes this value, and
 * `ctest -R layout-hash` fails until it is updated. S2's generator computes it
 * the same way over its own emitted slot list.
 */
#define MPIWRAPPER_LAYOUT_HASH 0x35d8051bu

/* One slot per ABI entry point, so 1376 in the finished library and 58 here:
 * MPI_X and PMPI_X get their own, and each leads to a wrapper body that calls
 * the implementation's correspondingly-shifted name. Routing both to a single
 * MPI_X slot would be cheaper, but then an application calling PMPI_Send to
 * bypass profiling would still be seen by a tool interposed between the wrapper
 * and the implementation -- it would have bypassed the ABI-level profiling
 * layer only. Keeping them distinct also makes the ledger 1:1 rather than 2:1,
 * so "each entry point has exactly one slot and one body" is a uniform
 * invariant with no special case.
 *
 * Both names are always available to link against, though not in the shape
 * NOTES.md #2 originally recorded from Linux: on macOS the conda-forge MPICH
 * 4.3.1 build keeps every PMPI_* in a separate libpmpi.dylib (libmpi.dylib has
 * a strong MPI_Send and no PMPI_ symbols at all), and Open MPI 5.0.10 has
 * MPI_Send and PMPI_Send as two distinct definitions rather than an alias pair.
 * Either way both names resolve as long as the wrapper links what mpicc links,
 * which is what src/mpiwrapper/ does. No probe and no fallback: an
 * implementation that really lacked PMPI_Send would fail to link here, naming
 * the symbol, which is the outcome NOTES.md #5.9 asks for.
 */
struct mpiwrapper_vtable {
  int (*MPI_Allreduce)(const void *, void *, int, MPIABI_Datatype, MPIABI_Op,
                       MPIABI_Comm);
  int (*PMPI_Allreduce)(const void *, void *, int, MPIABI_Datatype, MPIABI_Op,
                        MPIABI_Comm);
  MPIABI_Fint (*MPI_Comm_c2f)(MPIABI_Comm);
  MPIABI_Fint (*PMPI_Comm_c2f)(MPIABI_Comm);
  int (*MPI_Comm_create_errhandler)(MPIABI_Comm_errhandler_function *,
                                    MPIABI_Errhandler *);
  int (*PMPI_Comm_create_errhandler)(MPIABI_Comm_errhandler_function *,
                                     MPIABI_Errhandler *);
  MPIABI_Comm (*MPI_Comm_f2c)(MPIABI_Fint);
  MPIABI_Comm (*PMPI_Comm_f2c)(MPIABI_Fint);
  int (*MPI_Comm_free)(MPIABI_Comm *);
  int (*PMPI_Comm_free)(MPIABI_Comm *);
  int (*MPI_Comm_rank)(MPIABI_Comm, int *);
  int (*PMPI_Comm_rank)(MPIABI_Comm, int *);
  int (*MPI_Comm_set_errhandler)(MPIABI_Comm, MPIABI_Errhandler);
  int (*PMPI_Comm_set_errhandler)(MPIABI_Comm, MPIABI_Errhandler);
  int (*MPI_Comm_size)(MPIABI_Comm, int *);
  int (*PMPI_Comm_size)(MPIABI_Comm, int *);
  int (*MPI_Comm_split)(MPIABI_Comm, int, int, MPIABI_Comm *);
  int (*PMPI_Comm_split)(MPIABI_Comm, int, int, MPIABI_Comm *);
  int (*MPI_Error_string)(int, char *, int *);
  int (*PMPI_Error_string)(int, char *, int *);
  int (*MPI_File_close)(MPIABI_File *);
  int (*PMPI_File_close)(MPIABI_File *);
  int (*MPI_File_open)(MPIABI_Comm, const char *, int, MPIABI_Info,
                       MPIABI_File *);
  int (*PMPI_File_open)(MPIABI_Comm, const char *, int, MPIABI_Info,
                        MPIABI_File *);
  int (*MPI_Finalize)(void);
  int (*PMPI_Finalize)(void);
  int (*MPI_Get_count)(const MPIABI_Status *, MPIABI_Datatype, int *);
  int (*PMPI_Get_count)(const MPIABI_Status *, MPIABI_Datatype, int *);
  int (*MPI_Get_version)(int *, int *);
  int (*PMPI_Get_version)(int *, int *);
  int (*MPI_Ialltoallw)(const void *, const int[], const int[],
                        const MPIABI_Datatype[], void *, const int[],
                        const int[], const MPIABI_Datatype[], MPIABI_Comm,
                        MPIABI_Request *);
  int (*PMPI_Ialltoallw)(const void *, const int[], const int[],
                         const MPIABI_Datatype[], void *, const int[],
                         const int[], const MPIABI_Datatype[], MPIABI_Comm,
                         MPIABI_Request *);
  int (*MPI_Init)(int *, char ***);
  int (*PMPI_Init)(int *, char ***);
  int (*MPI_Irecv)(void *, int, MPIABI_Datatype, int, int, MPIABI_Comm,
                   MPIABI_Request *);
  int (*PMPI_Irecv)(void *, int, MPIABI_Datatype, int, int, MPIABI_Comm,
                    MPIABI_Request *);
  int (*MPI_Isend)(const void *, int, MPIABI_Datatype, int, int, MPIABI_Comm,
                   MPIABI_Request *);
  int (*PMPI_Isend)(const void *, int, MPIABI_Datatype, int, int, MPIABI_Comm,
                    MPIABI_Request *);
  int (*MPI_Op_create)(MPIABI_User_function *, int, MPIABI_Op *);
  int (*PMPI_Op_create)(MPIABI_User_function *, int, MPIABI_Op *);
  int (*MPI_Op_free)(MPIABI_Op *);
  int (*PMPI_Op_free)(MPIABI_Op *);
  int (*MPI_Recv)(void *, int, MPIABI_Datatype, int, int, MPIABI_Comm,
                  MPIABI_Status *);
  int (*PMPI_Recv)(void *, int, MPIABI_Datatype, int, int, MPIABI_Comm,
                   MPIABI_Status *);
  int (*MPI_Send)(const void *, int, MPIABI_Datatype, int, int, MPIABI_Comm);
  int (*PMPI_Send)(const void *, int, MPIABI_Datatype, int, int, MPIABI_Comm);
  int (*MPI_Type_commit)(MPIABI_Datatype *);
  int (*PMPI_Type_commit)(MPIABI_Datatype *);
  int (*MPI_Type_create_struct)(int, const int[], const MPIABI_Aint[],
                                const MPIABI_Datatype[], MPIABI_Datatype *);
  int (*PMPI_Type_create_struct)(int, const int[], const MPIABI_Aint[],
                                 const MPIABI_Datatype[], MPIABI_Datatype *);
  int (*MPI_Type_create_struct_c)(MPIABI_Count, const MPIABI_Count[],
                                  const MPIABI_Count[], const MPIABI_Datatype[],
                                  MPIABI_Datatype *);
  int (*PMPI_Type_create_struct_c)(MPIABI_Count, const MPIABI_Count[],
                                   const MPIABI_Count[],
                                   const MPIABI_Datatype[], MPIABI_Datatype *);
  int (*MPI_Type_free)(MPIABI_Datatype *);
  int (*PMPI_Type_free)(MPIABI_Datatype *);
  int (*MPI_Waitall)(int, MPIABI_Request[], MPIABI_Status *);
  int (*PMPI_Waitall)(int, MPIABI_Request[], MPIABI_Status *);
  double (*MPI_Wtime)(void);
  double (*PMPI_Wtime)(void);
};

/* The only symbol libmpiwrapper exports.
 *
 * Returns NULL and sets *diagnostic on any mismatch. A getter rather than an
 * exported struct, because reading a version field out of a struct means
 * trusting the layout you are trying to validate -- and because this is the
 * natural place for the wrapper to build its reverse handle map, and to check
 * its own symbol resolution, before anyone can call a slot.
 *
 * `size` is sizeof(struct mpiwrapper_vtable) as the *caller* understands it, and
 * it must match exactly. Serving a common prefix to a caller with fewer slots
 * was the earlier contract and was never reachable: the layout hash covers the
 * whole slot list, so such a caller is refused before the size is looked at.
 * What the size does catch is the one mismatch the hash cannot -- the hash is
 * taken over the *text* of the slot list, so two halves built for different
 * targets (32- against 64-bit) or with incompatible struct-layout settings hash
 * identically and disagree about sizeof.
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
