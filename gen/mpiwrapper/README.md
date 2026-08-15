# `gen/mpiwrapper/`

Generated (never hand-edited): `wrappers.c` (the per-function conversion
bodies) and `constants.c` (the ABI-to-implementation constant tables).
NOTES.md §3.

Written by `dev/generate.py`. As of S2, of the 688 entry points:

| | |
|---|---|
| 473 | generated, in the mechanical argument classes |
| 120 | named in the `HAND_WRITTEN` ledger — ten written, the rest S4's |
| 95 | deferred to S3, each with the argument class that blocks it |

`gen/report.txt` names every one of them, and the generator fails if an entry
point is in none of the three.

Every generated body is guarded on `MPIWRAPPER_HAVE_<name>`, and so is every
optional constant in `constants.c`. `dev/probe_impl.py` writes those at
configure time by asking the compiler about the implementation's own header —
never `#ifdef <the implementation's own name>`, which sees macros and not
enumerators and is therefore quietly false for `MPI_COMBINER_*` on MPICH and
`MPI_THREAD_*` on Open MPI. An entry point the implementation does not have
gets decision 6's stub, so the ABI surface never shrinks and what is missing is
discoverable at run time; the deferred entry points get the same stub.
