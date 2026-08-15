# Erik's open review questions

- test against MVAPICH as well

- test with Intel and Nvidia compilers

- test FreeBSD

- add a licence (same as mpif)

- remove the vtable size check. require exact vtable equality.

- getvtable: correct the comment.

- advise user to use `dlopen`.

- install: all files in same prefix (mpi.h, libmpiwrapper.so,
  libmpi_abi.so), not shared with any other MPI implementation or
  wrapper to avoid file name conflicts, and also not next to the MPI
  library it wraps.
