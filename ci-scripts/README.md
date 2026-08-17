# `ci-scripts/`

MPI install and build-shape checks: pinned released tarballs, built from source
and cached. NOTES.md §9 ("Provisioning MPI in CI").

Note the deliberate split from `ci-scripts/suite/`: the MPI-install cache key
must hash these scripts and must *not* hash the suite's expected-failure list,
or every edit to a reason rebuilds MPI on every variant (a mistake mpif's own
`ci-scripts/README.md` records getting wrong twice).

S6 (build, packaging, CI matrix) added the rest: the two pinned-tarball
installers and the install-consumption check. The Linux runner is older,
added in S1 because the developers' machines are macOS and the two platforms
do not fail the same way.

| script | runs | what it does |
|---|---|---|
| `linux-test.sh mpich\|openmpi` | inside Linux | installs packages (if root), reports the MPI version and its `MPI_`/`PMPI_` symbol binding, configures, builds, and runs `ctest` |
| `run-linux-docker.sh [row...]` | on the host | runs the above in a container: `mpich`, `openmpi`, or `floor`, each on the image its row needs |
| `linux-floor.sh` | inside Linux | the `floor` row: builds MPICH 3.1.4 from source and hands over to `linux-test.sh`. Opt-in — the build takes some fifteen minutes |
| `install-mpich.sh <prefix> [<version>]` | anywhere | downloads, configures (stock, no patches), builds and installs a pinned MPICH release |
| `install-openmpi.sh <prefix> [<version>]` | anywhere | the same for Open MPI |
| `check-install.sh mpich\|openmpi\|/path/to/mpicc` | anywhere | S6's exit check: configure, build and install this project into a prefix of its own, then build and run a program through each of the three consumption routes (NOTES.md #9) with the loader's search path cleared |

Unlike mpif's `install-mpich.sh`/`install-openmpi.sh`, these two are a stock
`configure && make && make install` with nothing carried: mpif needs an MPI
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
two-rank job; Open MPI stays on `ubuntu:24.04`, because
`suite/xfail-openmpi.txt` is calibrated line by line against the Open MPI 4.1.6
that image ships; and `floor` runs on `ubuntu:20.04`, because gcc 9's gfortran
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
  `PMPI_*`. It varies by implementation and platform (Ubuntu's MPICH and Open
  MPI define both strongly at one address; macOS MPICH keeps `PMPI_*` in a
  separate library), and the design only needs both names to exist and reach the
  same code.
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

`.github/workflows/ci.yaml` holds nine jobs over seventeen legs, and each one
calls a script from this directory wherever a script exists rather than
repeating its recipe in YAML. That is the point of the split: the reasons live
here, next to the code they are about, and stay runnable by hand.

| job | calls | provides its own MPI? |
|---|---|---|
| `checks` | `cmake -DMPI_ABI_BUILD_WRAPPER=OFF` + `ctest` | no MPI at all — the five generator and header checks, plus `exported-symbols`, which is oracle 1 |
| `linux-distro` | `linux-test.sh mpich\|openmpi` in `container:` | the distro's, installed by the script itself as root. Both arches |
| `linux-source` | `install-{mpich,openmpi}.sh`, then `linux-test.sh <mpicc>`, then `check-install.sh <mpicc>` | pinned tarballs, built once and cached. Both arches |
| `linux-i386` | `docker/mpich-i386.dockerfile`, whose last `RUN` is `linux-test.sh` | Debian i386's. The only 32-bit row, and the only one where an ABI handle is not 64 bits |
| `compile` | `cmake` with `icx` and with `nvc` | the pinned MPICH, restored from `linux-source`'s cache. Builds only — no launcher question |
| `sanitize` | `cmake -DMPI_ABI_SANITIZE=address,undefined` | the distro's, in `debian:13`. Excludes the tests that `dlopen` a wrapper, which ASan cannot load |
| `macos` | `cmake`/`ctest` directly, then `check-install.sh` | Homebrew, one formula per leg |
| `suite-mpich` | `suite/run-suite.sh <mpicc> --variant=mpich` | the pinned MPICH 4.3.1, restored from `linux-source`'s cache — the version `suite/xfail-mpich.txt` is calibrated against, which the distro's 4.2.1 is not |
| `suite-openmpi` | `suite/linux-suite.sh openmpi` in `container: ubuntu:24.04` | the 4.1.6 that image ships, installed by the script itself as root — the same pin `suite/xfail-openmpi.txt` is calibrated against |

**The two suite rows are report-only**, `continue-on-error: true`, on the rule
the `compile` job established: a row nobody has ever seen green cannot tell a
regression from the thing it was added to find. Neither list has ever been
gated on a GitHub runner, and both carry lines that are properties of the host
they were recorded on — `xfail-mpich.txt`'s group (g) is three tests that pass
standalone and exceed the suite's 180-second limit inside a full run, and
`xfail-openmpi.txt`'s header records the `rma/linked_list` family passing on a
quiet run and failing under load. Both directions of the gate can fire for that
reason alone. Each row keeps `summary.tap` and its logs as an artifact whether
it passed or not, which is what `run-suite.sh --gate-only` needs to retriage a
line without a fresh 40-minute run.

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
  third installer joins the key by existing. The version is in the key too,
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
