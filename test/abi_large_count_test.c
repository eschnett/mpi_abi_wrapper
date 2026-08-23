/* abi_large_count_test -- the ABI's large-count half, over an implementation
 * that may or may not have it.
 *
 * The ABI declares 159 `_c` entry points. MPICH >= 4.0 provides all of them;
 * no released Open MPI provides any, and there the wrapper narrows each call
 * onto its small twin and refuses only the values that will not fit
 * (NOTES.md #5.10). **This test is written to pass identically either way**,
 * which is what makes it an oracle rather than a description: the `_c` form
 * and its small twin must agree, and they must agree whether the agreement is
 * the implementation's doing or ours.
 *
 * The checks are the shapes a plausible-but-wrong fallback body gets wrong,
 * not the happy path:
 *
 *  - **a vector collective at a non-root rank passing a genuine NULL** for the
 *    arrays the standard makes insignificant there. A body that sized them by
 *    the group everywhere reads a null pointer, on a program that did nothing
 *    wrong. This is the sharpest hazard in the whole mechanism.
 *  - **a nonblocking vector collective whose caller overwrites its own count
 *    arrays the instant it is posted.** Illegal against a native MPI and
 *    deliberate here: we copy at initiation, so it must not change the answer,
 *    and it is what catches a body that handed the implementation the caller's
 *    array instead of its own.
 *  - **a persistent one started three times**, where a body that freed its
 *    staged arrays at the first completion reads freed memory on the second
 *    MPI_Start.
 *  - **a refused call's out parameter**, which must be defined rather than
 *    left as whatever the caller had there. Decision 6 says a report the
 *    caller can ignore into undefined behaviour is not a report, and a
 *    narrowing rejection is a report.
 *  - **a value that will not fit**, which must never come back as
 *    MPI_ERR_UNSUPPORTED_OPERATION. The two mean different things: one is
 *    permanent and one depends on the argument, and test/'s own unsupported()
 *    helper treats the former as "skip this check".
 */

#include <mpi.h>

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "expect_ranks.h"

static int failures;
static int rank, size;

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

/* What a call given a value too large for the implementation may answer. Over
 * an MPI with the `_c` entry point it simply succeeds -- the count is
 * perfectly ordinary there -- and over one without it the wrapper refuses with
 * MPI_ERR_VALUE_TOO_LARGE. MPI_ERR_UNSUPPORTED_OPERATION is the answer that
 * must not appear: the entry point is present in both configurations, and
 * conflating "this wrapper has no such call" with "your value will not fit"
 * is what NOTES.md #5.10 chose the separate class to avoid.
 */
#define CHECK_TOO_LARGE(ierror, what)                                          \
  CHECK((ierror) != MPI_ERR_UNSUPPORTED_OPERATION,                             \
        "%s answered MPI_ERR_UNSUPPORTED_OPERATION for an oversized value; "   \
        "MPI_ERR_VALUE_TOO_LARGE is the class that means that",                \
        (what))

/* Not every `_c` entry point has a small twin on every implementation, and
 * where neither exists the stub is the correct answer rather than a gap in the
 * fallback. The persistent collectives are the case that matters: they arrived
 * in **MPI-4.0**, so an MPI-3.0 implementation has neither MPI_Alltoallv_init_c
 * nor MPI_Alltoallv_init, and MPI_ERR_UNSUPPORTED_OPERATION is what decision 6
 * promises there.
 *
 * Found by the MPI-3.0 floor row and not by either of the two implementations
 * this is usually run against, both of which have the small twin.
 */
static int unsupported(int ierror, const char *what)
{
  if (ierror != MPI_ERR_UNSUPPORTED_OPERATION) return 0;
  if (rank == 0)
    printf("  skipping %s: neither it nor its small twin is in this "
           "implementation\n", what);
  return 1;
}

/* A count no implementation can express in an int, used wherever the point is
 * to reach the ceiling rather than to move data. */
static const MPI_Count too_large = (MPI_Count)INT_MAX + 7;

/* ------------------------------------------- the `_c` form and its twin -- */

/* Every one of these has a small twin asking the same question, so the answer
 * is checkable without knowing which arm the wrapper took.
 */
static void test_agrees_with_small_twin(void)
{
  MPI_Datatype t = MPI_DATATYPE_NULL;
  CHECK_MPI(MPI_Type_contiguous(5, MPI_INT, &t));
  CHECK_MPI(MPI_Type_commit(&t));

  int       small = -1;
  MPI_Count large = -1;
  CHECK_MPI(MPI_Type_size(t, &small));
  CHECK_MPI(MPI_Type_size_c(t, &large));
  CHECK(large == small, "MPI_Type_size_c said %lld, MPI_Type_size said %d",
        (long long)large, small);

  MPI_Aint  slb = -1, sext = -1;
  MPI_Count llb = -1, lext = -1;
  CHECK_MPI(MPI_Type_get_extent(t, &slb, &sext));
  CHECK_MPI(MPI_Type_get_extent_c(t, &llb, &lext));
  CHECK(llb == slb && lext == sext,
        "extent_c said (%lld, %lld), extent said (%lld, %lld)", (long long)llb,
        (long long)lext, (long long)slb, (long long)sext);

  CHECK_MPI(MPI_Type_get_true_extent(t, &slb, &sext));
  CHECK_MPI(MPI_Type_get_true_extent_c(t, &llb, &lext));
  CHECK(llb == slb && lext == sext, "true extents disagree");

  int       ssize = -1;
  MPI_Count lsize = -1;
  CHECK_MPI(MPI_Pack_size(3, MPI_INT, MPI_COMM_WORLD, &ssize));
  CHECK_MPI(MPI_Pack_size_c(3, MPI_INT, MPI_COMM_WORLD, &lsize));
  CHECK(lsize == ssize, "pack sizes disagree: %lld and %d", (long long)lsize,
        ssize);

  CHECK_MPI(MPI_Type_free(&t));
}

/* ------------------------------------------------------- point to point -- */

static void test_send_recv(void)
{
  if (size < 2) return;

  int buf[4] = {rank, rank + 1, rank + 2, rank + 3};
  if (rank == 0) {
    CHECK_MPI(MPI_Send_c(buf, 4, MPI_INT, 1, 7, MPI_COMM_WORLD));
  } else if (rank == 1) {
    int        got[4] = {-1, -1, -1, -1};
    MPI_Status st;
    CHECK_MPI(MPI_Recv_c(got, 4, MPI_INT, 0, 7, MPI_COMM_WORLD, &st));
    CHECK(got[0] == 0 && got[3] == 3, "payload %d..%d, expected 0..3", got[0],
          got[3]);

    /* The status query in both widths, on a status the `_c` receive filled. */
    int       scount = -1;
    MPI_Count lcount = -1;
    CHECK_MPI(MPI_Get_count(&st, MPI_INT, &scount));
    CHECK_MPI(MPI_Get_count_c(&st, MPI_INT, &lcount));
    CHECK(lcount == 4 && scount == 4, "counts %lld and %d, expected 4",
          (long long)lcount, scount);

    MPI_Count lelems = -1;
    CHECK_MPI(MPI_Get_elements_c(&st, MPI_INT, &lelems));
    CHECK(lelems == 4, "MPI_Get_elements_c said %lld", (long long)lelems);
  }
}

/* ------------------------------------------------- datatype constructors -- */

static void test_constructors(void)
{
  MPI_Datatype t = MPI_DATATYPE_NULL;
  CHECK_MPI(MPI_Type_contiguous_c(5, MPI_INT, &t));
  MPI_Count sz = -1;
  CHECK_MPI(MPI_Type_size_c(t, &sz));
  CHECK(sz == 5 * (MPI_Count)sizeof(int), "contiguous_c size %lld",
        (long long)sz);
  CHECK_MPI(MPI_Type_free(&t));

  const MPI_Count bl[2]    = {1, 1};
  const MPI_Count dis[2]   = {0, 8};
  MPI_Datatype    types[2] = {MPI_INT, MPI_DOUBLE};
  CHECK_MPI(MPI_Type_create_struct_c(2, bl, dis, types, &t));
  CHECK_MPI(MPI_Type_size_c(t, &sz));
  CHECK(sz == (MPI_Count)(sizeof(int) + sizeof(double)),
        "struct_c size %lld", (long long)sz);
  CHECK_MPI(MPI_Type_free(&t));

  const MPI_Count ibl[3] = {2, 1, 3};
  const MPI_Count idi[3] = {0, 4, 8};
  CHECK_MPI(MPI_Type_indexed_c(3, ibl, idi, MPI_INT, &t));
  CHECK_MPI(MPI_Type_size_c(t, &sz));
  CHECK(sz == 6 * (MPI_Count)sizeof(int), "indexed_c size %lld",
        (long long)sz);
  CHECK_MPI(MPI_Type_free(&t));
}

/* A refused call still owes the caller a defined out parameter. The scalar
 * count and the array element are separate rejection paths in the wrapper and
 * both are checked: the count of a contiguous, and a *blocklength* of a
 * struct -- which has to be the blocklength and not a displacement, because
 * displacements narrow to MPI_Aint and that is 64 bits on every host but the
 * i386 row, so an oversized one is legitimately accepted.
 */
static void test_too_large_leaves_the_handle_defined(void)
{
  MPI_Datatype t = (MPI_Datatype)0xdeadbeef;
  int          e = MPI_Type_contiguous_c(too_large, MPI_BYTE, &t);
  CHECK_TOO_LARGE(e, "MPI_Type_contiguous_c");
  CHECK(t != (MPI_Datatype)0xdeadbeef,
        "MPI_Type_contiguous_c left the out handle untouched");
  if (e == MPI_SUCCESS) CHECK_MPI(MPI_Type_free(&t));

  const MPI_Count bl[2]    = {1, too_large};
  const MPI_Count dis[2]   = {0, 8};
  MPI_Datatype    types[2] = {MPI_INT, MPI_DOUBLE};
  t                        = (MPI_Datatype)0xdeadbeef;
  e = MPI_Type_create_struct_c(2, bl, dis, types, &t);
  CHECK_TOO_LARGE(e, "MPI_Type_create_struct_c");
  CHECK(t != (MPI_Datatype)0xdeadbeef,
        "MPI_Type_create_struct_c left the out handle untouched");
  if (e == MPI_SUCCESS) CHECK_MPI(MPI_Type_free(&t));
}

/* ----------------------------------------------- vector collectives -------- */

/* The root_only hazard. Every non-root rank passes NULL for the arrays the
 * standard makes insignificant there, which is legal and which a body sizing
 * them by the group would dereference.
 */
static void test_root_only_arrays(void)
{
  int send[2] = {rank * 10, rank * 10 + 1};
  int recv[16];
  for (int i = 0; i < 16; ++i) recv[i] = -1;

  MPI_Count *counts = NULL;
  MPI_Aint  *displs = NULL;
  MPI_Count  cbuf[2];
  MPI_Aint   dbuf[2];
  if (rank == 0) {
    for (int i = 0; i < size; ++i) {
      cbuf[i] = 2;
      dbuf[i] = 2 * i;
    }
    counts = cbuf;
    displs = dbuf;
  }
  CHECK_MPI(MPI_Gatherv_c(send, 2, MPI_INT, recv, counts, displs, MPI_INT, 0,
                          MPI_COMM_WORLD));
  if (rank == 0)
    for (int i = 0; i < size; ++i)
      CHECK(recv[2 * i] == i * 10 && recv[2 * i + 1] == i * 10 + 1,
            "gatherv_c rank %d gave %d,%d", i, recv[2 * i], recv[2 * i + 1]);

  int src[16];
  int got[2] = {-1, -1};
  if (rank == 0)
    for (int i = 0; i < 16; ++i) src[i] = 100 + i;
  CHECK_MPI(MPI_Scatterv_c(src, counts, displs, MPI_INT, got, 2, MPI_INT, 0,
                           MPI_COMM_WORLD));
  CHECK(got[0] == 100 + 2 * rank && got[1] == 101 + 2 * rank,
        "scatterv_c gave %d,%d", got[0], got[1]);
}

static void test_vector_collectives(void)
{
  int       send[2], recv[2];
  MPI_Count sc[2], rc[2];
  MPI_Aint  sd[2], rd[2];
  for (int i = 0; i < size; ++i) {
    send[i] = rank * 100 + i;
    recv[i] = -1;
    sc[i] = rc[i] = 1;
    sd[i] = rd[i] = i;
  }
  CHECK_MPI(MPI_Alltoallv_c(send, sc, sd, MPI_INT, recv, rc, rd, MPI_INT,
                            MPI_COMM_WORLD));
  for (int i = 0; i < size; ++i)
    CHECK(recv[i] == i * 100 + rank, "alltoallv_c recv[%d] = %d", i, recv[i]);

  int red[2], scattered = -1;
  for (int i = 0; i < size; ++i) {
    red[i] = rank + i;
    rc[i]  = 1;
  }
  CHECK_MPI(MPI_Reduce_scatter_c(red, &scattered, rc, MPI_INT, MPI_SUM,
                                 MPI_COMM_WORLD));
  int expect = 0;
  for (int p = 0; p < size; ++p) expect += p + rank;
  CHECK(scattered == expect, "reduce_scatter_c gave %d, expected %d",
        scattered, expect);
}

/* The staged block has to outlive the call, so the caller's own arrays are
 * overwritten the instant the operation is posted. Against a native MPI that
 * is illegal; here it must not change the answer, because we copy at
 * initiation (NOTES.md #5.7).
 */
static void test_nonblocking_outlives_the_call(void)
{
  int       send[2], recv[2];
  MPI_Count sc[2], rc[2];
  MPI_Aint  sd[2], rd[2];
  for (int i = 0; i < size; ++i) {
    send[i] = rank * 100 + i;
    recv[i] = -1;
    sc[i] = rc[i] = 1;
    sd[i] = rd[i] = i;
  }
  MPI_Request req = MPI_REQUEST_NULL;
  CHECK_MPI(MPI_Ialltoallv_c(send, sc, sd, MPI_INT, recv, rc, rd, MPI_INT,
                             MPI_COMM_WORLD, &req));
  memset(sc, 0x7f, sizeof sc);
  memset(rc, 0x7f, sizeof rc);
  memset(sd, 0x7f, sizeof sd);
  memset(rd, 0x7f, sizeof rd);
  CHECK_MPI(MPI_Wait(&req, MPI_STATUS_IGNORE));
  for (int i = 0; i < size; ++i)
    CHECK(recv[i] == i * 100 + rank,
          "ialltoallv_c recv[%d] = %d after the caller's arrays were "
          "overwritten", i, recv[i]);

  /* The w-form, whose block carries datatypes beside the narrowed counts. */
  MPI_Datatype st[2], rt[2];
  for (int i = 0; i < size; ++i) {
    send[i] = rank * 100 + i;
    recv[i] = -1;
    sc[i] = rc[i] = 1;
    sd[i] = rd[i] = (MPI_Aint)(i * sizeof(int));
    st[i] = rt[i] = MPI_INT;
  }
  CHECK_MPI(MPI_Ialltoallw_c(send, sc, sd, st, recv, rc, rd, rt,
                             MPI_COMM_WORLD, &req));
  memset(sc, 0x7f, sizeof sc);
  memset(rc, 0x7f, sizeof rc);
  CHECK_MPI(MPI_Wait(&req, MPI_STATUS_IGNORE));
  for (int i = 0; i < size; ++i)
    CHECK(recv[i] == i * 100 + rank, "ialltoallw_c recv[%d] = %d", i, recv[i]);
}

/* A persistent request keeps its arrays until MPI_Request_free, not until
 * completion, so a body that freed at completion is a use-after-free on the
 * second MPI_Start. Three rounds, per NOTES.md #5.7.
 */
static void test_persistent_started_three_times(void)
{
  int       send[2], recv[2];
  MPI_Count sc[2], rc[2];
  MPI_Aint  sd[2], rd[2];
  for (int i = 0; i < size; ++i) {
    sc[i] = rc[i] = 1;
    sd[i] = rd[i] = i;
  }
  MPI_Request req    = MPI_REQUEST_NULL;
  const int   ierror = MPI_Alltoallv_init_c(send, sc, sd, MPI_INT, recv, rc,
                                            rd, MPI_INT, MPI_COMM_WORLD,
                                            MPI_INFO_NULL, &req);
  if (unsupported(ierror, "MPI_Alltoallv_init_c")) return;
  CHECK_MPI(ierror);
  memset(sc, 0x7f, sizeof sc);
  memset(sd, 0x7f, sizeof sd);

  for (int round = 0; round < 3; ++round) {
    for (int i = 0; i < size; ++i) {
      send[i] = rank * 100 + i + round;
      recv[i] = -1;
    }
    CHECK_MPI(MPI_Start(&req));
    CHECK_MPI(MPI_Wait(&req, MPI_STATUS_IGNORE));
    for (int i = 0; i < size; ++i)
      CHECK(recv[i] == i * 100 + rank + round,
            "round %d: alltoallv_init_c recv[%d] = %d", round, i, recv[i]);
  }
  CHECK_MPI(MPI_Request_free(&req));
}

/* --------------------------------------------------------- pack/unpack -- */

/* `position` is inout: the caller's offset in, the implementation's advanced
 * offset out. A body that narrowed it and forgot to widen it back leaves the
 * caller stuck at zero, and the second pack overwrites the first.
 */
static void test_pack_position_round_trips(void)
{
  char      buf[256];
  MPI_Count pos = 0;
  int       a[3] = {11, 22, 33};
  double    b    = 2.5;

  CHECK_MPI(MPI_Pack_c(a, 3, MPI_INT, buf, sizeof buf, &pos, MPI_COMM_WORLD));
  CHECK(pos > 0, "position did not advance past the first pack: %lld",
        (long long)pos);
  const MPI_Count after_first = pos;
  CHECK_MPI(
      MPI_Pack_c(&b, 1, MPI_DOUBLE, buf, sizeof buf, &pos, MPI_COMM_WORLD));
  CHECK(pos > after_first, "position did not advance past the second pack");

  MPI_Count out    = 0;
  int       ga[3]  = {0, 0, 0};
  double    gb     = 0;
  CHECK_MPI(MPI_Unpack_c(buf, pos, &out, ga, 3, MPI_INT, MPI_COMM_WORLD));
  CHECK_MPI(MPI_Unpack_c(buf, pos, &out, &gb, 1, MPI_DOUBLE, MPI_COMM_WORLD));
  CHECK(ga[0] == 11 && ga[1] == 22 && ga[2] == 33, "unpacked %d %d %d", ga[0],
        ga[1], ga[2]);
  CHECK(gb == 2.5, "unpacked %f", gb);
  CHECK(out == pos, "unpack ended at %lld, pack ended at %lld", (long long)out,
        (long long)pos);
}

/* ------------------------------------------------- envelope and contents -- */

/* Self-consistency rather than a fixed shape. NOTES.md #13.2: over an
 * implementation without the `_c` constructors the wrapper builds a
 * small-count type, so the envelope reports its counts as integers where a
 * native MPI-4 reports them as large counts. Both are right about the type
 * they describe, and the round trip every real consumer performs -- envelope,
 * then contents -- has to work either way.
 */
static void test_envelope_round_trip(void)
{
  const MPI_Count bl[2]    = {1, 1};
  const MPI_Count dis[2]   = {0, 8};
  MPI_Datatype    types[2] = {MPI_INT, MPI_DOUBLE};
  MPI_Datatype    t        = MPI_DATATYPE_NULL;
  CHECK_MPI(MPI_Type_create_struct_c(2, bl, dis, types, &t));

  MPI_Count ni = -1, na = -1, nl = -1, nd = -1;
  int       combiner = -1;
  CHECK_MPI(MPI_Type_get_envelope_c(t, &ni, &na, &nl, &nd, &combiner));
  CHECK(combiner == MPI_COMBINER_STRUCT, "combiner %d, expected STRUCT (%d)",
        combiner, MPI_COMBINER_STRUCT);
  CHECK(nd == 2, "num_datatypes %lld, expected 2", (long long)nd);
  CHECK(ni >= 0 && na >= 0 && nl >= 0,
        "the envelope left a count undefined: %lld %lld %lld", (long long)ni,
        (long long)na, (long long)nl);

  if (ni <= 8 && na <= 8 && nl <= 8 && nd <= 8) {
    int          gi[8];
    MPI_Aint     ga[8];
    MPI_Count    gl[8];
    MPI_Datatype gd[8];
    for (int i = 0; i < 8; ++i) gd[i] = MPI_DATATYPE_NULL;
    CHECK_MPI(MPI_Type_get_contents_c(t, ni, na, nl, nd, gi, ga, gl, gd));
    CHECK(gd[0] == MPI_INT, "first component is not MPI_INT");
    CHECK(gd[1] == MPI_DOUBLE, "second component is not MPI_DOUBLE");
  }
  CHECK_MPI(MPI_Type_free(&t));
}

/* --------------------------------------------------------------- driver -- */

int main(int argc, char **argv)
{
  CHECK_MPI(MPI_Init(&argc, &argv));
  CHECK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &size));
  CHECK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));

  if (size < 1 || size > 2) {
    if (rank == 0)
      printf("abi_large_count_test runs at 1 or 2 ranks, got %d\n", size);
    MPI_Finalize();
    return 1;
  }

  if (mpiabi_expect_ranks("abi_large_count_test", size, rank)) {
    MPI_Finalize();
    return 1;
  }

  if (rank == 0)
    printf("abi_large_count_test: %d rank%s\n", size, size == 1 ? "" : "s");

  test_agrees_with_small_twin();
  test_send_recv();
  test_constructors();
  test_too_large_leaves_the_handle_defined();
  test_root_only_arrays();
  test_vector_collectives();
  test_nonblocking_outlives_the_call();
  test_persistent_started_three_times();
  test_pack_position_round_trips();
  test_envelope_round_trip();

  int total = 0;
  CHECK_MPI(
      MPI_Allreduce(&failures, &total, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD));
  if (rank == 0)
    printf("abi_large_count_test: %d failure(s) across %d ranks\n", total,
           size);

  CHECK_MPI(MPI_Finalize());
  return total != 0;
}
