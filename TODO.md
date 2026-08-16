# Erik's open review questions

- test against MVAPICH as well

- test with Intel and Nvidia compilers

- test FreeBSD

- add a licence (same as mpif)

- remove the call overhead in the `mpi_abi` entrypoints: define the
  functions `static inline`? requires changing the MPI ABI `mpi.h`.

- why doesn't the status keep the error all the time?

- status needs to be converted on both input and output so that "error
  field is written" can be seen by the caller. (or maybe the current
  implementation is correct and efficient too.)

- `MPI_DISPLACEMENT_CURRENT` is a constant, not a sentinel
