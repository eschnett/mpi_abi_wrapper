**Filed as open-mpi/ompi#14430** on 2026-09-14. Kept here as the source of
that text, so a later edit has something to diff against.

A search of open-mpi/ompi found no existing report; the nearest neighbours are
#11756 (rank reorder in MPI_Dist_graph_create) and #10740 (Fortran
Neighbor_alltoallw datatype error), neither of which is this.

---

**Title:** `MPI_Ineighbor_alltoall` uses the wrong block matching for duplicate neighbour edges, where `MPI_Neighbor_alltoall` gets it right

**Body:**

For a Cartesian neighbourhood whose neighbour list contains the same process more
than once, `MPI_Ineighbor_alltoall` puts each arriving block in the wrong slot.
`MPI_Neighbor_alltoall` on the very same communicator puts it in the right one,
so the blocking and nonblocking paths of the library disagree with each other.

Reproduced on 5.0.6, 5.0.10 and `main` (6.1.0a1); 5.0.11 carries the same code.

### The topology, and what "slot" means

A 3-D Cartesian communicator, all three dimensions periodic, whose first two
dimensions have extent 1:

```c
int dims[3] = { 1, 1, size }, periods[3] = { 1, 1, 1 };
MPI_Cart_create(MPI_COMM_WORLD, 3, dims, periods, /*reorder=*/0, &cart);
for (int d = 0; d < 3; d++)
    MPI_Cart_shift(cart, d, 1, &nbrs[2*d], &nbrs[2*d+1]);
```

A Cartesian neighbourhood has 2 x ndims neighbours, ordered per dimension:
**slot `2d` is the negative-direction neighbour and slot `2d+1` the
positive-direction one** — exactly the pair `MPI_Cart_shift` returns for
dimension `d`. Call that index the *slot*.

Shifting along a periodic dimension of extent 1 lands on yourself, so slots 0-3
are all self here and only dimension 2 has real neighbours. At 4 ranks, rank 0's
neighbour list is

```
slot:  0  1  2  3  4  5
rank:  0  0  0  0  3  1
```

**That repetition is the point of the test.** Four slots name the same process, so
an implementation that matches an arriving block to a slot by *which rank sent
it* cannot tell those four apart; only the slot index distinguishes them.

### The matching rule

What I send in the negative direction of dimension `d` reaches that neighbour as
having come from *its* positive direction — so my slot `2d` block arrives in its
slot `2d+1`, and vice versa. The pairing is therefore always "the other slot of
the same dimension": 0 with 1, 2 with 3, 4 with 5. Since `2d` and `2d+1` differ
only in the low bit, that is `slot ^ 1`, which is the shorthand the code uses.
Nothing about this pairing is special to the topology above; the topology only
supplies the duplicate edges that make a mismatch observable.

So with `sendbuf[slot] = rank * 6 + slot`, the receive buffer is known in closed
form:

```c
for (int s = 0; s < 6; s++) {
    sbuf[s]     = rank * 6 + s;
    expected[s] = nbrs[s] * 6 + (s ^ 1);   /* their slot s^1 lands in my slot s */
}
```

### Reproducer

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

Open MPI's nonblocking path uses the identity instead: the block arrives in the
same slot it was sent from. In the output above rank 0's slot 0 holds `0`, which
is rank 0's own slot-0 value, where the standard says it should hold `1`, its
slot-1 value. MPI-4.1 Example 8.10 states the rule.

The self-edges are what make this visible at all: for slots 0-3 both the correct
and the incorrect answer come from rank 0, so the sender's identity cannot
distinguish them and only the payload can.

### Versions

| version | `MPI_Neighbor_alltoall` | `MPI_Ineighbor_alltoall` |
|---|---|---|
| 5.0.6 | passes | fails |
| 5.0.10 | passes | fails |
| 6.1.0a1 (`main`) | passes | fails |
| 5.0.11 | not run — `nbc_ineighbor_alltoall.c` is unchanged in the release | |

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
