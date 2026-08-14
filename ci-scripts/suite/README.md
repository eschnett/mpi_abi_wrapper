# `ci-scripts/suite/`

The MPICH C test suite runner, `mpiexec` filter, and per-variant
expected-failure list with a reason on every line. NOTES.md §9, §10.

Deliberately cache-keyed apart from `ci-scripts/` (see that directory's
README): editing a reason here must not invalidate the MPI-install cache.

Populated in S7 (MPICH C test suite).
