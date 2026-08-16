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
