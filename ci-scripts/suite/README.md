# `ci-scripts/suite/`

MPICH's own C test suite, run against this project (STAGES.md S7, NOTES.md
#10). It is the first oracle here that nothing in this repository wrote: ~900
programs that know only the MPI standard, compiled with the wrapper's `mpicc`
and linked against `libmpi_abi`, exactly as HDF5 or PETSc will be in S8.

Deliberately cache-keyed apart from `ci-scripts/` (see that directory's
README): editing a reason here must not invalidate the MPI-install cache.

| file | what it is |
|---|---|
| `run-suite.sh` | the runner: build and install this project, fetch and configure the suite against it, build the tests, run them, gate the result |
| `mpiexec-filter` | the `mpiexec` the suite sees, since a wrapper prefix has no launcher of its own |
| `check-tap.py` | the gate: runtests' TAP output against this variant's expected-failure list, in both directions |
| `xfail-mpich.txt`, `xfail-openmpi.txt` | the expected failures, one reason per line |
| `linux-suite.sh` | the same, inside a container: packages, `/tmp` for everything writable |

```sh
ci-scripts/suite/run-suite.sh mpich                     # a distro or PATH mpicc
ci-scripts/suite/run-suite.sh /path/to/mpicc --dirs=pt2pt,coll
MPIABI_LINUX_SCRIPT=/src/ci-scripts/suite/linux-suite.sh \
  ci-scripts/run-linux-docker.sh openmpi                # the Open MPI row
```

## How the suite is configured, and why each part matters

`--with-mpi=$prefix`, where `$prefix` is an installed *wrapper*, not an MPI.
`CC` is then `$prefix/bin/mpicc` and the suite's own probe answers **"Is the
MPI derived from MPICH... no"** — the ABI header defines no `MPICH` macro — so
it drops into its generic-MPI mode. That answer is load-bearing rather than
cosmetic: it is what keeps MPICH-specific expectations out of a run whose
implementation happens to be MPICH.

`--enable-strictmpi`, because this project implements the standard and nothing
else. The suite's non-strict tests reach for `MPIX_` entry points and MPICH
internals.

`MPIEXEC=ci-scripts/suite/mpiexec-filter`. The four jobs that file has to do
are in its own header; the short version is that the launcher lives in the
wrapped MPI's prefix rather than in the wrapper's, that testlists carry
launcher-specific arguments (`-disable-auto-cleanup` is hydra's spelling),
that `env=` settings reach the ranks only if the launcher forwards them, and
that a hung test otherwise costs the whole run.

Fortran and C++ are off. The ABI is a C ABI; `mpif` is the Fortran oracle and
it is S8's.

## What the runner covers, and what it leaves out

Everything in the suite's own top-level testlist except two directories, each
excluded in one place with its reason printed at the start of every run:

- **`impls`** — MPICH's own PMI, hydra and `MPIX_` tests. Not standard MPI, so
  not this project's to pass.
- **`spawn`** — off by default, `--with-spawn` to include, and with it the
  `spawn` subdirectories of `errors/` and `threads/`, which the top-level
  exclusion does not reach. `MPI_Comm_spawn` hangs under hydra on macOS with
  no wrapper involved (test/README.md measured it against a fifteen-line
  program), and every one of those tests reaching its 180-second timeout
  costs an hour to learn nothing.

`testlist.dtp` — the same call over hundreds of generated derived datatypes —
is included, and for a conversion layer it is the most valuable part of the
run. `--no-dtp` leaves it out for a quick pass.

## The gate

`check-tap.py` compares the TAP output against `xfail-<variant>.txt` **in both
directions**, which is the discipline `dev/check-c-bindings.py` and
`dev/check_prototype.py` already apply:

- a failure that is not listed fails the run;
- a listed failure that **passed** fails the run, because an expectation that
  has stopped firing is either fixed or was never about what it says;
- a listed test that did not run at all fails the run, so a line cannot
  outlive the test it names — unless the run did not cover that directory,
  which the script is told rather than left to guess;
- **a line with no reason fails the run.** `--update-xfail` writes exactly
  such lines, so a run that discovers new failures cannot be made green by
  re-running with it.

A line is `<dir>/<program> <np> : why`, with runtests' own name for the test
so the list can be compared with an unwrapped run by eye.

**The two lists are in different states, and that is deliberate rather than
unfinished-looking.** `xfail-mpich.txt` is fully triaged: 45 failures out of
1212 tests, each with a cause, and three of them bugs of ours that S7 found
(two fixed, one -- `MPI_DISPLACEMENT_CURRENT` -- diagnosed and left with the
diagnosis on the line). `xfail-openmpi.txt` is 171 failures out of 1231, of
which the entry points Open MPI 4.1.6 simply does not have are attributed
mechanically -- from the probe header that records what the implementation
provides -- and about half are honest placeholders saying what was observed
and claiming nothing more. Finishing that triage is the next session's first
task; the method is the one used throughout this stage, which is to build the
same test with the implementation's own `mpicc` and see whether it passes
without the wrapper.

## Two things about the environment

**`FI_PROVIDER`.** MPICH's `ch4:ofi` picks a VPN interface when one is up and
`MPI_Finalize` then fails (test/README.md). `FI_PROVIDER=tcp` avoids it. The
runner does not set it, because it is a property of a host rather than of this
suite — but it prints what the suite's configure decided, and one of those
decisions (`whether MPI_THREAD_MULTIPLE is supported`) is made by *running* an
MPI program. A broken environment therefore removes the whole `threads`
directory from the run rather than failing anything, which is how a green run
can cover less than the last one. That is why those three lines are printed.

**The Open MPI row runs on Linux.** No Open MPI 5.0.x launcher works on macOS
26, with or without this project (test/README.md), and a suite whose every
test is a launcher failure is a list of 900 excuses rather than a result. In a
container it works, so `linux-suite.sh` is where that row lives.
