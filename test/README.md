# `test/`

Our own tests (NOTES.md §9), as opposed to the MPICH suite in
`ci-scripts/suite/`.

- `compile_mpi_h.c`, `compile_mpiabi_h.c`, `compile_both_headers.c` -- the S0
  exit check (STAGES.md): both generated headers compile standalone in a TU
  of their own, and compile together in one TU (as `libmpiwrapper` will
  include them) with no tag/typedef/macro/enumerator collision.

`mpiwrapper_selftest` (S1's dynamic-handle collision probe and friends) and
the rest of this directory's content arrive with the stages that need them.
