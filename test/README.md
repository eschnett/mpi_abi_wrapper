# `test/`

Our own tests, as opposed to the MPICH suite in `ci-scripts/suite/`, which is
the first oracle here that nothing in this repository wrote. `CODE.md` §10 has
the same inventory in one table; `NOTES.md` #10 has the five oracles these
implement.

Two of the tests below carry checks that exist because that suite found what
they were missing: `abi_tools_test` now checks the *values* of the five
predefined attributes whose class the keyval decides (`MPI_HOST` and `MPI_IO`
are ranks, `MPI_LASTUSEDCODE` an error code, `MPI_WIN_CREATE_FLAVOR` and
`MPI_WIN_MODEL` enumerations), and `abi_prototype_test` now presets
`status.MPI_ERROR` and requires it back unchanged, which is what MPI-5.0
§3.2.5 says and the opposite of what it asserted from S1 until S7.

| test | needs an MPI? | what it establishes |
|---|---|---|
| `compile_mpi_h.c`, `compile_mpiabi_h.c`, `compile_both_headers.c` | no | both generated headers compile standalone in a TU of their own, and together in one TU with no tag/typedef/macro/enumerator collision — the configuration `src/mpi_abi/` is built in |
| `layout-hash` (`dev/layout_hash.py --check`) | no | `MPIWRAPPER_LAYOUT_HASH` still matches the slot list it summarizes |
| `generated-up-to-date` (`dev/generate.py --check`) | no | a fresh generation reproduces the committed `gen/` byte for byte, the frozen tallies still hold, and every one of the 688 entry points is generated, in the ledger, or deferred with a reason |
| `prototype-reproduced` (`dev/check_prototype.py`) | no | S2's exit check: the generator still reproduces `dev/s1-reference/`, item by item, or the difference is a named exemption that fails when it stops firing |
| `mpiwrapper_impl_config.h` (`dev/probe_impl.py`, at configure time) | yes, its header | not a test but a build step, and the thing every guard in the generated sources tests: which entry points and which optional constants this implementation actually has. Asked of the compiler, because `#ifdef` on the implementation's own name is quietly false wherever it spells a constant as an enumerator |
| `mpiwrapper_selftest.c` | yes, one rank | white box: every predefined handle in both directions, the rank/tag/error/mode maps, the status blob, staging, the staged-request table, the **dynamic-handle collision probe**, `MPI_DISPLACEMENT_CURRENT` in both halves — that the sentinel converts *and* that every other displacement, negative ones included, crosses untouched, which is what a mapper written as a family would break — the handle-serialization table's capacity behaviour (which has no error channel — `MPI_Comm_toint` returns an int, so a full table must answer 0 and no `_fromint` may accept it), the **error-code registry**, whose `toabi` direction has no error channel and so must fall back to `MPI_ERR_OTHER` when the table fills; and the two maps whose sharpest cases no black-box test can set up — the **keyval registry** and the error-code one, including that a recycled implementation value resolves to its newest registration rather than a stale one; and `MPI_T`'s handle sentinels |
| `abi_prototype_test.c` | yes, two ranks by preference | black box: an ordinary MPI application over the ABI header, linking `libmpi_abi` and nothing else, exercising all 29 prototype entry points |
| `abi_arrays_test.c` | yes, two ranks by preference | the same, for S3's argument classes: request and status arrays, the graph topologies, the `*` extents, the status accessors — and the **lifetime** of a staged temporary, which no assertion in the generator can see. A persistent `MPI_Alltoallw` started three times is what a body that frees at completion breaks on; 1200 create/free cycles against a 1024-entry table are what a body that never frees breaks on |
| `abi_tools_test.c` | yes, two ranks by preference | the same, for S3's second half: the five entry points MPI-3.0 deleted, which `libmpi_abi` answers in terms of their replacements — including the three attribute-callback sentinels the two spellings must agree about, which cannot be a `_Static_assert` because a cast to a pointer type is not an integer constant expression; keyvals, output-string buffers with an explicit length, `MPI_T`'s handle and enumerated classes, and the `obj_handle` whose class comes from a query rather than from its own argument list. Its sharpest case is the one no generator assertion can see either — **`MPI_T` lets a caller pass a null pointer for any OUT parameter**, so every query here is called twice, once asking for everything and once for one field, and a body that copied its converted results back unconditionally writes through the nulls of the second call. It also covers `MPI_T_PVAR_ALL_HANDLES`, which is 1 in the ABI, -1 in Open MPI and an `extern ... * const` in MPICH. Every test skips rather than fails on `MPI_ERR_UNSUPPORTED_OPERATION`, which is how it stays meaningful over Open MPI 5.0.6, whose `MPI_T` has no events at all |
| `abi_converters_test.c` | yes, two ranks by preference | the same, for S4a's converter face: the 44 handle converters, the four status converters, the ten status-consuming functions, the ten output-string buffers with no length argument, and the six `MPI_Abi_*` calls. Two of its checks are not round trips, and those are the ones that matter: **`_toint` of a predefined handle is compared against the ABI header's own constant**, which MPI-5.0 §20.4.5 requires and which no round trip through our own code in both directions could detect; and **a status converted to the Fortran form and back is asked `MPI_Get_count`**, which only answers if the implementation's private bytes travelled with it — a converter that copied the three named fields passes every inspection of `MPI_SOURCE` and `MPI_TAG` and fails there. It also reads `MPI_F_SOURCE` straight out of the integer array, since a converter forwarding to MPICH's own `MPI_Status_c2f` produces a valid status in the wrong layout. What it *cannot* check is whether the `_c2f`/`_f2c` integers would be accepted by an implementation's own Fortran layer; mpif is the oracle for that (S8) |
| `abi_state_test.c` | yes, two ranks by preference | the same, for S4b: the state the wrapper owns. Its sharpest checks are the ones a plausible-but-wrong body passes everywhere else — each of the four error-handler classes asserts that the trampoline handed the callback **the handle it set the handler on**, not the implementation's bits; `MPI_COMM_DUP_FN` is installed as a copy callback and the attribute is then checked to have survived `MPI_Comm_dup`, which only happens if the ABI's `(function *)0x1` was replaced by the implementation's own function rather than passed through or wrapped; the generalized request is asked `MPI_Get_count` on the status its query callback filled, which needs the whole blob to have crossed in both directions; `MPI_BUFFER_AUTOMATIC` is attached and detached, which is a sentinel translation on MPICH and the wrapper's own emulation on Open MPI, and both must answer `MPI_BUFFER_AUTOMATIC`; and a dynamic error class is added and then asked about through `MPI_Error_class` and `MPI_Error_string`, which are generated bodies that never heard of the registry. It is also the only test that can ask `MPI_Initialized` **before** `MPI_Init` and `MPI_Finalized` **after** `MPI_Finalize`, which is the whole reason those two are answered from the wrapper's own state |
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

### Three things `abi_state_test` cannot reach, and why

None is a gap in the test: each is a property of every implementation
available here, measured rather than assumed.

- **User datareps do not work on either.** MPICH's ROMIO answers
  `MPI_Register_datarep` with "Read and Write datarep conversions are currently
  not supported by MPI-IO", and Open MPI does the same, so no conversion
  callback can be reached. The test skips and says so.
- **`MPI_T` raises no events.** Open MPI 5.0.6 has no event interface at all;
  MPICH 4.3.1 declares every entry point and reports `MPI_T_event_get_num == 0`,
  so no registration handle can be allocated and nothing keys the map in
  `src/mpiwrapper/toolevents.c`.
- **`MPI_Comm_spawn` hangs on this host**, under MPICH 4.3.1's hydra, with no
  wrapper involved — a fifteen-line C program spawning itself does the same.
  The same program under MPICH in a Linux container returns a clean error
  instead. So the spawn case is behind `-DMPI_ABI_TEST_SPAWN=ON` and off by
  default: a test that hangs `ctest` is worse than one that skips.

`-DMPI_ABI_TEST_USE_LAUNCHER=OFF` runs the behavioural tests directly, as
singletons, for an implementation whose launcher does not work on the machine at
hand. `abi_prototype_test` then runs its one-rank path: the ordered blocking
exchange becomes a nonblocking one, and the reduction expectations relax to what
MPI actually guarantees for a single contribution. It is weaker than two ranks
and much better than nothing, since every conversion still crosses the boundary
twice.

## Linux

`ci-scripts/run-linux-docker.sh` builds and runs all of this on Linux against
MPICH and Open MPI, from a macOS host with Docker. It also runs S7's suite
there, which is where the Open MPI row of that suite lives:

```sh
MPIABI_LINUX_SCRIPT=/src/ci-scripts/suite/linux-suite.sh \
  MPIABI_LINUX_OUT=$PWD/build/linux-out \
  ci-scripts/run-linux-docker.sh openmpi
```

Worth doing early and often:
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
