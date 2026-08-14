# `src/include/`

`mpiwrapper_vtable.h`, the only thing `libmpi_abi` and `libmpiwrapper` share.

**Hand-written, and the one S1 file that does not survive S2**: the real one is
`gen/include/mpiwrapper_vtable.h`, emitted by the generator from the 1376-slot
list. This is the 56-slot prototype of it, written first so that the generator is
designed against a known output shape rather than the other way round (NOTES.md
§11, decision 17).

`MPIWRAPPER_LAYOUT_HASH` is computed from the slot list by `dev/layout_hash.py`
and checked by `ctest -R layout-hash`, so it tracks what it claims to summarize
instead of becoming a constant nobody updates.
