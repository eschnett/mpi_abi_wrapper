# `dev/abort-exit-status/`

`errors/comm/intercomm_abort 6` flaps on the `mpich .. rest` CI shards, and the
question a flaky line has to answer first is whose flap it is. **It is the
launcher's, and the wrapper is not in that path.** `ci-scripts/suite/flaky-ci-mpich.txt`
carries the verdict and the run IDs; this is the measurement behind it.

```sh
dev/abort-exit-status/run.sh build/suite-mpich/prefix/bin/mpicc \
                             /path/to/impl/bin/mpicc [iterations]
```

The suite source is taken from `$MPIABI_SUITE_SRC` (default `build/suite-src`),
so `ci-scripts/suite/run-suite.sh mpich` has to have run once. On the development
laptop, put `scripts/host-env.sh` in front of it as usual, and note that the conda
MPICH's `mpicc` names a compiler that is not installed -- the control build passes
`-cc=clang` for that reason, overridable with `MPIABI_PROBE_CC`.

## What the test asks for, and which part of it fails

Rank 0 calls `MPI_Abort` on an intercommunicator; under hydra's
`-disable-auto-cleanup` the surviving ranks are meant to keep running, and rank 2
prints `No Errors` when it does. `errors/comm/testlist` grades that with
`resultTest=TestStatusNoErrors`, which `runtests` implements as **three** separate
demands rather than one:

| | demand | on a red CI run |
|---|---|---|
| 1 | `No Errors` appears in the output | met |
| 2 | no stray output | met |
| 3 | **mpiexec exits nonzero** | **not met** |

So the abort works and the survivors survive; what varies is whether hydra reports
a nonzero status for a job whose rank 0 aborted. Failing only #3 is what produces
runtests' single line, `mpiexec-filter returned a zero status but the program
intercomm_abort returned a nonzero status`, and that line is the entire failure --
in particular it is **not** the segfault it sits next to in the job log, which
belongs to the neighbouring `*_nullarg` xfails (`NOTES.md` #13.3).

## The control, and what it shows

The probe builds the same source twice -- once with the wrapper's `mpicc`, once
with the wrapped implementation's own -- and runs both the same two ways. On this
laptop, with hydra 4.3.1:

| | raw launcher, 30 runs | runtests |
|---|---|---|
| wrapper build | exit 0, 30/30 | fails, demand #3 |
| **native `mpicc` build** | **exit 0, 30/30** | **fails, demand #3, same message** |

The control fails identically with no wrapper anywhere, which is the point. CI is
the same mechanism landing on the other side often enough to look green: the same
job re-run on the identical commit passed (run 32882434798, job 97914916643 failed
and job 97921543964 passed).

**One real difference the probe does surface, and it is not this one.** The abort
message reads `Abort(7)` through the wrapper where native MPICH says `Abort(8)`,
because `MPI_Abort`'s error code is converted like any other (`src/mpiwrapper/hw_lifecycle.c`,
`NOTES.md` #5) and `MPIABI_ERR_ROOT` is 8 where `MPI_ERR_ROOT` is 7. `runtests`
never reads that value, so it has nothing to do with the flap -- but an application
that expects `MPI_Abort(comm, N)` to reach the invoking environment as `N` does not
get that here, and `NOTES.md` #13 is where that belongs if it is worth deciding.
