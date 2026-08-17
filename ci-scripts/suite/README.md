# `ci-scripts/suite/`

MPICH's own C test suite, run against this project (HISTORY.md S7, NOTES.md
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
unfinished-looking.** `xfail-mpich.txt` is fully triaged: **41** failures, each
with a cause. The three bugs of ours that this suite found are not in it,
because all three were fixed -- the last of them, `MPI_DISPLACEMENT_CURRENT`,
emptied a whole group out of the file. `xfail-openmpi.txt` is **168**, of
which the entry points Open MPI 4.1.6 simply does not have are attributed
mechanically -- from the probe header that records what the implementation
provides -- and about half are honest placeholders saying what was observed
and claiming nothing more. Finishing that triage is the next session's first
task; the method is the one used throughout this stage, which is to build the
same test with the implementation's own `mpicc` and see whether it passes
without the wrapper.

**Failure counts above are `grep -cvE '^\s*(#|$)'` on the lists -- most of each
file is comment, so plain `wc -l` is not the number -- and the per-run *totals*
are not quoted here on purpose.** They moved twice as the lists were retriaged and
the copies of them did not all follow -- four documents disagreed (1212, 1229,
1230, 1231) before this note replaced them. A total is only as good as the run
that produced it, and three configure decisions change it, including whether
the `threads` directory exists at all. `run-suite.sh` prints all three; take
the total from a run rather than from prose.

## Two things about the environment

**`FI_PROVIDER`.** MPICH's `ch4:ofi` picks a VPN interface when one is up and
`MPI_Finalize` then fails (test/README.md). `FI_PROVIDER=tcp` avoids it. The
runner does not set it, because it is a property of a host rather than of this
suite; on the machine this project is developed on, `scripts/host-env.sh` is
what sets it, and this runner is one more thing to put that script in front of.
The runner does print what the suite's configure decided, and one of those
decisions (`whether MPI_THREAD_MULTIPLE is supported`) is made by *running* an
MPI program. A broken environment therefore removes the whole `threads`
directory from the run rather than failing anything, which is how a green run
can cover less than the last one. That is why those three lines are printed.

**The Open MPI row runs on Linux.** When it was established, no Open MPI 5.0.x
launcher would run a job on the development laptop, and a suite whose every
test is a launcher failure is a list of 900 excuses rather than a result. In a
container it works, so `linux-suite.sh` is where that row lives. That laptop has
since been fixed (`scripts/host-env.sh`, and test/README.md's third environment
quirk for why it needed fixing), so a macOS Open MPI row is now possible rather
than impossible — but it has not been run, and this row is the one that exists.

## In CI

`.github/workflows/ci.yaml` runs both rows, one job each: `suite-mpich` over the
pinned MPICH 4.3.1 that `ci-scripts/install-mpich.sh` builds — restored from the
cache the `linux-source` rows save, and 4.3.1 because that is the version
`xfail-mpich.txt` is calibrated against where the distro rows' 4.2.1 is not — and
`suite-openmpi` through `linux-suite.sh` in the `ubuntu:24.04` container whose
Open MPI 4.1.6 `xfail-openmpi.txt` is calibrated against.

**Both are report-only there until they have been green**, which is the rule that
workflow's `compile` job records: a row nobody has seen pass cannot tell a
regression from the thing it was added to find. Each job keeps `summary.tap` and
the logs as an artifact whether it passed or not, because `--gate-only` retriages
from a TAP file in hand rather than from a fresh 40-minute run. Deleting
`continue-on-error` is what makes a row gate.

**What the first run there (32069590099) found**, which is why neither gates yet:

| | wall | tests | listed failures that fired | differences |
|---|---|---|---|---|
| `suite-mpich` | 39 min | 847, 795 passed | 39 of 41 | 3, every one timing |
| `suite-openmpi` | 75 min | 847, 664 passed | 165 of 168 | 9, none timing |

The MPICH row's three are the host: `pt2pt/sendflood 8` spent the whole
180-second limit against 2.6 s on the calibration machine, and two of group (g)'s
three passed here in under two seconds. The cause is measured — at four vCPUs the
runner is 5–25× *faster* below np 4 and 4.7–7× slower above it, MPICH's progress
engine busy-polling once ranks exceed cores — and the workflow sets
`MPITEST_TIMEOUT_MULTIPLIER: 2` on that row for it.

The Open MPI row's nine are not: `io/setviewcur 4` and `io/i_setviewcur 4` pass
now, the `MPI_DISPLACEMENT_CURRENT` fix having emptied that group out of the
MPICH list and never out of this one; `rma/linked_list_bench_lock_shr 4` is the
load-sensitive family this list's header already describes; and **six are new and
unattributed** — `coll/allred 4` answering 512 wrong results for `MPI_SUM` over
the 8-bit integer types, and five `threads/pt2pt/mt_*probe*` tests, four of them
dying in `MPI_Recv` with `MPI_ERR_COUNT`. The known difference from the run this
list was calibrated on is the architecture: that one was aarch64 under Docker
Desktop, the runner is x86_64. Attributing them is this stage's next task and the
method is S7's throughout — build the same test with Open MPI's own `mpicc` and
see whether it passes unwrapped.
