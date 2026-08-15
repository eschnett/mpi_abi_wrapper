# `gen/mpi_abi/`

Generated (never hand-edited): `entrypoints.c`, the 1376 one-line `MPI_*`/
`PMPI_*` forwarders that make up `libmpi_abi`. NOTES.md §2, §3.

Written by `dev/generate.py` in S2. `test/check_exports.cmake` checks the
built library's exports against the ABI header's entry-point list in both
directions, so a forwarder the generator dropped fails as loudly as one it
invented.

Two forwarders are not the cast-free one-liners the rest are, and both say why
in the file: `MPI_Pcontrol`, because C cannot forward `...`, and the four
`MPI_T` entry points whose enum *tags* the renaming had to touch, which are
the only place in 1376 forwarders where a cast is right.
