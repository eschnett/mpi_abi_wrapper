# Erik's open tasks

- add a licence (same as mpif)

- remove the call overhead in the `mpi_abi` entrypoints: define the
  functions `static inline`? requires changing the MPI ABI `mpi.h`.

- add more tests: sanitizer, 32 bit architecture

- let the MPICH C suite CI rows gate: they are report-only
  (`continue-on-error`) until a run on a GitHub runner has been green

- add README

- add badges to README

- use separate xfail lists for local runs and github ci. do not ignore mpich failures (not report-only).

- add test with MPI 3.0? or 3.1?

- speed up github CI (slow runs only on merge, or when manually triggered; PRs remain fast)
