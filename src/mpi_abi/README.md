# `src/mpi_abi/`

Hand-written: bootstrap, `dlopen` isolation, vtable acquisition. NOTES.md §2.

- `bootstrap.c` — permanent. The constructor, the environment-variable /
  build-time-path lookup for `libmpiwrapper`, the per-platform `dlopen` mode
  (`RTLD_LOCAL` on macOS, `RTLD_LOCAL | RTLD_DEEPBIND` or `dlmopen` on Linux),
  and the version/subversion/layout-hash handshake. It passes the address of one
  of its own functions to `mpiwrapper_get_vtable`, which is how the wrapper
  proves at load that its `MPI_*` calls resolved outward — and then makes one
  `MPI_Get_version` call through a decoy vtable, because on macOS the address
  check can say "outward" while dyld's weak coalescing sends the actual call
  back into us (`NOTES.md` §2).
The entry points themselves are `gen/mpi_abi/entrypoints.c` since S2: one line
per entry point, no conversion, no cast, no initialization check, 688 of them
(1376 definitions). S1's 29-entry-point stand-in is frozen in
`dev/s1-reference/` as what the generator has to reproduce.

Environment variables it reads:

| | |
|---|---|
| `MPI_ABI_WRAPPER_LIB` | which `libmpiwrapper` to load; overrides the built-in path |
| `MPI_ABI_WRAPPER_DLOPEN_MODE` | `dlmopen` on glibc (kept selectable, but known not to work with any MPI that `dlopen`s components — `NOTES.md` §2), or `capture` (tests only: the unisolated load, so that the outward-resolution check can be shown to fire) |
| `MPI_ABI_WRAPPER_BIND_NOW` | use `RTLD_NOW` instead of the default `RTLD_LAZY` |
