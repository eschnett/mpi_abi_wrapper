# MPI ABI wrapper — the code as it stands

What this repository contains, where each thing lives, and the artifact behind
every number. One of four documents:

| | holds |
|---|---|
| `CLAUDE.md` | how to work here: the reading protocol, the host, the session rules |
| **`CODE.md`** | what the repository contains now, and the number behind every claim |
| `NOTES.md` | the design, its reasons, and what is missing, broken or undecided |
| `HISTORY.md` | roads not taken, beliefs that were measured false, and the stage record |

**Every count here has an authority column, and the authority is an artifact
rather than another sentence.** Eleven counts in the predecessor documents were
wrong at one point or another, each one `grep` from being right
(`HISTORY.md` §4), so a number without a way to re-derive it does not belong on
this page.

Nothing here explains *why*. That is `NOTES.md`, whose section numbers (`#2`,
`#5.7`, …) are cited from roughly two hundred places in the source and are
stable.

---

## 1. What is built

```
application
    |  MPI_Send(...)                        ABI types only
    v
libmpi_abi.so          exports MPI_* and PMPI_* (1376 symbols)
    |                  includes the ABI mpi.h and nothing else
    |  vt->MPI_Send(...)                    ABI types only, 1366 slots
    v
libmpiwrapper.so       exports mpiwrapper_get_vtable and nothing else
    |                  includes the implementation's mpi.h + generated mpiabi.h
    |                  owns every conversion, trampoline and map
    |  MPI_Send(...)                        implementation types, direct call
    v
libmpi.so              linked normally, not dlopened
```

| | built | needs an MPI? |
|---|---|---|
| `libmpi_abi` + `mpi.h` | once, implementation-independent | no |
| `libmpiwrapper` | once per MPI installation | yes |

`cmake && make install` builds both into one prefix and the split is not
user-visible; `-DMPI_ABI_BUILD_WRAPPER=OFF` exposes the independence as a
developer option, which is what the cross test uses.

## 2. The numbers

| | | authority |
|---|---|---|
| entry points | **688** | `gen/report.txt`; the ABI header's prototypes |
| — core / `MPI_T_*` / Fortran converters | 611 / 51 / 26 | the header |
| — marked deprecated | 12 | `grep '; /\* deprecated' gen/include/mpi.h` |
| exported symbols in `libmpi_abi` | **1376** | `nm`; `test/check_exports.cmake`, both directions |
| **vtable slots** | **1366** | `gen/report.txt`; 683 × 2 — the five of §5 have no slot |
| generated bodies | **563** | `gen/report.txt` |
| hand-written (the ledger) | **120**, all with bodies | `gen/report.txt`, `src/mpiwrapper/handwritten.h` |
| answered by `libmpi_abi` itself | **5** | `gen/report.txt` |
| deferred | **0**, frozen | `gen/report.txt` |
| staged past return | **8** | `gen/report.txt` |
| large-count entry points | **159** | `grep -c '^int MPI_[A-Za-z0-9_]*_c(' gen/include/mpi.h` — 148 generated, 11 in the ledger |
| — with a fallback body | **148** generated, all 11 hand-written | `gen/report.txt`; `NOTES.md` #5.10 |
| — whose fallback stages past return | **18** | `gen/report.txt` |
| handle classes | 11 | `gen/mpiwrapper/constants.c` |
| predefined handles | 103 | `PREDEF(...)` rows in `constants.c` |
| error classes | 80 | 62 `MPI_ERR_*` + 18 `MPI_T_ERR_*`; `MPI_ERR_LASTCODE` is a bound |
| callback registrars | 15 in the ledger, 16 counting `MPI_Keyval_create` | `gen/report.txt`, `NOTES.md` #6.1 |

563 + 120 + 5 = 688. 683 × 2 = 1366 slots, while all 688 × 2 = 1376 names are
still exported, because the five of §5 are implemented on the ABI side rather
than forwarded.

The two large-count rows are the ones that move with the implementation rather
than with the ABI. All 159 `_c` entry points exist in every build; over MPICH
≥ 4.0 the implementation answers them and over anything older the wrapper does,
by narrowing each call onto its small twin (`NOTES.md` #5.10). The "staged past
return" row above counts the *primary* bodies only; the fallback's 18 are
frozen separately because a vector collective's count arrays are a pointer cast
in one arm and a staged block in the other.

**Every one of these is a frozen tally in `dev/generate.py`**, so a new
`apis.json` or a new ABI header that reclassifies anything fails generation
rather than moving a number quietly. `ctest -R generated-up-to-date` is the
check.

## 3. Repository layout

```
.github/workflows/ ci.yaml -- ten CI jobs over thirty-eight legs, each calling a
                     ci-scripts entry point rather than repeating its recipe.
                     Twenty of those legs are the MPICH C suite: five
                     environments x four shards, the MPICH ones gating. The
                     two MVAPICH legs are report-only for one upstream hang;
                     the Intel MPI job gates (NOTES.md #9's third and fourth
                     implementations)
bin/               mpicc.in, mpicxx.in -- the compiler wrappers, configured at install
ci-scripts/        MPI install and build-shape checks
  check-install.sh   the six-leg installed-prefix consumption test
  install-mpich.sh, install-openmpi.sh, install-mvapich.sh
  linux-test.sh, run-linux-docker.sh
  linux-floor.sh     the MPI-3.0 floor row: builds MPICH 3.1.4, then hands over
                       to linux-test.sh (§11)
  suite/             MPICH C suite runner, the local and CI lists -- xfail,
                       flaky, exclude and timelimit -- the mpiexec filter, the
                       TAP gate, and i386-suite.sh (§10)
cmake/             FindMPI.cmake (the shim), mpiwrapper_marker.h (the installed
                     self-wrap marker, §8), mpi_abiConfig.cmake.in, mpi_abi.pc.in,
                     mpi_abi.version (ELF), mpi_abi.exported_symbols (Mach-O)
dev/               the Python generator and the dev-time cross-checks
  generate.py        the generator
  generate_headers.py  the S0 header step, imported rather than duplicated
  probe_impl.py      the configure-time availability probe
  layout_hash.py     the vtable layout hash and its --check
  check_prototype.py the S1-reproduction check
  check-c-bindings.py  the MPI-5.0 Appendix A.3 cross-check
  check_out_params.py  the out-parameter check over generated bodies
  apis.json          vendored (dev/vendor/), ~2 MB
  s1-reference/      S1's four hand-written stand-ins, frozen; not compiled
  dispatch-bench/ dlopen-probe/ get-contents-extent/ handle-map-bench/
  request-identity/ type-identity/   the six probe directories NOTES.md cites
  third-implementations/  what the MVAPICH and Intel MPI rows assume, checked
                       in a container rather than asserted (§11)
doc/               mpi.h.patch, mpi50-report.pdf
examples/          narrated excerpts of each shape; src/ is the reference
gen/               committed generated output, never hand-edited
src/mpi_abi/       hand-written: bootstrap, dlopen, vtable acquisition
src/mpiwrapper/    hand-written: the ledger's bodies, trampolines, maps, conversion
scripts/           the CI recipes, runnable locally, plus host-env.sh
  host-env.sh        the four variables this development machine needs; §12
test/              our own tests, plus expect_ranks.h -- the shared rank-count
                     guard the five black-box tests include (§11)
test-consume/      a consumer project used by check-install.sh
```

The `ci-scripts/` versus `ci-scripts/suite/` split is deliberate: the MPI-install
cache key must hash the install scripts and must **not** hash the suite's
expected-failure lists, or every edit to a reason rebuilds MPI on every variant.
`ci-scripts/*` also does not mean what it looks like in `@actions/glob` — a
matched directory expands to all of its descendants. The place that rule is
honoured is `.github/workflows/ci.yaml`'s `linux-source` cache key, which now
spells it `ci-scripts/install-*.sh` — the install scripts and nothing else,
which is both narrower than the directory and exactly what the sentence above
says. The cache step prints the key it used, so a commit touching only
`ci-scripts/suite/` that moves the hash means it is wrong again.

## 4. The generator and its seven artifacts

One command writes all seven; `dev/generate.py --check` regenerates and requires
an empty diff.

| artifact | includes | checked by |
|---|---|---|
| `gen/include/mpi.h` | — | the mpi-abi-stubs header plus `doc/mpi.h.patch`, names untouched |
| `gen/include/mpiabi.h` | — | the renamed `MPIABI_` view, prototypes dropped |
| `gen/include/mpiwrapper_vtable.h` | `mpiabi.h` | shared by both halves; carries `MPIWRAPPER_LAYOUT_HASH` |
| `gen/mpi_abi/entrypoints.c` | ABI `mpi.h` | compiles against the real header; `nm` vs its symbol set, both ways |
| `gen/mpiwrapper/wrappers.c` | impl `mpi.h` + `mpiabi.h` | compiles against the implementation's header |
| `gen/mpiwrapper/constants.c` | both | every `case` names a real implementation macro |
| `gen/report.txt` | — | the ledger, the frozen tallies, the unsupported list |

The first two are the S0 step, in `dev/generate_headers.py`, which
`dev/generate.py` imports rather than duplicates, so one `--check` covers all
seven.

**`gen/report.txt` is the authority for "what is where".** It names all 688
entry points exactly once — generated, hand-written, ABI-side, or deferred —
groups the ledger by reason, marks each `[done]`, and carries the one limitation
that is this library's rather than an implementation's.

### The availability probe

`dev/probe_impl.py` runs at configure time and writes
`mpiwrapper_impl_config.h`, one `MPIWRAPPER_HAVE_<name>` per available entry
point *and per available optional constant*. Every guard in the generated and
hand-written sources tests one of those and nothing else.

- **One compile, not one per name.** All questions go into a single translation
  unit, one probe per line, compiled `-fsyntax-only`. A name that is a macro
  answers `#ifdef`; anything else must be declared for its probe to compile, and
  the probe differs by what the name is — `sizeof &name` for an entry point,
  `sizeof(name)` for a constant, whose address may not be takeable. The constant
  form is valid C for a *type* name too, which is how
  `MPIWRAPPER_HAVE_MPI_T_event_registration` is asked.
- When the compile fails it reads the diagnostics' **line numbers** — never
  their wording — drops those probes and compiles again, so every answer is
  confirmed by a compile that succeeded.
- Measured: **0.3–0.6 s for 521 names.** MPICH 4.3.1 reports 7 constants absent
  (the five sized Fortran logicals, `MPI_ERR_ABI`, `MPIX_TYPECLASS_LOGICAL`)
  plus the 28 entry points it lacks; Open MPI 5.0.10 reports 166 absent, mostly
  the `_c` forms. Both agree exactly with `nm` where `nm` can answer.
- It reads **both** `gen/` and `src/mpiwrapper/`; `CMakeLists.txt` names the two
  source lists once so the probe's dependencies cannot drift from the library's.
- `internal.h` `#error`s if that file did not come from the probe, because a
  missing probe would otherwise turn the whole library into stubs — which links,
  loads, and answers `MPI_ERR_UNSUPPORTED_OPERATION` to everything.

Everything about it is compile-only, so cross-compiling works.

## 5. The five entry points `libmpi_abi` answers itself

No vtable slot, no wrapper body; `libmpi_abi` calls the slot of the entry point
that replaced each. They work over any implementation with the MPI-2 attribute
interface.

| deleted by MPI-3.0 | answered as |
|---|---|
| `MPI_Attr_delete` | `MPI_Comm_delete_attr` |
| `MPI_Attr_get` | `MPI_Comm_get_attr` |
| `MPI_Attr_put` | `MPI_Comm_set_attr` |
| `MPI_Keyval_create` | `MPI_Comm_create_keyval` |
| `MPI_Keyval_free` | `MPI_Comm_free_keyval` |

The set is closed by the header — exactly the entry points marked `deprecated:
MPI-2.0` — and the generator fails if a sixth appears without a replacement
named for it. Each pair's return type, arity and parameter types are checked,
with the two callback typedefs compared by the function type they name rather
than by spelling. `PMPI_Attr_get` reaches `PMPI_Comm_get_attr`, so the
shifted-name rule survives the substitution.

## 6. The hand-written ledger, by reason

120 entries, all implemented. `gen/report.txt` names each.

| n | group | file |
|---|---|---|
| 44 | handle converters (`_c2f`/`_f2c`/`_toint`/`_fromint`, 11 classes) | `hw_converters.c`, `serialize.c` |
| 14 | callback registration: installs a trampoline or a pair | `hw_callbacks.c` |
| 12 | buffer attach/detach: `MPI_BUFFER_AUTOMATIC` and ownership | `hw_buffers.c`, `buffers.c` |
| 10 | consumes a status in the *in* direction | `hw_status.c` |
| 10 | output string buffer with no length argument | `hw_strings.c` |
| 8 | lifecycle: initialization state the wrapper itself tracks | `hw_lifecycle.c` |
| 6 | `MPI_Abi_*` introspection: about this library, not the wrapped MPI | `hw_abi.c` |
| 6 | dynamic error codes, renumbered into the ABI's range | `hw_errors.c`, `errorcodes.c` |
| 4 | Fortran status converters: memcpy-shaped, not argument-shaped | `hw_converters.c` |
| 2 | attribute value whose class the *keyval* decides | `hw_attr.c` |
| 2 | spawn: `argv`, `array_of_argv`, `array_of_errcodes` together | `hw_spawn.c` |
| 1 | `MPI_T_event_handle_free`: a callback on the way back into user code | `hw_callbacks.c` |
| 1 | genuinely variadic (`MPI_Pcontrol`) | `hw_pcontrol.c` |

## 7. The conversion runtime

`src/mpiwrapper/`, all hand-written. The `hw_*.c` files hold ledger *bodies*;
the rest hold the machinery those bodies and the generated ones share.

| file | what it owns |
|---|---|
| `getvtable.c` | the one exported symbol: the handshake, the outward-resolution check, and building the reverse maps |
| `handles.c` | the eleven classes' conversions, including the perfect-hash reverse map |
| `constants.c` (generated) | every mapped integer family, as a switch over the implementation's own macros |
| `status.c` | the 32-byte blob, both directions, plus the keep-`MPI_ERROR` variant |
| `staging.c` | staged temporaries, the request-keyed table for those outliving their call, and the policy over it (NOTES.md #13.2's (a), (b), (c)) |
| `extents.c` | array extents `apis.json` records as `*`, asked of the implementation |
| `serialize.c` | the intern table behind `_toint`/`_fromint` |
| `keyvals.c` | the dynamic keyval registry |
| `errorcodes.c` | the dynamic error-code registry, in both directions |
| `callbacks.c` | the trampoline pools: 2 user-op variants × 1024, 4 errhandler classes × 256 |
| `extrastate.c` | the `{user_fn, user_extra}` families: three keyval kinds, generalized requests, datareps |
| `toolevents.c` | `MPI_T`'s registration-keyed map |
| `toolobj.c` | asks `get_info` for the class of an `obj_handle` |
| `buffers.c` | the attached buffer's ownership record and the `MPI_BUFFER_AUTOMATIC` emulation |
| `internal.h` | the `_Static_assert` battery, and the `#error` guarding include order and the probe |

`src/mpi_abi/bootstrap.c` is the whole of the ABI side that is not generated:
the constructor, the `dlopen` with per-platform isolation, the version /
subversion / layout / size handshake, and the behavioural probe with its decoy
vtable.

## 8. Building

```sh
cmake -S . -B build/mpich -DMPI_C_COMPILER=/path/to/mpicc
cmake --build build/mpich -j8
ctest --test-dir build/mpich --output-on-failure
```

One build directory per MPI; `MPI_C_COMPILER` is what selects it. The launcher
is taken from beside that compiler rather than from `PATH`, because launching
one implementation's binaries under another's `mpiexec` silently produces N
singletons instead of an N-rank job.

**Options.**

| | default | |
|---|---|---|
| `MPI_ABI_BUILD_WRAPPER` | `ON` | `OFF` builds `libmpi_abi` alone, with no MPI present |
| `MPI_ABI_WRAP_ABI_IMPL` | `OFF` | wrap a genuine ABI-implementing MPI (oracle 5); warns |
| `MPI_ABI_TEST_USE_LAUNCHER` | `ON` | `OFF` runs the behavioural tests as singletons. Either way the build states the rank count it expects (`MPI_ABI_EXPECT_RANKS`) and a test given a different one fails: §11 |
| `MPI_ABI_TEST_SPAWN` | `OFF` | the spawn case hangs under hydra on macOS 26 |

**Four configure-time checks, all compile-only** so cross-compiling works:
`MPI_VERSION >= 3` hard and `>= 4` as a warning; no self-wrapping (a hard error
if the found MPI prefix contains our `cmake/mpiwrapper_marker.h`); the
`_Static_assert` battery; and every generated constant `case` naming a real
implementation macro, which is free because it is a compile error.

**Generated code stays committed**, with a `regenerate` target outside `all` and
a test asserting an empty diff. Python is a dev dependency, never a build one.

**The install prefix is exclusive** — no second wrapper, no other MPI, and never
the wrapped MPI's own prefix. `NOTES.md` #9 has the reason; the check is
`ci-scripts/check-install.sh`'s fifth leg.

**Two headers are installed, and only two**: `gen/include/mpi.h` and
`cmake/mpiwrapper_marker.h`. `mpiabi.h` and `mpiwrapper_vtable.h` are the two
halves' private contract, needed by every build of this project from source and
by no consumer of an installed prefix; `check-install.sh` leg 1 asserts their
absence as well as the marker's presence.

## 9. Consuming an installed prefix

Three routes, all generated from one source of truth for flags:

- **`bin/mpicc`, `bin/mpicxx`** — name this prefix's `mpi.h` and `libmpi_abi`,
  with the rpath set so the produced executable starts without help, and never
  naming `libmpiwrapper`. On macOS they bake in `-isysroot` from
  `CMAKE_OSX_SYSROOT`, because `CMAKE_C_COMPILER` can be the Xcode toolchain's
  own binary rather than the `/usr/bin/cc` shim. **Those paths are baked in at
  *configure* time**, from `CMAKE_INSTALL_FULL_INCLUDEDIR`/`LIBDIR`, so
  `cmake --install --prefix P` relocates the files and leaves the wrapper
  pointing at the configured prefix — `/usr/local` if none was given.
  `find_package(MPI)` then rejects the result with `missing:
  MPI_mpi_abi_LIBRARY MPI_C_HEADER_DIR MPI_C_WORKS`, which reads like a bug in
  this project rather than a mis-built prefix. Configure with
  `-DCMAKE_INSTALL_PREFIX`, the way `ci-scripts/check-install.sh` does.
- **CMake package files** — `find_package(mpi_abi)`, plus a `FindMPI` shim for
  consumers written against `find_package(MPI)`. Two mechanisms: CMake's own
  bundled `FindMPI` already finds this project through compiler-wrapper
  interrogation, which `bin/mpicc.in` answers the way a real one would; the
  shipped `cmake/FindMPI.cmake` covers the case interrogation cannot reach.
  Only the shim is exercised as a leg below — the interrogation path is
  CMake's own behaviour against `bin/mpicc` and needs nothing from the
  installed package files, so nothing here tests it.
- **pkg-config** — `mpi_abi.pc`.

`ci-scripts/check-install.sh` takes any `mpicc` as its one argument and runs
**six** legs (`grep -c '^step "' ci-scripts/check-install.sh`): the configure,
build and install into a prefix of its own, the prefix-exclusivity assertion,
and then four route legs over the three consumption routes of `NOTES.md` #9 —
`bin/mpicc`, `find_package(mpi_abi)`, `find_package(MPI)` via the shim, and
`pkg-config` — each *building and running* a program from the installed prefix
with `LD_LIBRARY_PATH`/`DYLD_LIBRARY_PATH` cleared. `find_package` is two legs
because this project ships two answers to it; the pkg-config leg reports itself
skipped on a host with no pkg-config. All six pass on macOS against a distro
Open MPI and on Linux against MPICH 5.0.1 and Open MPI 5.0.10 built from source
by `ci-scripts/install-mpich.sh` / `install-openmpi.sh`.

Those installers are far shorter than mpif's equivalents on purpose: mpif needs
an MPI that already implements the standard ABI, hence its pinned MPICH `main`
commit and header substitution. This project wraps any MPI-3.0+ implementation
through its own conversion layer, so a stock `configure && make && make install`
against a released tarball is the whole of what CI needs.

**Symbol visibility** is `-fvisibility=hidden` plus an export macro, *and* a
version script on ELF / `-exported_symbols_list` on Mach-O. The second half is
not redundant: the handful of symbols the linker inserts into every shared
object (`_init`, `_fini`, `_edata`, `_end`, `__bss_start`) are reachable by no
visibility attribute. **Both ELF version scripts use an anonymous node**, so
they filter the export set and version nothing (decision 22): `libmpi_abi`'s
dynamic table is exactly the `MPI_*`/`PMPI_*` names with no node symbol beside
them, which is why `test/check_exports.cmake` carries no exemption list.

**`libmpi_abi` carries a soname and `libmpiwrapper` does not** (decision 21).
`SOVERSION` is `MPI_ABI_VERSION`, read out of `gen/include/mpi.h` at configure
time so the two cannot drift, and `VERSION` is `PROJECT_VERSION`:

| | installed | recorded by a client binary |
|---|---|---|
| ELF | `libmpi_abi.so.1.0.0`, `.so.1`, `.so` | `libmpi_abi.so.1` |
| Mach-O | `libmpi_abi.1.0.0.dylib`, `.1.dylib`, `.dylib` | `@rpath/libmpi_abi.1.dylib` |

The leaf name in the right-hand column is the one Open MPI's ABI branch also
installs (`libmpi_abi.1.dylib`, checked with `otool -D`), which is the point:
that name is what decides whether a binary built here starts against someone
else's `libmpi_abi`. All three consumption routes name `-lmpi_abi` or an
imported target and so resolve through the unversioned symlink;
`ci-scripts/check-install.sh` runs all five legs against the versioned layout.

## 10. Tests

`ctest` runs fifteen tests. Seven need no MPI at all and are the cheapest gate
in CI — `.github/workflows/ci.yaml`'s `checks` job is exactly them, configured
`-DMPI_ABI_BUILD_WRAPPER=OFF`.

| test | needs an MPI? | what it establishes |
|---|---|---|
| `headers-up-to-date`, `compile_mpi_h`, `compile_mpiabi_h`, `compile_both_headers` | no | both generated headers compile standalone and *together* in one TU, with no tag/typedef/macro/enumerator collision |
| `generated-up-to-date` | no | a fresh generation reproduces `gen/` byte for byte, the frozen tallies hold, and all 688 are accounted for |
| `prototype-reproduced` | no | the generator still reproduces `dev/s1-reference/`: **194 items, 190 exactly, 4 exempted with a reason** that fails when it stops firing |
| `out-params-defined` | no | no early return in a generated body leaves an out handle or out scalar undefined — **415 of the 705 generated arms own one**, and the rule closed 71 such returns across 26 entry points, 22 of them in narrowing arms an earlier revision of the check could not see (`NOTES.md` #7 decision 6) |
| `layout-hash` | no | `MPIWRAPPER_LAYOUT_HASH` still matches the slot list it summarizes |
| `c-bindings-cross-check` | no | all 688 signatures match MPI-5.0 Appendix A.3, or are one of 8 named exemptions |
| `exported-symbols` | no | `libmpi_abi`'s exports are exactly the header's 1376, both directions — the leg that needs no MPI, and the reason this test is registered outside the wrapper block. Given a wrapper it adds two more: `libmpiwrapper` exports exactly one symbol, and the application's only MPI dependency is `libmpi_abi`. Its summary names the legs it ran, so a no-MPI pass cannot be read as all three |
| `isolation-check` | yes | an unisolated wrapper is refused at load, or crashes; a *successful* run of one is the failure |
| `mpiwrapper_selftest` | one rank | white box: all 103 predefined handles both ways, every constant map, the status blob, staging, the staged-request table **and the policy over it**, the dynamic-handle collision probe, and the capacity behaviour of the maps that have no error channel |
| `abi_prototype_test` | **two ranks required** for the staged-request round | S1's 29 entry points as an ordinary application over the ABI header. At one rank the nonblocking `MPI_Ialltoallw` is complete on return, so #13.2's (b) frees its block and the table is never exercised |
| `abi_arrays_test` | two ranks preferred | S3a's classes, and the **lifetime** no generator assertion can see: a persistent `MPI_Alltoallw` started three times, 1200 create/free cycles against a 1024-entry table |
| `abi_tools_test` | two ranks preferred | S3b's classes, the five ABI-side entry points and their sentinels, and **`MPI_T`'s null OUT pointers** — every query called twice, once for everything and once for one field |
| `abi_converters_test` | two ranks preferred | S4a's 70. Two checks are not round trips: `_toint` against the header's own constant, and a status through Fortran and back asked `MPI_Get_count` |
| `abi_state_test` | two ranks preferred | S4b's state, each check written against what a plausible-but-wrong body gets wrong |
| `abi_large_count_test` | **two ranks required** for the vector rounds | the ABI's 159 `_c` entry points, over an implementation that has them and over one where they are the narrowing fallback — the same assertions either way, since the `_c` form and its small twin must agree. Its sharpest cases are a vector collective at a non-root rank passing a genuine `NULL`, a nonblocking one whose caller overwrites its own count arrays the instant it is posted, and a refused call's out handle |

`mpiwrapper_selftest` compiles the conversion runtime into itself rather than
loading the shared library, so it can walk the maps in both directions. That
does not weaken the one-exported-symbol property, which `exported-symbols`
checks on the library itself.

`-DMPI_ABI_TEST_USE_LAUNCHER=OFF` runs the behavioural tests as singletons for
an implementation whose launcher does not work on the machine at hand. It is
weaker than two ranks and much better than nothing, since every conversion still
crosses the boundary twice.

### MPICH's C test suite

`ci-scripts/suite/run-suite.sh` builds and installs this project, configures
MPICH 5.0.1's `test/mpi` **against the wrapper's prefix** rather than an MPI's,
runs the tests through `ci-scripts/suite/mpiexec-filter`, and gates the TAP
output against `xfail-<variant>.txt` with `check-tap.py`.

The gate runs in **both directions**: an unlisted failure fails, a listed
failure that *passed* fails, a listed test that did not run fails, and a line
with no reason fails — so `--update-xfail`, which writes bare lines, cannot be
used to make a red run green.

| variant | expected failures | state |
|---|---|---|
| MPICH 4.3.1 | **41** in `xfail-mpich.txt` | fully triaged, every line with a cause |
| Open MPI 4.1.6, on Linux | **168** in `xfail-openmpi.txt` | about half attributed, the rest honest placeholders |

The two lists above are the **local** rows and are pinned to the pair of MPIs
named in them. **CI runs five environments of its own**, each with its own list:
`suite` over MPICH 5.0.1 and Open MPI 5.0.10 built from the pinned tarballs, on
x86_64 and aarch64, and `suite-i386` over a MPICH 5.0.1 built inside a
`linux/386` container. Each gates against a shared `xfail-ci-<mpi>.txt` plus a
per-architecture `xfail-ci-<mpi>-<arch>.txt`, which `check-tap.py` reads as one
file while rejecting a test listed in both — the split exists because three runs
showed one file cannot describe two machines, timing moving in both directions at
once on a four-vCPU runner and one architecture returning wrong 8-bit reductions
that the other did not. **The three MPICH environments gate**
(`continue-on-error: false`, which is TODO.md's "do not ignore mpich failures");
the two Open MPI legs are still report-only, and about half of
`xfail-ci-openmpi.txt` still says "not yet attributed". Each leg keeps
`summary.tap` and the run's logs as an artifact whether it passed or not, which
is what `--gate-only` writes a list from.

**Four kinds of list, chosen by how a test fails** (§10): `xfail-ci-*` for a
repeatable failure, `flaky-ci-*` for one that does either, `timelimit-ci-*` for
one that *hangs* — capped with the suite's own `timeLimit=` key so it still runs,
reports and gates — and `exclude-ci-*` only for a test that takes the runner down
before it can report. The counts, by
`grep -cvE '^\s*(#|$)' ci-scripts/suite/<file>`:

| list | lines |
|---|---|
| `xfail-ci-mpich.txt` + `-i386` delta | **41** + **6** (x86_64 and aarch64 deltas are empty) |
| `xfail-ci-openmpi.txt` + `-x86_64` delta | **106** + **1** |
| `flaky-ci-mpich.txt`, `flaky-ci-openmpi.txt` | **1**, **5** |
| `timelimit-ci-openmpi.txt` | **15** patterns over 41 lines, 18 of them seen to hang |
| `exclude-ci-openmpi.txt` | **2**, both inert while `rma` is off the Open MPI legs |

The authority for both is `grep -cvE '^\s*(#|$)' ci-scripts/suite/xfail-*.txt`,
which is also how `check-tap.py` reads them (it skips blanks and comments at
line 85). Plain `wc -l` is *not* the number — these files are mostly reasons,
119 and 230 lines of them — and an earlier version of this table said it was.

The Open MPI row is dominated by MPI-4.0: that release provides 466 of the ABI's
688 entry points, so sessions, partitioned communication, persistent collectives
and the `_c` forms are stubs answering `MPI_ERR_UNSUPPORTED_OPERATION`. It runs
in the container `ci-scripts/suite/linux-suite.sh` starts. That was once a
necessity — no Open MPI 5.0.x launcher would run a job on the development
laptop — and is now a choice: `scripts/host-env.sh` makes both local 5.0.x
prefixes launch (`HISTORY.md` §2.13), so a macOS Open MPI row of this suite has
become possible. It has not been run, and until it is, this row is the Linux
one.

**The per-run test totals in these files disagree and should not be quoted until
a run re-establishes them.** The headers say 1212 and 1231, the suite README
says 1229 and 1231, and the counts moved twice without every copy following. The
failure counts above are a `grep -c` and are reliable; a total is only as good as
the run that produced it, and `run-suite.sh` prints the three configure
decisions that change it — including whether the `threads` directory exists at
all, which the suite decides by *running* an MPI program.

`spawn` is excluded by default (`--with-spawn` to include), and `errors/spawn`
and `threads/spawn` with it: `MPI_Comm_spawn` hangs under hydra on macOS with no
wrapper involved, and 31 tests reaching a 180-second timeout costs an hour to
learn what `test/README.md` already records.

## 11. Where it runs, as measured

Isolation is a claim with a per-platform truth value, so this table says what
has been *seen*, not what should happen.

**The macOS rows now come from two machines, and the difference between them is
the point.** On the one development laptop, MPICH 4.3.1, Open MPI 5.0.6 and Open
MPI 5.0.10 each need `scripts/host-env.sh` in front of `ctest` or fail 6 of them.
On GitHub's `macos-15` runners the same implementations need none of it. That is
§12's attribution — the machine, not the MPI — measured on a second machine
rather than argued from one.

**Rows quoting 13/13 ran the whole of `ctest` as it stood then** under
`.github/workflows/ci.yaml`; the 6/6 rows quote only the tests that need an MPI.
Both are green runs, counted differently. The suite is **fourteen** tests since
`abi_large_count_test` joined it, so a row recorded after that reads 14/14 --
the older figures are left as they were measured rather than rewritten.

**"two ranks" in this table is now checked rather than asserted.** It used to be
neither: the tests accept one rank as well as two, so a launcher that answered
`-n 2` with two singletons produced a green run at a job size nobody asked for,
and the MPICH row said "two ranks" for a year of runs that had none
(`HISTORY.md` §2.14). CMake now puts the count it asked the launcher for into
each test's environment and `test/expect_ranks.h` fails a test given a different
one, so every row re-run since carries the claim in its exit status. Every row
above with an image and a rank count has been re-run under it, `MPICH 3.1.4`
included; the two `refused at load` rows and the ones marked unverified inherit
their old evidence.

| configuration | mechanism | status |
|---|---|---|
| macOS, native MPICH 4.3.1 | `RTLD_LOCAL` + two-level namespace | **works**, 6/6 tests, two ranks — under `scripts/host-env.sh` |
| macOS, native Open MPI 5.0.6 | same | **works**, 6/6, two ranks — same script |
| macOS, conda-forge Open MPI 5.0.10 | same | **works**, 6/6, two ranks — same script |
| macOS, native Open MPI 6.1.0a1 | same | **works**, 6/6, two ranks — despite 698 weak `MPI_*`; needs no such script |
| macOS 15 arm64, Homebrew MPICH 5.0.1 | same | **works**, 13/13, two ranks — GitHub Actions, no `host-env.sh`. The first MPI-5.0 implementation this has run against (`mpicc` reports `MPI 5.0`), and it does **not** ship the standard ABI, so it is an ordinary wrapped implementation like every other row here |
| macOS 15 arm64, Homebrew Open MPI 5.0.9 | same | **works**, 13/13, two ranks — GitHub Actions, no `host-env.sh`; reports `MPI 3.1` |
| macOS, wrapper forced `-flat_namespace` | none | **refused at load**, and that refusal is a test |
| macOS, an ABI-implementing MPI | none available | **refused at load**; dyld coalesces weak definitions. Still *none available*, and the MPICH 5.0.1 row above is why that is worth saying twice: implementing MPI-5.0 and shipping the standard ABI are different things, and that one is the former without the latter |
| Linux glibc, MPICH 4.2.1 | `RTLD_LOCAL \| RTLD_DEEPBIND` | **works**, 6/6, two ranks (Debian 13, aarch64, Docker); and 13/13, two ranks on **x86_64** (Debian 13 container, GitHub Actions, `MPI 4.1`) |
| Linux glibc, MPICH 4.2.0 | same | **cannot run two ranks** (Ubuntu 24.04): PMIx-only `libmpi`, PMI-1 hydra, so `-n 2` is two singletons. `HISTORY.md` §2.14 |
| Linux glibc, Open MPI 4.1.6 | same | **works**, 6/6, two ranks (Ubuntu 24.04, aarch64, Docker); and 13/13, two ranks on **x86_64** (Ubuntu 24.04 container, GitHub Actions, `MPI 3.1`) |
| Linux glibc x86_64, MPICH 4.3.1 | same | **works**, 13/13, two ranks — built from the pinned tarball by `install-mpich.sh`, GitHub Actions. The primary MPICH row of `NOTES.md` #9's version table. Was "the only one providing the `_c` surface"; the MVAPICH and Intel MPI rows below both provide it too |
| Linux glibc x86_64, Open MPI 5.0.6 | same | **works**, 13/13, two ranks — built from the pinned tarball by `install-openmpi.sh`, GitHub Actions. The only Open MPI 5.x row on Linux |
| Linux glibc x86_64 and aarch64, MVAPICH 4.1 | same | **works, 12/13**, two ranks — built from the pinned tarball by `install-mvapich.sh`, in a container (`dev/third-implementations/run.sh mvapich`). `abi_arrays_test` is the one failure and it is upstream: `MPI_Dist_graph_create` does not return over MVAPICH's ch4:ofi device, reproduced with two ranks and no wrapper loaded, unaffected by `FI_PROVIDER`. `check-install.sh` passes. Needs `libibverbs-dev`/`librdmacm-dev` to build at all. Confirmed on **GitHub Actions, both arches**, run 32611538158: identical 12/13 with `abi_arrays_test` timing out at the 45 s cap. The two `linux-source` legs stay report-only while that hang is upstream; 12/13 is the state to expect from them |
| Linux glibc x86_64, Intel MPI 2021.15 | same | **works**, 13/13 and all `check-install.sh` legs, two ranks — `apt install intel-oneapi-mpi-devel=2021.15.0-493`, in a container under qemu (`dev/third-implementations/run.sh intelmpi`). Declares `MPI 3.1` and yet links `MPI_Type_size_c`, so the declared level understates the surface. **Pinned below 2021.17**, where Intel began shipping its own `libmpi_abi.so` — wrapping such a release is redundant, and its library also captures ours through `LD_LIBRARY_PATH` (`NOTES.md` #13.2). Not 2021.16 either: `NOTES.md` #13.4. `linux-oneapi` gates, and is green at the pin on GitHub Actions — run 32655424315, all steps including the install step's guard, which fails the job if the release turns out to ship an ABI library after all |
| Linux glibc, MPICH 3.1.4 (MPI-3.0) | same | **works**, 6/6, two ranks — the configure floor, verified. `run-linux-docker.sh floor`, built from source on Ubuntu 20.04 / gcc 9, the newest gfortran its configure accepts |
| Linux glibc, unisolated `dlopen` | none | **refused at load**, with the capture diagnostic |
| Linux glibc, `dlmopen(LM_ID_NEWLM)` | — | **does not work with a real MPI** (`NOTES.md` #2) |
| Linux musl | neither mechanism exists | expected to be refused at load; no Alpine support |
| Linux glibc i386 (32-bit) | `RTLD_LOCAL \| RTLD_DEEPBIND` | **works**, 13/13, two ranks (`debian:13` i386, GitHub Actions). The only row where an ABI handle is 32 bits, so the only one where §5.1's dynamic-handle collision probe is a real test |
| FreeBSD 14.3 | `RTLD_DEEPBIND`, narrower there than on glibc | **not supported**, and **refused at load**, which is the correct outcome: it builds, it `dlopen`s, and the outward-resolution check catches the capture and says so. FreeBSD promotes only the loaded library's own symbols, not its dependencies', and the symbol we need is a dependency's. `NOTES.md` #13.4. **No CI row** — dropped once it had established this |
| Windows | — | open |

32-bit (i386, arm32v7) has been run for the loader probe and the type-identity
probe, not for the library.

## 12. Environment quirks that cost time to attribute

None is about this project.

- **`OMPI_MCA_btl_vader_single_copy_mechanism=none` breaks RMA on Open MPI
  4.1.6**: `MPI_Win_create` answers `MPI_ERR_WIN` and the default errhandler
  aborts the job. `ci-scripts/linux-test.sh` sets it, so it is the Linux row's
  configuration rather than an accident of a shell. Measured both ways.
- **MPICH's `ch4:ofi` picks a VPN interface** when one is up, and `MPI_Finalize`
  then fails with `OFI poll failed (default nic=utun0)`. `FI_PROVIDER=tcp`
  avoids it. Not set in `CMakeLists.txt`, since it is a property of the host —
  but it silently removes the suite's whole `threads` directory, which is why
  `run-suite.sh` prints that decision. `ctest` against MPICH fails 6 of 13
  without it.
- **Open MPI 5.0.x's launcher needs pinning to loopback on the development
  laptop**, and its TCP BTL keeping off the same VPN interface. Without them,
  conda-forge's 5.0.10 and a 5.0.6 built from source behave identically, and
  identically *without* any of this project: a wrapper-free hello world reports
  "rank 0 of 1" from both processes and segfaults in `PMIx_Finalize`. The cause
  is that machine's application firewall, which blocks incoming connections to
  `prte`; PMIx 5 has only a TCP client-server transport and advertises a
  non-loopback address by default, so the ranks cannot reach their own launcher.
  `test/README.md` has the attribution and the three measurements,
  `HISTORY.md` §2.13 what this entry used to claim.

**The two above are the same shape** — an MPI chooses a network interface and
on this host chooses wrong — and `scripts/host-env.sh` carries all four
variables that settle them, exported unconditionally because each is inert for
the MPIs it is not addressed to. Measured, not assumed: with all four set,
`test/`'s suite passes 13/13 against MPICH 4.3.1, Open MPI 5.0.6 and Open MPI
5.0.10 alike. **Local to one host**, which is why they live in a script this
repository owns rather than in an account-wide `mca-params.conf` or a shell
profile, where they would reshape every MPI run by this account with nothing
recording why.
