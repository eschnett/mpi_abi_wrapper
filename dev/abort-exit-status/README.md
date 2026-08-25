# `dev/abort-exit-status/`

Two questions about `MPI_Abort`, both answered by an exit status rather than by
output, and both reached from the same CI line. `run.sh` settles the first;
`exit-status.sh` and `abort-status.c` settle the second.

| | question | answer |
|---|---|---|
| `run.sh` | does `errors/comm/intercomm_abort` flap because of this project? | no — it is the launcher's flap, and the wrapper is not in that path |
| `exit-status.sh` | does `MPI_Abort(comm, N)` reach the invoking environment as `N`? | it does **now**; before `NOTES.md` #5.6a it did not |

## The flap measurement (`run.sh`)

`errors/comm/intercomm_abort 6` flaps on the `mpich .. rest` CI shards, and the
question a flaky line has to answer first is whose flap it is.
`ci-scripts/suite/flaky-ci-mpich.txt` carries the verdict and the run IDs; this
is the measurement behind it.

```sh
dev/abort-exit-status/run.sh build/suite-mpich/prefix/bin/mpicc \
                             /path/to/impl/bin/mpicc [iterations]
```

The suite source is taken from `$MPIABI_SUITE_SRC` (default `build/suite-src`),
so `ci-scripts/suite/run-suite.sh mpich` has to have run once. On the development
laptop, put `scripts/host-env.sh` in front of it as usual, and note that the conda
MPICH's `mpicc` names a compiler that is not installed -- the control build passes
`-cc=clang` for that reason, overridable with `MPIABI_PROBE_CC`.

### What the test asks for, and which part of it fails

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

### The control, and what it shows

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

## The exit-status measurement (`exit-status.sh`)

The flap probe surfaced one real difference on its way past: the abort message
read `Abort(7)` through the wrapper where native MPICH said `Abort(8)`, because
`MPI_Abort`'s `errorcode` was converted like any other error code and
`MPIABI_ERR_ROOT` is 8 where MPICH's `MPI_ERR_ROOT` is 7. `runtests` never reads
that value, so it had nothing to do with the flap. But `MPI_Abort(comm, N)` is
supposed to reach the invoking environment as `N`, and **the `ERR_ROOT` row was
the mild case**. This probe is the general one.

```sh
scripts/host-env.sh dev/abort-exit-status/exit-status.sh \
    build/mpich /path/to/impl/bin/mpiexec [/path/to/impl/bin/mpicc]
```

`abort-status.c` aborts with the number given on its command line and prints
nothing, so `$?` is the whole answer. The first argument is a configured wrapper
build tree, which supplies `libmpi_abi` and the `libmpiwrapper` for that
implementation; the control is the same source built with the implementation's
own `mpicc`, so each row is a comparison and not a number. The `verdict` column
is `same` when the two agree, which is the whole test.

### What it measured, on 2026-08-25

MPICH 4.3.1 (conda) and Open MPI 5.0.6, one rank each, on this laptop. **Both
natives are the same function: the status is `N & 0xff`**, which is all `wait(2)`
carries -- 256 comes back as 0, 1000000 as 64, 1001 as 233. Neither
implementation interprets `N` as an error code anywhere along the way.

Before the change, with `mpiwrapper_errorcode_fromabi` in the path:

| `N` | wrapped/MPICH | native/MPICH | wrapped/OMPI | native/OMPI |
|---|---|---|---|---|
| 1 | 1 | 1 | 1 | 1 |
| 8 | **7** | 8 | 8 | 8 |
| 16 | **15** | 16 | 16 | 16 |
| 42 | **37** | 42 | 42 | 42 |
| 62 | **15** | 62 | **16** | 62 |
| 100 | **15** | 100 | **16** | 100 |
| 137 | **15** | 137 | **16** | 137 |
| 255 | **15** | 255 | **16** | 255 |
| 1001 | **61** | 233 | **56** | 233 |
| 1000000 | **15** | 64 | **16** | 64 |

Two distinct failures, and the second is the one that matters:

- **Renumbering**, for an `N` that happens to be a predefined ABI class: 8 → 7,
  16 → 15, 42 → 37 over MPICH. Visible only where the two headers disagree, so
  Open MPI's 1--62 look fine and hide it.
- **Collapse to `MPI_ERR_OTHER`**, for every `N` above 62 outside MPI_T's
  1001--1018 -- which is to say for every ordinary exit status an application
  would pick for its own reasons. 100, 137 and 255 all arrive as 15 over MPICH
  and as 16 over Open MPI, because they are not predefined classes and
  `errorcodes.c` never issued them, so `mpiwrapper_errorcode_dynamic_fromabi`
  takes its `return MPI_ERR_OTHER` arm. `137` is the row to look at: a script
  testing `[ $? -eq 137 ]` cannot work through the wrapper, and no diagnostic
  says why.

After the change, every row is `same` over both implementations -- including
`Abort(8)` in MPICH's message, which is where this started:

| `N` | 1 | 8 | 16 | 42 | 62 | 63 | 100 | 137 | 200 | 255 | 256 | 1001 | 16384 | 1000000 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| both, both MPIs | 1 | 8 | 16 | 42 | 62 | 63 | 100 | 137 | 200 | 255 | 0 | 233 | 0 | 64 |

**What the probe does not settle**, and `NOTES.md` #5.6a says why it is accepted:
an application that aborts with a code from `MPI_Add_error_class` now hands the
implementation an ABI-side number the implementation does not know. Nothing reads
it as an error code, so there is nothing for it to be wrong about -- but if some
future implementation did, this is the row that would move.
