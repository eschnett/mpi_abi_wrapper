# `ci-scripts/`

MPI install and build-shape checks: pinned released tarballs, built from source
and cached. NOTES.md §9 ("Provisioning MPI in CI").

Note the deliberate split from `ci-scripts/suite/`: the MPI-install cache key
must hash these scripts and must *not* hash the suite's expected-failure list,
or every edit to a reason rebuilds MPI on every variant (a mistake mpif's own
`ci-scripts/README.md` records getting wrong twice).

S6 (build, packaging, CI matrix) added the rest: the pinned-tarball installers —
two then, three now that MVAPICH has a row — and the install-consumption check.
The Linux runner is older, added in S1 because the developers' machines are macOS
and the two platforms do not fail the same way.

| script | runs | what it does |
|---|---|---|
| `linux-test.sh mpich\|openmpi` | inside Linux | installs packages (if root), reports the MPI version and its `MPI_`/`PMPI_` symbol binding, configures, builds, and runs `ctest` |
| `run-linux-docker.sh [row...]` | on the host | runs the above in a container: `mpich`, `openmpi`, or `floor`, each on the image its row needs |
| `linux-floor.sh` | inside Linux | the `floor` row: builds MPICH 3.1.4 from source and hands over to `linux-test.sh`. Opt-in — the build takes some fifteen minutes |
| `install-mpich.sh <prefix> [<version>]` | anywhere | downloads, configures (stock, no patches), builds and installs a pinned MPICH release |
| `install-openmpi.sh <prefix> [<version>]` | anywhere | the same for Open MPI |
| `install-mvapich.sh <prefix> [<version>]` | anywhere | the same for MVAPICH. Needs `libibverbs-dev` and `librdmacm-dev` present — its bundled libfabric carries two providers MPICH's does not (`prov/mverbs`, `prov/ucr`) and they include `<infiniband/ib.h>` unconditionally |
| `suite/i386-suite.sh` | inside a `linux/386` container | builds MPICH from source, asserts that pointers really are 4 bytes, launches two ranks with no wrapper involved, and hands over to `suite/run-suite.sh` |
| `check-install.sh mpich\|openmpi\|/path/to/mpicc` | anywhere | S6's exit check: configure, build and install this project into a prefix of its own, then build and run a program through each of the three consumption routes (NOTES.md #9) with the loader's search path cleared |

Unlike mpif's `install-mpich.sh`/`install-openmpi.sh`, all three of these are a
stock `configure && make && make install` with nothing carried: mpif needs an MPI
that already implements the standard ABI, hence its pinned commit from
MPICH's `main`, its header substitution and its pruning of everything the ABI
does not define (NOTES.md #9, "Provisioning MPI in CI"). This project wraps
*any* MPI-3.0+ implementation through its own conversion layer, so a released
tarball, unmodified, is the whole of what CI needs to provision.

```sh
ci-scripts/run-linux-docker.sh                 # mpich and openmpi
ci-scripts/run-linux-docker.sh mpich
ci-scripts/run-linux-docker.sh floor           # the MPI-3.0 floor, from source
MPIABI_IMAGE=ubuntu:24.04 ci-scripts/run-linux-docker.sh   # override the image
```

**The image is per-row**, and `run-linux-docker.sh`'s header says why each
default is the one it is. In short: MPICH runs on `debian:13`, because Ubuntu
24.04's MPICH links `libpmix` and imports `PMIx_Init` while shipping a PMI-1
hydra, so `mpiexec -n 2` there returns two singletons and exit 0 rather than a
two-rank job; Open MPI stays on `ubuntu:24.04` for what its 4.1.6 covers and
nothing else does — that release provides 466 of the ABI's 688 entry points, so
it is the broadest exercise of decision 6's unsupported-operation paths, and the
local `suite/xfail-openmpi.txt` is calibrated against it (CI's Open MPI suite
legs build 5.0.10 from source and no longer share this pin); and `floor` runs on
`ubuntu:20.04`, because gcc 9's gfortran
is the newest MPICH 3.1.4's configure will accept. `MPIABI_IMAGE` overrides all
three, which is how the comparison above was made.

`floor` is not in the default set: it builds an MPI from source, which takes
some fifteen minutes against under two for either distro row. It is worth
running when anything under `dev/` or `CMakeLists.txt` changes, because it is
the only row that tests the *toolchain* floor rather than the standard one —
Python 3.8, CMake 3.16 as the distro ships it, gcc 9 — and that kind of drift is
invisible everywhere else (`HISTORY.md` §3, S6).

That failure is silent by nature, so the build no longer takes the launcher's
word for it: CMake puts the rank count it asked for into each test's
environment and `test/expect_ranks.h` fails a test that was handed a different
one. Two singletons where two ranks were requested is now a red run.

`linux-test.sh` installs packages only when it is root and `apt-get` exists, so
a prepared CI runner can call it directly with no container involved.

## Two things about it that are deliberate

**The source tree is mounted read-only.** That is what caught the first Linux
bug in the S0 header generator: GNU `patch -o -` writes a temporary file into
the *current directory*, where BSD `patch` does not, so `--check` failed
outright on a read-only checkout. It also keeps a Linux build from leaving
artifacts in a macOS working tree. Everything is built under `/tmp` in the
container.

**Gating and informational steps are separated, and only gating steps affect the
exit status.** Two steps measure the environment rather than this project:

- *symbol binding* — evidence for NOTES.md §2's claims about `MPI_*` versus
  `PMPI_*`. It varies by implementation and platform, and **three distinct
  patterns have now been measured**: Ubuntu's MPICH and Open MPI define both
  strongly at one address; macOS MPICH keeps `PMPI_*` in a separate library; and
  MVAPICH 4.1 (672) and Intel MPI 2021.18.1 (680) both define *weak* `MPI_*`
  over *strong* `PMPI_*` (`dev/third-implementations/`). `MPI_Send` and
  `PMPI_Send` share an address in every one of them, which is all the design
  needs. Worth noting that the two agreeing rows are the MPICH-derived ones
  while Ubuntu's MPICH is not among them, so the pattern tracks the build rather
  than the family — which is the reason this stays a per-row report rather than
  a table written down once.
- *`dlmopen` mode* — known not to work with any real MPI, for a reason outside
  this project: PMIx `dlopen`s components with `RTLD_GLOBAL`, and glibc cannot
  add to the global scope of a namespace with no main map, so `MPI_Init`
  segfaults inside the loader. It is run so that the day this changes is
  noticed, and never gated on. NOTES.md §2 has the backtrace and what it costs
  S9.

A failing gating step exits non-zero all the way out through
`run-linux-docker.sh`; that is worth stating because the first draft of this
script piped `ctest` into `tail` and would have reported success for a failed
run.

## What GitHub Actions runs

`.github/workflows/ci.yaml` holds ten jobs over thirty-eight legs, and each one
calls a script from this directory wherever a script exists rather than
repeating its recipe in YAML. That is the point of the split: the reasons live
here, next to the code they are about, and stay runnable by hand.

| job | calls | provides its own MPI? |
|---|---|---|
| `checks` | `cmake -DMPI_ABI_BUILD_WRAPPER=OFF` + `ctest` | no MPI at all — the five generator and header checks, plus `exported-symbols`, which is oracle 1 |
| `linux-distro` | `linux-test.sh mpich\|openmpi` in `container:` | the distro's, installed by the script itself as root. Both arches |
| `linux-source` | `install-{mpich,openmpi,mvapich}.sh`, then `linux-test.sh <mpicc>`, then `check-install.sh <mpicc>` | pinned tarballs, built once and cached. Both arches. The two MVAPICH legs are report-only until one has been seen green |
| `linux-oneapi` | apt, then `linux-test.sh <mpicc>`, then `check-install.sh <mpicc>` | Intel MPI from the oneAPI repository — a binary distribution, so there is no installer to call. x86_64 only, because Intel ships no aarch64 build. Report-only until seen green |
| `linux-i386` | `docker/mpich-i386.dockerfile`, whose last `RUN` is `linux-test.sh` | Debian i386's. The only 32-bit row, and the only one where an ABI handle is not 64 bits |
| `compile` | `cmake` with `icx` and with `nvc` | the pinned MPICH, restored from `linux-source`'s cache. Builds only — no launcher question |
| `sanitize` | `cmake -DMPI_ABI_SANITIZE=address,undefined` | the distro's, in `debian:13`. Excludes the tests that `dlopen` a wrapper, which ASan cannot load |
| `macos` | `cmake`/`ctest` directly, then `check-install.sh` | Homebrew, one formula per leg |
| `suite` | `suite/run-suite.sh <mpicc> --variant=ci-<mpi>-<arch> --xfail=… <shard>` | pinned tarballs — MPICH 5.0.1 or Open MPI 5.0.10 — restored from `linux-source`'s cache, with ccache behind the miss. **Sixteen legs**: two implementations × x86_64/aarch64 × four shards of the suite |
| `suite-i386` | `suite/i386-suite.sh` through `run-linux-docker.sh` | its own MPICH 5.0.1, built from source *inside* a `linux/386` container and cached by the 64-bit host. Four legs, the same four shards |

**The MPICH legs gate; the Open MPI legs are still report-only.** That is per
leg, not per job: `continue-on-error: ${{ matrix.leg.report_only }}`. The rule the
`compile` job established is that a row nobody has ever seen green cannot tell a
regression from the thing it was added to find, and the MPICH rows have now been
green twice — 842 tests, 789 passed, 41 failed, the 41 identical on both
architectures and matched in both directions by `suite/xfail-ci-mpich.txt`. The
Open MPI lists are still empty because no Open MPI leg has run to completion in
this CI environment; `suite/README.md` has the durations and what killed them.

Each leg gates against a shared per-implementation list plus a per-architecture
delta (`suite/xfail-ci-<mpi>.txt` and `suite/xfail-ci-<mpi>-<arch>.txt`). The
per-architecture files for MPICH are empty *by measurement*: x86_64 and aarch64
produced the same 41 failures test for test. Each leg keeps `summary.tap` and its
logs as an artifact whether it passed or not, which is what `run-suite.sh
--gate-only` needs to write a list without paying for another run.

**The suite is sharded by test directory, and that is what makes the slow legs
finishable rather than merely parallel.** Four shards — `coll`, `rma`,
`threads+pt2pt+part`, and the complement of those — chosen from measured
per-directory cost. `rma` is alone because over Open MPI it is 37 of the suite's
78 minutes, one directory larger than the whole budget, so isolating it is what
makes the other three safe. The last shard is expressed as `--skip-dirs` rather
than a fourth list, so a directory a later suite release adds is covered by
construction. `check-tap.py` needs no telling: it derives the directories a run
covered from the TAP itself, so a line for a directory this shard skipped counts
as "not run" rather than as a stale entry.

**Why one list per implementation *and* one per architecture.** Three runs of the
previous arrangement established that a single file cannot describe two machines.
On a four-vCPU runner every test asking for more ranks than there are cores runs
5–7× slower while everything at four ranks or fewer runs 5–25× *faster* than on
the development laptop, so timing expectations move in both directions at once;
and over Open MPI 4.1.6 the x86_64 runner returned wrong results for `MPI_SUM`
over the 8-bit integer types where an aarch64 run of the same list saw none. The
shared file holds what every architecture sees, the delta holds the rest, and
`check-tap.py` reads them as one while rejecting a test listed in both.

**FreeBSD had a row and no longer does.** It established that the platform
cannot be supported — `RTLD_DEEPBIND` there promotes only the loaded library's
own symbols, not its dependencies', so the wrapper is captured and refuses at
load (NOTES.md #13.4). A row that can only ever be red teaches nothing after the
first run. `git show 236b99a` has the recipe.

Three things about it that are decisions rather than defaults:

- **The MPI cache key hashes the install scripts, and only those.** It is
  `hashFiles('ci-scripts/install-*.sh')`: those two files are what put bytes in
  a prefix, so they are what can invalidate one. It began as `ci-scripts/**`
  minus the suite, which honoured the letter of the rule above and was broader
  than its reason — an edit to `linux-test.sh`, which cannot change a byte of an
  installed MPI, rebuilt four of them. A glob rather than the two names, so a
  third installer joins the key by existing — which `install-mvapich.sh` now
  has, with no edit to the key. The version is in the key too,
  because it is passed as a matrix argument rather than read from a file, so the
  installer's own default is hashed and the value actually used is not.
  (`ci-scripts/*` would still be the wrong spelling if the broad form ever comes
  back, for the `@actions/glob` reason above.)
- **The MPI is saved to the cache immediately after it installs**, through the
  `actions/cache/restore` + `save` pair rather than plain `actions/cache`, whose
  post step is `post-if: success()` — a failing test later in the job would
  otherwise discard an MPI that had just taken twenty minutes to build.
- **Every row that provisions its own MPI first launches two ranks with no
  wrapper involved**, and fails loudly if it gets one. This separates "this
  runner cannot launch a job" from "the wrapper is broken", which are otherwise
  the same red `ctest`. It exists because the tempting answer to the first —
  `-DMPI_ABI_TEST_USE_LAUNCHER=OFF` — sets `MPI_ABI_EXPECT_RANKS=1` and turns
  the row green at a job size nobody chose, which is the failure `HISTORY.md`
  §2.14 records going unnoticed for a year.

Two ways the container rows there are **weaker** than `run-linux-docker.sh`
here, worth knowing before reading a green run as parity: GitHub does not mount
the source tree read-only, so the `patch -o -` property that mount was written
to catch is untested; and its runners are x86_64, while every Linux row in
`CODE.md` §11 was measured on aarch64 under Docker Desktop.

The row this repository has a script for and that workflow does **not** run:
`floor`.
