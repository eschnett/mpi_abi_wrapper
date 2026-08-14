# `src/mpiwrapper/`

Hand-written: the conversion runtime, the trampolines, and the `HAND_WRITTEN`
set. NOTES.md §5, §6, §8.

Permanent:

| file | what |
|---|---|
| `internal.h` | the interface between this runtime and the wrapper bodies, plus the `_Static_assert` battery of §5.9 and the `#error` that catches `<mpi.h>` resolving to the ABI header |
| `handles.c` | the implementation → ABI direction: the perfect-hash reverse map, its construction, and the dynamic-handle collision check |
| `status.c` | the status blob, both directions, with the layout assertions |
| `staging.c` | call-scoped temporaries, and the request-keyed table for the ones that outlive their call |
| `callbacks.c` | the op and error-handler trampoline pools |
| `handwritten.c`, `handwritten.h` | the entry points needing per-function judgement; the header *is* the S1 `HAND_WRITTEN` ledger |
| `getvtable.c` | the single exported symbol: handshake, outward-resolution check, map construction |

S1 stand-ins for generated files, which S2 replaces:

| file | becomes |
|---|---|
| `constants.c` | `gen/mpiwrapper/constants.c` |
| `wrappers.c` | `gen/mpiwrapper/wrappers.c` |

S4 completes the hand-written ~50 (lifecycle, the remaining callback
registrations, spawn, buffer attach, `MPI_Pcontrol`, dynamic error codes, the 26
Fortran converters, the ten output-string functions).
