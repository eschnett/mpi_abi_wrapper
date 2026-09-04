# Does a launcher forward an `MPI_ABI_WRAPPER_*` variable to its ranks?

`run.sh` asks it. The answer decided that `bin/mpiexec` gets **no**
environment-rewriting logic (decision 27), and it overturned the belief the
logic would have been written against.

## The belief being tested

`ci-scripts/suite/mpiexec-filter`'s header says, of its third job:

> Hydra does. Open MPI's launcher forwards almost nothing without being told,
> so the variables that matter are re-stated as `-x` arguments.

Taken at face value that says a user who exports `MPI_ABI_WRAPPER_LIB` to
re-point a binary at another wrapper — the thing decision 5 exists for — gets
it honoured under MPICH and silently ignored under Open MPI, every rank falling
back to the baked-in default. Silently answering with the wrong wrapper is
worse than failing, so `bin/mpiexec` would have had to re-state the variables
itself.

## Measured, 2026-09-04, macOS 26 (development laptop), single node

Both launchers, `MPI_ABI_WRAPPER_PROBE=wrapper-value` and a control
`UNRELATED_PROBE` set in the launcher's own environment, `-n 2`:

| launcher | `--version` says | our variable | the control | with `-x` |
|---|---|---|---|---|
| conda MPICH 4.3.1 | `HYDRA build details:` | forwarded to both ranks | forwarded to both ranks | **`-x` rejected**: `unrecognized argument x` |
| Open MPI 5.0.10 (`build/mpi/openmpi`) | `mpiexec (Open MPI) 5.0.10` | forwarded to both ranks | forwarded to both ranks | forwarded to both ranks |

Two findings, and the second is the one that would have cost time:

1. **Open MPI 5.0.10's `mpiexec` forwards a plain environment variable to
   single-node ranks without being told.** So the premise does not hold for
   this case, and `bin/mpiexec` needs nothing. The filter's own statement is
   not wrong about *its* case — `runtests` communicates a testlist's `env=`
   through the filter's environment, and the filter is CI-proven — but "forwards
   almost nothing" does not describe a variable exported into `mpiexec`'s
   environment on one node.
2. **Hydra does not accept `-x` at all.** A shim that emitted `-x NAME`
   unconditionally would not degrade on MPICH, it would fail to launch: hydra
   answers `match_arg (lib/utils/args.c:166): unrecognized argument x`. So the
   forwarding feature would have needed the launcher-kind detection
   `run-suite.sh:204-216` does, to avoid breaking the implementation that
   needed no help in the first place. That is the whole feature paying for
   itself twice over and buying nothing.

The corollary for `ci-scripts/suite/mpiexec-filter`: its default `env_re`
listed `MPI_ABI_[A-Z_]*|MPIABI_[A-Z_]*`, and nothing that reaches that script
ever sets such a variable — `MPI_ABI_WRAPPER_LIB` is set only by
`CMakeLists.txt`'s `MPIABI_TEST_ENV` (ctest's own environment, which does not
go through the filter) and by `test/check_isolation.cmake` (a `-P` script, no
launcher). Dead by two independent measures, and removed.

## What this does not establish

**Multi-node.** `-x` is classically about getting an environment onto *other*
hosts, and this laptop has one. A launcher that forwards on one node may well
not forward across a fabric, so an HPC user re-pointing
`MPI_ABI_WRAPPER_LIB` across nodes may still have to state it with the
launcher's own flag — `-x` for prte, `-genv` for hydra. `README.md` says so
rather than promising otherwise. Re-run this probe on a multi-node allocation
to settle it; the script takes any number of launchers and needs no MPI
program, only `sh`.
