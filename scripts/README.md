# `scripts/`

The same build/check recipes as `ci-scripts/`, runnable locally rather than
only in CI. NOTES.md §9.

| script | what it does |
|---|---|
| `check-install.sh [<mpicc>]` | thin wrapper around `ci-scripts/check-install.sh`, defaulting to whatever `mpicc` is on `PATH` |
| `install-mpi.sh mpich\|openmpi [<version>]` | thin wrapper around the matching `ci-scripts/install-*.sh`, installing into `build/mpi/<name>-<version>` (under this project's own gitignored `build/`) instead of a temporary directory |

Both of these need no container: `ci-scripts/check-install.sh` only
configures, builds and installs this project and then drives ordinary
consumer builds against the result, and the two installers are a stock
`configure && make && make install` against a downloaded tarball. Neither
reaches for anything Linux-specific, unlike `ci-scripts/linux-test.sh`, which
`run-linux-docker.sh` runs in a container for exactly that reason.

```sh
scripts/install-mpi.sh mpich              # build/mpi/mpich-default (4.3.1)
scripts/install-mpi.sh openmpi 4.1.6      # build/mpi/openmpi-4.1.6
scripts/check-install.sh build/mpi/mpich-default/bin/mpicc
```
