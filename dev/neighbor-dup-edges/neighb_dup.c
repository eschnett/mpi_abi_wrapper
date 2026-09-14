/* Minimal standalone form of MPICH 5.0.2rc2's test/mpi/coll/neighb_dup_edges.c,
 * Cartesian part only, with no mtest dependency -- so it can be built against
 * any MPI's own mpicc with no wrapper involved.
 *
 * A 3-D grid with two periodic dimensions of size 1: four of the six blocks are
 * exchanged with self. MPI-4.1 Example 8.10 says the block sent in the negative
 * direction of dimension d is received into block 2*d+1 of the neighbor and
 * vice versa, so block s of the sender lands in block s^1 of the receiver.
 */
#include <stdio.h>
#include <string.h>
#include <mpi.h>
#define NDIMS 3
#define NSLOT (2 * NDIMS)
int main(int argc, char **argv)
{
    int rank, size, errs = 0;
    int nbrs[NSLOT], sbuf[NSLOT], rbuf[NSLOT], expected[NSLOT];
    MPI_Comm cart;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int dims[NDIMS] = { 1, 1, size }, periods[NDIMS] = { 1, 1, 1 };
    MPI_Cart_create(MPI_COMM_WORLD, NDIMS, dims, periods, 0, &cart);
    for (int d = 0; d < NDIMS; d++)
        MPI_Cart_shift(cart, d, 1, &nbrs[2 * d], &nbrs[2 * d + 1]);
    for (int s = 0; s < NSLOT; s++) {
        sbuf[s] = rank * NSLOT + s;
        expected[s] = nbrs[s] * NSLOT + (s ^ 1);
    }
#define CHECK(what_)                                                          \
    for (int s = 0; s < NSLOT; s++)                                           \
        if (rbuf[s] != expected[s]) {                                         \
            printf("%s: rank %d block %d is %d, expected %d\n",               \
                   what_, rank, s, rbuf[s], expected[s]);                     \
            errs++;                                                           \
        }

    memset(rbuf, 0, sizeof rbuf);
    MPI_Neighbor_alltoall(sbuf, 1, MPI_INT, rbuf, 1, MPI_INT, cart);
    CHECK("MPI_Neighbor_alltoall");

    MPI_Request req;
    memset(rbuf, 0, sizeof rbuf);
    MPI_Ineighbor_alltoall(sbuf, 1, MPI_INT, rbuf, 1, MPI_INT, cart, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    CHECK("MPI_Ineighbor_alltoall");
    int total = 0;
    MPI_Reduce(&errs, &total, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    if (rank == 0) printf(total ? "FAIL: %d mismatched blocks\n" : "No Errors\n", total);
    MPI_Comm_free(&cart);
    MPI_Finalize();
    return total != 0;
}
