# `test/`

Our own tests (NOTES.md §9), as opposed to the MPICH suite in
`ci-scripts/suite/`.

| test | needs an MPI? | what it establishes |
|---|---|---|
| `compile_mpi_h.c`, `compile_mpiabi_h.c`, `compile_both_headers.c` | no | both generated headers compile standalone in a TU of their own, and together in one TU with no tag/typedef/macro/enumerator collision — the configuration `src/mpi_abi/` is built in |
| `layout-hash` (`dev/layout_hash.py --check`) | no | `MPIWRAPPER_LAYOUT_HASH` still matches the slot list it summarizes |
| `mpiwrapper_selftest.c` | yes, one rank | white box: every predefined handle in both directions, the rank/tag/error/mode maps, the status blob, staging, the staged-request table, and the **dynamic-handle collision probe** |
| `abi_prototype_test.c` | yes, two ranks by preference | black box: an ordinary MPI application over the ABI header, linking `libmpi_abi` and nothing else, exercising all 29 prototype entry points |
| `check_exports.cmake` | no | `libmpiwrapper` exports exactly one symbol, `libmpi_abi` exports only entry points, and the application's only MPI dependency is `libmpi_abi` (MPI-5.0 §20.2.1) |

`mpiwrapper_selftest` compiles the conversion runtime's sources into itself
rather than loading the shared library, so it can walk the maps in both
directions instead of inferring them from MPI results. That does not weaken the
one-exported-symbol property, which `check_exports.cmake` checks on the library
itself.

## Running them

```sh
cmake -S . -B build/mpich -DMPI_C_COMPILER=/path/to/mpicc
cmake --build build/mpich -j8
ctest --test-dir build/mpich --output-on-failure
```

One build directory per MPI. `MPI_C_COMPILER` is what selects the MPI; the
launcher is taken from beside it rather than from `PATH`, because launching one
implementation's binaries under another's `mpiexec` silently produces N
singletons instead of an N-rank job.

`-DMPI_ABI_TEST_USE_LAUNCHER=OFF` runs the two behavioural tests directly, as
singletons, for an implementation whose launcher does not work on the machine at
hand. `abi_prototype_test` then runs its one-rank path: the ordered blocking
exchange becomes a nonblocking one, and the reduction expectations relax to what
MPI actually guarantees for a single contribution. It is weaker than two ranks
and much better than nothing, since every conversion still crosses the boundary
twice.

## Two environment quirks seen on the machine this was written on

Neither is about this project, and both cost an hour to attribute, so they are
written down.

- **MPICH's `ch4:ofi` picks a VPN interface** when one is up (`utun0`), and then
  `MPI_Finalize` fails with `OFI poll failed (default nic=utun0)`. `FI_PROVIDER=tcp`
  in the environment avoids it. It is not set in `CMakeLists.txt`, since it is a
  property of this host rather than of the tests.
- **No Open MPI 5.0.x launcher works on this host.** conda-forge's 5.0.10 for
  osx-arm64 and a 5.0.6 built from source behave identically, and identically
  *without* any of this project involved: a wrapper-free `hello world` under
  their own `mpiexec` reports "rank 0 of 1" from both processes, complains that
  the shared-memory segment cannot be created, and segfaults in `PMIx_Finalize`.
  mpif's Open MPI 6.1.0a1 does work, so it is a 5.0.x-on-macOS-26 problem rather
  than a machine problem. Until a working 5.x launcher is available here, the
  Open MPI column is covered by singleton runs
  (`-DMPI_ABI_TEST_USE_LAUNCHER=OFF`), which is how the S1 results against Open
  MPI were obtained. `build/mpi/` is where this repository's `.gitignore`
  expects such prefixes to live.
