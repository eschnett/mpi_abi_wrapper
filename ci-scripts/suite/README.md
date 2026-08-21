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
ci-scripts/suite/run-suite.sh /path/to/mpicc --skip-dirs=coll,rma   # the complement
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
  outlive the test it names — unless the run did not cover that directory, which
  the script is told with `--dirs` and otherwise derives from the TAP, a
  directory that produced no result at all not being one this run can judge;
- **a line with no reason fails the run.** `--update-xfail` writes exactly
  such lines, so a run that discovers new failures cannot be made green by
  re-running with it;
- **a test listed in two of the files fails the run.** Several lists are read as
  one, and a test named in both the shared and the per-architecture file would
  mean two reasons for one test, one of which nobody maintains.

A line is `<dir>/<program> <np> : why`, with runtests' own name for the test
so the list can be compared with an unwrapped run by eye.

**The two local lists are in different states, and that is deliberate rather
than unfinished-looking.** (Both describe the development laptop against the older
pair of MPIs. The CI lists are further down, are written from real runs, and are
not these.)
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

| environment | MPI | gates? | list(s) it gates against |
|---|---|---|---|
| `suite` × x86_64 | MPICH 5.0.1, from source | **yes** | `xfail-ci-mpich.txt` + `xfail-ci-mpich-x86_64.txt` |
| `suite` × aarch64 | MPICH 5.0.1, from source | **yes** | `xfail-ci-mpich.txt` + `xfail-ci-mpich-aarch64.txt` |
| `suite` × x86_64 | Open MPI 5.0.10, from source | not yet | `xfail-ci-openmpi.txt` + `xfail-ci-openmpi-x86_64.txt` |
| `suite` × aarch64 | Open MPI 5.0.10, from source | not yet | `xfail-ci-openmpi.txt` + `xfail-ci-openmpi-aarch64.txt` |
| `suite-i386` | MPICH 5.0.1, from source, in a `linux/386` container | not yet | `xfail-ci-mpich.txt` + `xfail-ci-mpich-i386.txt` |

**Each of those five runs as four jobs**, one per shard of the suite — **eighteen
legs**, not twenty: `rma` is excluded on the two Open MPI legs, for the measured and
upstream reason the sections below give, so those two run three shards each. The
shards are `coll`, `rma`, `threads+pt2pt+part`, and the complement of those three, and
they exist so that the slow legs can finish at all rather than for parallelism.
Measured per-directory cost is what picked them:

| shard | MPICH 5.0.1 | Open MPI (4.1.6, for shape) |
|---|---|---|
| `coll` | 9.8 min | 4.6 min |
| `rma` | 3.8 min | **37.1 min** |
| `p2p` = threads, pt2pt, part | 11.8 min | 19.3 min |
| `rest` = everything else | 0.6 min | 16.6 min |

`rma` is alone because over Open MPI it is half the suite by time — one directory
larger than the whole budget — so isolating it is what makes the other three safe
rather than what makes `rma` safe. If the `rma` shard is still killed, that is a
fact about one directory instead of about the row.

The last shard is `--skip-dirs=coll,rma,threads,pt2pt,part` rather than a fourth
list of names, so a directory a later suite release adds lands in it by
construction. `check-tap.py` needs no telling which shard it is looking at: it
derives the covered directories from the TAP itself, so a line for a directory
this shard skipped counts as "not run" rather than as a stale entry. Verified
against a real run before it was wired up — the four shards of the aarch64 TAP
gate at exit 0 individually and their failures sum to the same 41.

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

**Validation, and what two runs have established.** The four shards are an exact
partition of the suite, checked against an unsharded run of the same MPI and suite
version: 842 distinct tests and 1245 invocations either way, nothing missing,
nothing duplicated, and no two shards sharing a test — which also settles that
`testlist.dtp` follows the directory filter rather than escaping it, the one way
sharding could have quietly run tests twice or not at all. The 41-line MPICH list
has reproduced across three runs and all three architectures, i386 included, where
it held unchanged and ILP32 only *added* eleven lines. The 110-line Open MPI list
reproduced in full. Two entries filed as architecture differences turned out to be
intermittent tests and were removed when the second run passed them; a test that
flaps cannot be listed at all, since listing it fails the run it passes and not
listing it fails the run it fails.

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

## The Open MPI and i386 lists are empty because their legs cannot finish here

**Long jobs do not survive in this CI environment**, and the numbers say so
plainly. Run 32155423441, every job in it:

| duration | outcome |
|---|---|
| 0.4–1.2 min (15 jobs), 4.1 min (1) | success |
| 28.4 min — `suite / mpich / aarch64` | ran to completion |
| 44.2 min — `suite / mpich / x86_64` | ran to completion |
| 51.3 min — `suite / mpich / i386` | killed: "the runner has received a shutdown signal" |
| 58.0 min — `suite / openmpi / x86_64` | killed, the same, exit 143 |
| 75.1 min — `suite / openmpi / aarch64` | killed, the same |

All three kills landed inside `running the suite` with nothing in the job's own
log — no build error, no failing test, no message — which is what a runner losing
its host looks like from inside a job. The kills are not at one fixed duration
(51, 58, 75), so this reads as capacity being reclaimed rather than a per-job
timeout; either way nothing above about 45 minutes has finished here.

Worth reading beside those numbers: a job that builds MPICH 5.0.1 from source,
builds the wrapper, runs 13 `ctest` tests and three consumption routes finished
in **1.2 minutes** in the same run. That is not a duration a real `ubuntu-24.04`
runner produces for that work, so most of this environment's work is accelerated
while the suite legs — hundreds of actual MPI launches — are the only jobs
spending real time, and they are the only ones that hit the wall. On a stock
GitHub runner the job limit is six hours and these legs took 39 and 75 minutes,
so this is a property of where they ran and not of the suite.

That is what the sharding above is for, and two runs of it said how far it got:
**seventeen of the then-twenty legs completed and gated.** The three that did not were
`rma` on both Open MPI legs and `rest` on i386. Those two `rma` legs are no longer run
at all — the matrix excludes them — so the live count is seventeen of eighteen, with
`rest` on i386 the one outstanding leg. The rest of this section is the record of how
the `rma` question was answered; the answer itself is under "Round three".

**Isolating `rma` did what it was for — the answer is now about 176 tests rather
than about a row.** With runtests' output teed into the job log (a killed job
uploads no artifact, so stdout is the only record left), the shard's last line is

    Running tests in ./rma/testlist.dtp [176 tests - 00:01:36]

after the main rma testlist has run through to `win_dynamic_rma_flush_get_collattach`.
So it is the *datatype-pool* half of rma over Open MPI that takes the runner down,
not rma as such. `--no-dtp` on that shard would let the rest of the directory be
listed; it is not set, because the shard dimension is shared with the MPICH legs
whose rma shard runs those same dtp tests green. A shard of their own is the move
that costs nothing else.

**One caution the second run added:** `openmpi / aarch64 / p2p` completed in ten
minutes in the first run and was killed after fifty in the second, leaving orphan
`sleep` processes behind. Open MPI has a shard that hangs intermittently as well as
one that dies reproducibly, and a green Open MPI leg needs both settled.

### Debugging the rma dtp shard: what is known, and where to start

**Those tests declare how much memory they need, which is where this started and is
not where it ended** — the memory reading below is corrected two paragraphs on, and
the table is kept because it is what decides *which* of these tests run. The suite
generates its datatype-pool tests from
`test/mpi/maint/dtp-test-config.txt`, whose fourth field is passthrough attributes,
and the rma entries there carry `mem=`:

| test | declared | ranks |
|---|---|---|
| `epochtest`, `lockall_dt_flushlocal` | 5 GB | 4 |
| `putpscw1` | **4 GB** | 4 |
| `lockall_dt_flushlocalall` | 3 GB | 4 |
| `lock_contention_dt` | 2.1 GB | 4 |
| `getfence1`, `putfence1` | 2 GB | 2 |
| `accpscw1` | 1.7 GB | 4 |
| `accfence1` | 1.2 GB | 4 |
| `lockall_dt` | 1.1 GB | 4 |

`runtests` skips a test whose `mem=` exceeds its `memory_total`, **which defaults to
4** — `$g_opt{memory_total} = 4;` at line 72 of `test/mpi/runtests`, and the skip at
lines 539–540 of the same file. So the 5 GB tests are skipped and **everything from
1.1 GB up to 4 GB runs**. That is not inference: the MPICH rma shard's own TAP
carries `ok N - ./rma/epochtest 4 # SKIP xfail due to memory requirement`.

**Corrected: `mem=` is a whole-job figure, not a per-rank one.** `runtests` compares
it against `memory_total`, whose own comment is `# Total memory in GB`, and the
implicit-lock arithmetic beside it — `$test_opt->{mem} * $g_opt{memory_multiplier} >
$g_opt{memory_total}`, where `memory_multiplier` is "No of simultaneous jobs" —
multiplies by concurrent *jobs* and never by ranks. Measured, to settle it rather
than argue it: `putpscw1` with the heaviest of its four argument sets
(`-type=MPI_INT:4+MPI_DOUBLE:8 -count=262144 -testsize=16`, 4 ranks) plateaus at
**3.2 GB of sampled RSS summed over all four ranks** through the wrapper over MPICH
— sampled on a laptop over several minutes and stopped there rather than run to
completion, so read it as the plateau and not as a final figure, which is enough when
a per-rank reading of the annotation would predict about 16. So `mem=4`
means four gigabytes for the whole job, and the "4 GB × 4 ranks = 16 GB" reading that
made this the prime suspect does not hold: one test at a time against a 16 GB runner
has four times the headroom it needs.

**The second hypothesis — that it is ours — is refuted, statically and completely.**
Every RMA body in `gen/mpiwrapper/wrappers.c` is strict passthrough of the choice
buffer: `MPI_Put` converts five handles and an integer and hands `origin_addr`
straight to the target, which is what §3's argument-class table already promises — a
choice buffer emits "one test" for a sentinel and never a copy. Of the 58 allocation
and copy sites in 25,571 generated lines, **all eight `malloc`s are
`nstaged * sizeof *block`** in the six `alltoallw` variants, sized by communicator
size rather than by data. On the hand-written side the only allocations are
per-handle pairs in `extrastate.c` and one fixed `MPIWRAPPER_AUTOBUF_BYTES` (8 MB)
buffered-send pool. Nothing in this project's path scales with message size, so a
4 GB operation cannot become 8 GB here.

**What the evidence points at instead is the Open MPI version.** There is a
*completed* Linux Open MPI suite run on disk, in `build/linux-out/suite-openmpi/`:
1229 tests, 996 passed, and the rma directory ran all 383 of its tests including the
whole dtp list. `putpscw1` passed all twelve instances, in 0.08 s to 17.5 s, and the
only memory skips in rma are the two 5 GB tests — `epochtest` and
`lockall_dt_flushlocal`, four instances each — exactly what the table above predicts
at `memory_total=4`. The whole rma directory cost 523.9 s of test time. That run's
wrapper was built against `/usr/lib/aarch64-linux-gnu/openmpi`, which is the **4.1.6
Ubuntu 24.04 ships**, on the same architecture as one of the legs that now dies,
while CI's Open MPI legs build **5.0.10 from source** (`ci-scripts/install-openmpi.sh`,
`version=${2:-5.0.10}`). So the variable between "runs the dtp list green" and "takes
the runner down" is the implementation, not the memory and not this project.

**And there is a mechanism that fits exactly this directory and nothing else.**
Open MPI 5.0 **removed the `osc/pt2pt` one-sided component**: the 5.0.6 tree in
`build/mpi/ompi-src/ompi/mca/osc/` holds `rdma`, `sm`, `ucx`, `portals4` and
`monitoring` and no `pt2pt`, while the shipped changelogs still discuss `osc/pt2pt`
for 3.0.x and 3.1.x. A CI runner has no RDMA fabric, so every window these tests
create was served by `osc/pt2pt` under 4.1.6 and must be served by `osc/rdma` under
5.0.10 — a different code path, reached by the one directory that dies.

One candidate inside that path was checked and does *not* apply.
`allocate_state_shared` in `osc_rdma_component.c` sizes a single shared-memory
segment as the **sum over all local ranks' window sizes** and attaches all of it in
every rank, which is precisely "backs a window per rank rather than once per job" —
but the loop adding each rank's data is guarded by
`if (MPI_WIN_FLAVOR_ALLOCATE == module->flavor)`, and every rma dtp test uses
`MPI_Win_create`. Under `FLAVOR_CREATE` that segment carries only per-rank state and
the window data stays in the user's buffer. If `osc/rdma` is the mechanism it is not
by duplicating the data, and the next reader should not spend the afternoon there.

**So the experiment to run was a version and component bisect, not a memory test** —
and it has now been run; the results are in the section above, and all three of the
checks below fired as written. Kept as the record of what was asked and why:

- **`--dirs=rma` against 4.1.6 and against 5.0.10 in the same run.** If 4.1.6
  completes and 5.0.10 dies, the finding is an Open MPI regression and this project
  is not involved — worth establishing before anything else is measured.
- **`OMPI_MCA_osc=sm`, or `^rdma`, on the 5.0.10 leg.** One variable, and it names
  the component if the component is the cause.
- **`RUNTESTS_VERBOSE=1`.** Unchanged and still necessary: passing tests are not
  printed, so only this makes the last line before the kill *name* the test.

A local `docker run` remains the better instrument for the reason it always was — a
container's peak RSS and cgroup memory events prove an OOM where CI can only show the
corpse. One shortcut is closed, though: the local macOS Open MPI 5.0.6 in
`build/mpi/openmpi-native/` fails `MPI_Init` even for a two-rank hello world, dying in
`PMIx_Finalize` after a shared-memory segment error, so the comparison has to be made
on Linux. The watchdog was also cleared in passing: `prterun`'s ranks each sit in
their own process group, so `mpiexec-filter`'s `kill -TERM -$child` looked like it
would strand them, but measured on 5.0.6 both TERM and a hard KILL of the launcher's
group leave no survivors. The orphan `sleep` processes the second run left behind are
the watchdog's own sleeps, which hold no memory.

### What the probe measured: an Open MPI 5.x regression, quantified

Run [32284464458](https://github.com/eschnett/mpi_abi_wrapper/actions/runs/32284464458),
four cases, `--dirs=rma` on `ubuntu-24.04`. It reproduced the death in **14m50s** and
named the test.

| case | outcome | where |
|---|---|---|
| 5.0.10, as CI builds it | runner shutdown signal | `getfence1 -count=16000000`: **10.93 + 3.72 GB** |
| 5.0.10, `--mca btl self,sm` | runner shutdown signal | the same test: **10.71 + 3.88 GB** |
| 5.0.10, `MPITEST_MEMORY_TOTAL=1` | survived | that test skipped; low-water 1.2 GB |
| **4.1.8, the control** | **survived, `No Errors`** | the same test: **1.69 + 1.03 GB**, 105.5 s |

**The culprit is `rma/getfence1 -type=MPI_INT -count=16000000 -seed=171 -testsize=4`,
two ranks, `mem=3` — not `putpscw1`.** The README's prime suspect was wrong twice
over: wrong test, and wrong number of ranks. The monitor's last two samples are the
whole finding:

    [mon 18:09:58] avail=13.1G  top=[getfence1:0.87G getfence1:0.55G ...]
    [mon 18:09:59] ... ./getfence1 -type=MPI_INT -count=16000000 -seed=171 -testsize=4
    [mon 18:10:08] avail=0.6G   top=[getfence1:10.60G getfence1:3.40G ...]
    [mon 18:10:18] avail=0.2G   top=[getfence1:10.93G getfence1:3.72G ...]
    [18:10:53] The runner has received a shutdown signal.

**Same wrapper, same test, same runner image, 5.4× the memory.** 4.1.8 peaks at about
2.7 GB summed over its two ranks and 5.0.10 at about 14.6 GB, on a 16 GB runner. On
the origin rank alone it is 1.69 GB against 10.93 GB, a factor of 6.5. And the
*shapes* differ, which is the mechanism showing through: 4.1.8's two ranks oscillate
between 1.0 and 1.7 GB for a hundred seconds — it streams the transfer — while
5.0.10 climbs monotonically to 10.9 GB in under twenty seconds and never comes back
down. That is buffering a whole transfer where the older component pipelined it.

The component lists confirm the story exactly, per run rather than from a laptop:

| | `osc` | `btl` |
|---|---|---|
| 4.1.8 | monitoring, **pt2pt**, rdma, sm | self, tcp, **vader** |
| 5.0.10 | monitoring, rdma, sm | self, sm, tcp |

**Three things this settles.**

- **It is not this project.** The wrapper build is identical in both rows; only the
  wrapped implementation differs, and only the newer one dies. The static refutation
  above said the wrapper cannot allocate in proportion to the data; the measurement
  says something in Open MPI 5.0.10's path does.
- **`--mca btl self,sm` does not help, and that was predicted.** Taking TCP out
  changes nothing because `btl/sm` fails `check_accelerated_btl` just as `tcp` does,
  so the transfer is still `osc/rdma`'s active-message emulation either way. This is
  the measurement that closes the "shared memory should be enough" question: it was
  tried, and it is not.
- **Window *creation* is not where the memory goes.** Four ranks each creating a
  256 MB `MPI_Win_create` window cost 13.6 MB of peak RSS in the same job, so
  `allocate_state_shared`'s sum-over-local-ranks segment really is state-only under
  `FLAVOR_CREATE`, as the source said. The blowup is in the data path.

**Where this leaves the memory story, honestly.** The proximate cause *is* memory
exhaustion, so the section above is too dismissive of it — but the reasoning it
replaced was still wrong in both of its steps. `mem=` is a whole-job figure, and
4.1.8's ~2.7 GB against its `mem=3` annotation confirms that the annotation is
well-calibrated and read correctly. What defeats `memory_total=4` is not the
arithmetic: it is that 5.0.10 spends **five times what the test declares**, so a guard
calibrated on any streaming implementation cannot hold it. Raising `memory_total`
would make things worse, and lowering it only hides the test.

**What to do about it, in order of how much it costs.**

1. **Done: the two `getfence1` lines are excluded, and `--xfail` could not have done
   it.** A killed runner produces no TAP line, so there is nothing for an
   expected-failure list to match — which is why `run-suite.sh` gained `--exclude` and
   `ci-scripts/suite/exclude-ci-openmpi.txt`, applied to the Open MPI legs only. The
   list drops test *lines* from the generated testlists before runtests sees them, and
   it is deliberately narrower than the alternative: `MPITEST_MEMORY_TOTAL=2` would
   also drop `putpscw1`, `lock_contention_dt` and `lockall_dt_flushlocalall`, all of
   which the probe watched complete, *and* would still run `putfence1`'s
   `-count=16000000` line at `mem=2`.

   **That last point is the open one.** The probe proves only that `getfence1` dies
   *first*; `putfence1` carries the same `-count=16000000 -testsize=4` shape two
   hundred lines later and the baseline run never reached it. The probe's fifth case,
   `excluded-5.0.10`, exists to answer exactly that — if it dies in `putfence1`, the
   list needs a second entry, measured rather than guessed.
2. ~~**`--with-ofi` or `--with-ucx`** would give `MPI_Win_create` an accelerated path
   and is the fix rather than the workaround.~~ **Tested in round three and refuted.**
   `btl/ofi` built and in one-sided mode changed nothing, because the amplification is
   per datatype element and happens above the transport. Left here struck through
   rather than deleted, because it is the inference the accelerated-BTL analysis
   invites and the next reader will have it too.
3. **Upstream.** A 5.4× memory regression against 4.1.x on a stock `MPI_Get` with a
   large derived datatype, reproducible on a two-rank single-node job with no fabric,
   is worth an Open MPI issue on its own account. The probe run above is a complete
   reproducer.

### Round two: the exclusion does not hold, and the reason changes the conclusion

Run [32291327971](https://github.com/eschnett/mpi_abi_wrapper/actions/runs/32291327971)
verified the mitigation and refuted it.

| case | outcome | where |
|---|---|---|
| 5.0.10 + the getfence1 exclusion | **runner shutdown** | `putfence1 -count=16000000`: 8.09 + 6.79 GB |
| 5.0.10, `MPITEST_MEMORY_TOTAL=1` | **runner shutdown** | `rma/transpose7`: 10.73 + 3.93 GB |
| 5.0.10, `--mca btl self,sm` | runner shutdown | as before |
| 4.1.8 | survived, `No Errors` | as before |

**The exclusion worked and did not help.** Two lines were dropped, `getfence1` never
ran, and the shard got twenty-six minutes further before `putfence1` — same
`-count=16000000 -testsize=4` shape, `mem=2` rather than `mem=3` — died the same way.
That much was predicted and is now measured.

**What was not predicted is `transpose7`, and it is the finding that matters.** It is
an entry in the *main* `rma/testlist`, not in `testlist.dtp`. It reads `transpose7 2`
— no `mem=` annotation, no `-count`, no `-testsize`, nothing to suggest it is
expensive. It reached **10.73 GB on one rank** and took the runner down. In the
previous run the same test survived with 1.2 GB to spare, which is why this shard is
*flaky* as well as failing: `transpose7` sits right at the edge of a 16 GB runner and
falls off it depending on what else the image is doing.

Three conclusions follow, and they replace the mitigation advice above.

- **The amplification is a property of the rma directory over 5.0.10, not of the
  datatype-pool tests.** `transpose7` is neither a dtp test nor an annotated one. So
  `--no-dtp` would not have saved this shard either, and neither would any
  `MPITEST_MEMORY_TOTAL`: the suite's own memory annotations cannot describe a cost
  the implementation invents.
- **No list of test-line patterns converges.** Each exclusion reveals the next
  casualty, and the casualties include tests nobody would have flagged by reading the
  testlist. `exclude-ci-openmpi.txt` keeps its two measured patterns because they buy
  the shard twenty-six minutes, and its header now says plainly that it is not a fix.
- **So the fix has to be the build or upstream.** Which makes the `ofi` probe case the
  whole of the remaining question: `btl/ofi` is the one BTL a fabric-less runner can
  have that passes `ompi_osc_rdma_check_accelerated_btl`, Open MPI's configure detects
  libfabric on its own, and if that removes the emulation then every one of these
  tests stops being expensive and no exclusion is needed at all. That round runs
  `stock` and `ofi` **side by side in one run**, because `transpose7` proved the
  margin is real and a green `ofi` row means nothing without a `stock` row dying
  beside it.

### Round three: libfabric does not fix it, and the real cost is per *element*

Run [32297365052](https://github.com/eschnett/mpi_abi_wrapper/actions/runs/32297365052),
`stock` and `ofi` side by side, same image, same hour.

| case | `btl` list | died in | peak RSS |
|---|---|---|---|
| `stock-5.0.10` | self, sm, tcp | `getfence1 -count=16000000` | 10.62 + 3.98 GB |
| `ofi-5.0.10` | self, sm, tcp, **ofi** | **`rma/transpose1`** | 8.02 + 6.61 GB |

**`btl/ofi` really was built and really was in one-sided mode.** `ompi_info` lists it,
and `mca_btl_ofi_component.mode` defaults to `MCA_BTL_OFI_MODE_ONE_SIDED`
(`btl_ofi_component.c:143`), whose `required_caps` is `FI_RMA | FI_ATOMIC` — the
capability set the accelerated test wants. So this is not a case of the option being
silently ignored. **It did not help, and the `ofi` row died *earlier* in the directory
than the `stock` row beside it.** Attributing "worse" to libfabric would be
over-reading a margin this noisy, but "no improvement" is not an over-reading: the row
that was supposed to be rescued was not.

**And `transpose1` is what explains all of it.** The test transposes a 1000×1000 matrix
of `int` — **3.8 MB of payload** — with

```c
MPI_Type_vector(1000, 1, 1000, MPI_INT, &column);
MPI_Type_create_hvector(1000, 1, sizeof(int), column, &xpose);
MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, CommDeuce, &win);   /* rank 0: empty window */
MPI_Put(&A[0][0], 1000*1000, MPI_INT, 1, 0, 1, xpose, win);
```

The origin buffer is contiguous. The *target* datatype is an hvector of vectors
describing **one million discontiguous single-`int` elements**. Open MPI 5.0.10 spends
at least **14.6 GB** moving those 3.8 MB, which is upwards of **3900×** the payload —
a lower bound, because the host died rather than the allocation completing.

**So the cost tracks the element count of the derived datatype, not the byte count**,
and that single sentence accounts for every observation in this section:

- **Why the whole directory is unsafe, not the dtp half.** `transpose1` and
  `transpose7` are plain entries in the main `rma/testlist` with no `mem=` and no
  arguments. What makes them expensive is the *shape* of their datatype, which no
  annotation describes.
- **Why the suite's memory guard cannot help.** `mem=` and `memory_total` are
  denominated in the data a test moves. This cost is denominated in how many pieces it
  is moved in, and 3.8 MB in a million pieces looks free to the guard.
- **Why `getfence1 -count=16000000` was the first casualty.** Sixteen million elements,
  and it happens to be the first test in the dtp list at that element count.
- **Why 4.1.8 is unaffected.** `osc/pt2pt` handed the datatype to the point-to-point
  engine, which packs and streams it; `osc/rdma` appears to build per-element state
  instead.
- **Why no transport option fixes it.** The per-element state is allocated above the
  BTL, so `--mca btl self,sm`, `libfabric`, and by extension UCX, are all interventions
  at the wrong layer. This retires the "missing configure option" answer: it was a
  reasonable inference from the accelerated-BTL requirement, and it is wrong.

**What remains.** Nothing at this project's disposal fixes this, and the exclusion
mechanism cannot chase it — the casualties are selected by datatype shape, which is not
visible in a testlist line. The honest options are to stop running `rma` on the Open
MPI legs until upstream moves, or to keep the shard as a known, report-only death. The
finding itself is worth carrying upstream: **a 3.8 MB `MPI_Put` whose target datatype
has a million elements exhausts a 16 GB host on a two-rank single-node job, where
Open MPI 4.1.8 completes it**, and `rma/transpose1` from MPICH's test suite is a
twenty-line reproducer.

### Why `MPI_Win_create` has nowhere to go in Open MPI 5.x

The `osc/pt2pt` removal is not an inference from a missing directory. Open MPI's own
5.0.x changelog, shipped in the tarball this project builds
(`docs/html/_sources/release-notes/changelog/v5.0.x.rst.txt`), says it under
"Transport updates and improvements → One-sided Communication":

> Removed the legacy `pt2pt` one-sided component. Users should now utilize the
> `rdma` one-sided component instead. The `rdma` component will use BTL components
> — such as the TCP BTL — to effect one-sided communications.

Two entries above it, in the same list:

> Many MPI one-sided and RDMA emulation fixes for the `tcp` BTL. This patch series
> fixs many issues when running with `--mca osc rdma --mca btl tcp`, i.e., TCP
> support for one sided MPI calls.

So the replacement path was *new enough in 5.0 to need a patch series of its own*,
and it is the path a runner with no fabric is left with. What makes that inescapable
rather than merely likely is three facts read out of the 5.0.6 tree in
`build/mpi/ompi-src/`, each of which closes one alternative:

- **`osc/sm` declines these windows outright.** `component_query` in
  `ompi/mca/osc/sm/osc_sm_component.c` returns `-1` unless the flavor is
  `MPI_WIN_FLAVOR_SHARED` or `MPI_WIN_FLAVOR_ALLOCATE`. Every rma dtp test uses
  `MPI_Win_create`, which is `MPI_WIN_FLAVOR_CREATE`. Being single-node does not
  help: the flavor is checked before locality is.
- **`osc/ucx` and `osc/portals4` are not built.** A stock configure with no
  `--with-ucx` and no UCX headers on the runner leaves them out.
- **So `osc/rdma` is the only component left**, and inside it `btl/sm` does not
  qualify for the fast path. `ompi_osc_rdma_check_accelerated_btl` requires four
  things — `MCA_BTL_FLAGS_RDMA`, `MCA_BTL_FLAGS_ATOMIC_FOPS`,
  `MCA_BTL_FLAGS_RDMA_REMOTE_COMPLETION` and `MCA_BTL_ATOMIC_SUPPORTS_ADD` — and
  `opal/mca/btl/sm/` sets **only the first**, and only when an `smsc` single-copy
  component is present (`btl_sm_component.c`, `mca_btl_sm.super.btl_flags |=
  MCA_BTL_FLAGS_RDMA`). It never sets an atomic flag or a remote-completion flag
  anywhere in the component. `btl/tcp` does not qualify either.

The consequence is that `ompi_osc_rdma_query_accelerated_btls` fails and
`ompi_osc_rdma_query_alternate_btls` takes over, wrapping whichever BTLs exist in
`opal_btl_base_am_rdma_create` — **active-message RDMA emulation**, sorted by BTL
latency so shared memory is preferred over TCP but neither is native. Every `MPI_Put`
in the directory becomes emulated one-sided traffic over a path that, in 4.1.6, was
`osc/pt2pt`.

**This is the answer to "RDMA should work with shared memory".** It is a reasonable
expectation and it is not what 5.x does: `btl/sm` advertises RDMA *put and get* and
osc/rdma's fast path additionally demands *remote-completion notification and
fetching atomics*, which shared memory does not advertise even though a CPU could
obviously perform them. `vader` is the 4.x name for the same component and changes
nothing here — it was renamed to `sm` in 5.0.

**What follows for the knobs.** There is no runtime setting that restores a native
one-sided path for `MPI_Win_create` on a machine with no RDMA fabric, because the
component that used to serve it no longer exists. What is worth setting is narrower:

- `--mca btl self,sm` keeps TCP out of the emulation entirely. osc/rdma already
  sorts by latency, so this should be a no-op — and if it is not, the tcp BTL's
  emulation is implicated and the changelog above says why that is plausible.
- `--mca osc_base_verbose 100` names the component that served each window, which
  turns all of the above from a reading of source into a line in a log.
- `MPITEST_MEMORY_TOTAL=1` remains the way to close the memory story rather than
  open it.

**And for the build (`ci-scripts/install-openmpi.sh`).** Nothing is missing in the
sense of a subtree: the installer takes the official
`openmpi-<version>.tar.bz2`, which bundles PMIx, PRRTE, libevent and hwloc, so there
is no `--recursive` clone to get wrong. The stock configure has two consequences
worth naming rather than one:

- **No BTL this build can produce passes the accelerated test, and that is the
  configure answer.** Grepping the five relevant components in
  `opal/mca/btl/` for the four flags `ompi_osc_rdma_check_accelerated_btl` wants:

  | BTL | RDMA | ATOMIC_FOPS | REMOTE_COMPLETION | SUPPORTS_ADD | accelerated? |
  |---|---|---|---|---|---|
  | `sm` | yes¹ | — | — | — | no |
  | `tcp` | — | — | — | — | no |
  | `self` | yes | — | yes | — | no |
  | `ofi` | yes | yes | yes | yes | **yes**² |
  | `uct` | yes | yes | yes | yes | **yes**³ |

  ¹ only when an `smsc` single-copy component is present.
  ² `btl/ofi` needs libfabric, and instantiates only for a provider advertising
  `FI_RMA | FI_ATOMIC` (`MCA_BTL_OFI_ONE_SIDED_REQUIRED_CAPS`) plus
  `FI_DELIVERY_COMPLETE`; libfabric's `shm` and `tcp` providers do.
  ³ `btl/uct` needs UCX.

  A stock configure with neither libfabric nor UCX present leaves exactly `self`,
  `sm` and `tcp` — so osc/rdma running in emulation is not bad luck on this runner,
  it is the only thing that can happen. That made `--with-ofi` or `--with-ucx` look
  like the configure-time answer, and **round three above measured it and it is not**:
  `btl/ofi` was built, in one-sided mode, and the shard still died. The reason is in
  that section — the cost is per datatype *element* and is incurred above the BTL, so
  no transport option reaches it. The table stands as the account of why emulation is
  unavoidable here; it is just not where the memory goes.
- **A stock configure is not hermetic.** It picks up whatever dev packages the
  runner image happens to carry, so the component set is a property of the image as
  much as of the version. That is why the probe printed `ompi_info`'s component list
  and configure line per run: the alternative is assuming the laptop's component set is
  CI's.

### Hand-off

Everything needed is on disk. **`ci-scripts/suite/run-suite.sh`** is the runner —
`--dirs`, `--skip-dirs`, `--xfail`, the teed runtests invocation and the environment
it exports; **`ci-scripts/suite/mpiexec-filter`** is the launcher the suite actually
gets, and its header explains `--oversubscribe` and the watchdog;
**`ci-scripts/suite/xfail-ci-openmpi.txt`** holds the 110 expectations and states the
rma gap in its own header; **`.github/workflows/ci.yaml`**'s `suite` job holds the
shard matrix and is where a per-leg environment variable would go;
**`ci-scripts/install-openmpi.sh`** builds the 5.0.10 being wrapped;
**`ci-scripts/run-linux-docker.sh`** and **`ci-scripts/suite/i386-suite.sh`** are the
local-container route. On the suite's own side, unpack the pinned tarball and read
`test/mpi/maint/dtp-test-config.txt` for the annotations and `test/mpi/runtests` for
lines 72, 539 and 183; the generated per-directory list is `rma/testlist.dtp` in the
configured build tree, which the 5.0.1 run reports as 176 tests (a locally configured
4.3.1 tree has 184 lines, 40 of them annotated, which is the same shape and not the
same numbers). For the "is it ours" half, the RMA bodies are in
`gen/mpiwrapper/wrappers.c` and anything that stages lives in `src/mpiwrapper/`;
`NOTES.md` #3 lists which argument classes stage a temporary at all. The runs this
section is written from are 32182485327 and 32189118611, whose artifacts carry every
shard's `summary.tap` and logs.

**The probe that measured all of the above has been deleted with the branch it lived
on**, `.github/workflows/rma-dtp-probe.yaml` on `investigate-rma-dtp-openmpi5`. It was
`workflow_dispatch`-plus-branch-`push` and deliberately disposable; what it produced is
the three rounds above, and the runs keep the evidence:
[32284464458](https://github.com/eschnett/mpi_abi_wrapper/actions/runs/32284464458),
[32291327971](https://github.com/eschnett/mpi_abi_wrapper/actions/runs/32291327971) and
[32297365052](https://github.com/eschnett/mpi_abi_wrapper/actions/runs/32297365052).

**Four things in it are worth rebuilding rather than reinventing, if this ever needs
probing again.**

- **Everything on stdout, and no step depending on the collect step.** The failure
  kills the runner, and a killed job keeps its log while losing its artifacts. Its
  resource monitor therefore printed memory, `/dev/shm` and the top RSS consumers every
  ten seconds into the job log — which is the only instrument that reports from the
  wrong side of an OOM, and the thing that turned "the rma shard dies" into "`transpose1`
  reached 8.02 GB at 20:31:02".
- **`RUNTESTS_VERBOSE=1`.** runtests prints nothing for a passing test, so without it
  the last line before a kill is a directory header rather than a test name.
- **`ompi_info`'s component list and configure line, per run.** `install-openmpi.sh`
  runs a stock configure, so the component set is partly a property of whatever dev
  packages the runner image carries. Printing it is the difference between measuring
  `btl/ofi` was built and assuming it.
- **A same-run control.** `transpose7` survived with 1.2 GB spare in one run and killed
  the runner in the next, so any claim of the form "configuration X fixes it" needs the
  unfixed configuration dying beside it, on the same image and the same hour. The
  libfabric round was structured that way and it is what made "no improvement"
  reportable rather than arguable.

One trap it paid for: `MPITEST_MEMORY_TOTAL` must be left *unset* rather than set to
the empty string. runtests reads it with `defined($ENV{...})`, an empty string is
defined, and `''` compares numerically as 0 — so a matrix that spells the default case
as `MPITEST_MEMORY_TOTAL=""` skips every `mem=`-annotated test in the directory and runs
none of what it exists to run.

### The i386 `rest` shard: not ILP32, a 64 MB `/dev/shm`

**The row still dies the same way, and it has got harder to instrument.** In run
[32425661806](https://github.com/eschnett/mpi_abi_wrapper/actions/runs/32425661806)
job `suite / mpich 5.0.1 / i386 / rest` ended at 50 min 01 s with "The hosted runner
lost communication with the server", the same 49–52 minute wall this section opens
with and the same one `.github/workflows/ci.yaml` records five consecutive attempts
of. **What is new is that the log is gone too.**
`gh run view --job 96606969905 --log` answers `log not found`, and the run's own log
archive contains no entry for that job at all, while every other job in the run is
there. So the instrument that answered the rma question — everything on stdout, because
a killed job keeps its log even when it loses its artifacts — does not reach this
failure. A probe of this row has to *end on its own terms*: a smaller unit of work, or
a self-imposed timeout below the wall.

**One run did leave a log, because it was cancelled rather than killed**: job
96590527783 of run
[32420200083](https://github.com/eschnett/mpi_abi_wrapper/actions/runs/32420200083),
superseded by a newer push 24 minutes in. It reached the `session` directory, and it
says something no list on this branch records yet.

| | i386 | x86_64, same shard, same run |
|---|---|---|
| failures | **172** (160 distinct programs) | 33, every one listed |
| of those, `EXIT STRING: Bus error (signal 7)` | **139** | 0 |
| `io` | **18.2 minutes**, 14.7 of them the window the runner was reclaimed in | 4 seconds |
| everything but `io` | 2.5 minutes | 55 seconds |

The failures are not confined to anything: `info` 12 of 12, `io` 31 of 31, `datatype`
29 of 71, `errors/*` almost entirely, and `comm`'s 46 tests clean. The first SIGBUS
lands 13 seconds into `datatype`, after `attr` and `comm` have run 71 tests without
one. So the ILP32 delta list, which has eleven lines, *looked* short by something like
140 — and the next paragraph is why it is not short by any.

**That log also caught the reclaim happening**, which is worth separating from the
stalls it gets blamed on. The 882 seconds are not one test running long: inside them
`external32_derived_dtype` hit hydra's own 180-second limit, hydra then failed to kill
it — `unable to send signal downstream`, `sock write error` — and
"The runner has received a shutdown signal" appears seconds later, with the job
cancelled 15 seconds after that. So that window is the environment coming apart rather
than a measurement of the test, and the only stall in it measured against a working
launcher is `simple_collective`'s 147 seconds.

**It is not about 32 bits. It is one missing flag.** i386 is the only suite row that
runs inside `docker run` — a 32-bit userspace cannot host the runner's own x86_64 node
actions, which is why `ci-scripts/suite/i386-suite.sh` exists — and
`ci-scripts/run-linux-docker.sh` passed no `--shm-size`, so that container got Docker's
default `/dev/shm`: **64 MB**, `size=65536k` in the mount line. The other four rows run
directly on the runner, whose own `/dev/shm` is **7.9 GB**. MPICH maps its shared
segments in that tmpfs, and touching a page the tmpfs cannot back is SIGBUS — not the
signal x86 raises for the misaligned access ILP32 would otherwise be suspected of.

`.github/workflows/i386-shm-probe.yaml` and `ci-scripts/suite/i386-shm-probe.sh`
measured it: the same shard twice, one flag apart, same image, same minutes, `io` left
out because 18 of the shard's 20 minutes are in that one directory while the storm
starts before it. Run
[32431588989](https://github.com/eschnett/mpi_abi_wrapper/actions/runs/32431588989):

| | default (64 MB) | `--shm-size=8g` |
|---|---|---|
| the job | **did not finish** — 40-minute limit, inside `docker run` | **3 min 13 s**, 44 s of it testing |
| tests | no result | 283: 248 passed, **33 failed** |
| unlisted failures — what the gate rejects | no result | **0** |
| SIGBUS | no result | **0** |
| the gate | no result | **"suite matches its expected-failure list"** |

**So there is no ILP32 delta here at all.** With room in `/dev/shm`, 32-bit x86
reproduces the 41-line shared expected-failure list exactly — 33 of its lines fired,
nothing else failed — in 44 seconds against x86_64's 55, and
`xfail-ci-mpich-i386.txt` needs not one line for any of the ~140 failures that were
about to be written into it as properties of ILP32.

**The mechanism is a leak, and the clean leg is what shows it.** During those 44
seconds `/dev/shm` went 3 → 22 → 24 entries and 4.1 → 36 → 39 MB, and all 24
`mpich_shm_*_0` files were **still there** when the suite finished. Nothing reclaims
them. So the tmpfs fills as the shard runs, and against a 64 MB cap it is a question of
when rather than whether — which is the shape of "`attr` and `comm` run 71 tests clean
and then everything fails".

**Not ours.** In the leg with room, `info/infotest` and `datatype/pairtype_size_extent`
both pass standalone under gdb and under `mpiexec -n 2`, through the wrapper *and* built
against MPICH's own `mpicc`. The wrapper is not in this failure anywhere.

**What this does not settle, stated so the green leg is not over-read.**

- **Where the control leg's 40 minutes went is unknown, because its log is gone.** A job
  that exceeds its timeout loses its log here exactly as a reclaimed one does —
  `gh run view --job 96624103650 --log` answers "log not found" — so what is established
  is that the identical script finished in 3m13s with 8 GB and had not finished in 40
  minutes with 64 MB, not the shape of the difference in between. That lesson is now in
  the probe: it imposes a `timeout` of its own inside the container, so the step
  completes and the log is kept.
- **39 MB is less than 64 MB.** The leak alone does not arithmetically prove the cap was
  crossed in the CI runs; what is measured is a monotonic leak with no reclaim, a 64 MB
  cap, and a storm that disappears when the cap is raised. A failing test may well leak
  more than a passing one, and the shard as CI runs it has 314 tests to this leg's 283.
  The exact crossing point was not measured.
- **`io` was in neither leg**, so `simple_collective`'s 147 seconds and whatever
  `external32_derived_dtype` really costs are untouched by this. The probe has an `io-8g` leg for exactly that question, and the two
  tests it blames are the two that stalled.
- **The runner death is not explained by a 64 MB tmpfs**, which cannot starve a 16 GB
  runner. What the shm answer does change is the premise: a shard that spends its time
  crashing and leaking is not the same shard as one that finishes in 44 seconds, so the
  death has to be re-measured after the flag lands rather than reasoned about from the
  runs before it.

**A separate red row in the same run, noted here because its cause is one line of
drift.** `suite / mpich 5.0.1 / i386 / coll` fails the gate for the opposite reason —
`EXPECTED FAILURE THAT PASSED coll/reduce 10` — and i386 is the one MPICH leg that does
*not* set `MPITEST_TIMEOUT_MULTIPLIER: 2`. That variable exists because `coll/reduce 10`
landed at 179.7 s of a 180 s limit on these runners, which is also the likeliest reason
the line was ever recorded. Giving the row the multiplier the other MPICH rows have is
the probable fix and is deliberately *not* done yet: it would double every limit in the
shard whose failure mode under investigation is a job that runs too long.

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
