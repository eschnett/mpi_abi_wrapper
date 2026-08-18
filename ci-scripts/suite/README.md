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
| `xfail-mpich.txt`, `xfail-openmpi.txt` | the **local** expected failures, one reason per line |
| `xfail-ci-<mpi>.txt`, `xfail-ci-<mpi>-<arch>.txt` | the **CI** expected failures: what every architecture of an implementation sees, plus a per-architecture delta beside it |
| `i386-suite.sh` | the 32-bit leg: builds MPICH inside a `linux/386` container, asserts pointers really are 4 bytes, then runs the suite |
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
  not this project's to pass. MPICH 5.0.1's own testlist no longer carries that
  directory at all, so against the current pin this exclusion is a no-op that
  costs nothing and still covers 4.3.x.
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

`check-tap.py` compares the TAP output against `xfail-<variant>.txt`, or against
however many lists `--xfail=` named, **in both directions** — which is the
discipline `dev/check-c-bindings.py` and `dev/check_prototype.py` already apply:

- a failure that is not listed fails the run;
- a listed failure that **passed** fails the run, because an expectation that
  has stopped firing is either fixed or was never about what it says;
- a listed test that did not run at all fails the run, so a line cannot
  outlive the test it names — unless the run did not cover that directory,
  which the script is told rather than left to guess;
- **a line with no reason fails the run.** `--update-xfail` writes exactly
  such lines, so a run that discovers new failures cannot be made green by
  re-running with it;
- **a test listed in two of the files fails the run.** Several lists are read as
  one, and a test named in both the shared and the per-architecture file would
  mean two reasons for one test, one of which nobody maintains.

A line is `<dir>/<program> <np> : why`, with runtests' own name for the test
so the list can be compared with an unwrapped run by eye.

**The two local lists are in different states, and that is deliberate rather
than unfinished-looking.** (Both describe the development laptop against the
older pair of MPIs; the CI lists are further down and are empty.)
`xfail-mpich.txt` is fully triaged: **41** failures, each
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

`.github/workflows/ci.yaml` runs **five environments**, and each has its own
expected-failure list:

| leg | MPI | list(s) it gates against |
|---|---|---|
| `suite` × x86_64 | MPICH 5.0.1, from source | `xfail-ci-mpich.txt` + `xfail-ci-mpich-x86_64.txt` |
| `suite` × aarch64 | MPICH 5.0.1, from source | `xfail-ci-mpich.txt` + `xfail-ci-mpich-aarch64.txt` |
| `suite` × x86_64 | Open MPI 5.0.10, from source | `xfail-ci-openmpi.txt` + `xfail-ci-openmpi-x86_64.txt` |
| `suite` × aarch64 | Open MPI 5.0.10, from source | `xfail-ci-openmpi.txt` + `xfail-ci-openmpi-aarch64.txt` |
| `suite-i386` | MPICH 5.0.1, from source, in a `linux/386` container | `xfail-ci-mpich.txt` + `xfail-ci-mpich-i386.txt` |

The shared file holds what every architecture of that implementation sees and the
delta holds the rest; `check-tap.py` reads them as one and rejects a test listed
in both. **Why the split exists is measured, not tidiness** — see the two runs
below. The laptop's own `xfail-mpich.txt` and `xfail-openmpi.txt` stay where they
are and describe that machine, pinned to the older pair of MPIs and to the 4.3.1
suite.

The two implementations are not symmetric and the lists should not be expected to
look alike. MPICH 5.0.1 is the first release that is a complete MPI-5.0 — its own
header says `MPI_VERSION 5` / `MPI_SUBVERSION 0` — so it provides the ABI's whole
surface including the `_c` large-count forms. Open MPI 5.0.10 still declares
`MPI_VERSION 3` / `MPI_SUBVERSION 1` and still has no `_c` entry point at all, so
that half of the ABI is decision 6's stubs on its legs. MPICH 5.0.1 can implement
the standard ABI itself, and these legs deliberately do not ask it to: that is
behind `--enable-mpi-abi` and a separate `mpicc_abi`, and wrapping a library that
already exports the ABI is a *different* oracle, the one that refuses at load on
macOS by design.

**All five are report-only until they have been green**, which is the rule that
workflow's `compile` job records: a row nobody has seen pass cannot tell a
regression from the thing it was added to find. Every CI list is empty as this
lands, which states that nothing has been triaged for these five environments
rather than that the runs are clean — MPICH 5.0.1 answers `init/version`
correctly and fills in the entry points the older lists' largest group was about,
so those lines could not simply be carried over. Each leg keeps `summary.tap` and
its logs as an artifact whether it passed or not, because `--gate-only` writes a
list from a TAP file in hand rather than from a fresh 40-minute run. Deleting
`continue-on-error` is what makes a leg gate.

## What the previous pins measured, and why the split exists

Three runs over **MPICH 4.3.1 and the Open MPI 4.1.6 Ubuntu 24.04 ships**, with
the 4.3.1 suite. Neither pair is what CI provisions any more, and the numbers are
kept because they are what the per-environment split is built on rather than
because they describe the current rows.

| | wall | tests | listed failures that fired | differences |
|---|---|---|---|---|
| MPICH | 39–43 min | 847, 795–796 passed | 38–39 of 41 | 4, every one timing |
| Open MPI | 75–80 min | 847, 663–665 passed | 165 of 168 | 10 tests, 5 failing every run |

**The MPICH differences were all the machine.** `pt2pt/sendflood 8` spent the
whole limit where the laptop runs it in 2.6 s, and all three of the timing group
passed on the runner at least once. The cause was measured: at four vCPUs the
runner is 5–25× *faster* at four ranks or fewer and 4.7–7× slower above that,
MPICH's progress engine busy-polling once ranks exceed cores. Doubling the limit
did not rescue `sendflood` — it spent 360 s too — but it did rescue
`coll/reduce 10`, which landed at 179.7 s of a 180 s base limit. That is why the
MPICH legs set `MPITEST_TIMEOUT_MULTIPLIER: 2` and the Open MPI legs do not:
Open MPI had nine tests at the limit and they were hangs, where doubling pays the
limit twice over and buys nothing.

**The Open MPI differences were not timing, and one of them is why there is a
per-architecture file at all.** `coll/allred 4` returned 512 wrong results for
`MPI_SUM` over `MPI_UNSIGNED_CHAR`, `MPI_INT8_T` and `MPI_UINT8_T` on x86_64,
where the aarch64 run that produced the list saw none; Open MPI's AVX reduction
component is x86-only, and `OMPI_MCA_op=^avx` is the one-variable check nobody
has run yet. Four `threads/pt2pt/mt_*probe*` tests died in `MPI_Recv` with
`MPI_ERR_COUNT` and a fifth hung. All of them pass over MPICH on the same runner
with the same wrapper build, so whatever they are, they are specific to the Open
MPI side. Two `io/` lines passed every run, the `MPI_DISPLACEMENT_CURRENT` fix
having been taken out of the MPICH list and never out of that one.
