# Erik's open tasks

- remove the call overhead in the `mpi_abi` entrypoints: define the
  functions `static inline`? requires changing the MPI ABI `mpi.h`.

- add test with MPI 3.0? or 3.1?
  (the MPI-3.0 floor row `ci-scripts/linux-floor.sh` exists and is
  deliberately not in CI, which also leaves the *toolchain* floor —
  CMake 3.20, Python 3.8, gcc 9 — with no coverage anywhere. If PR
  wall-clock is the objection, a push-only or scheduled row buys it back
  without slowing PRs.)

- the `spawn` directory is run nowhere in CI. `run-suite.sh` excludes it by
  default because `MPI_Comm_spawn` hangs under hydra *on macOS*, which is
  not a fact about the Linux runners — so dynamic-process coverage is zero
  for a reason that does not apply there. `--with-spawn` on one Linux leg
  settles whether that is a gap or a second finding.

- one-sided over Open MPI has no suite coverage since the `rma` shard was
  dropped (upstream `osc/rdma` memory blowup, measured — see
  `ci-scripts/suite/README.md`). Open MPI 4.1.8 completes the same tests in
  1.7 GB, so a cheap 4.1.x leg running only that shard would close the hole
  without waiting for upstream.

- `sanitize` covers `address,undefined`; S9's other half, the
  `thread` sanitizer over `MPI_THREAD_MULTIPLE`, is still a comment in
  `ci.yaml` rather than a leg.

Done, and dropped from the list above: the CI rows all gate now (no
`continue-on-error` anywhere in `ci.yaml` — `ci-scripts/README.md` has how the
last two got there), local and CI xfail lists are separate, and the workflow is
about 20 minutes rather than 49.
