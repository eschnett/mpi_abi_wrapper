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

Every generated body is guarded on `MPIWRAPPER_HAVE_<name>`, which
`dev/probe_entrypoints.py` writes at configure time from the implementation's
own header; one the implementation does not have gets decision 6's stub, so
the ABI surface never shrinks and what is missing is discoverable at run time.
The deferred entry points get the same stub.
