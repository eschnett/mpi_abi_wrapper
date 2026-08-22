# `dev/suite-timings/`

Where a suite shard's wall clock went, read out of its own TAP file. `runtests`
writes `# time=<seconds>` on every line it produces, so a shard decomposes exactly,
with nothing to instrument and nothing to re-run.

```sh
gh run download 32586710591 -n suite-openmpi-x86_64-p2p -D /tmp/p2p
dev/suite-timings/tap-timings.py /tmp/p2p/summary.tap
```

**What it is for is the share spent at the time limit.** A test that hangs costs
`runtests`' whole limit every run — 180 seconds by default — and nothing in the job
log distinguishes that from 180 seconds of work. A shard can be almost entirely made
of hangs and still read as a slow shard.

## The measurement that produced `timelimit-ci-openmpi.txt`

Run 32586710591, the last complete run before the caps, 48m47s wall and 256 runner
minutes. The critical path was `suite / openmpi 5.0.10 / {x86_64,aarch64} / p2p` at
47 minutes, of which ~1 was fixed cost and 46.3 the test loop:

| shard | tests | wall | at the 180 s limit |
|---|---|---|---|
| openmpi p2p, both arches | 300 | 46.3 min | **39.1 min — 13 tests, all failing** |
| openmpi aarch64 rest | 323 | 13.0 min | **12.0 min — 4 tests** |
| mpich coll, x86_64 and i386 | 220 | 12.5 min | 3.0 min — 1 test (`coll/reduce` np=10) |
| mpich p2p | 300 | 19.0 min | none |
| mpich rma | 375 | 9.3 min | none |

**83% of the whole workflow's critical path was seventeen tests hanging**, every one
of them already a line in `xfail-ci-openmpi.txt`. The same seventeen run in
0.04–0.19 s on the MPICH legs. Capping them at 30 s takes the `p2p` shard to about
13.8 minutes and the workflow to roughly 21, where the ceiling becomes the MPICH
`p2p` shard's 20.0 minutes of real work.

The tool also prints the slowest test that *passed*, which is the floor any cap has
to clear: 26.5 s in `p2p`, 10.3 s in `rest`, but **84.4 s in `coll`**
(`bcast_comm_world_only`). That last number is why the cap is per-line rather than a
cut to `MPITEST_TIMEOUT_MULTIPLIER`, which is global.

## Two things measured about the mechanism, not argued

**The cap reaches `runtests`.** `threads/coll/allred` takes 1.29 s on the
development laptop; capped at 1 s it was killed at 1.10 s and its TAP entry read
`Timeout: 1`, while the uncapped `iallred` beside it finished normally at 1.08 s.
Its output was `No Errors` — a too-tight cap kills a *passing* test and reports it
as a failure, which is what the 30-second choice buys headroom against.

**Per-test launcher overhead is not the problem, so `MPITEST_RUN_INDIVIDUAL=1` need
not be reopened.** The cheapest Open MPI tests in the same run complete in 0.085 s
wall *including* the launch, so one MPI job per test costs under a minute across 300
tests. `NOTES.md` #6.2's three reasons for it stand at no meaningful wall-clock
price.

**And `runtests` has no parallelism to turn on.** `RunTests` is a serial `foreach`;
`memory_multiplier`, whose comment reads "No of simultaneous jobs", is only consulted
to skip tests whose `mem=` annotation exceeds `memory_total`. Sharding is the only
parallelism available, which is what `.github/workflows/ci.yaml` already does.
