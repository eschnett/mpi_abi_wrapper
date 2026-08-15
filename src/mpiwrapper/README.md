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
| `handwritten.c`, `handwritten.h` | the entry points needing per-function judgement. The header is the list of bodies that *exist*; the ledger itself is `HAND_WRITTEN` in `dev/generate.py`, and the generator fails if the two disagree in either direction |
| `getvtable.c` | the single exported symbol: handshake, outward-resolution check, map construction |

`constants.c` and `wrappers.c` moved to `gen/mpiwrapper/` in S2; S1's versions
are frozen in `dev/s1-reference/` as what the generator must reproduce.

`handwritten.c` has ten entry points, not S1's nine: S2 moved `MPI_Waitall`
here, because its request array is inout and its staged temporaries are
released at completion, which is S3's class rather than one S2 generates. S3
takes `MPI_Waitall` and `MPI_Ialltoallw` back and deletes both bodies.

The other 110 members of the ledger have no body yet and their slots report
`MPI_ERR_UNSUPPORTED_OPERATION`; `gen/report.txt` lists exactly which. S4
writes them: lifecycle, the remaining callback registrations, spawn, buffer
attach, `MPI_Pcontrol`, dynamic error codes, the Fortran and integer handle
converters, the ten output-string functions.
