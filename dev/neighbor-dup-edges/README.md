# `dev/neighbor-dup-edges/`

**Open MPI 5.0.10's *nonblocking* neighbourhood collectives use the wrong block
matching when a neighbour list repeats a process; its blocking ones get it
right.** Measured with Open MPI's own `mpicc` and no wrapper anywhere, which is
what makes the three `coll/neighb_dup_edges` lines in
`ci-scripts/suite/xfail-ci-openmpi.txt` an attributed failure rather than a
placeholder.

```sh
source scripts/host-env.sh
OMPI_CC=clang build/mpi/openmpi/bin/mpicc -o /tmp/nd dev/neighbor-dup-edges/neighb_dup.c
build/mpi/openmpi/bin/mpiexec -n 4 /tmp/nd
```

`OMPI_CC` is not optional on the development laptop: the conda Open MPI names a
compiler that is not installed, the same quirk `CLAUDE.md` records for `mpifort`
and `dev/abort-exit-status/run.sh` handles with `MPIABI_PROBE_CC`.

## What the standard says

MPI-4.1 Example 8.10 fixes the matching for a Cartesian neighbourhood: the block
sent in the negative direction of dimension `d` is received into block `2*d+1` of
the neighbour and vice versa — so **block `s` of the sender lands in block `s^1`
of the receiver**. `s^1` swaps 0 with 1 and 2 with 3.

The probe builds a 3-D grid with `dims = {1, 1, size}` and all dimensions
periodic, so four of the six blocks are exchanged with self and the neighbour
list repeats. With `sendbuf[s] = rank * NSLOT + s` the expected receive buffer is
known in closed form, `nbrs[s] * NSLOT + (s ^ 1)`.

## The measurement

Open MPI 5.0.10, four ranks, no wrapper:

| call | result |
|---|---|
| `MPI_Neighbor_alltoall` | **passes** |
| `MPI_Ineighbor_alltoall` | **fails**, 16 mismatched blocks |

Every mismatch is the identity matching where the standard asks for `s^1`:

```
MPI_Ineighbor_alltoall: rank 0 block 0 is 0, expected 1
MPI_Ineighbor_alltoall: rank 0 block 1 is 1, expected 0
MPI_Ineighbor_alltoall: rank 0 block 2 is 2, expected 3
MPI_Ineighbor_alltoall: rank 0 block 3 is 3, expected 2
```

So Open MPI delivers block `s` into block `s`, not `s^1` — but only on the
nonblocking path. **The two halves of one implementation disagree with each
other**, which is why this is worth a probe rather than a shrug: an
implementation-wide misreading of Example 8.10 would be a defensible
interpretation, and blocking-versus-nonblocking disagreement inside one library
is not.

## Why this project cares

MPICH `6d7c89b05` ("coll: fix block matching of duplicate neighbor edges") fixed
exactly this in 5.0.2rc2 and added `test/mpi/coll/neighb_dup_edges.c` with it.
The suite is MPICH's, so bumping the pin to 5.0.2rc2 put three new tests —
`neighb_dup_edges` at 4, 2 and 1 ranks — in front of *both* implementations'
legs. They pass over MPICH and fail over Open MPI, at the `MPI_Ineighbor_*`
checks only, on both architectures (run 34855861925).

**The wrapper is not involved.** That was worth establishing rather than
assuming: the failure is a block *permutation*, and a conversion layer that
reordered buffers would be a serious bug. It does not — the three calls are
generated forwarders — and this probe shows the same permutation with the
implementation's own `mpicc`, which is the method `ci-scripts/suite/README.md`
prescribes for exactly this question.

The probe covers the `alltoall` form; the suite also fails the `alltoallv` and
`alltoallw` forms of the same call, which are not reproduced here because the
attribution does not need them.
