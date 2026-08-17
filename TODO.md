# Erik's open review questions

- test against MVAPICH as well

- test with Intel and Nvidia compilers

- test FreeBSD

- add a licence (same as mpif)

- remove the call overhead in the `mpi_abi` entrypoints: define the
  functions `static inline`? requires changing the MPI ABI `mpi.h`.

- add more tests: cross-tests, sanitizer, 32 bit architecture

- add those tests to CI

- add MPICH C test to CI

- test on aarch64

- update README

- add badges to README
