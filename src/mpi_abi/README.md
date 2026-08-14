# `src/mpi_abi/`

Hand-written: bootstrap, `dlopen` isolation, vtable acquisition. NOTES.md §2.

Populated in S1 (the 15-entry-point prototype): the constructor, the
environment-variable/build-time-path lookup for `libmpiwrapper`, the
version/subversion/layout-hash handshake, and the outward-resolution check
described in NOTES.md §2 ("Symbol resolution when loading the wrapper").
