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
| `extents.c` | how long an array is where `apis.json` answers `*` -- the group's size, the topology's degrees, the datatype's envelope. Every call in it is `PMPI_`, because none of it is application traffic |
| `keyvals.c` | the *dynamic* half of the keyval family: the predefined thirteen convert through a generated switch, and a keyval the implementation handed out at run time cannot, because an `int` has no slack to tag around (§5.6) |
| `toolobj.c` | what class of object an `MPI_T` variable binds to, for the `obj_handle` whose class is not in its own argument list. Same shape and same `PMPI_` rule as `extents.c` |
| `callbacks.c` | the op and error-handler trampoline pools |
| `handwritten.c`, `handwritten.h` | the entry points needing per-function judgement. The header is the list of bodies that *exist*; the ledger itself is `HAND_WRITTEN` in `dev/generate.py`, and the generator fails if the two disagree in either direction |
| `getvtable.c` | the single exported symbol: handshake, outward-resolution check, map construction |

`constants.c` and `wrappers.c` moved to `gen/mpiwrapper/` in S2; S1's versions
are frozen in `dev/s1-reference/` as what the generator must reproduce.

`handwritten.c` has S1's eight. S2 also kept `MPI_Waitall` and
`MPI_Ialltoallw` here, as stand-ins for the two array classes it could not
generate -- an inout request array released at completion, and temporaries that
outlive their call. S3's first half generates both, with every other member of
their families, so the bodies are gone.

S3's second half added `MPI_T_event_handle_free` *to* the ledger --  it takes a
callback-typed parameter, so installing it needs a trampoline like the two
`MPI_T` registrars beside it (§6.1) -- and took `MPI_Keyval_create` away again,
since that one is `MPI_Comm_create_keyval` under a name MPI-3.0 deleted and
`libmpi_abi` now forwards it (see `src/mpi_abi/README.md`). The ledger is
**118**, and since nothing is deferred any more it is exactly what the
generator does not emit.

The other 110 members of the ledger have no body yet and their slots report
`MPI_ERR_UNSUPPORTED_OPERATION`; `gen/report.txt` lists exactly which. S4
writes them: lifecycle, the callback registrations, spawn, buffer attach,
`MPI_Pcontrol`, dynamic error codes, the Fortran and integer handle converters,
the ten output-string functions. Two of the runtime pieces they need are
already here and complete, because generated bodies needed them first:
`keyvals.c`'s registry is what `MPI_*_create_keyval` fills, and `callbacks.c`'s
pools are what the errhandler registrars draw from.
