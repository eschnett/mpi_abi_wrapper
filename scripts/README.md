# `scripts/`

The same build/check recipes as `ci-scripts/`, runnable locally rather than
only in CI. NOTES.md §9.

| script | what it does |
|---|---|
| `check-install.sh [<mpicc>]` | thin wrapper around `ci-scripts/check-install.sh`, defaulting to whatever `mpicc` is on `PATH` |
| `install-mpi.sh mpich\|openmpi [<version>]` | thin wrapper around the matching `ci-scripts/install-*.sh`, installing into `build/mpi/<name>-<version>` (under this project's own gitignored `build/`) instead of a temporary directory |
| `host-env.sh [<command>...]` | not a recipe but the four environment variables **the laptop this project is developed on** needs before any MPI here runs correctly |

The first two need no container: `ci-scripts/check-install.sh` only
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

## `host-env.sh`

The odd one out. It carries no recipe; it carries **host configuration**, for
the single machine this project is developed on, so that the configuration is
version-controlled and opted into by name instead of living in an account-wide
`~/.pmix/mca-params.conf`, `~/.openmpi/mca-params.conf` or a shell profile —
where it would apply to every MPI run by this account, including runs that have
nothing to do with this project, and where nothing would record why it was
there. Anything else this repository ever needs to say about one machine belongs
here for the same reason.

Two of `test/README.md`'s three environment quirks are in it, and they are the
same shape: **an MPI picks a network interface on its own, and on this host the
one it picks does not work.**

| variable | for | what goes wrong without it |
|---|---|---|
| `PMIX_MCA_pif_base_retain_loopback`, `PMIX_MCA_ptl_base_if_include` | Open MPI 5.0.x | this laptop's application firewall blocks the launcher from accepting the connections its own ranks make, and PMIx 5 has no non-TCP path to fall back to, so `mpiexec -n 2` yields two singletons |
| `OMPI_MCA_btl_tcp_if_include` | Open MPI 5.0.x | the TCP BTL picks a VPN interface; `MPI_Comm_spawn` fails with "No route to host" |
| `FI_PROVIDER=tcp` | MPICH | `ch4:ofi` picks a VPN interface and `MPI_Finalize` fails with `OFI poll failed` |

All four are exported unconditionally. Each is inert for the MPIs it is not
addressed to, and the combination is measured rather than assumed: `test/`'s
suite passes 13/13 with all four set against MPICH 4.3.1, Open MPI 5.0.6 and
Open MPI 5.0.10, and Open MPI 6.1.0a1 — which needs none of them — is unharmed.
So there is nothing to dispatch on, and a script that had to be told which MPI
you meant would be a worse interface than one you can put in front of anything.

**None of it is worth anything on another machine.** It depends on this host's
firewall being on and on this host's interfaces, and pinning MPI to loopback is
free only because this laptop could not run a job across nodes in any case.

```sh
. scripts/host-env.sh                          # export into this shell
scripts/host-env.sh mpiexec -n 4 ./prog        # set them for one command
scripts/host-env.sh ctest --test-dir build/mpich --output-on-failure
eval "$(scripts/host-env.sh)"                  # for a shell that cannot source it
```
