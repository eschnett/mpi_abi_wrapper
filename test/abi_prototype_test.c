/* abi_prototype_test -- the black-box half of S1.
 *
 * An ordinary MPI application: it includes the ABI mpi.h, links libmpi_abi and
 * nothing else, and knows nothing about any implementation. Everything it
 * observes has crossed the vtable boundary twice.
 *
 * It exercises each of the 29 prototype entry points at least once, and each of
 * the argument classes NOTES.md #11 chose them for. Two ranks by preference, one
 * where the implementation's launcher cannot manage two.
 *
 * Note that it deliberately uses MPI_ names throughout except where it is
 * checking the PMPI_ path: the two reach different slots and different
 * implementation entry points, and "PMPI_Send works too" is a claim worth
 * testing rather than assuming.
 */

#include <mpi.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* Two ranks is the interesting configuration and one rank is a supported
 * fallback, because an implementation whose launcher cannot form a two-rank job
 * on the machine at hand would otherwise contribute no black-box coverage at
 * all -- which is exactly the situation Open MPI 5.0.x is in on macOS 26. At one
 * rank the peer is this process, so the ordered blocking exchange would
 * deadlock and the nonblocking form is used instead; MPI_Send's own conversion
 * path stays covered by the MPI_PROC_NULL case below.
 */
static void exchange(const void *sendbuf, void *recvbuf, int count,
                     MPI_Datatype datatype, int tag, MPI_Status *status)
{
  if (size == 2) {
    if (rank == 0) {
      CHECK_MPI(MPI_Send(sendbuf, count, datatype, peer, tag, MPI_COMM_WORLD));
      CHECK_MPI(MPI_Recv(recvbuf, count, datatype, peer, tag, MPI_COMM_WORLD,
                         status));
    } else {
      CHECK_MPI(MPI_Recv(recvbuf, count, datatype, peer, tag, MPI_COMM_WORLD,
                         status));
      CHECK_MPI(MPI_Send(sendbuf, count, datatype, peer, tag, MPI_COMM_WORLD));
    }
    return;
  }

  MPI_Request request;
  CHECK_MPI(MPI_Isend(sendbuf, count, datatype, peer, tag, MPI_COMM_WORLD,
                      &request));
  CHECK_MPI(MPI_Recv(recvbuf, count, datatype, peer, tag, MPI_COMM_WORLD,
                     status));
  CHECK_MPI(MPI_Waitall(1, &request, MPI_STATUSES_IGNORE));
}

/* ------------------------------------------------ send/recv, ranks and tags */

static void test_sendrecv(void)
{
  const int tag = 7;

  int sendbuf[4] = {rank, rank + 1, rank + 2, rank + 3};
  int recvbuf[4] = {-1, -1, -1, -1};

  /* MPI_ERROR is preset and expected back unchanged: MPI-5.0 3.2.5 says the
   * error field "is never modified" except by the multiple-completion calls
   * of 3.7.5, which take an array of statuses and are not what this is. S1
   * asserted MPI_SUCCESS here instead, which was the wrong assertion and
   * passed only because the wrapper was writing the field -- S7 found it from
   * the other side, in MPICH's own pt2pt/mprobe.
   */
  MPI_Status status;
  status.MPI_ERROR = MPI_ERR_DIMS;
  exchange(sendbuf, recvbuf, 4, MPI_INT, tag, &status);

  for (int i = 0; i < 4; ++i)
    CHECK(recvbuf[i] == peer + i, "recvbuf[%d] is %d, expected %d", i,
          recvbuf[i], peer + i);

  /* The status crossed the boundary as a 32-byte ABI status carrying the
   * implementation's private bytes; MPI_Get_count hands them back to it.
   */
  CHECK(status.MPI_SOURCE == peer, "status.MPI_SOURCE is %d, expected %d",
        status.MPI_SOURCE, peer);
  CHECK(status.MPI_TAG == tag, "status.MPI_TAG is %d, expected %d",
        status.MPI_TAG, tag);
  CHECK(status.MPI_ERROR == MPI_ERR_DIMS,
        "status.MPI_ERROR is %d, and a single completion must leave it at the "
        "MPI_ERR_DIMS (%d) it was set to",
        status.MPI_ERROR, MPI_ERR_DIMS);

  int count = -1;
  CHECK_MPI(MPI_Get_count(&status, MPI_INT, &count));
  CHECK(count == 4, "MPI_Get_count says %d, expected 4", count);

  /* MPI_ANY_SOURCE and MPI_ANY_TAG are -1/-2 in the ABI and -2/-1 in MPICH:
   * swapped, so getting either one wrong is visible rather than benign.
   */
  memset(recvbuf, 0, sizeof recvbuf);
  {
    MPI_Request request;
    CHECK_MPI(MPI_Isend(sendbuf, 4, MPI_INT, peer, tag, MPI_COMM_WORLD,
                        &request));
    CHECK_MPI(MPI_Recv(recvbuf, 4, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
                       MPI_COMM_WORLD, &status));
    CHECK_MPI(MPI_Waitall(1, &request, MPI_STATUSES_IGNORE));
  }
  CHECK(status.MPI_SOURCE == peer, "wildcard receive says source %d",
        status.MPI_SOURCE);
  CHECK(status.MPI_TAG == tag, "wildcard receive says tag %d", status.MPI_TAG);

  /* MPI_PROC_NULL is -3 in the ABI and -1 in MPICH, and a send to it must
   * complete immediately rather than block or fail.
   */
  CHECK_MPI(MPI_Send(sendbuf, 4, MPI_INT, MPI_PROC_NULL, tag, MPI_COMM_WORLD));
  memset(recvbuf, 0x5a, sizeof recvbuf);
  CHECK_MPI(MPI_Recv(recvbuf, 4, MPI_INT, MPI_PROC_NULL, tag, MPI_COMM_WORLD,
                     &status));
  CHECK(status.MPI_SOURCE == MPI_PROC_NULL,
        "a receive from MPI_PROC_NULL reports source %d, expected %d",
        status.MPI_SOURCE, MPI_PROC_NULL);
  CHECK(status.MPI_TAG == MPI_ANY_TAG,
        "a receive from MPI_PROC_NULL reports tag %d, expected MPI_ANY_TAG",
        status.MPI_TAG);
  CHECK_MPI(MPI_Get_count(&status, MPI_INT, &count));
  CHECK(count == 0, "a receive from MPI_PROC_NULL reports %d elements", count);

  /* The PMPI_ path is a different slot and a different implementation entry
   * point, so it gets its own exchange rather than an assumption.
   */
  memset(recvbuf, 0, sizeof recvbuf);
  {
    MPI_Request request;
    CHECK_MPI(PMPI_Isend(sendbuf, 4, MPI_INT, peer, tag, MPI_COMM_WORLD,
                         &request));
    CHECK_MPI(PMPI_Recv(recvbuf, 4, MPI_INT, peer, tag, MPI_COMM_WORLD,
                        &status));
    CHECK_MPI(PMPI_Waitall(1, &request, MPI_STATUSES_IGNORE));
  }
  for (int i = 0; i < 4; ++i)
    CHECK(recvbuf[i] == peer + i, "PMPI recvbuf[%d] is %d, expected %d", i,
          recvbuf[i], peer + i);
}

/* ------------------------------------- request arrays and MPI_STATUSES_IGNORE */

static void test_waitall(void)
{
  int         out[2] = {rank * 10, rank * 10 + 1};
  int         in[2]  = {-1, -1};
  MPI_Request requests[4];
  MPI_Status  statuses[4];

  CHECK_MPI(MPI_Irecv(&in[0], 1, MPI_INT, peer, 11, MPI_COMM_WORLD,
                      &requests[0]));
  CHECK_MPI(MPI_Irecv(&in[1], 1, MPI_INT, peer, 12, MPI_COMM_WORLD,
                      &requests[1]));
  CHECK_MPI(MPI_Isend(&out[0], 1, MPI_INT, peer, 11, MPI_COMM_WORLD,
                      &requests[2]));
  CHECK_MPI(MPI_Isend(&out[1], 1, MPI_INT, peer, 12, MPI_COMM_WORLD,
                      &requests[3]));

  CHECK_MPI(MPI_Waitall(4, requests, statuses));

  CHECK(in[0] == peer * 10, "in[0] is %d, expected %d", in[0], peer * 10);
  CHECK(in[1] == peer * 10 + 1, "in[1] is %d, expected %d", in[1],
        peer * 10 + 1);
  for (int i = 0; i < 4; ++i)
    CHECK(requests[i] == MPI_REQUEST_NULL,
          "MPI_Waitall left requests[%d] non-null", i);
  CHECK(statuses[0].MPI_TAG == 11 && statuses[1].MPI_TAG == 12,
        "MPI_Waitall status tags are %d and %d", statuses[0].MPI_TAG,
        statuses[1].MPI_TAG);

  /* MPI_STATUSES_IGNORE is NULL in the ABI and (MPI_Status *)1 in MPICH, and
   * the wrapper must short-circuit it before allocating anything.
   */
  CHECK_MPI(MPI_Irecv(&in[0], 1, MPI_INT, peer, 13, MPI_COMM_WORLD,
                      &requests[0]));
  CHECK_MPI(MPI_Isend(&out[0], 1, MPI_INT, peer, 13, MPI_COMM_WORLD,
                      &requests[1]));
  CHECK_MPI(MPI_Waitall(2, requests, MPI_STATUSES_IGNORE));
  CHECK(in[0] == peer * 10, "in[0] is %d after the ignoring wait", in[0]);

  /* Zero requests, which must not allocate and must not touch the array. */
  CHECK_MPI(MPI_Waitall(0, requests, MPI_STATUSES_IGNORE));
}

/* ------------------------------------------ MPI_IN_PLACE and a user-defined op */

/* The trampoline receives an *implementation* datatype and must present the ABI
 * one, which is what makes a user op the only place the reverse handle map runs
 * on a hot path.
 */
static int saw_datatype_mismatch;

static void max_abs(void *invec, void *inoutvec, int *len,
                    MPI_Datatype *datatype)
{
  if (*datatype != MPI_DOUBLE) {
    saw_datatype_mismatch = 1;
    return;
  }
  double *in  = invec;
  double *out = inoutvec;
  for (int i = 0; i < *len; ++i) {
    const double a = in[i] < 0 ? -in[i] : in[i];
    const double b = out[i] < 0 ? -out[i] : out[i];
    out[i]         = a > b ? a : b;
  }
}

static void test_allreduce(void)
{
  MPI_Op op;
  CHECK_MPI(MPI_Op_create(max_abs, 1, &op));

  double values[3] = {rank == 0 ? -3.0 : 1.0, rank == 0 ? 2.0 : -5.0, 0.5};
  double result[3] = {0, 0, 0};

  /* At two ranks the op has to run and the answer is the elementwise
   * max-of-absolute-values. At one rank there is nothing to combine, and MPI
   * does not require the op to be invoked at all -- Open MPI does not invoke it
   * -- so the only defensible expectation is "either reduced or untouched".
   * Saying so explicitly beats asserting whichever one this implementation
   * happens to do.
   */
  double expect[3] = {3.0, 5.0, 0.5};

  CHECK_MPI(MPI_Allreduce(values, result, 3, MPI_DOUBLE, op, MPI_COMM_WORLD));
  if (size == 2) {
    CHECK(result[0] == expect[0] && result[1] == expect[1] &&
              result[2] == expect[2],
          "user op gave {%g, %g, %g}, expected {%g, %g, %g}", result[0],
          result[1], result[2], expect[0], expect[1], expect[2]);
    CHECK(!saw_datatype_mismatch,
          "the user op was handed a datatype that is not MPI_DOUBLE -- the "
          "implementation -> ABI datatype map is wrong");
  } else {
    CHECK((result[0] == 3.0 || result[0] == values[0]) &&
              result[2] == values[2],
          "one-rank user op gave {%g, %g, %g} from {%g, %g, %g}", result[0],
          result[1], result[2], values[0], values[1], values[2]);
    CHECK(!saw_datatype_mismatch,
          "the user op was handed a datatype that is not MPI_DOUBLE");
  }

  /* MPI_IN_PLACE is (void *)1 in the ABI and (void *)-1 in MPICH. */
  double inplace[3] = {rank == 0 ? -3.0 : 1.0, rank == 0 ? 2.0 : -5.0, 0.5};
  CHECK_MPI(MPI_Allreduce(MPI_IN_PLACE, inplace, 3, MPI_DOUBLE, op,
                          MPI_COMM_WORLD));
  if (size == 2)
    CHECK(inplace[0] == expect[0] && inplace[1] == expect[1] &&
              inplace[2] == expect[2],
          "MPI_IN_PLACE user op gave {%g, %g, %g}", inplace[0], inplace[1],
          inplace[2]);
  else
    CHECK(inplace[2] == 0.5, "MPI_IN_PLACE at one rank corrupted the buffer: "
                             "{%g, %g, %g}", inplace[0], inplace[1], inplace[2]);

  /* A predefined op, so that the ABI -> implementation switch is exercised on
   * the same path.
   */
  int one = 1, total = 0;
  CHECK_MPI(MPI_Allreduce(&one, &total, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD));
  CHECK(total == size, "MPI_SUM over ones gave %d, expected %d", total, size);

  CHECK_MPI(MPI_Op_free(&op));
  CHECK(op == MPI_OP_NULL, "MPI_Op_free left the handle non-null");
}

/* ----------------------------------------------- out handles and the reverse map */

static void test_comm_split(void)
{
  MPI_Comm newcomm = MPI_COMM_NULL;
  CHECK_MPI(MPI_Comm_split(MPI_COMM_WORLD, rank % 2, 0, &newcomm));
  CHECK(newcomm != MPI_COMM_NULL, "MPI_Comm_split produced MPI_COMM_NULL");

  int newsize = -1, newrank = -1;
  CHECK_MPI(MPI_Comm_size(newcomm, &newsize));
  CHECK_MPI(MPI_Comm_rank(newcomm, &newrank));
  CHECK(newsize == 1 && newrank == 0,
        "the split communicator has size %d rank %d", newsize, newrank);

  CHECK_MPI(MPI_Comm_free(&newcomm));
  /* This is the reverse map returning a *predefined* handle: the
   * implementation set its own MPI_COMM_NULL and we must produce the ABI's.
   */
  CHECK(newcomm == MPI_COMM_NULL,
        "MPI_Comm_free did not leave MPI_COMM_NULL behind");

  /* MPI_UNDEFINED as a colour, which is a mapped integer in the in direction
   * and produces MPI_COMM_NULL in the out direction.
   */
  MPI_Comm nocomm = (MPI_Comm)0x1;
  CHECK_MPI(MPI_Comm_split(MPI_COMM_WORLD, MPI_UNDEFINED, 0, &nocomm));
  CHECK(nocomm == MPI_COMM_NULL,
        "splitting with MPI_UNDEFINED did not give MPI_COMM_NULL");
}

/* ------------------------------------------------------ const handle arrays */

static void test_type_create_struct(void)
{
  /* .rodata on purpose: this is the array that would crash a legal program if
   * the wrapper converted handle arrays in place (NOTES.md #5.7).
   */
  static const MPI_Datatype types[2]         = {MPI_INT, MPI_DOUBLE};
  static const int          blocklengths[2]  = {1, 1};
  static const MPI_Aint     displacements[2] = {0, sizeof(double)};

  MPI_Datatype newtype = MPI_DATATYPE_NULL;
  CHECK_MPI(MPI_Type_create_struct(2, blocklengths, displacements, types,
                                   &newtype));
  CHECK(newtype != MPI_DATATYPE_NULL, "MPI_Type_create_struct gave null");
  CHECK_MPI(MPI_Type_commit(&newtype));

  struct {
    double d;
    int    i;
  } sendval = {rank + 0.5, rank}, recvval = {0, 0};

  /* Sent with the derived type, so the type really reached the implementation
   * rather than merely being created.
   */
  MPI_Status status;
  exchange(&sendval.i, &recvval.i, 1, newtype, 21, &status);
  CHECK(recvval.i == peer, "derived type transferred %d, expected %d",
        recvval.i, peer);

  CHECK_MPI(MPI_Type_free(&newtype));
  CHECK(newtype == MPI_DATATYPE_NULL, "MPI_Type_free left the handle non-null");

  CHECK(types[0] == MPI_INT && types[1] == MPI_DOUBLE,
        "the caller's const type array was modified");

  /* The large-count twin. An implementation older than MPI-4.0 has no _c entry
   * points, and then the slot must still exist and report itself missing --
   * never be absent from the ABI (NOTES.md #1, decision 6).
   */
  static const MPI_Count cblocklengths[2]  = {1, 1};
  static const MPI_Count cdisplacements[2] = {0, sizeof(double)};
  MPI_Datatype           ctype             = MPI_DATATYPE_NULL;
  const int              cerr = MPI_Type_create_struct_c(2, cblocklengths,
                                                         cdisplacements, types,
                                                         &ctype);
  if (cerr == MPI_ERR_UNSUPPORTED_OPERATION) {
    if (rank == 0)
      printf("  MPI_Type_create_struct_c: reported unsupported, as expected "
             "against an implementation without the MPI-4.0 _c forms\n");
  } else {
    CHECK(cerr == MPI_SUCCESS, "MPI_Type_create_struct_c returned %d", cerr);
    CHECK(ctype != MPI_DATATYPE_NULL, "MPI_Type_create_struct_c gave null");
    if (ctype != MPI_DATATYPE_NULL) CHECK_MPI(MPI_Type_free(&ctype));
  }
}

/* ------------------------------------------------------ output string buffers */

static void test_error_string(void)
{
  char string[MPI_MAX_ERROR_STRING];
  int  resultlen = -1;

  memset(string, 0x7f, sizeof string);
  CHECK_MPI(MPI_Error_string(MPI_ERR_TRUNCATE, string, &resultlen));
  CHECK(resultlen >= 0 && resultlen < MPI_MAX_ERROR_STRING,
        "MPI_Error_string reports length %d", resultlen);
  CHECK(string[resultlen] == '\0',
        "MPI_Error_string did not terminate at *resultlen");
  CHECK(strlen(string) == (size_t)resultlen,
        "MPI_Error_string length %d disagrees with the string it wrote (%zu)",
        resultlen, strlen(string));
  if (rank == 0) printf("  MPI_ERR_TRUNCATE is \"%s\"\n", string);

  /* MPI_SUCCESS must have a string too, and the error-code map is the in
   * direction here.
   */
  CHECK_MPI(MPI_Error_string(MPI_SUCCESS, string, &resultlen));
  CHECK(resultlen > 0, "MPI_Error_string(MPI_SUCCESS) gave an empty string");
}

/* --------------------------------------------------------- Fortran converters */

static void test_fortran_converters(void)
{
  const MPI_Fint fworld = MPI_Comm_c2f(MPI_COMM_WORLD);
  const MPI_Comm back   = MPI_Comm_f2c(fworld);
  CHECK(back == MPI_COMM_WORLD,
        "MPI_Comm_c2f/f2c did not round trip MPI_COMM_WORLD (got %p)",
        (void *)back);

  MPI_Comm dup = MPI_COMM_NULL;
  CHECK_MPI(MPI_Comm_split(MPI_COMM_WORLD, 0, rank, &dup));
  const MPI_Fint fdup = MPI_Comm_c2f(dup);
  CHECK(MPI_Comm_f2c(fdup) == dup,
        "MPI_Comm_c2f/f2c did not round trip a dynamic communicator");
  CHECK_MPI(MPI_Comm_free(&dup));

  CHECK(MPI_Comm_f2c(MPI_Comm_c2f(MPI_COMM_NULL)) == MPI_COMM_NULL,
        "MPI_Comm_c2f/f2c did not round trip MPI_COMM_NULL");
}

/* ------------------------------------------------------- error handlers */

static int  errhandler_calls;
static int  errhandler_code;
static void my_errhandler(MPI_Comm *comm, int *error_code, ...)
{
  (void)comm;
  ++errhandler_calls;
  errhandler_code = *error_code;
}

static void test_errhandler(void)
{
  MPI_Comm       comm;
  MPI_Errhandler errhandler;

  CHECK_MPI(MPI_Comm_split(MPI_COMM_WORLD, 0, rank, &comm));
  CHECK_MPI(MPI_Comm_create_errhandler(my_errhandler, &errhandler));
  CHECK(errhandler != MPI_ERRHANDLER_NULL,
        "MPI_Comm_create_errhandler gave null");
  CHECK_MPI(MPI_Comm_set_errhandler(comm, errhandler));

  /* An invalid rank: the implementation invokes our variadic trampoline, which
   * converts its communicator and error code back into ABI terms and calls the
   * function above.
   */
  int buf = 0;
  (void)MPI_Send(&buf, 1, MPI_INT, size + 100, 0, comm);

  CHECK(errhandler_calls == 1, "the error handler ran %d times, expected 1",
        errhandler_calls);
  CHECK(errhandler_code != MPI_SUCCESS,
        "the error handler was given MPI_SUCCESS");

  /* S1 asserted `errhandler_code <= MPI_ERR_LASTCODE` here, on the reasoning
   * that an ABI error code must be one the ABI header can name. That premise
   * is false and S4b measured why: MPICH answers essentially every error with
   * an instance-specific code rather than with a class, and the wrapper now
   * interns those rather than flattening them to MPI_ERR_OTHER
   * (src/mpiwrapper/errorcodes.c) -- so what arrives here is above
   * MPI_ERR_LASTCODE exactly as a code from MPI_Add_error_code would be.
   *
   * What must hold is the property the standard actually states: the *class*
   * of the code is one of the predefined ones, and MPI_Error_string can say
   * something about the code. Both reach the implementation through the
   * registry, so this is also the round trip that a lossy mapping fails.
   */
  int errhandler_class = MPI_SUCCESS;
  CHECK_MPI(MPI_Error_class(errhandler_code, &errhandler_class));
  CHECK(errhandler_class > 0 && errhandler_class <= MPI_ERR_LASTCODE,
        "the error handler's code %d has class %d, which is not an ABI error "
        "class",
        errhandler_code, errhandler_class);

  char errhandler_string[MPI_MAX_ERROR_STRING];
  int  errhandler_len = 0;
  errhandler_string[0] = '\0';
  CHECK_MPI(MPI_Error_string(errhandler_code, errhandler_string,
                             &errhandler_len));
  CHECK(errhandler_len > 0 && errhandler_string[0] != '\0',
        "MPI_Error_string said nothing about the error handler's code %d",
        errhandler_code);

  CHECK_MPI(MPI_Comm_set_errhandler(comm, MPI_ERRORS_ARE_FATAL));
  CHECK_MPI(MPI_Comm_free(&comm));
}

/* ------------------------------------------ staged temporaries outliving a call */

static void test_ialltoallw(void)
{
  const int    n = size;
  int         *sendbuf = malloc((size_t)n * sizeof(int));
  int         *recvbuf = malloc((size_t)n * sizeof(int));
  int         *counts  = malloc((size_t)n * sizeof(int));
  int         *displs  = malloc((size_t)n * sizeof(int));
  MPI_Datatype *types  = malloc((size_t)n * sizeof(MPI_Datatype));
  MPI_Request  request;

  for (int i = 0; i < n; ++i) {
    sendbuf[i] = rank * 100 + i;
    recvbuf[i] = -1;
    counts[i]  = 1;
    displs[i]  = i * (int)sizeof(int);
    types[i]   = MPI_INT;
  }

  CHECK_MPI(MPI_Ialltoallw(sendbuf, counts, displs, types, recvbuf, counts,
                           displs, types, MPI_COMM_WORLD, &request));

  /* The datatype arrays the implementation is reading are ours, on the heap,
   * owned by the request -- so overwriting the caller's arrays here must not
   * disturb the operation. That is the property staging exists for.
   */
  for (int i = 0; i < n; ++i) types[i] = MPI_DATATYPE_NULL;

  CHECK_MPI(MPI_Waitall(1, &request, MPI_STATUSES_IGNORE));
  CHECK(request == MPI_REQUEST_NULL, "MPI_Waitall left the request non-null");

  for (int i = 0; i < n; ++i)
    CHECK(recvbuf[i] == i * 100 + rank, "recvbuf[%d] is %d, expected %d", i,
          recvbuf[i], i * 100 + rank);

  /* Whether the temporaries were actually released is not observable from out
   * here one call at a time -- but it is observable in bulk. The staged-request
   * table is fixed at 1024 entries by default, so a wrapper that attached
   * without releasing runs out and starts returning MPI_ERR_INTERN. Running the
   * cycle well past that capacity turns "the release path works" into something
   * this test can see.
   */
  for (int i = 0; i < n; ++i) types[i] = MPI_INT;
  for (int round = 0; round < 1200; ++round) {
    const int ierror =
        MPI_Ialltoallw(sendbuf, counts, displs, types, recvbuf, counts, displs,
                       types, MPI_COMM_WORLD, &request);
    if (ierror != MPI_SUCCESS) {
      CHECK(0, "MPI_Ialltoallw returned %d in round %d -- staged temporaries "
               "are not being released at completion", ierror, round);
      break;
    }
    CHECK_MPI(MPI_Waitall(1, &request, MPI_STATUSES_IGNORE));
  }

  free(types);
  free(displs);
  free(counts);
  free(recvbuf);
  free(sendbuf);
}

/* -------------------------------------------------------------- file handles */

static void test_file_open(void)
{
  const char *path = "abi_prototype_test.tmp";
  MPI_File    fh   = MPI_FILE_NULL;

  /* An OR-combined amode: MPI_MODE_RDWR is 32 in the ABI and 4 in MPICH, and
   * MPI_MODE_CREATE is 2 against 1, so a switch instead of a decomposition
   * would fail here rather than in production.
   */
  const int ierror = MPI_File_open(MPI_COMM_WORLD, path,
                                   MPI_MODE_CREATE | MPI_MODE_RDWR |
                                       MPI_MODE_DELETE_ON_CLOSE,
                                   MPI_INFO_NULL, &fh);
  if (ierror == MPI_ERR_UNSUPPORTED_OPERATION) {
    if (rank == 0) printf("  MPI_File_open: unsupported by this build\n");
    return;
  }
  CHECK(ierror == MPI_SUCCESS, "MPI_File_open returned %d", ierror);
  CHECK(fh != MPI_FILE_NULL, "MPI_File_open gave MPI_FILE_NULL");

  CHECK_MPI(MPI_File_close(&fh));
  CHECK(fh == MPI_FILE_NULL, "MPI_File_close left the handle non-null");
}

/* ------------------------------------------------------------------- timing */

static void test_wtime(void)
{
  /* MPI_Wtime is "time elapsed since some point in the past", and Open MPI's
   * point in the past is startup -- so it answers 0.000000 immediately after
   * MPI_Init, natively as well as through the wrapper. Non-negative and
   * non-decreasing is all the standard promises and all this may assert.
   */
  const double t0 = MPI_Wtime();
  CHECK(t0 >= 0.0, "MPI_Wtime returned %g", t0);
  const double t1 = PMPI_Wtime();
  CHECK(t1 >= t0, "PMPI_Wtime went backwards: %g then %g", t0, t1);

  int version = 0, subversion = -1;
  CHECK_MPI(MPI_Get_version(&version, &subversion));
  CHECK(version >= 3 && subversion >= 0, "MPI_Get_version says %d.%d", version,
        subversion);
  if (rank == 0) printf("  the implementation reports MPI %d.%d\n", version,
                        subversion);
}

int main(int argc, char **argv)
{
  CHECK_MPI(MPI_Init(&argc, &argv));
  CHECK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &size));
  CHECK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));

  if (size < 1 || size > 2) {
    if (rank == 0)
      printf("abi_prototype_test runs at 1 or 2 ranks, got %d\n", size);
    MPI_Finalize();
    return 1;
  }
  peer = (size == 2) ? 1 - rank : rank;

  if (rank == 0)
    printf("abi_prototype_test: %d rank%s\n", size, size == 1 ? "" : "s");

  test_sendrecv();
  test_waitall();
  test_allreduce();
  test_comm_split();
  test_type_create_struct();
  test_error_string();
  test_fortran_converters();
  test_errhandler();
  test_ialltoallw();
  test_file_open();
  test_wtime();

  int total = 0;
  CHECK_MPI(MPI_Allreduce(&failures, &total, 1, MPI_INT, MPI_SUM,
                          MPI_COMM_WORLD));
  if (rank == 0)
    printf("abi_prototype_test: %d failure(s) across %d ranks\n", total, size);

  CHECK_MPI(MPI_Finalize());
  return total != 0;
}
