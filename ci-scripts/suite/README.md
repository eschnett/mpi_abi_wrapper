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

**Each of those five runs as four jobs**, one per shard of the suite — twenty legs
in all. The shards are `coll`, `rma`, `threads+pt2pt+part`, and the complement of
those three, and they exist so that the slow legs can finish at all rather than
for parallelism. Measured per-directory cost is what picked them:

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
it held unchanged and ILP32 only *added* eleven lines. The 109-line Open MPI list
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

That is what the sharding above is for, and two runs of it have said how far it
gets: **seventeen of the twenty legs complete and gate.** The three that do not are
`rma` on both Open MPI legs and `rest` on i386.

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

**So the experiment to run is a version and component bisect, not a memory test.**
`MPITEST_MEMORY_TOTAL=1` is still plumbed and still cheap, but it now tests a story
the evidence disfavours; run it to close the question rather than to open it. The
three that matter, in order:

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
  it is the only thing that can happen. **`--with-ofi` (libfabric) or `--with-ucx`
  is therefore the configure-time answer**, and either would give `MPI_Win_create` a
  native path with no special hardware, on shared memory, which is what the
  shared-memory intuition was reaching for. Both mean a new dev-package dependency
  and a different thing under test, so this is a decision rather than an oversight —
  and worth taking only if the probe shows the emulation is the cause.
- **A stock configure is not hermetic.** It picks up whatever dev packages the
  runner image happens to carry, so the component set is a property of the image as
  much as of the version. That is why the probe workflow prints `ompi_info`'s
  component list and configure line per run: the alternative is assuming the laptop's
  component set is CI's.

### Hand-off

Everything needed is on disk. **`ci-scripts/suite/run-suite.sh`** is the runner —
`--dirs`, `--skip-dirs`, `--xfail`, the teed runtests invocation and the environment
it exports; **`ci-scripts/suite/mpiexec-filter`** is the launcher the suite actually
gets, and its header explains `--oversubscribe` and the watchdog;
**`ci-scripts/suite/xfail-ci-openmpi.txt`** holds the 109 expectations and states the
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

**The probe that measures all of the above is
`.github/workflows/rma-dtp-probe.yaml`**, on the `investigate-rma-dtp-openmpi5`
branch. It is `workflow_dispatch`-only and deliberately disposable: four cases over
`--dirs=rma` — 5.0.10 as it stands, 5.0.10 with `--mca btl self,sm`, 5.0.10 with
`MPITEST_MEMORY_TOTAL=1`, and 4.1.8 as the control that still has `osc/pt2pt`. Three
things in it are worth keeping if it is ever rewritten. It prints `ompi_info`'s
component list and configure line per run, because a stock configure inherits the
runner image's dev packages and the component set is therefore a measurement rather
than a constant. It runs a four-rank `MPI_Win_create` under `--mca
osc_base_verbose 100` *before* the suite, so the component that serves these windows
is named in the log even for a case that dies later. And its resource monitor writes
to **stdout** every ten seconds rather than to a file, because the failure being
chased kills the runner and a killed job keeps its log while losing its artifacts —
which is the same reason the run step must not depend on the collect step.

One trap that workflow already paid for: `MPITEST_MEMORY_TOTAL` must be left *unset*
rather than set to the empty string. runtests reads it with `defined($ENV{...})`, an
empty string is defined, and `''` compares numerically as 0 — so a matrix that spells
the default case as `MPITEST_MEMORY_TOTAL=""` skips every `mem=`-annotated test in
the directory and runs none of what it exists to run.

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
