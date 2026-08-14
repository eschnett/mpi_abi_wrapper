/* Excerpt of the generated gen/include/mpiwrapper_vtable.h.
 *
 * This is the only thing the two libraries share. Both sides are generated from
 * the same slot list, so they cannot disagree about the layout by accident -- but
 * they are built at different times against different MPIs, so they can disagree
 * by *version*, and MPIWRAPPER_LAYOUT_HASH is what turns that into a clean
 * failure instead of a call through a shifted slot.
 *
 * The real file has 688 slots. Six are shown.
 */

#ifndef MPIWRAPPER_VTABLE_H
#define MPIWRAPPER_VTABLE_H

#include <stddef.h>
#include <stdint.h>

/* The MPIABI_ view of the ABI: every type and constant renamed, no prototypes.
 *
 * The renaming touches typedef names and macro/enum names only. Struct *tags* are
 * left alone, so `MPIABI_Comm` and the ABI header's own `MPI_Comm` are both
 * `struct MPI_ABI_Comm *` -- the same type, not two incompatible ones. That is
 * what lets the ABI side forward its arguments into a slot without a single cast
 * (see mpi_abi_side.c). Struct *member* names are left alone too, since members
 * live in a per-struct namespace and cannot collide: MPIABI_Status has fields
 * MPI_SOURCE, MPI_TAG, MPI_ERROR, MPI_internal, exactly as the ABI header does.
 */
#include "mpiabi.h"

/* Generated from the slot list: names, order, and each slot's signature. Any
 * edit that changes the struct below changes this value.
 */
#define MPIWRAPPER_LAYOUT_HASH 0x9f2c41beu

struct mpiwrapper_vtable {
  int    (*MPI_Send)(const void *, int, MPIABI_Datatype, int, int, MPIABI_Comm);
  int    (*MPI_Recv)(void *, int, MPIABI_Datatype, int, int, MPIABI_Comm,
                     MPIABI_Status *);
  int    (*MPI_Waitall)(int, MPIABI_Request *, MPIABI_Status *);
  int    (*MPI_Op_create)(MPIABI_User_function *, int, MPIABI_Op *);
  int    (*MPI_File_open)(MPIABI_Comm, const char *, int, MPIABI_Info,
                          MPIABI_File *);
  int    (*MPI_Error_string)(int, char *, int *);
  double (*MPI_Wtime)(void);
  /* ... 681 more ... */
};

/* The only symbol libmpiwrapper exports.
 *
 * Returns NULL and sets *diagnostic on any mismatch. A getter rather than an
 * exported struct, because reading a version field out of a struct means trusting
 * the layout you are trying to validate -- and because this is the natural place
 * for the wrapper to build its reverse handle map, and to check its own symbol
 * resolution, before anyone can call a slot.
 *
 * `size` is sizeof(struct mpiwrapper_vtable) as the *caller* understands it. A
 * wrapper may accept a smaller size than its own and serve the common prefix; it
 * must refuse a larger one, since the caller would read past the end.
 *
 * `abi_probe` is the address of any function in libmpi_abi. The wrapper dladdr()s
 * it together with the MPI_Send it actually resolved and refuses if the two share
 * a base object, which would mean the loader bound the wrapper's calls back into
 * libmpi_abi instead of out to the implementation -- infinite recursion, and on
 * ELF the default outcome unless the wrapper is loaded into its own namespace or
 * with RTLD_DEEPBIND. See the long comment in mpi_abi_side.c.
 */
const struct mpiwrapper_vtable *
mpiwrapper_get_vtable(uint32_t abi_version, uint32_t layout_hash, size_t size,
                      const void *abi_probe, const char **diagnostic);

#endif /* MPIWRAPPER_VTABLE_H */
