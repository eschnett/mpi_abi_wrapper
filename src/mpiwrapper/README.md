# `src/mpiwrapper/`

Hand-written: the ~50-function `HAND_WRITTEN` set, trampolines, maps, status
conversion. NOTES.md §8.

Populated starting in S1 (perfect-hash reverse map, status blob, rank/tag,
error codes, bitmask, staging, op trampoline pool) and completed in S4 (the
remaining hand-written ~50: lifecycle, callback registration, spawn, buffer
attach, `MPI_Pcontrol`, dynamic error codes, the 26 Fortran converters, the
ten output-string functions).
