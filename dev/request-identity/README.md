# Do implementation request handles identify operations?

Three programs, because `src/mpiwrapper/staging.c` keys its table of staged
temporaries on the implementation's request handle:

| | asks |
|---|---|
| `probe.c` | does a handle identify an operation? (no, twice over) |
| `probe-staged.c` | does it happen to the *staged* entry points? (yes) |
| `reproduce.c` | what does the wrapper then do? (refuses a legal call) |

```sh
cc -I$MPI/include -L$MPI/lib -o probe probe.c -lmpi   # add -lpmpi for MPICH
./probe
cc -I$MPI/include -L$MPI/lib -o probe-staged probe-staged.c -lmpi
./probe-staged
MPIR_CVAR_IALLTOALLW_INTRA_ALGORITHM=tsp_inplace ./probe-staged   # MPICH
```

`reproduce.c` links against the ABI rather than an implementation; its header
comment has the two-line recipe. One rank is enough for all three: every
shortcut below is a property of this rank's own schedule.

## Results

MPICH 4.3.1 (conda-forge, osx-arm64) and Open MPI 5.0.6 (built from source),
2026-08-14 for `probe.c` and 2026-08-16 for the rest:

| `probe.c` | MPICH | Open MPI |
|---|---|---|
| `MPI_Isend` to `MPI_PROC_NULL` x4 | `0x6c000001` x4 | `0x1013f7920` x4 |
| `MPI_Irecv` from `MPI_PROC_NULL` x4 | `0x6c000002` x4 | `0x1013f7920` x4 |
| `MPI_Ibarrier` on `MPI_COMM_SELF` x4 | `0x6c00000b` x4 | `0x1013f7920` x4 |
| `MPI_Ialltoallw` on `MPI_COMM_SELF` x4 | distinct | distinct |
| a completed handle, then a new operation | **value reused** | **value reused** |

Two separate mechanisms, and it is worth keeping them apart:

1. **Shared built-in requests.** An operation that is complete on return does not
   need a per-operation object, and neither implementation allocates one. MPICH
   has one built-in per operation kind; Open MPI has a single
   `ompi_request_empty` shared across *all* of them, so a `MPI_PROC_NULL` send
   and an `MPI_Ibarrier` on `MPI_COMM_SELF` hand back the same pointer.

2. **Recycling.** Once an operation completes, its handle value comes back for
   the next one. Both implementations reuse it immediately.

### The staged entry points get one too

**`probe.c`'s fourth row is the one shape of `MPI_Ialltoallw` that does not
share**, and reading it as "the staged family is safe" was wrong. It passes
distinct buffers and a count of 1, which is exactly what builds a non-empty
schedule on a one-rank communicator; every zero-work shape shares. `probe.c`'s
row is left above as measured, and `probe-staged.c` is what the design should be
read from:

| `probe-staged.c` | MPICH, default | MPICH, `tsp_inplace` | Open MPI |
|---|---|---|---|
| `MPI_Isend` to `MPI_PROC_NULL` x3 | `0x6c000001` x3 | `0x6c000001` x3 | shared |
| an ordinary 1-int `MPI_Isend` | `0x6c000001` | `0x6c000001` | **the same shared value** |
| `MPI_Ialltoallw`, `MPI_IN_PLACE`, counts 0 | distinct | `0x6c00000b` x3 | **shared** |
| `MPI_Ialltoallw`, `MPI_IN_PLACE`, count 1 | distinct | `0x6c00000b` x3 | **shared** |
| `MPI_Ialltoallw`, counts 0 | distinct | distinct | **shared** |
| `MPI_Ineighbor_alltoallw`, degree 0 | distinct | distinct | **shared** |
| `MPI_Alltoallw_init`, counts 0 | distinct | distinct | distinct |
| `MPI_Neighbor_alltoallw_init`, degree 0 | distinct | distinct | distinct |

On Open MPI the shared value is the same `ompi_request_empty` the `MPI_PROC_NULL`
send returns, so the collision is across kinds as well as within one.

Where each comes from, read out of the sources rather than guessed — Open MPI
5.0.6 (the tarball `ci-scripts/install-openmpi.sh` fetches) and MPICH 5.0.1
(current `main`-series source; 4.3.1 is what was *run*, and its measured values
agree, but the file and line references below are 5.0.1's):

- **Open MPI** does it as a blanket rule. `NBC_Schedule_request`
  (`ompi/mca/coll/libnbc/nbc.c`) opens with *"no operation (e.g. one process
  barrier)?"* and answers any empty schedule with `nbc_get_noop_request`, which
  is `&ompi_request_empty` for the nonblocking case. `a2aw_sched_linear` skips
  zero-span sends and receives, so an `MPI_Ialltoallw` in which this rank
  exchanges nothing with anybody has an empty schedule — **on any communicator
  size, not only `MPI_COMM_SELF`**. `nbc_ialltoallw.c` has a second, earlier
  shortcut for the in-place zero-span case. Default configuration; no CVAR,
  no MCA parameter.
- **MPICH** does it only through the generic transport:
  `MPIR_TSP_sched_start` short-circuits `total_vtcs == 0` to
  `MPIR_Request_create_complete(MPIR_REQUEST_KIND__COLL)`. Its default
  selection for `ialltoallw` is `sched_inplace`/`sched_blocked`, and
  `MPIDU_Sched_start` always allocates, so the default is safe;
  `MPIR_CVAR_IALLTOALLW_INTRA_ALGORITHM=tsp_inplace` on a size-1 communicator
  reaches it, because `MPIR_TSP_Ialltoallw_sched_intra_inplace`'s loop skips
  `i == rank` and so emits nothing (which is why the count is irrelevant in
  that column). `tsp_blocked` never does: its per-block fence is a vertex.
- **Both** keep persistent requests out of it, and are forced to: a persistent
  request must be independently startable and freeable, so it cannot be a
  shared singleton. MPICH's `MPIR_Alltoallw_init_impl` always calls
  `MPIR_Request_create(PREQUEST_COLL)`; Open MPI's
  `nbc_get_noop_request(persistent = true)` calls
  `ompi_request_persistent_noop_create`, which `OBJ_NEW`s a fresh object.
- **Both** answer an *eagerly completed ordinary send* with the built-in too,
  not just a `MPI_PROC_NULL` one — Open MPI in `mca_pml_ob1_send_inline`,
  MPICH in `posix_send.h`, `ofi_send.h`, `ucx_send.h` and `ch4_self.c`. So the
  release side is exercised at this key by everyday traffic, not rarely.

### And the wrapper refused

`reproduce.c`, against `build/openmpi/libmpiwrapper.*`, stock Open MPI 5.0.6,
before NOTES.md #13.2's fix:

```
two zero-work MPI_Ialltoallw, posted before either is waited on
  first:  rc=0   request=0x1055cf920
  second: rc=17  request=0x1055cf920
          MPI_ERR_INTERN: internal error
  *** a legal call was refused ***
```

and the same for two degree-0 `MPI_Ineighbor_alltoallw` — where both extents
are zero, so there was nothing to keep alive at all and the wrapper attached a
zero-length block only because the attach was unconditional. Against
`build/mpich/libmpiwrapper.*` both cases were accepted. With the default error
handler rather than this program's `MPI_ERRORS_RETURN`, the refusal aborted the
job.

**This is a regression test now**, and it exits nonzero if either case is
refused again. Green on both implementations, on macOS and on Linux under
`ci-scripts/run-linux-docker.sh`. Keep it: the two cases are what (a) and (b)
each close, and neither is reachable from `ctest`, which is single-rank.

### What `MPI_Request_get_status` answers

Measured because NOTES.md #13.2 turns on it:

| | MPICH | Open MPI |
|---|---|---|
| shared built-in (`MPI_PROC_NULL` `MPI_Isend`) | `flag = 1` | `flag = 1` |
| zero-work `MPI_Ialltoallw` | `flag = 1` | `flag = 1` |
| in flight (`MPI_Irecv`, no matching send yet) | `flag = 0` | `flag = 0` |
| **persistent, fresh, never started** | **`flag = 1`** | **`flag = 1`** |
| cost, on a complete request | 6.2 ns/call | 2.3 ns/call |

The fourth row is MPI-5.0 §3.7.6's trap, measured: an inactive request answers
"complete", so freeing a persistent request's block on that answer is a
use-after-free at the first `MPI_Start`. The first three rows are what a fix
needs, and the four `_init` entry points are separated from the four `I*` ones
statically, by the generator, not by asking.

## Why it matters here

Only for the request-keyed table of temporaries that outlive their call
(NOTES.md #6.3, decision 10). Neither the callback pools (keyed on a slot index
we hand out) nor the predefined-handle maps (built once from known values) are
affected.

**What attaches is a short list**: the `MPI_Ialltoallw` family and the
persistent `_init` forms, eight entry points. `MPI_Isend` and friends attach
nothing — their arguments are all scalars. But they *release*, because every
completion call releases by handle value, so a wait on a `MPI_PROC_NULL`
`MPI_Isend` does look its key up. Normally it is absent and the probe stops at
the first empty slot; where it is not — Open MPI shares one object across kinds
— the block gets freed by the completion of an unrelated operation.

That is safe, and the reason is semantic rather than empirical: MPI requires
every request to be independently testable and completable, so two *live*
operations cannot share a handle. Sharing implies the handle carries no
per-operation state, which implies both are already complete. So a block freed
early is a block nobody is reading. What remained was not memory-unsafety but a
legal program being told `MPI_ERR_INTERN` — which `reproduce.c` showed was no
hypothesis. NOTES.md #13.2's (b) closes that door from the other side: a shared
built-in is only handed out for an operation already complete on return, and
such an operation no longer attaches, so a shared key never enters the table for
an unrelated completion to find.

Mechanism 2 is why *every* completion entry point has to release, not just the
ones an author happens to think of: a completion we fail to observe leaves a
stale entry whose key a later operation will be handed again.

## Guarantees, and how few there are

Neither implementation documents anything about handle uniqueness, and the
inventories are small enough to state exactly (same two source trees as above).

- **MPICH** initializes `MPIR_Request_builtin[MPIR_REQUEST_N_BUILTIN]`, which is
  17 slots, 15 of them used: one per request *kind*, handle
  `0x6c000000 | kind`, plus `MPIR_REQUEST_NULL_RECV` at index 16. Five are
  observable by a user: `SEND 0x6c000001`, `RECV 0x6c000002`,
  `COLL 0x6c00000b`, `RMA 0x6c00000d`, `NULL_RECV 0x6c000010`.
- **Open MPI** has three predefined request objects: `ompi_request_null` (that
  is `MPI_REQUEST_NULL`), `ompi_request_empty`, and `ompi_request_empty_send`
  (MPI-4 branch, identical but for a cancel callback that warns). One shared
  value covers essentially everything completed on return.

The values are not a contract and are not even internally consistent. MPICH's
header calls the built-ins *"internally used and … not exposed to the user"*,
which the code beneath it contradicts; its four named `MPIR_REQUEST_COMPLETE_*`
macros are all dead, and two carry stale values — `COLL` says `0x6c000006` where
the code produces `0x6c00000b` — because the index is the kind and the kind
enum has grown. Open MPI's justification in ob1 is status semantics, not
identity: returning the shared object is legal because the only valid field of a
send completion status is the cancelled flag.

Two regularities are real, both forced by the standard rather than promised, and
both load-bearing for the fix:

1. **A persistent request is never a shared built-in** (measured above; #6.3
   already relies on this).
2. **A shared built-in is always already complete** — MPICH sets `cc = 0`, Open
   MPI sets `req_complete = REQUEST_COMPLETED`. A shared object cannot hold
   per-operation completion state.

Open MPI makes the same move a fix here would:
`ompi_coll_base_retain_datatypes_w`, which attaches per-operation state to a
request, opens with `if (REQUEST_COMPLETE(req)) return OMPI_SUCCESS;`.
