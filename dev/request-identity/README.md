# Do implementation request handles identify operations?

`probe.c` asks two questions of an MPI, because `src/mpiwrapper/staging.c` keys
its table of staged temporaries on the implementation's request handle and both
answers are "no".

```sh
cc -I$MPI/include -L$MPI/lib -o probe probe.c -lmpi   # add -lpmpi for MPICH
./probe
```

## Results

MPICH 4.3.1 (conda-forge, osx-arm64) and Open MPI 5.0.6 (built from source),
2026-08-14:

| | MPICH | Open MPI |
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
   and an `MPI_Ibarrier` on `MPI_COMM_SELF` hand back the same pointer. Note
   that `MPI_Ibarrier` is a *nonblocking collective* -- the shortcut is not
   confined to point-to-point, and nothing in the standard stops an
   implementation from applying it to a zero-work `MPI_Ialltoallw`, which is
   the family that stages temporaries.

2. **Recycling.** Once an operation completes, its handle value comes back for
   the next one. Both implementations reuse it immediately.

## Why it matters here

Only for the request-keyed table of temporaries that outlive their call
(NOTES.md #6.3, decision 10). Neither the callback pools (keyed on a slot index
we hand out) nor the predefined-handle maps (built once from known values) are
affected.

**What actually attaches is a short list**, and it is worth being exact about it
before drawing conclusions from the table above. In S1 there is one attach site,
`MPI_Ialltoallw`; in S3 it becomes the `MPI_Ialltoallw` family and the persistent
`_init` forms. `MPI_Isend` and friends attach nothing -- their arguments are all
scalars -- so the `MPI_PROC_NULL` rows are not an attach collision. They are in
the probe because they are the cheapest way to *see* the shared-built-in
behaviour; the row that bears on the design is `MPI_Ibarrier`, which shows a
nonblocking collective taking the same shortcut.

Requests that never attach still reach the table, through the other door: every
completion call releases by handle value, so a wait on a `MPI_PROC_NULL` `Isend`
does look its key up. Normally it is absent and the probe stops at the first
empty slot. Where it is *not* absent -- Open MPI shares one object across kinds,
so a zero-work staged collective and a `MPI_PROC_NULL` send would present the
same key -- the block gets freed by the completion of an unrelated operation.

That is safe, and the reason is semantic rather than empirical: MPI requires
every request to be independently testable and completable, so two *live*
operations cannot share a handle. Sharing implies the handle carries no
per-operation state, which implies both are already complete. So a block freed
early is a block nobody is reading. What remains is not memory-unsafety but a
legal program being told `MPI_ERR_INTERN` where an implementation shortcuts a
staged-array collective and the program posts two of them before waiting.
NOTES.md #6.3 records the fix S3 should carry.

Mechanism 2 is why *every* completion entry point has to release, not just the
ones an author happens to think of: a completion we fail to observe leaves a
stale entry whose key a later operation will be handed again.
