# `dev/component-probe/`

Whether an object the implementation `dlopen`s *after* the wrapper is loaded —
an Open MPI MCA component — reaches the implementation or `libmpi_abi`. It was
found as a crash, not predicted: every rank of a ROMIO write through the
wrapper on HPC-X died of a smashed stack. `NOTES.md` §2, "Components the
implementation loads later", has the rule this settled, and
`src/mpiwrapper/component_bind.c` the fix.

```sh
dev/component-probe/run.sh <env-abi> <env-direct> <scratch dir> [ranks]
```

`status_guard.jl` makes one `MPI_File_write_at_all` and one
`MPI_File_write_at` of each of two buffers, and passes as the status a guard
buffer of `0xAA` bytes eight statuses long; each rank prints the last byte that
changed. It is MPI.jl because the crash was found from Julia (TreeIOHDF5's
checkpoint stress test), and because MPI.jl loads either library by a
preference, so one script runs on both sides of the wrapper.

## Results

HPC-X 2.20 (Open MPI 4.1.7a1) on Symmetry's AMD nodes, mpi_abi_wrapper 1.2.0,
2026-11-18, files on BeeGFS.

**The component.** `mca_io_romio321.so` imports 72 public `MPI_*`/`PMPI_*`
names — `MPI_Status_set_elements_x` and `MPI_Type_size_x` among them — and its
`NEEDED` list is `librt libm libutil libz libpthread libc`: no `libmpi`. It
relies on `libmpi` being in the global scope. `mca_io_ompio.so` imports none.

**Where its calls bind** (`LD_DEBUG=bindings`, two ranks, job 605605):

| MPI.jl loads | `MPI_Status_set_elements_x` | `MPI_Type_size_x` |
|---|---|---|
| `libmpi_abi` (through the wrapper) | `libmpi_abi.so` | `libmpi_abi.so` |
| `libmpi` directly | `libmpi.so` | `libmpi.so` |

**What it writes** (32 ranks on four nodes, job 605604; the fixed wrapper's row
eight ranks on one node, job 605616, in which wrapper 1.2.0 crashed on all eight
again):

| MPI.jl loads | `romio321` | `ompio` |
|---|---|---|
| `libmpi` directly | 24 bytes of the guard written, `sizeof(MPI_Status)` | 24 bytes |
| `libmpi_abi`, wrapper 1.2.0 | `stack smashing detected` on every rank | 32 bytes |
| `libmpi_abi`, wrapper with `component_bind.c` | 32 bytes | 32 bytes |

So HPC-X's ROMIO writes exactly the status it is given; the overrun is the
ABI's 32-byte `MPI_Status_set_elements_x`, reached by the captured component
call, writing into the 24-byte Open MPI status that `w_MPI_File_write_at_all`
keeps on its stack — whose canary is what aborts. OMPIO is unaffected because
it calls Open MPI's internals, never the public names.

`test/component_bind_test.c` is the portable form: a stand-in component linked
to no MPI, loaded by the application and by the wrapper's `dlopen`. Over HPC-X
on the fixed wrapper (job 605616) it reports the application's copy at
`libmpi_abi` and the three loaded through the wrapper — by path, by `$ORIGIN`
and by bare name — at HPC-X's `libmpi.so.40`.

## What this establishes

**1. `RTLD_DEEPBIND` does not reach components.** `dev/dlopen-probe/`'s T2
modelled ROMIO as a call from inside `libimpl`, which is loaded by the
wrapper's `dlopen` and is covered. A component is loaded by the
implementation's own `dlopen` afterwards and is not.

**2. The isolation checks cannot see it.** Both run at load, before any
component exists; the wrapper loaded, its calls provably reached the
implementation, and a later call from the implementation's own component did
not.

**3. The capture is memory-unsafe, as `NOTES.md` §2 predicted for
`RTLD_GLOBAL`** — the bullet about "Open MPI's ROMIO and io components" was
right about the consequence and wrong only about which configuration produces
it.
