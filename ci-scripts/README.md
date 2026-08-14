# `ci-scripts/`

MPI install and build-shape checks: pinned released tarballs, built from
source and cached. NOTES.md §9 ("Provisioning MPI in CI").

Note the deliberate split from `ci-scripts/suite/`: the MPI-install cache key
must hash these scripts and must *not* hash the suite's expected-failure
list, or every edit to a reason rebuilds MPI on every variant (a mistake
mpif's own `ci-scripts/README.md` records getting wrong twice).

Populated in S6 (build, packaging, CI matrix), which floats and needs only S1.
