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
case, and mpif is where that has actually been seen: it clones the stubs rather
than vendoring them, and through v1.0.0 that clone was unpinned. **What happens
next depends on which `patch` runs**, which is why two accounts of it
disagreed. Reproduced against `a8470014` with mpif v1.0.0's patch:

```sh
git clone --depth 1 https://github.com/mpi-forum/mpi-abi-stubs stubs
git -C stubs fetch --depth 1 origin a8470014382bf4a4f39f9b3539857b36ac7b35c9
git -C stubs checkout a8470014382bf4a4f39f9b3539857b36ac7b35c9
mkdir inc && cp stubs/mpi.h inc/
patch -d inc -p1 <path/to/mpif/fortran/mpi.h.patch
```

- **GNU `patch`**, the CI runner's, defaults its "Assume -R?" prompt to `n`:
  skips, 3 of 3 hunks ignored, exit 1. That is run 32868258767, where both
  `abi` rows died here after a full MPI build.
- **`patch 2.0-12u11-Apple`**, this laptop's, defaults to **`y`**: it
  reverse-applies the two `Psend`/`Precv` hunks and rejects the Fortran one.
  The header comes out with the Fortran declarations absent *and* upstream's
  `MPI_Count` correction undone, back to `int count`.

Add `--forward` and the two agree -- Apple `patch` then ignores all three,
exits 1, and leaves `MPI_Count` intact. mpif v1.0.1 pins the stubs to
`a8470014`, the same commit named above, so neither outcome is reachable
there; `ci-scripts/mpif-version.sh` is where this project names that version.

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
