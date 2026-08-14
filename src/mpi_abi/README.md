# `src/mpi_abi/`

Hand-written: bootstrap, `dlopen` isolation, vtable acquisition. NOTES.md §2.

- `bootstrap.c` — permanent. The constructor, the environment-variable /
  build-time-path lookup for `libmpiwrapper`, the per-platform `dlopen` mode
  (`RTLD_LOCAL` on macOS, `RTLD_LOCAL | RTLD_DEEPBIND` or `dlmopen` on Linux),
  and the version/subversion/layout-hash handshake. It passes the address of one
  of its own functions to `mpiwrapper_get_vtable`, which is how the wrapper
  proves at load that its `MPI_*` calls resolved outward.
- `entrypoints.c` — **S1 stand-in for `gen/mpi_abi/entrypoints.c`**. One line per
  entry point, no conversion, no cast, no initialization check; 28 entry points
  now, 688 when the generator writes it in S2.

Environment variables it reads:

| | |
|---|---|
| `MPI_ABI_WRAPPER_LIB` | which `libmpiwrapper` to load; overrides the built-in path |
| `MPI_ABI_WRAPPER_DLOPEN_MODE` | `dlmopen` on glibc, or `capture` (tests only: the unisolated load, so that the outward-resolution check can be shown to fire) |
| `MPI_ABI_WRAPPER_BIND_NOW` | use `RTLD_NOW` instead of the default `RTLD_LAZY` |
