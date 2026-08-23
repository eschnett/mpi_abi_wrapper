/* libmpiwrapper -- array extents the ABI call does not carry.
 *
 * Hand-written and permanent. Every one of these answers the same question for
 * a generated body: *how many elements* of an array parameter does this call
 * read or write? `apis.json` records the answer as the name of another
 * parameter wherever it is one, and the generator uses it directly. Where it
 * records `*` instead, the length is a property of an object rather than of the
 * argument list, and the only place to ask is the implementation.
 *
 * Two rules govern the file:
 *
 *  - **Every call here is `PMPI_`.** These are calls the wrapper makes for its
 *    own purposes, not application traffic, and a profiling tool interposed
 *    below us must not count them (NOTES.md #2).
 *  - **Each returns an implementation error code**, which the caller maps like
 *    any other. A generated body runs its extent queries *before* the call it
 *    wraps and returns the query's error if one fails; the two coincide in
 *    practice -- asking `MPI_Graph_get`'s edge count of a communicator with no
 *    graph topology fails with exactly the error `MPI_Graph_get` itself would
 *    have returned.
 */

#include "internal.h"

#include <limits.h>

/* The group whose size sizes MPI_Alltoallw's four vector arrays. On an
 * intercommunicator that is the *remote* group -- the kind of detail that used
 * to make this function hand-written.
 */
int mpiwrapper_comm_extent(MPI_Comm comm, int *n)
{
  int inter  = 0;
  int ierror = PMPI_Comm_test_inter(comm, &inter);
  if (ierror != MPI_SUCCESS) return ierror;
  return inter ? PMPI_Comm_remote_size(comm, n) : PMPI_Comm_size(comm, n);
}

/* The same group, but zero wherever the array is *not significant* -- which is
 * every rank but the root, for MPI_Gatherv's recvcounts/displs and
 * MPI_Scatterv's sendcounts/displs (MPI-5.0 6.6).
 *
 * This exists only for the large-count fallback (NOTES.md #5.10), and the
 * reason it has to is the sharpest hazard in that whole mechanism. A count
 * array crosses as a pointer cast normally, so nothing reads it and a non-root
 * rank may legally pass a null pointer or an uninitialized array -- and real
 * programs do, because the standard says they may. Narrowing has to read it.
 * Answering zero here is what keeps the staging loop from ever touching it.
 *
 * The intercommunicator case is not the intracommunicator one with a different
 * group. There, `root` is MPI_ROOT at the single root in the local group,
 * MPI_PROC_NULL at every other member of it, and a rank in the *other* group
 * at every member of the receiving one -- so the test is against MPI_ROOT and
 * never against this process's own rank, which would answer wrongly for both
 * of the other two cases.
 */
int mpiwrapper_root_extent(MPI_Comm comm, int root, int *n)
{
  int inter  = 0;
  int ierror = PMPI_Comm_test_inter(comm, &inter);
  if (ierror != MPI_SUCCESS) return ierror;

  *n = 0;
  if (inter)
    return root == MPI_ROOT ? PMPI_Comm_remote_size(comm, n) : MPI_SUCCESS;

  int rank = 0;
  ierror   = PMPI_Comm_rank(comm, &rank);
  if (ierror != MPI_SUCCESS) return ierror;
  return rank == root ? PMPI_Comm_size(comm, n) : MPI_SUCCESS;
}

/* The neighbourhood collectives' send and receive arrays are sized by the
 * number of outgoing and incoming neighbours, which depends on which of the
 * three topologies the communicator carries. A Cartesian communicator has
 * 2*ndims of each (some of them MPI_PROC_NULL, which is why the arrays are that
 * long and not shorter); a graph communicator has one degree serving both
 * directions; only a distributed graph distinguishes them.
 *
 * MPI_ERR_TOPOLOGY for a communicator with no topology is the error the
 * neighbourhood collective itself would return.
 */
int mpiwrapper_neighbor_extents(MPI_Comm comm, int *indegree, int *outdegree)
{
  int status = MPI_UNDEFINED;
  int ierror = PMPI_Topo_test(comm, &status);
  if (ierror != MPI_SUCCESS) return ierror;

  if (status == MPI_CART) {
    int ndims = 0;
    ierror    = PMPI_Cartdim_get(comm, &ndims);
    if (ierror != MPI_SUCCESS) return ierror;
    *indegree = *outdegree = 2 * ndims;
    return MPI_SUCCESS;
  }
  if (status == MPI_GRAPH) {
    int rank = 0, nneighbors = 0;
    ierror   = PMPI_Comm_rank(comm, &rank);
    if (ierror != MPI_SUCCESS) return ierror;
    ierror = PMPI_Graph_neighbors_count(comm, rank, &nneighbors);
    if (ierror != MPI_SUCCESS) return ierror;
    *indegree = *outdegree = nneighbors;
    return MPI_SUCCESS;
  }
  if (status == MPI_DIST_GRAPH) return mpiwrapper_dist_graph_extents(
      comm, indegree, outdegree);
  return MPI_ERR_TOPOLOGY;
}

/* MPI_Dist_graph_neighbors_count without its `weighted` answer, which no caller
 * here wants and which would otherwise need a dummy at every site.
 */
int mpiwrapper_dist_graph_extents(MPI_Comm comm, int *indegree, int *outdegree)
{
  int weighted = 0;
  return PMPI_Dist_graph_neighbors_count(comm, indegree, outdegree, &weighted);
}

/* MPI_Graphdims_get's second answer, for MPI_Graph_get's `edges`. */
int mpiwrapper_graph_nedges(MPI_Comm comm, int *nedges)
{
  int nnodes = 0;
  return PMPI_Graphdims_get(comm, &nnodes, nedges);
}

/* MPI_Graph_neighbors_count under this file's PMPI_ rule, for
 * MPI_Graph_neighbors' `neighbors`.
 */
int mpiwrapper_graph_nneighbors(MPI_Comm comm, int rank, int *nneighbors)
{
  return PMPI_Graph_neighbors_count(comm, rank, nneighbors);
}

/* How many datatypes MPI_Type_get_contents will write. Asked of the envelope,
 * which is the standard's own way to size the four arrays, and asked *before*
 * the call so that the staged array is converted element by element and the
 * caller's array is never written past what the implementation filled --
 * mapping the tail would convert uninitialized handles, which is both a wrong
 * answer and a sanitizer report.
 */
int mpiwrapper_type_ndatatypes(MPI_Datatype datatype, int *ndatatypes)
{
  int nintegers = 0, naddresses = 0, combiner = 0;
  return PMPI_Type_get_envelope(datatype, &nintegers, &naddresses, ndatatypes,
                                &combiner);
}

int mpiwrapper_type_ndatatypes_c(MPI_Datatype datatype, MPI_Count *ndatatypes)
{
#ifdef MPIWRAPPER_HAVE_MPI_Type_get_envelope_c
  MPI_Count nintegers = 0, naddresses = 0, nlarge_counts = 0;
  int       combiner  = 0;
  return PMPI_Type_get_envelope_c(datatype, &nintegers, &naddresses,
                                  &nlarge_counts, ndatatypes, &combiner);
#else
  /* Unreachable in any implementation that has MPI_Type_get_contents_c at all
   * -- both arrived in MPI-4.0 -- but the guard is what keeps this file
   * compiling against one that has neither, where the generated body it serves
   * is a stub anyway.
   */
  (void)datatype;
  *ndatatypes = 0;
  return MPI_ERR_ARG;
#endif
}

/* MPI_Dist_graph_create's `destinations` and `weights` are as long as the
 * degrees sum to, which is the one extent that is a property of the argument
 * list and still not a parameter. Rejecting a negative degree or an overflowing
 * sum here rather than allocating first is what keeps a bad argument list from
 * becoming a bad allocation size; the implementation would reject it too, but
 * only after we had already sized a temporary from it.
 */
int mpiwrapper_sum_degrees(const int *degrees, int n, int *total)
{
  long long sum = 0;

  if (n < 0) return 0;
  for (int i = 0; i < n; ++i) {
    if (degrees[i] < 0) return 0;
    sum += degrees[i];
    if (sum > INT_MAX) return 0;
  }
  *total = (int)sum;
  return 1;
}
