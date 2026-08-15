# `test/`

Our own tests (NOTES.md §9), as opposed to the MPICH suite in
`ci-scripts/suite/`.

| test | needs an MPI? | what it establishes |
|---|---|---|
| `compile_mpi_h.c`, `compile_mpiabi_h.c`, `compile_both_headers.c` | no | both generated headers compile standalone in a TU of their own, and together in one TU with no tag/typedef/macro/enumerator collision — the configuration `src/mpi_abi/` is built in |
| `layout-hash` (`dev/layout_hash.py --check`) | no | `MPIWRAPPER_LAYOUT_HASH` still matches the slot list it summarizes |
| `generated-up-to-date` (`dev/generate.py --check`) | no | a fresh generation reproduces the committed `gen/` byte for byte, the frozen tallies still hold, and every one of the 688 entry points is generated, in the ledger, or deferred with a reason |
| `prototype-reproduced` (`dev/check_prototype.py`) | no | S2's exit check: the generator still reproduces `dev/s1-reference/`, item by item, or the difference is a named exemption that fails when it stops firing |
| `mpiwrapper_impl_config.h` (`dev/probe_impl.py`, at configure time) | yes, its header | not a test but a build step, and the thing every guard in the generated sources tests: which entry points and which optional constants this implementation actually has. Asked of the compiler, because `#ifdef` on the implementation's own name is quietly false wherever it spells a constant as an enumerator |
| `mpiwrapper_selftest.c` | yes, one rank | white box: every predefined handle in both directions, the rank/tag/error/mode maps, the status blob, staging, the staged-request table, the **dynamic-handle collision probe**, and the two maps no black-box test can reach yet — the **keyval registry**, whose only writer is S4's `MPI_*_create_keyval`, including that a recycled implementation keyval resolves to its newest registration rather than a stale one; and `MPI_T`'s handle sentinels |
| `abi_prototype_test.c` | yes, two ranks by preference | black box: an ordinary MPI application over the ABI header, linking `libmpi_abi` and nothing else, exercising all 29 prototype entry points |
| `abi_arrays_test.c` | yes, two ranks by preference | the same, for S3's argument classes: request and status arrays, the graph topologies, the `*` extents, the status accessors — and the **lifetime** of a staged temporary, which no assertion in the generator can see. A persistent `MPI_Alltoallw` started three times is what a body that frees at completion breaks on; 1200 create/free cycles against a 1024-entry table are what a body that never frees breaks on |
| `abi_tools_test.c` | yes, two ranks by preference | the same, for S3's second half: keyvals, output-string buffers with an explicit length, `MPI_T`'s handle and enumerated classes, and the `obj_handle` whose class comes from a query rather than from its own argument list. Its sharpest case is the one no generator assertion can see either — **`MPI_T` lets a caller pass a null pointer for any OUT parameter**, so every query here is called twice, once asking for everything and once for one field, and a body that copied its converted results back unconditionally writes through the nulls of the second call. It also covers `MPI_T_PVAR_ALL_HANDLES`, which is 1 in the ABI, -1 in Open MPI and an `extern ... * const` in MPICH. Every test skips rather than fails on `MPI_ERR_UNSUPPORTED_OPERATION`, which is how it stays meaningful over Open MPI 5.0.6, whose `MPI_T` has no events at all |
| `check_exports.cmake` | no | `libmpiwrapper` exports exactly one symbol; `libmpi_abi`'s exports are exactly the ABI header's 1376 entry points, checked in both directions; and the application's only MPI dependency is `libmpi_abi` (MPI-5.0 §20.2.1) |

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

## Linux

`ci-scripts/run-linux-docker.sh` builds and runs all of this on Linux against
MPICH and Open MPI, from a macOS host with Docker. Worth doing early and often:
the first Linux build needed four fixes that macOS could not have surfaced, and
one of them was in the S0 header generator rather than in any of this stage's
code.

## Three environment quirks

None is about this project, and each cost time to attribute, so they are
written down.

- **`OMPI_MCA_btl_vader_single_copy_mechanism=none` breaks RMA on Open MPI
  4.1.6**: `MPI_Win_create` answers `MPI_ERR_WIN`, and with the default
  errhandler that aborts the job. `ci-scripts/linux-test.sh` sets that variable,
  so it is the Linux row's configuration rather than an accident of a
  developer's shell. Measured both ways -- a wrapper-free program does the same
  thing, and removing the variable fixes both. `abi_tools_test` therefore asks
  for a window with `MPI_ERRORS_RETURN` and skips its two window keyvals if it
  cannot have one; the window is a means there, not the subject.

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
