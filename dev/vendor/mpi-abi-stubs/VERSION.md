Vendored from https://github.com/mpi-forum/mpi-abi-stubs

- commit: a1183ce6e048341cc65414fd21d928b8cfc9709f
- commit date: 2025-11-24T09:08:58+03:00
- file: mpi.h (unpatched upstream stub header)

`doc/mpi.h.patch` is applied on top of this file by `dev/generate_headers.py`
to produce `gen/include/mpi.h`. Do not hand-edit `mpi.h` in this directory;
re-vendor from upstream instead.

## Before re-vendoring, read this

`doc/mpi.h.patch`'s first hunk corrects `MPI_Psend_init`/`MPI_Precv_init` to
take `MPI_Count count` with no `_c` twin. **Upstream has since made that same
correction** -- it is absent from the commit above and present at
`a8470014` (2026-08-20) -- so the hunk stops applying as soon as this file
moves past it.

`patch` reports "Reversed (or previously applied) patch detected!" in that
case. With no terminal to ask, it answers its own prompt and reverse-applies --
which is how mpif, which clones the stubs unpinned rather than vendoring them,
ended up installing a header with the Fortran declarations *removed*.
NOTES.md #1 has that account.

Here it is a build failure instead, for two reasons worth knowing rather than
assuming: `dev/generate_headers.py` checks `patch`'s exit code and raises, and
this patch's four hunks do not reverse into the Fortran block the way mpif's
three do. Both were measured against a simulated future stub rather than
argued. `--forward` is passed regardless, so the refusal is stated rather than
inferred from an exit code that happened to be non-zero.

What is left for whoever moves the pin: **drop the `Psend`/`Precv` hunk from
`doc/mpi.h.patch`**, which `ctest -R headers-up-to-date` will demand by then.
