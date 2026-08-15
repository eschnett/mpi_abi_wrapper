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

**Five of them do not forward to their own slot**, and they are the only place
this library contains anything but a forwarder. `MPI_Attr_delete`,
`MPI_Attr_get`, `MPI_Attr_put`, `MPI_Keyval_create` and `MPI_Keyval_free` are
what MPI-3.0 *deleted* from the standard. The ABI header still declares them —
an ABI is a promise about symbols, and withdrawing one breaks a binary that was
linked years ago — but an implementation is under no obligation to define them,
and Open MPI main's `libmpi_abi` does not: it declares all 688 and defines 683.

A slot would make that a **link** failure of the whole wrapper rather than the
run-time report decision 6 promises, because `dev/probe_impl.py` asks the
compiler and the compiler sees the declaration. So these five are answered here
instead, each calling the slot of the MPI-2 entry point that replaced it —
`MPI_Attr_get` reaches the implementation's `MPI_Comm_get_attr` and
`PMPI_Attr_get` its `PMPI_Comm_get_attr`, so the shifted-name rule survives the
rename. The generator checks that each pair really is the same call: return
type, arity and every parameter type, with the two MPI-1 callback typedefs
compared by the function type they name. They have no slot and no wrapper body,
so `libmpiwrapper` never mentions them; `libmpi_abi` still exports all 1376
names, which is what the ABI actually promises. `gen/report.txt` lists them.

Environment variables it reads:

| | |
|---|---|
| `MPI_ABI_WRAPPER_LIB` | which `libmpiwrapper` to load; overrides the built-in path |
| `MPI_ABI_WRAPPER_DLOPEN_MODE` | `dlmopen` on glibc (kept selectable, but known not to work with any MPI that `dlopen`s components — `NOTES.md` §2), or `capture` (tests only: the unisolated load, so that the outward-resolution check can be shown to fire) |
| `MPI_ABI_WRAPPER_BIND_NOW` | use `RTLD_NOW` instead of the default `RTLD_LAZY` |
