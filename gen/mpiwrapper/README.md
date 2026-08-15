# `gen/mpiwrapper/`

Generated (never hand-edited): `wrappers.c` (the per-function conversion
bodies) and `constants.c` (the ABI-to-implementation constant tables).
NOTES.md §3.

Written by `dev/generate.py`. As of S3's first half, of the 688 entry points:

| | |
|---|---|
| 518 | generated: S2's mechanical classes plus S3a's arrays, statuses and lifetimes |
| 118 | named in the `HAND_WRITTEN` ledger — eight written, the rest S4's |
| 52 | deferred to S3's second half, each with the argument class that blocks it |

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

Six bodies carry no guard, and they are the only ones:
`MPI_Status_get_source`/`_tag`/`_error` and the three setters read and write a
named field of the caller's own ABI status and never reach the implementation,
so a stub would replace a working answer with
`MPI_ERR_UNSUPPORTED_OPERATION` for no reason (NOTES.md §5.2).
