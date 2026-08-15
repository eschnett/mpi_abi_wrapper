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
| `run-linux-docker.sh [mpi...]` | on the host | runs the above in a container, for one MPI or both |
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
MPIABI_IMAGE=debian:13 ci-scripts/run-linux-docker.sh
```

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
