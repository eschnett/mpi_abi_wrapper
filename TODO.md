# Erik's open tasks (not to be modified by Claude)

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
