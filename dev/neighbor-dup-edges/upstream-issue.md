Draft for open-mpi/ompi. Not filed from this repository — Erik posts it.

A search of open-mpi/ompi found no existing report; the nearest neighbours are
#11756 (rank reorder in MPI_Dist_graph_create) and #10740 (Fortran
Neighbor_alltoallw datatype error), neither of which is this.

---

**Title:** `MPI_Ineighbor_alltoall` uses the wrong block matching for duplicate neighbour edges, where `MPI_Neighbor_alltoall` gets it right

**Body:**

For a Cartesian neighbourhood whose neighbour list contains the same process more
than once, the nonblocking neighbourhood collectives deliver block `s` into block
`s`, where MPI-4.1 Example 8.10 requires block `s` of the sender to arrive in
block `s^1` of the receiver. The blocking forms of the same calls honour the
standard, so the two halves of the library disagree with each other.

Reproduced on 5.0.6, 5.0.10 and `main` (6.1.0a1).

### Reproducer

Build a 3-D Cartesian communicator with two degenerate periodic dimensions, so
four of the six blocks are exchanged with self and the neighbour list repeats:

```c
int dims[3] = { 1, 1, size }, periods[3] = { 1, 1, 1 };
MPI_Cart_create(MPI_COMM_WORLD, 3, dims, periods, 0, &cart);
for (int d = 0; d < 3; d++)
    MPI_Cart_shift(cart, d, 1, &nbrs[2*d], &nbrs[2*d+1]);
for (int s = 0; s < 6; s++) {
    sbuf[s]     = rank * 6 + s;
    expected[s] = nbrs[s] * 6 + (s ^ 1);   /* MPI-4.1 Example 8.10 */
}
```

Full source attached below. `mpicc neighb_dup.c && mpiexec -n 4 ./a.out`:

```
MPI_Ineighbor_alltoall: rank 0 block 0 is 0, expected 1
MPI_Ineighbor_alltoall: rank 0 block 1 is 1, expected 0
MPI_Ineighbor_alltoall: rank 0 block 2 is 2, expected 3
MPI_Ineighbor_alltoall: rank 0 block 3 is 3, expected 2
...
FAIL: 16 mismatched blocks
```

`MPI_Neighbor_alltoall`, checked in the same program immediately before, produces
no mismatches at all.

### Expected versus actual

MPI-4.1 Example 8.10 fixes the matching: the block sent in the negative direction
of dimension `d` is received into block `2*d+1` of the neighbour, and vice versa.
So block `s` of the sender lands in block `s^1` of the receiver — `s^1` swapping
0 with 1 and 2 with 3. The nonblocking path uses the identity instead.

### Versions

| version | `MPI_Neighbor_alltoall` | `MPI_Ineighbor_alltoall` |
|---|---|---|
| 5.0.6 | passes | fails |
| 5.0.10 | passes | fails |
| 6.1.0a1 (`main`) | passes | fails |

macOS/arm64 in each case; nothing about the reproducer is platform-specific.

### How this was found

MPICH fixed the same defect on its side in 5.0.2rc2 (`6d7c89b05`, "coll: fix
block matching of duplicate neighbor edges") and added
`test/mpi/coll/neighb_dup_edges.c` with the fix. That test now fails against Open
MPI at its `MPI_Ineighbor_*` checks and passes at its blocking ones. The
reproducer above is the Cartesian half of it, reduced to remove the MPICH test
harness so it builds against any MPI.

The MPICH test also exercises `MPI_Ineighbor_alltoallv` and
`MPI_Ineighbor_alltoallw`, which fail the same way, and a
`MPI_Dist_graph_create_adjacent` neighbourhood; only the `alltoall` form is
reduced here.
