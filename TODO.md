# Erik's open tasks

- remove the call overhead in the `mpi_abi` entrypoints: define the
  functions `static inline`? requires changing the MPI ABI `mpi.h`.

- let the MPICH C suite CI rows gate: they are report-only
  (`continue-on-error`) until a run on a GitHub runner has been green

- use separate xfail lists for local runs and github ci. do not ignore mpich failures (not report-only).

- add test with MPI 3.0? or 3.1?

- speed up github CI


