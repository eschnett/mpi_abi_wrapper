# `src/mpiwrapper/`

Hand-written: the conversion runtime, the trampolines, and the `HAND_WRITTEN`
set. NOTES.md §5, §6, §8.

The runtime — state and conversions, none of it naming an `MPI_*` entry point
of the ABI:

| file | what |
|---|---|
| `internal.h` | the interface between this runtime and the wrapper bodies, plus the `_Static_assert` battery of §5.9 and the `#error` that catches `<mpi.h>` resolving to the ABI header |
| `handles.c` | the implementation → ABI direction: the perfect-hash reverse map, its construction, and the dynamic-handle collision check |
| `status.c` | the status blob, both directions, with the layout assertions |
| `staging.c` | call-scoped temporaries, and the request-keyed table for the ones that outlive their call |
| `extents.c` | how long an array is where `apis.json` answers `*` -- the group's size, the topology's degrees, the datatype's envelope. Every call in it is `PMPI_`, because none of it is application traffic |
| `keyvals.c` | the *dynamic* half of the keyval family: the predefined thirteen convert through a generated switch, and a keyval the implementation handed out at run time cannot, because an `int` has no slack to tag around (§5.6) |
| `errorcodes.c` | the same for error codes, and it absorbs the implementation's own as well as the application's -- MPICH answers essentially every error with an instance-specific code, and interning it is what keeps its class and its message reachable (§5.6) |
| `serialize.c` | the intern table behind `MPI_<class>_toint`/`_fromint` (§20.4.5), which cannot be a cast where a dynamic handle is a 64-bit address |
| `buffers.c` | the attached buffer's ownership record: which scope holds a buffer *we* allocated, for `MPI_BUFFER_AUTOMATIC` over an implementation that does not have it |
| `toolobj.c` | what class of object an `MPI_T` variable binds to, for the `obj_handle` whose class is not in its own argument list. Same shape and same `PMPI_` rule as `extents.c` |
| `callbacks.c` | the six trampoline *pools* of §6.1's first row: two user-op variants and four error-handler classes, none of which has an extra-state argument to carry a user pointer |
| `extrastate.c` | §6.1's second row, the families that do: attribute keys, generalized requests and datareps, each a heap-allocated `{user_fn, user_extra}` pair -- and the recognition of `MPI_COMM_DUP_FN` and friends, which are sentinels rather than functions |
| `toolevents.c` | §6.1's third row: `MPI_T`'s event callbacks, keyed on the implementation's registration handle because `MPI_T_event_set_dropped_handler` has no user pointer to carry anything through |
| `getvtable.c` | the single exported symbol: handshake, outward-resolution check, map construction |

The bodies — the `HAND_WRITTEN` ledger, one file per family, each declared in
`handwritten.h`:

| file | what | stage |
|---|---|---|
| `hw_lifecycle.c` | `MPI_Init`, `_thread`, `MPI_Finalize`, `MPI_Initialized`, `MPI_Finalized`, `MPI_Abort`, the session pair -- and the initialization state the first four are answered from | S1, S4b |
| `hw_callbacks.c` | the fifteen callback registrars | S1, S4b |
| `hw_buffers.c` | the twelve buffer attach and detach forms | S4b |
| `hw_errors.c` | `MPI_Add_error_*` and `MPI_Remove_error_*` | S4b |
| `hw_spawn.c` | `MPI_Comm_spawn` and `_multiple` | S4b |
| `hw_pcontrol.c` | `MPI_Pcontrol`, the one genuinely variadic entry point | S4b |
| `hw_converters.c` | the 44 handle converters and the four status converters | S4a |
| `hw_status.c` | the ten functions that consume a status in the *in* direction | S4a |
| `hw_strings.c` | the ten output-string buffers with no length argument | S4a |
| `hw_abi.c` | the six `MPI_Abi_*` calls, which answer about this library | S4a |

`handwritten.h` is the list of bodies that *exist*; the ledger itself is
`HAND_WRITTEN` in `dev/generate.py`, and the generator fails if the two
disagree in either direction. Since S4b it also freezes the *count* of bodies,
so one going missing fails generation rather than quietly becoming a
run-time-reporting stub.

`constants.c` and `wrappers.c` moved to `gen/mpiwrapper/` in S2; S1's versions
are frozen in `dev/s1-reference/` as what the generator must reproduce.

## How the set got to where it is

S1 wrote eight bodies, in a file called `handwritten.c`. S2 kept `MPI_Waitall`
and `MPI_Ialltoallw` there too, as stand-ins for the two array classes it could
not generate; S3's first half generates both, with every other member of their
families. S3's second half added `MPI_T_event_handle_free` *to* the ledger --
it takes a callback-typed parameter, so installing it needs a trampoline like
the two `MPI_T` registrars beside it (§6.1) -- and took `MPI_Keyval_create`
away again, since that one is `MPI_Comm_create_keyval` under a name MPI-3.0
deleted and `libmpi_abi` now forwards it (see `src/mpi_abi/README.md`).

S4a wrote 70 of the remaining 110 and gave four families a file each. S4b wrote
the last 40 and did the same for the rest, so `handwritten.c` is gone: each of
its eight bodies belongs to a family that has a file now, and a file named
after the stage that wrote it was never going to survive the stage after that.

**The ledger is 118 and every one of them has a body.** What still answers
`MPI_ERR_UNSUPPORTED_OPERATION` is decided per build by `dev/probe_impl.py`,
for hand-written and generated entry points alike (decision 6), and
`gen/report.txt` explains the one limitation that is this library's rather than
an implementation's: `MPI_BUFFER_AUTOMATIC` emulated with a buffer of a fixed
size where the implementation has no such mode.
