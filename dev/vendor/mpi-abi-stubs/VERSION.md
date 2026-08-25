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

What is left for whoever moves the pin, from a diff against `a8470014`:

- **Drop two hunks** from `doc/mpi.h.patch`, `@@ -941` and `@@ -1609` -- the
  `MPI_Psend_init`/`MPI_Precv_init` correction and its `PMPI_` twin. Upstream
  has both. `ctest -R headers-up-to-date` will demand it.
- **Keep the other two.** `@@ -37`'s `MPI_Status` struct tag is not upstream,
  and `@@ -1896`'s Fortran block never will be: MPI-5.0 20.4 excludes
  `MPI_Fint` from the ABI deliberately.
- **Re-check the renaming rules** (NOTES.md #2, "Naming"): upstream renamed the
  MPI_T handle tags, `struct MPI_T_enum_t` becoming `struct MPI_ABI_T_enum` and
  five more like it.
- Expect noise in the diff that means nothing: `MPI_ERR_LASTCODE`,
  `MPI_ORDER_C` and `MPI_ORDER_FORTRAN` were respelled from hex to decimal at
  the same values. `MPI_Aint`/`Offset`/`Count` gained an MSVC branch, which is
  the first Windows acknowledgement in the stub header (NOTES.md #13.4).
