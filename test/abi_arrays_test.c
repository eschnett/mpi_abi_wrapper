/* abi_arrays_test -- the black-box half of S3's first part.
 *
 * The same shape as abi_prototype_test: an ordinary MPI application over the
 * ABI header, linking libmpi_abi and nothing else. What it adds is the classes
 * S3 taught the generator -- out and inout arrays, status arrays, the graph
 * topologies, the extents apis.json records as `*`, and above all the
 * **lifetime** of a staged temporary, which is the one property none of the
 * generator's own assertions can see (HISTORY.md, S3).
 *
 * Three of the tests below are written to fail rather than to pass, in the
 * sense that they are the shapes a plausible-but-wrong body gets wrong:
 *
 *  - a persistent collective started more than once, where a body that freed
 *    its staged datatype arrays at the first completion reads freed memory on
 *    the second MPI_Start;
 *  - the same, run past the staged table's capacity, where a body that never
 *    releases runs the table out and starts answering MPI_ERR_INTERN;
 *  - MPI_Group_range_incl with a *negative stride*, where a body that mapped
 *    the third column of the triplet as a rank -- which is what apis.json's
 *    kind says it is -- would hand MPICH a stride of -2 for the -1 it was
 *    given, because -1 is MPI_ANY_SOURCE in the ABI.
 */

#include <mpi.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "expect_ranks.h"

static int failures;
static int rank, size, peer;

#define CHECK(cond, ...)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      ++failures;                                                              \
      printf("[%d] FAIL %s:%d: ", rank, __FILE__, __LINE__);                   \
      printf(__VA_ARGS__);                                                     \
      printf("\n");                                                            \
      fflush(stdout);                                                          \
    }                                                                          \
  } while (0)

#define CHECK_MPI(call)                                                        \
  do {                                                                         \
    const int ierror_ = (call);                                                \
    CHECK(ierror_ == MPI_SUCCESS, "%s returned %d", #call, ierror_);           \
  } while (0)

/* An entry point the implementation does not have keeps its slot and reports
 * at run time (decision 6), so a test for one of those has to be able to skip
 * rather than to fail. Nothing else is allowed to answer this way.
 */
static int unsupported(int ierror, const char *what)
{
  if (ierror != MPI_ERR_UNSUPPORTED_OPERATION) return 0;
  if (rank == 0) printf("  skipping %s: not in this implementation\n", what);
  return 1;
}

/* ------------------------------------------------ inout arrays of requests */

/* MPI_Waitany, MPI_Waitsome, MPI_Testall, MPI_Testany and MPI_Testsome: the
 * request array is staged in both directions, and the *some forms convert only
 * `outcount` statuses rather than the `incount` they allocated.
 */
static void test_request_arrays(void)
{
  int         out[4], in[4];
  MPI_Request requests[8];
  MPI_Status  statuses[8];

  for (int i = 0; i < 4; ++i) {
    out[i] = rank * 100 + i;
    in[i]  = -1;
  }

  /* MPI_Waitany, one completion at a time, and the index it reports. */
  for (int i = 0; i < 4; ++i) {
    CHECK_MPI(MPI_Irecv(&in[i], 1, MPI_INT, peer, 20 + i, MPI_COMM_WORLD,
                        &requests[i]));
    CHECK_MPI(MPI_Isend(&out[i], 1, MPI_INT, peer, 20 + i, MPI_COMM_WORLD,
                        &requests[4 + i]));
  }
  for (int done = 0; done < 8; ++done) {
    int        index = -1;
    MPI_Status status;
    CHECK_MPI(MPI_Waitany(8, requests, &index, &status));
    CHECK(index >= 0 && index < 8, "MPI_Waitany reported index %d", index);
    if (index >= 0 && index < 8)
      CHECK(requests[index] == MPI_REQUEST_NULL,
            "MPI_Waitany left requests[%d] non-null", index);
  }
  for (int i = 0; i < 4; ++i)
    CHECK(in[i] == peer * 100 + i, "MPI_Waitany: in[%d] is %d, expected %d", i,
          in[i], peer * 100 + i);

  /* MPI_Waitany over an all-null array answers MPI_UNDEFINED, which is the one
   * value in `index` that is a mapped constant rather than a position.
   */
  {
    int        index = 0;
    MPI_Status status;
    CHECK_MPI(MPI_Waitany(8, requests, &index, &status));
    CHECK(index == MPI_UNDEFINED, "MPI_Waitany over null requests reported %d",
          index);
  }

  /* MPI_Waitsome: the status array is allocated for `incount` and converted
   * for `outcount`. */
  for (int i = 0; i < 4; ++i) in[i] = -1;
  for (int i = 0; i < 4; ++i) {
    CHECK_MPI(MPI_Irecv(&in[i], 1, MPI_INT, peer, 30 + i, MPI_COMM_WORLD,
                        &requests[i]));
    CHECK_MPI(MPI_Isend(&out[i], 1, MPI_INT, peer, 30 + i, MPI_COMM_WORLD,
                        &requests[4 + i]));
  }
  int completed = 0;
  while (completed < 8) {
    int outcount  = 0;
    int indices[8];
    CHECK_MPI(MPI_Waitsome(8, requests, &outcount, indices, statuses));
    CHECK(outcount > 0 && outcount <= 8 - completed,
          "MPI_Waitsome reported outcount %d", outcount);
    if (outcount <= 0) break;
    for (int i = 0; i < outcount; ++i) {
      const int idx = indices[i];
      CHECK(idx >= 0 && idx < 8, "MPI_Waitsome index %d out of range", idx);
      if (idx >= 0 && idx < 4)
        CHECK(statuses[i].MPI_TAG == 30 + idx,
              "MPI_Waitsome status %d has tag %d, expected %d", i,
              statuses[i].MPI_TAG, 30 + idx);
    }
    completed += outcount;
  }
  for (int i = 0; i < 4; ++i)
    CHECK(in[i] == peer * 100 + i, "MPI_Waitsome: in[%d] is %d, expected %d", i,
          in[i], peer * 100 + i);

  /* MPI_Waitsome over an all-null array answers MPI_UNDEFINED in outcount, and
   * the wrapper must convert *no* statuses -- which it can only do by clamping
   * a negative outcount to zero.
   */
  {
    int outcount = 0;
    int indices[8];
    CHECK_MPI(MPI_Waitsome(8, requests, &outcount, indices,
                           MPI_STATUSES_IGNORE));
    CHECK(outcount == MPI_UNDEFINED,
          "MPI_Waitsome over null requests reported outcount %d", outcount);
  }

  /* MPI_Testall and MPI_Testsome, including the MPI_STATUSES_IGNORE path. */
  for (int i = 0; i < 4; ++i) in[i] = -1;
  for (int i = 0; i < 4; ++i) {
    CHECK_MPI(MPI_Irecv(&in[i], 1, MPI_INT, peer, 40 + i, MPI_COMM_WORLD,
                        &requests[i]));
    CHECK_MPI(MPI_Isend(&out[i], 1, MPI_INT, peer, 40 + i, MPI_COMM_WORLD,
                        &requests[4 + i]));
  }
  int flag = 0;
  while (!flag) CHECK_MPI(MPI_Testall(8, requests, &flag, statuses));
  for (int i = 0; i < 4; ++i)
    CHECK(in[i] == peer * 100 + i, "MPI_Testall: in[%d] is %d, expected %d", i,
          in[i], peer * 100 + i);
  for (int i = 0; i < 8; ++i)
    CHECK(requests[i] == MPI_REQUEST_NULL,
          "MPI_Testall left requests[%d] non-null", i);

  /* MPI_Testany over an already-drained array: flag true, index MPI_UNDEFINED. */
  {
    int        index = 0;
    MPI_Status status;
    flag = 0;
    CHECK_MPI(MPI_Testany(8, requests, &index, &flag, &status));
    CHECK(flag && index == MPI_UNDEFINED,
          "MPI_Testany over null requests reported flag %d index %d", flag,
          index);
  }
  {
    int outcount = 0;
    int indices[8];
    flag = 0;
    CHECK_MPI(MPI_Testsome(8, requests, &outcount, indices,
                           MPI_STATUSES_IGNORE));
    CHECK(outcount == MPI_UNDEFINED,
          "MPI_Testsome over null requests reported outcount %d", outcount);
  }

  /* Zero requests: no allocation, nothing touched. */
  CHECK_MPI(MPI_Waitall(0, requests, MPI_STATUSES_IGNORE));
  CHECK_MPI(MPI_Startall(0, requests));
}

/* MPI_Startall over persistent point-to-point requests, which the same inout
 * request array class covers -- and where the implementation does *not* null
 * the handles, so the release rule must leave them alone.
 */
static void test_startall(void)
{
  int         out[2] = {rank * 7, rank * 7 + 1};
  int         in[2]  = {-1, -1};
  MPI_Request requests[4];

  CHECK_MPI(MPI_Recv_init(&in[0], 1, MPI_INT, peer, 50, MPI_COMM_WORLD,
                          &requests[0]));
  CHECK_MPI(MPI_Recv_init(&in[1], 1, MPI_INT, peer, 51, MPI_COMM_WORLD,
                          &requests[1]));
  CHECK_MPI(MPI_Send_init(&out[0], 1, MPI_INT, peer, 50, MPI_COMM_WORLD,
                          &requests[2]));
  CHECK_MPI(MPI_Send_init(&out[1], 1, MPI_INT, peer, 51, MPI_COMM_WORLD,
                          &requests[3]));

  for (int round = 0; round < 3; ++round) {
    in[0] = in[1] = -1;
    CHECK_MPI(MPI_Startall(4, requests));
    for (int i = 0; i < 4; ++i)
      CHECK(requests[i] != MPI_REQUEST_NULL,
            "MPI_Startall nulled requests[%d]", i);
    CHECK_MPI(MPI_Waitall(4, requests, MPI_STATUSES_IGNORE));
    for (int i = 0; i < 4; ++i)
      CHECK(requests[i] != MPI_REQUEST_NULL,
            "a completion nulled the persistent requests[%d]", i);
    CHECK(in[0] == peer * 7 && in[1] == peer * 7 + 1,
          "round %d received %d, %d", round, in[0], in[1]);
  }

  for (int i = 0; i < 4; ++i) {
    CHECK_MPI(MPI_Request_free(&requests[i]));
    CHECK(requests[i] == MPI_REQUEST_NULL,
          "MPI_Request_free left requests[%d] non-null", i);
  }
}

/* ------------------------------------------- temporaries outliving the call */

/* The eight *alltoallw* forms whose staged datatype arrays the implementation
 * may keep reading after the call returns. Two lifetimes, one rule: the block
 * dies when the implementation nulls the handle, which is at completion for a
 * nonblocking operation and at MPI_Request_free for a persistent one.
 */
static void test_persistent_alltoallw(void)
{
  const int     n       = size;
  int          *sendbuf = malloc((size_t)n * sizeof(int));
  int          *recvbuf = malloc((size_t)n * sizeof(int));
  int          *counts  = malloc((size_t)n * sizeof(int));
  int          *displs  = malloc((size_t)n * sizeof(int));
  MPI_Datatype *types   = malloc((size_t)n * sizeof(MPI_Datatype));
  MPI_Request   request = MPI_REQUEST_NULL;

  for (int i = 0; i < n; ++i) {
    sendbuf[i] = rank * 100 + i;
    counts[i]  = 1;
    displs[i]  = i * (int)sizeof(int);
    types[i]   = MPI_INT;
  }

  const int ierror = MPI_Alltoallw_init(sendbuf, counts, displs, types, recvbuf,
                                        counts, displs, types, MPI_COMM_WORLD,
                                        MPI_INFO_NULL, &request);
  if (unsupported(ierror, "MPI_Alltoallw_init")) goto done;
  CHECK(ierror == MPI_SUCCESS, "MPI_Alltoallw_init returned %d", ierror);
  if (ierror != MPI_SUCCESS) goto done;

  /* Overwriting the caller's array must not disturb the operation: what the
   * implementation reads is our copy, and for a persistent request it may read
   * it at every MPI_Start until MPI_Request_free.
   */
  for (int i = 0; i < n; ++i) types[i] = MPI_DATATYPE_NULL;

  /* Three rounds, and the second is the one that matters: a body that freed
   * the block at the first completion would be reading freed memory here.
   */
  for (int round = 0; round < 3; ++round) {
    for (int i = 0; i < n; ++i) recvbuf[i] = -1;
    CHECK_MPI(MPI_Start(&request));
    CHECK(request != MPI_REQUEST_NULL, "MPI_Start nulled a persistent request");
    CHECK_MPI(MPI_Wait(&request, MPI_STATUS_IGNORE));
    CHECK(request != MPI_REQUEST_NULL,
          "a completion nulled the persistent request in round %d", round);
    for (int i = 0; i < n; ++i)
      CHECK(recvbuf[i] == i * 100 + rank,
            "round %d: recvbuf[%d] is %d, expected %d", round, i, recvbuf[i],
            i * 100 + rank);
  }
  CHECK_MPI(MPI_Request_free(&request));
  CHECK(request == MPI_REQUEST_NULL, "MPI_Request_free left a request non-null");

  /* Whether the block is released at MPI_Request_free is not observable one
   * call at a time, and is in bulk: the staged-request table holds 1024
   * entries, so a body that never released would run it out. Same trick as
   * abi_prototype_test's nonblocking round, aimed at the other lifetime -- and
   * unlike that one, this loop bites at any rank count, because a persistent
   * request is never complete on return and so never takes NOTES.md #13.2's
   * (b) exit. That is the property the round above checks directly.
   */
  for (int i = 0; i < n; ++i) types[i] = MPI_INT;
  for (int round = 0; round < 1200; ++round) {
    const int e = MPI_Alltoallw_init(sendbuf, counts, displs, types, recvbuf,
                                     counts, displs, types, MPI_COMM_WORLD,
                                     MPI_INFO_NULL, &request);
    if (e != MPI_SUCCESS) {
      CHECK(0, "MPI_Alltoallw_init returned %d in round %d -- staged "
               "temporaries are not being released at MPI_Request_free", e,
            round);
      break;
    }
    CHECK_MPI(MPI_Request_free(&request));
  }

  /* MPI_IN_PLACE makes sendcounts, sdispls and sendtypes *ignored*, and a
   * legal program may pass a null pointer for all three -- so the wrapper must
   * not read the datatype array it would otherwise stage. Both implementations
   * accept the call with those arguments null (dev/generate.py's
   * IN_PLACE_IGNORES records the measurement), so this is what a wrapper that
   * dereferenced them would segfault on.
   */
  for (int i = 0; i < n; ++i) recvbuf[i] = rank * 100 + i;
  const int e = MPI_Alltoallw(MPI_IN_PLACE, NULL, NULL, NULL, recvbuf, counts,
                              displs, types, MPI_COMM_WORLD);
  CHECK(e == MPI_SUCCESS, "in-place MPI_Alltoallw returned %d", e);
  for (int i = 0; i < n; ++i)
    CHECK(recvbuf[i] == i * 100 + rank,
          "in-place MPI_Alltoallw: recvbuf[%d] is %d, expected %d", i,
          recvbuf[i], i * 100 + rank);

done:
  free(types);
  free(displs);
  free(counts);
  free(recvbuf);
  free(sendbuf);
}

/* ------------------------------------------------------- graph topologies */

/* MPI_Graph_create's `edges` is as long as the last entry of its index array
 * says -- an extent that is an expression over two other parameters and the
 * one the generator computes itself. MPI_Graph_get and MPI_Graph_neighbors
 * then read arrays back, with the caller's maximum and the topology's actual
 * extent being different numbers.
 */
static void test_graph(void)
{
  /* A ring at two ranks, a single node with no edges at one -- which also
   * exercises a zero-length edge array. */
  int index[2], edges[2];
  int nnodes = size;
  if (size == 2) {
    index[0] = 1;
    index[1] = 2;
    edges[0] = 1;
    edges[1] = 0;
  } else {
    index[0] = 0;
  }
  const int nedges = index[nnodes - 1];

  MPI_Comm graph = MPI_COMM_NULL;
  CHECK_MPI(MPI_Graph_create(MPI_COMM_WORLD, nnodes, index, edges, 0, &graph));
  if (graph == MPI_COMM_NULL) return;

  int topo = MPI_UNDEFINED;
  CHECK_MPI(MPI_Topo_test(graph, &topo));
  CHECK(topo == MPI_GRAPH, "MPI_Topo_test reported %d, expected MPI_GRAPH",
        topo);

  int gnodes = 0, gedges = 0;
  CHECK_MPI(MPI_Graphdims_get(graph, &gnodes, &gedges));
  CHECK(gnodes == nnodes && gedges == nedges,
        "MPI_Graphdims_get reported %d nodes and %d edges, expected %d and %d",
        gnodes, gedges, nnodes, nedges);

  /* Deliberately over-sized, so that the wrapper has to map exactly the
   * entries the implementation wrote and leave the tail alone -- and the tail
   * is seeded with -2 rather than an arbitrary number, because -2 is
   * MPI_ANY_SOURCE to MPICH and MPI_PROC_NULL to Open MPI. A wrapper that
   * mapped the whole of maxedges would turn it into -1 on one and -3 on the
   * other, so this fill is what makes the check discriminate at all.
   */
  int got_index[8], got_edges[8];
  for (int i = 0; i < 8; ++i) got_index[i] = got_edges[i] = -2;
  CHECK_MPI(MPI_Graph_get(graph, 8, 8, got_index, got_edges));
  for (int i = 0; i < nnodes; ++i)
    CHECK(got_index[i] == index[i], "MPI_Graph_get index[%d] is %d, expected %d",
          i, got_index[i], index[i]);
  for (int i = 0; i < nedges; ++i)
    CHECK(got_edges[i] == edges[i], "MPI_Graph_get edges[%d] is %d, expected %d",
          i, got_edges[i], edges[i]);
  for (int i = nedges; i < 8; ++i)
    CHECK(got_edges[i] == -2,
          "MPI_Graph_get mapped past the topology's %d edges: [%d] is %d",
          nedges, i, got_edges[i]);

  int nneighbors = -1;
  CHECK_MPI(MPI_Graph_neighbors_count(graph, rank, &nneighbors));
  CHECK(nneighbors == (size == 2 ? 1 : 0),
        "MPI_Graph_neighbors_count reported %d", nneighbors);
  int neighbors[8];
  for (int i = 0; i < 8; ++i) neighbors[i] = -2;
  CHECK_MPI(MPI_Graph_neighbors(graph, rank, 8, neighbors));
  for (int i = 0; i < nneighbors; ++i)
    CHECK(neighbors[i] == peer, "MPI_Graph_neighbors[%d] is %d, expected %d", i,
          neighbors[i], peer);
  for (int i = nneighbors; i < 8; ++i)
    CHECK(neighbors[i] == -2,
          "MPI_Graph_neighbors mapped past the %d it wrote: [%d] is %d",
          nneighbors, i, neighbors[i]);

  int newrank = -1;
  CHECK_MPI(MPI_Graph_map(graph, nnodes, index, edges, &newrank));
  CHECK(newrank >= 0 && newrank < size, "MPI_Graph_map answered %d", newrank);

  CHECK_MPI(MPI_Comm_free(&graph));
}

/* The distributed graph, its weight sentinels, and a neighbourhood alltoallw
 * over it -- where the send and receive arrays are sized by two *different*
 * degrees that only the topology knows.
 */
static void test_dist_graph(void)
{
  const int sources[1]      = {peer};
  const int destinations[1] = {peer};
  const int degrees[1]      = {1};

  /* MPI_UNWEIGHTED is (int *)10 in the ABI, and GCC's -Wstringop-overread sees
   * a four-byte read from a zero-sized object at address 10 -- which is what
   * every sentinel pointer looks like to it, and what an implementation's own
   * `const int sourceweights[]` prototype invites. Reading the value through a
   * volatile passes exactly the same address and denies the diagnostic the
   * constant. Found by ci-scripts/run-linux-docker.sh; clang on macOS does not
   * warn.
   */
  int *volatile unweighted = MPI_UNWEIGHTED;

  MPI_Comm adjacent = MPI_COMM_NULL;
  CHECK_MPI(MPI_Dist_graph_create_adjacent(
      MPI_COMM_WORLD, 1, sources, unweighted, 1, destinations,
      unweighted, MPI_INFO_NULL, 0, &adjacent));
  if (adjacent == MPI_COMM_NULL) return;

  int indegree = 0, outdegree = 0, weighted = -1;
  CHECK_MPI(MPI_Dist_graph_neighbors_count(adjacent, &indegree, &outdegree,
                                           &weighted));
  CHECK(indegree == 1 && outdegree == 1,
        "MPI_Dist_graph_neighbors_count reported %d in, %d out", indegree,
        outdegree);

  /* No tail assertion here, unlike MPI_Graph_get above: MPICH zero-fills the
   * whole of maxindegree itself, so the tail says nothing about what the
   * wrapper did with it. The two graph queries above cover that property.
   */
  int got_sources[4], got_destinations[4];
  for (int i = 0; i < 4; ++i) got_sources[i] = got_destinations[i] = -2;
  CHECK_MPI(MPI_Dist_graph_neighbors(adjacent, 4, got_sources, unweighted, 4,
                                     got_destinations, unweighted));
  CHECK(got_sources[0] == peer && got_destinations[0] == peer,
        "MPI_Dist_graph_neighbors reported source %d and destination %d",
        got_sources[0], got_destinations[0]);

  /* MPI_Neighbor_alltoallw: one datatype per neighbour in each direction, and
   * the two arrays are sized by the two degrees rather than by any argument.
   */
  int          sendbuf[1] = {rank * 1000};
  int          recvbuf[1] = {-1};
  int          counts[1]  = {1};
  MPI_Aint     displs[1]  = {0};
  MPI_Datatype types[1]   = {MPI_INT};
  CHECK_MPI(MPI_Neighbor_alltoallw(sendbuf, counts, displs, types, recvbuf,
                                   counts, displs, types, adjacent));
  CHECK(recvbuf[0] == peer * 1000, "MPI_Neighbor_alltoallw received %d, "
                                   "expected %d", recvbuf[0], peer * 1000);

  /* The nonblocking form stages past its return, so overwriting the caller's
   * datatype array while the operation is in flight must change nothing.
   */
  MPI_Request request = MPI_REQUEST_NULL;
  recvbuf[0]          = -1;
  CHECK_MPI(MPI_Ineighbor_alltoallw(sendbuf, counts, displs, types, recvbuf,
                                    counts, displs, types, adjacent, &request));
  types[0] = MPI_DATATYPE_NULL;
  CHECK_MPI(MPI_Wait(&request, MPI_STATUS_IGNORE));
  CHECK(recvbuf[0] == peer * 1000, "MPI_Ineighbor_alltoallw received %d, "
                                   "expected %d", recvbuf[0], peer * 1000);
  types[0] = MPI_INT;

  CHECK_MPI(MPI_Comm_free(&adjacent));

  /* MPI_Dist_graph_create's destinations array is as long as the degrees sum
   * to, which is the extent the wrapper adds up itself.
   */
  MPI_Comm dist = MPI_COMM_NULL;
  CHECK_MPI(MPI_Dist_graph_create(MPI_COMM_WORLD, 1, &rank, degrees,
                                  destinations, unweighted, MPI_INFO_NULL, 0,
                                  &dist));
  if (dist != MPI_COMM_NULL) {
    int in2 = 0, out2 = 0, w2 = -1;
    CHECK_MPI(MPI_Dist_graph_neighbors_count(dist, &in2, &out2, &w2));
    CHECK(in2 == 1 && out2 == 1,
          "MPI_Dist_graph_create produced %d in, %d out", in2, out2);
    CHECK_MPI(MPI_Comm_free(&dist));
  }
}

/* --------------------------------------------------------- rank arrays out */

static void test_group_arrays(void)
{
  MPI_Group world = MPI_GROUP_NULL, first = MPI_GROUP_NULL;
  CHECK_MPI(MPI_Comm_group(MPI_COMM_WORLD, &world));

  /* MPI_Group_translate_ranks answers MPI_UNDEFINED for a rank that is not in
   * the second group, which is the out-direction sentinel that makes an out
   * rank array a mapped array rather than a passthrough (NOTES.md #5.4).
   */
  const int ranks1[2] = {0, size - 1};
  int       ranks2[2] = {-12345, -12345};
  const int only0[1]  = {0};
  CHECK_MPI(MPI_Group_incl(world, 1, only0, &first));
  CHECK_MPI(MPI_Group_translate_ranks(world, 2, ranks1, first, ranks2));
  CHECK(ranks2[0] == 0, "MPI_Group_translate_ranks mapped rank 0 to %d",
        ranks2[0]);
  if (size == 2)
    CHECK(ranks2[1] == MPI_UNDEFINED,
          "MPI_Group_translate_ranks answered %d for a rank outside the group, "
          "expected MPI_UNDEFINED", ranks2[1]);

  /* The triplet array, with a negative stride. apis.json calls the whole
   * triplet a RANK; mapping its third column would turn a stride of -1 into
   * MPICH's MPI_ANY_SOURCE and the call would fail or select the wrong ranks.
   */
  MPI_Group reversed = MPI_GROUP_NULL;
  int       ranges[1][3];
  ranges[0][0] = size - 1;
  ranges[0][1] = 0;
  ranges[0][2] = -1;
  CHECK_MPI(MPI_Group_range_incl(world, 1, ranges, &reversed));
  if (reversed != MPI_GROUP_NULL) {
    int n = 0;
    CHECK_MPI(MPI_Group_size(reversed, &n));
    CHECK(n == size, "MPI_Group_range_incl with a -1 stride selected %d of %d "
                     "ranks", n, size);
    if (n == size) {
      const int all[2] = {0, size - 1};
      int       back[2] = {-12345, -12345};
      CHECK_MPI(MPI_Group_translate_ranks(reversed, size, all, world, back));
      /* Reversed, so position 0 of the new group is the last world rank. */
      CHECK(back[0] == size - 1,
            "MPI_Group_range_incl did not reverse: position 0 is world rank %d",
            back[0]);
    }
    CHECK_MPI(MPI_Group_free(&reversed));
  }

  MPI_Group excluded = MPI_GROUP_NULL;
  ranges[0][0] = 0;
  ranges[0][1] = 0;
  ranges[0][2] = 1;
  CHECK_MPI(MPI_Group_range_excl(world, 1, ranges, &excluded));
  if (excluded != MPI_GROUP_NULL) {
    int n = -1;
    CHECK_MPI(MPI_Group_size(excluded, &n));
    CHECK(n == size - 1, "MPI_Group_range_excl left %d of %d ranks", n, size);
    CHECK_MPI(MPI_Group_free(&excluded));
  }

  CHECK_MPI(MPI_Group_free(&first));
  CHECK_MPI(MPI_Group_free(&world));
}

/* --------------------------------------------------- out arrays of handles */

/* MPI_Type_get_contents writes as many datatypes as the envelope says and
 * leaves the rest of the caller's array alone -- and each one it does write is
 * an implementation handle that has to come back as the ABI handle the caller
 * put in.
 */
static void test_type_get_contents(void)
{
  const int          blocklengths[2] = {1, 1};
  const MPI_Aint     displacements[2] = {0, (MPI_Aint)sizeof(int)};
  const MPI_Datatype types[2]        = {MPI_INT, MPI_DOUBLE};
  MPI_Datatype       newtype         = MPI_DATATYPE_NULL;

  CHECK_MPI(MPI_Type_create_struct(2, blocklengths, displacements, types,
                                   &newtype));
  CHECK_MPI(MPI_Type_commit(&newtype));

  int ni = 0, na = 0, nd = 0, combiner = 0;
  CHECK_MPI(MPI_Type_get_envelope(newtype, &ni, &na, &nd, &combiner));
  CHECK(combiner == MPI_COMBINER_STRUCT, "MPI_Type_get_envelope reported "
                                         "combiner %d", combiner);
  CHECK(nd == 2, "MPI_Type_get_envelope reported %d datatypes, expected 2", nd);

  int          got_int[8];
  MPI_Aint     got_addr[8];
  MPI_Datatype got_types[8];
  for (int i = 0; i < 8; ++i) got_types[i] = (MPI_Datatype)0x7f;
  CHECK_MPI(MPI_Type_get_contents(newtype, ni, na, 8, got_int, got_addr,
                                  got_types));
  CHECK(got_types[0] == MPI_INT && got_types[1] == MPI_DOUBLE,
        "MPI_Type_get_contents returned datatypes that are not MPI_INT and "
        "MPI_DOUBLE");
  for (int i = nd; i < 8; ++i)
    CHECK(got_types[i] == (MPI_Datatype)0x7f,
          "MPI_Type_get_contents converted past the %d datatypes it wrote", nd);

  CHECK_MPI(MPI_Type_free(&newtype));
}

/* ------------------------------------------------------ status accessors */

/* The six pure ABI-side accessors: they read and write named fields of the
 * caller's own status and never reach the implementation, so they have to
 * agree with what a receive put there and must leave the private blob -- which
 * MPI_Get_count reads -- untouched.
 */
static void test_status_accessors(void)
{
  int        out[3] = {1, 2, 3};
  int        in[3]  = {0, 0, 0};
  MPI_Status status;
  memset(&status, 0, sizeof status);

  MPI_Request requests[2];
  CHECK_MPI(MPI_Irecv(in, 3, MPI_INT, peer, 60, MPI_COMM_WORLD, &requests[0]));
  CHECK_MPI(MPI_Isend(out, 3, MPI_INT, peer, 60, MPI_COMM_WORLD, &requests[1]));
  CHECK_MPI(MPI_Wait(&requests[0], &status));
  CHECK_MPI(MPI_Wait(&requests[1], MPI_STATUS_IGNORE));

  int source = -1, tag = -1, error = -1;
  CHECK_MPI(MPI_Status_get_source(&status, &source));
  CHECK_MPI(MPI_Status_get_tag(&status, &tag));
  CHECK_MPI(MPI_Status_get_error(&status, &error));
  CHECK(source == peer, "MPI_Status_get_source answered %d, expected %d",
        source, peer);
  CHECK(tag == 60, "MPI_Status_get_tag answered %d, expected 60", tag);
  CHECK(source == status.MPI_SOURCE && tag == status.MPI_TAG &&
            error == status.MPI_ERROR,
        "the accessors disagree with the fields they read");

  int count = -1;
  CHECK_MPI(MPI_Get_count(&status, MPI_INT, &count));
  CHECK(count == 3, "MPI_Get_count answered %d, expected 3", count);

  /* Setting a named field must not disturb the implementation's private bytes,
   * so MPI_Get_count still answers from the same blob afterwards.
   */
  CHECK_MPI(MPI_Status_set_source(&status, MPI_PROC_NULL));
  CHECK_MPI(MPI_Status_set_tag(&status, MPI_ANY_TAG));
  CHECK_MPI(MPI_Status_set_error(&status, MPI_ERR_TRUNCATE));
  CHECK_MPI(MPI_Status_get_source(&status, &source));
  CHECK_MPI(MPI_Status_get_tag(&status, &tag));
  CHECK_MPI(MPI_Status_get_error(&status, &error));
  CHECK(source == MPI_PROC_NULL && tag == MPI_ANY_TAG &&
            error == MPI_ERR_TRUNCATE,
        "a set/get round trip gave %d, %d, %d", source, tag, error);

  count = -1;
  CHECK_MPI(MPI_Get_count(&status, MPI_INT, &count));
  CHECK(count == 3, "MPI_Get_count answered %d after the field writes, "
                    "expected 3", count);
}

/* ------------------------------------------------------- external packing */

/* `datarep` is a string that apis.json spells as an array, and `position` is
 * an inout byte offset -- the two things that kept these four out of S2.
 */
static void test_pack_external(void)
{
  int      in[3] = {rank, rank + 1, rank + 2};
  int      out[3] = {-1, -1, -1};
  MPI_Aint size_needed = 0;

  int ierror = MPI_Pack_external_size("external32", 3, MPI_INT, &size_needed);
  if (unsupported(ierror, "MPI_Pack_external_size")) return;
  CHECK(ierror == MPI_SUCCESS, "MPI_Pack_external_size returned %d", ierror);
  if (ierror != MPI_SUCCESS || size_needed <= 0) return;

  char *buffer = malloc((size_t)size_needed);
  MPI_Aint position = 0;
  CHECK_MPI(MPI_Pack_external("external32", in, 3, MPI_INT, buffer,
                              size_needed, &position));
  CHECK(position > 0, "MPI_Pack_external left position at %lld",
        (long long)position);

  position = 0;
  CHECK_MPI(MPI_Unpack_external("external32", buffer, size_needed, &position,
                                out, 3, MPI_INT));
  for (int i = 0; i < 3; ++i)
    CHECK(out[i] == in[i], "external32 round trip gave out[%d] = %d, expected "
                           "%d", i, out[i], in[i]);
  free(buffer);
}

int main(int argc, char **argv)
{
  CHECK_MPI(MPI_Init(&argc, &argv));
  CHECK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &size));
  CHECK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));

  if (size < 1 || size > 2) {
    if (rank == 0)
      printf("abi_arrays_test runs at 1 or 2 ranks, got %d\n", size);
    MPI_Finalize();
    return 1;
  }
  peer = (size == 2) ? 1 - rank : rank;

  if (mpiabi_expect_ranks("abi_arrays_test", size, rank)) {
    MPI_Finalize();
    return 1;
  }

  if (rank == 0)
    printf("abi_arrays_test: %d rank%s\n", size, size == 1 ? "" : "s");

  test_request_arrays();
  test_startall();
  test_persistent_alltoallw();
  test_graph();
  test_dist_graph();
  test_group_arrays();
  test_type_get_contents();
  test_status_accessors();
  test_pack_external();

  int total = 0;
  CHECK_MPI(MPI_Allreduce(&failures, &total, 1, MPI_INT, MPI_SUM,
                          MPI_COMM_WORLD));
  if (rank == 0)
    printf("abi_arrays_test: %d failure(s) across %d ranks\n", total, size);

  CHECK_MPI(MPI_Finalize());
  return total != 0;
}
