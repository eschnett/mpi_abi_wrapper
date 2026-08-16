/* abi_converters_test -- the black-box half of S4a.
 *
 * The same shape as abi_tools_test: an ordinary MPI application over the ABI
 * header, linking libmpi_abi and nothing else. What it covers is the converter
 * face -- the 44 handle converters, the four status converters, the ten
 * status-consuming functions, the ten output-string buffers with no length
 * argument, and the six MPI_Abi_* calls.
 *
 * Each group is written against the shape a plausible-but-wrong body gets
 * wrong rather than against the happy path:
 *
 *  - **_toint on a predefined handle.** MPI-5.0 20.4.5 requires the integer to
 *    be "the same as the values listed in Section A", and Section A is the
 *    ABI's own table -- so the answer is checkable against the header's
 *    constant rather than only against a round trip. A body that forwarded to
 *    an implementation's own _toint, or that interned predefined handles along
 *    with the rest, passes every round trip here and fails this one line.
 *  - **_toint on a *dynamic* handle.** On Open MPI an ABI handle is a 64-bit
 *    object address, so a body that cast it to an int would truncate, and the
 *    matching _fromint would hand back a handle that is not the one that went
 *    in. That is why the round trip is checked on handles the program created
 *    rather than only on predefined ones.
 *  - **a status that has been to Fortran and back.** The ABI status's 20
 *    scratch bytes carry the implementation's own private bytes, so a status
 *    converted to the Fortran form and back must still answer MPI_Get_count.
 *    A converter that copied only the three named fields passes an inspection
 *    of MPI_SOURCE and MPI_TAG and fails here.
 *  - **the Fortran status's layout.** MPI-5.0 20.4.3 fixes MPI_F_SOURCE at
 *    index 0, and MPICH's own Fortran status puts it at index 2. A converter
 *    that forwarded to the implementation's MPI_Status_c2f produces a valid
 *    status in the wrong layout, which this reads directly out of the integer
 *    array.
 *  - **an output string longer than the ABI's maximum.** Not reachable
 *    against either implementation today -- no MPI_MAX_* of theirs exceeds the
 *    ABI's -- so what is testable is the other half of the contract:
 *    string[*resultlen] == '\0', which a body reporting the implementation's
 *    length rather than the copied length breaks.
 *  - **the second call to an MPI_Abi_set_* function**, which must report
 *    MPI_ERR_ABI rather than overwrite.
 */

#include <mpi.h>

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

static int unsupported(int ierror, const char *what)
{
  if (ierror != MPI_ERR_UNSUPPORTED_OPERATION) return 0;
  if (rank == 0) printf("  skipping %s: not in this implementation\n", what);
  return 1;
}

/* MPI_Info_get_string is MPI-4.0 and Open MPI 4.1 does not have it, so its
 * slot reports at run time (decision 6). Info is a *tool* in this test rather
 * than its subject, so the deprecated MPI_Info_get -- which every
 * implementation above the MPI-3.0 floor has -- is the fallback.
 */
static int info_get(MPI_Info info, const char *key, char *value, int size,
                    int *flag)
{
  int       buflen = size;
  const int ierror = MPI_Info_get_string(info, key, &buflen, value, flag);
  if (ierror != MPI_ERR_UNSUPPORTED_OPERATION) return ierror;
  return MPI_Info_get(info, key, size - 1, value, flag);
}

/* ------------------------------------------------- handle serialization ---- */

/* One block per class, because _toint and _fromint are per-class functions and
 * a table of function pointers would need one cast per class anyway -- the
 * repetition is the test.
 *
 * PREDEF is checked against the ABI header's own constant, which is the
 * standard's requirement rather than a round-trip property; LIVE is a handle
 * this program created, which is where truncation would show.
 */
#define TEST_SERIAL(CLASS, TYPE, PREDEF, LIVE)                                 \
  do {                                                                         \
    const int pi = MPI_##CLASS##_toint(PREDEF);                                \
    CHECK(pi == (int)(intptr_t)(PREDEF),                                       \
          #CLASS "_toint(" #PREDEF ") is %d, not the ABI's 0x%x", pi,          \
          (unsigned)(uintptr_t)(PREDEF));                                      \
    const TYPE pback = MPI_##CLASS##_fromint(pi);                              \
    CHECK(pback == (PREDEF), #CLASS "_fromint did not invert _toint on "       \
                                    #PREDEF);                                  \
                                                                               \
    const int li = MPI_##CLASS##_toint(LIVE);                                  \
    CHECK(li == MPI_##CLASS##_toint(LIVE),                                     \
          #CLASS "_toint is not stable across calls");                         \
    CHECK(li != pi, #CLASS "_toint gave one integer for two handles");         \
    const TYPE lback = MPI_##CLASS##_fromint(li);                              \
    CHECK(lback == (LIVE),                                                     \
          #CLASS "_fromint did not invert _toint on a live handle");           \
  } while (0)

/* The one integer no _fromint may accept: 1 is below the ABI's predefined
 * range (which starts at 0x20) and is not an index this library ever hands
 * out, so every class must answer its null handle rather than fabricate one.
 */
static void test_serialization_rejects_junk(void)
{
  CHECK(MPI_Comm_fromint(1) == MPI_COMM_NULL,
        "MPI_Comm_fromint(1) invented a communicator");
  CHECK(MPI_Group_fromint(1) == MPI_GROUP_NULL,
        "MPI_Group_fromint(1) invented a group");
  CHECK(MPI_Type_fromint(1) == MPI_DATATYPE_NULL,
        "MPI_Type_fromint(1) invented a datatype");
}

static void test_serialization(void)
{
  if (rank == 0) printf("test_serialization\n");

  MPI_Comm comm;
  CHECK_MPI(MPI_Comm_dup(MPI_COMM_WORLD, &comm));
  MPI_Group group;
  CHECK_MPI(MPI_Comm_group(MPI_COMM_WORLD, &group));
  MPI_Datatype datatype;
  CHECK_MPI(MPI_Type_contiguous(3, MPI_INT, &datatype));
  CHECK_MPI(MPI_Type_commit(&datatype));
  MPI_Info info;
  CHECK_MPI(MPI_Info_create(&info));

  TEST_SERIAL(Comm, MPI_Comm, MPI_COMM_WORLD, comm);
  TEST_SERIAL(Group, MPI_Group, MPI_GROUP_EMPTY, group);
  TEST_SERIAL(Type, MPI_Datatype, MPI_INT, datatype);
  TEST_SERIAL(Info, MPI_Info, MPI_INFO_ENV, info);

  /* The classes with no live handle cheap to make here still have to answer
   * for their predefined values, which is the half the standard pins.
   */
  CHECK(MPI_Op_toint(MPI_SUM) == (int)(intptr_t)MPI_SUM,
        "MPI_Op_toint(MPI_SUM) is not the ABI's value");
  CHECK(MPI_Op_fromint(MPI_Op_toint(MPI_SUM)) == MPI_SUM,
        "MPI_Op_fromint did not invert _toint");
  CHECK(MPI_Request_toint(MPI_REQUEST_NULL)
            == (int)(intptr_t)MPI_REQUEST_NULL,
        "MPI_Request_toint(MPI_REQUEST_NULL) is not the ABI's value");
  CHECK(MPI_Errhandler_toint(MPI_ERRORS_RETURN)
            == (int)(intptr_t)MPI_ERRORS_RETURN,
        "MPI_Errhandler_toint(MPI_ERRORS_RETURN) is not the ABI's value");
  CHECK(MPI_Win_toint(MPI_WIN_NULL) == (int)(intptr_t)MPI_WIN_NULL,
        "MPI_Win_toint(MPI_WIN_NULL) is not the ABI's value");
  CHECK(MPI_File_toint(MPI_FILE_NULL) == (int)(intptr_t)MPI_FILE_NULL,
        "MPI_File_toint(MPI_FILE_NULL) is not the ABI's value");
  CHECK(MPI_Message_toint(MPI_MESSAGE_NULL)
            == (int)(intptr_t)MPI_MESSAGE_NULL,
        "MPI_Message_toint(MPI_MESSAGE_NULL) is not the ABI's value");
  CHECK(MPI_Session_toint(MPI_SESSION_NULL)
            == (int)(intptr_t)MPI_SESSION_NULL,
        "MPI_Session_toint(MPI_SESSION_NULL) is not the ABI's value");

  test_serialization_rejects_junk();

  CHECK_MPI(MPI_Info_free(&info));
  CHECK_MPI(MPI_Type_free(&datatype));
  CHECK_MPI(MPI_Group_free(&group));
  CHECK_MPI(MPI_Comm_free(&comm));
}

/* --------------------------------------------------- c2f / f2c round trips -- */

/* What this cannot check is the thing that matters most: whether the integer
 * would be accepted by the implementation's own Fortran layer. Both directions
 * are ours, so a consistently wrong pair passes. mpif is the oracle for that
 * (S8); this catches a pair that is not even self-consistent, which is the
 * failure a per-class copy-paste actually produces.
 */
#define TEST_C2F(CLASS, TYPE, HANDLE)                                          \
  do {                                                                         \
    const MPI_Fint f    = MPI_##CLASS##_c2f(HANDLE);                           \
    const TYPE     back = MPI_##CLASS##_f2c(f);                                \
    CHECK(back == (HANDLE),                                                    \
          #CLASS "_f2c(" #CLASS "_c2f(" #HANDLE ")) is not " #HANDLE);         \
  } while (0)

static void test_c2f(void)
{
  if (rank == 0) printf("test_c2f\n");

  MPI_Comm comm;
  CHECK_MPI(MPI_Comm_dup(MPI_COMM_WORLD, &comm));
  MPI_Group group;
  CHECK_MPI(MPI_Comm_group(MPI_COMM_WORLD, &group));
  MPI_Datatype datatype;
  CHECK_MPI(MPI_Type_contiguous(3, MPI_INT, &datatype));
  CHECK_MPI(MPI_Type_commit(&datatype));
  MPI_Info info;
  CHECK_MPI(MPI_Info_create(&info));

  TEST_C2F(Comm, MPI_Comm, MPI_COMM_WORLD);
  TEST_C2F(Comm, MPI_Comm, MPI_COMM_SELF);
  TEST_C2F(Comm, MPI_Comm, comm);
  TEST_C2F(Group, MPI_Group, MPI_GROUP_EMPTY);
  TEST_C2F(Group, MPI_Group, group);
  TEST_C2F(Type, MPI_Datatype, MPI_INT);
  TEST_C2F(Type, MPI_Datatype, MPI_DOUBLE);
  TEST_C2F(Type, MPI_Datatype, datatype);
  TEST_C2F(Info, MPI_Info, info);
  TEST_C2F(Op, MPI_Op, MPI_SUM);
  TEST_C2F(Op, MPI_Op, MPI_MAX);
  TEST_C2F(Request, MPI_Request, MPI_REQUEST_NULL);
  TEST_C2F(Errhandler, MPI_Errhandler, MPI_ERRORS_RETURN);
  TEST_C2F(Errhandler, MPI_Errhandler, MPI_ERRORS_ARE_FATAL);
  TEST_C2F(Message, MPI_Message, MPI_MESSAGE_NULL);
  TEST_C2F(Win, MPI_Win, MPI_WIN_NULL);
  TEST_C2F(File, MPI_File, MPI_FILE_NULL);

  /* A live request, which is where a class whose null handle is the only
   * value ever tested would hide a bug.
   */
  {
    MPI_Request request;
    int         buf = rank;
    /* MPI_COMM_SELF, so the peer is rank 0 of *that* communicator whatever
     * this process's rank in MPI_COMM_WORLD is.
     */
    CHECK_MPI(MPI_Isend(&buf, 1, MPI_INT, 0, 7, MPI_COMM_SELF, &request));
    TEST_C2F(Request, MPI_Request, request);

    MPI_Status status;
    int        got = -1;
    CHECK_MPI(MPI_Recv(&got, 1, MPI_INT, 0, 7, MPI_COMM_SELF, &status));
    CHECK_MPI(MPI_Wait(&request, MPI_STATUS_IGNORE));
    CHECK(got == rank, "MPI_COMM_SELF exchange delivered %d, not %d", got,
          rank);
  }

  /* MPI_SESSION_NULL only when the implementation has sessions at all: the
   * class exists in the ABI either way, and the slot then reports at run time.
   */
  {
    const MPI_Fint f = MPI_Session_c2f(MPI_SESSION_NULL);
    if (f != 0)
      CHECK(MPI_Session_f2c(f) == MPI_SESSION_NULL,
            "MPI_Session_f2c did not invert MPI_Session_c2f");
    else if (rank == 0)
      printf("  skipping MPI_Session_c2f: not in this implementation\n");
  }

  CHECK_MPI(MPI_Info_free(&info));
  CHECK_MPI(MPI_Type_free(&datatype));
  CHECK_MPI(MPI_Group_free(&group));
  CHECK_MPI(MPI_Comm_free(&comm));
}

/* ------------------------------------------------------ status converters ---- */

/* Fills a status the way an application does, so that the private bytes in it
 * are the implementation's own rather than anything this test invented.
 */
static void fill_status(MPI_Status *status, int *count)
{
  int buf[4] = {1, 2, 3, 4};
  int got[4] = {0, 0, 0, 0};

  CHECK_MPI(MPI_Sendrecv(buf, 4, MPI_INT, 0, 11, got, 4, MPI_INT, 0, 11,
                         MPI_COMM_SELF, status));
  CHECK_MPI(MPI_Get_count(status, MPI_INT, count));
  CHECK(*count == 4, "MPI_Get_count on a fresh status says %d, not 4", *count);
}

static void test_status_converters(void)
{
  if (rank == 0) printf("test_status_converters\n");

  MPI_Status status;
  int        count = 0;
  fill_status(&status, &count);

  /* MPI-5.0 20.4.3: the ABI's Fortran status is eight INTEGERs with
   * MPI_F_SOURCE at index 0. Read straight out of the array, because this is
   * the property a converter forwarding to MPICH's own c2f gets wrong -- and
   * gets wrong while still producing a status that round-trips through its own
   * f2c.
   */
  MPI_Fint f[MPI_F_STATUS_SIZE];
  memset(f, 0x5a, sizeof f);
  CHECK_MPI(MPI_Status_c2f(&status, f));
  CHECK(f[MPI_F_SOURCE] == status.MPI_SOURCE,
        "the Fortran status's MPI_F_SOURCE is %d, not %d", (int)f[MPI_F_SOURCE],
        status.MPI_SOURCE);
  CHECK(f[MPI_F_TAG] == status.MPI_TAG,
        "the Fortran status's MPI_F_TAG is %d, not %d", (int)f[MPI_F_TAG],
        status.MPI_TAG);
  CHECK(f[MPI_F_ERROR] == status.MPI_ERROR,
        "the Fortran status's MPI_F_ERROR is %d, not %d", (int)f[MPI_F_ERROR],
        status.MPI_ERROR);

  /* Back again, and then asked a question only the private bytes can answer.
   * This is the whole point of the pair: a converter that copied the three
   * named fields and left the rest passes every check above.
   */
  MPI_Status back;
  memset(&back, 0, sizeof back);
  CHECK_MPI(MPI_Status_f2c(f, &back));
  CHECK(back.MPI_SOURCE == status.MPI_SOURCE && back.MPI_TAG == status.MPI_TAG
            && back.MPI_ERROR == status.MPI_ERROR,
        "a status did not survive c2f/f2c");
  int back_count = -1;
  CHECK_MPI(MPI_Get_count(&back, MPI_INT, &back_count));
  CHECK(back_count == count,
        "MPI_Get_count on a status that went through Fortran says %d, not %d",
        back_count, count);

  /* The mpi_f08 pair, which the ABI makes the same 32 bytes again. */
  MPI_F08_Status f08;
  memset(&f08, 0x5a, sizeof f08);
  CHECK_MPI(MPI_Status_c2f08(&status, &f08));
  MPI_Status from08;
  memset(&from08, 0, sizeof from08);
  CHECK_MPI(MPI_Status_f082c(&f08, &from08));
  int f08_count = -1;
  CHECK_MPI(MPI_Get_count(&from08, MPI_INT, &f08_count));
  CHECK(f08_count == count,
        "MPI_Get_count after c2f08/f082c says %d, not %d", f08_count, count);
  CHECK(from08.MPI_TAG == status.MPI_TAG,
        "the tag did not survive c2f08/f082c");
}

/* --------------------------------------------- the status-consuming ten ---- */

static void test_status_consumers(void)
{
  if (rank == 0) printf("test_status_consumers\n");

  MPI_Status status;
  int        count = 0;
  fill_status(&status, &count);

  /* The four count queries must agree with each other on a contiguous
   * message of a predefined type, where elements and count coincide.
   */
  MPI_Count count_c = -1;
  int       ierror  = MPI_Get_count_c(&status, MPI_INT, &count_c);
  if (!unsupported(ierror, "MPI_Get_count_c")) {
    CHECK(ierror == MPI_SUCCESS, "MPI_Get_count_c returned %d", ierror);
    CHECK(count_c == count, "MPI_Get_count_c says %lld, MPI_Get_count %d",
          (long long)count_c, count);
  }

  int elements = -1;
  CHECK_MPI(MPI_Get_elements(&status, MPI_INT, &elements));
  CHECK(elements == count, "MPI_Get_elements says %d, MPI_Get_count %d",
        elements, count);

  MPI_Count elements_c = -1;
  ierror               = MPI_Get_elements_c(&status, MPI_INT, &elements_c);
  if (!unsupported(ierror, "MPI_Get_elements_c")) {
    CHECK(ierror == MPI_SUCCESS, "MPI_Get_elements_c returned %d", ierror);
    CHECK(elements_c == count, "MPI_Get_elements_c says %lld, not %d",
          (long long)elements_c, count);
  }

  MPI_Count elements_x = -1;
  ierror               = MPI_Get_elements_x(&status, MPI_INT, &elements_x);
  if (!unsupported(ierror, "MPI_Get_elements_x")) {
    CHECK(ierror == MPI_SUCCESS, "MPI_Get_elements_x returned %d", ierror);
    CHECK(elements_x == count, "MPI_Get_elements_x says %lld, not %d",
          (long long)elements_x, count);
  }

  int cancelled = -1;
  CHECK_MPI(MPI_Test_cancelled(&status, &cancelled));
  CHECK(!cancelled, "a completed MPI_Sendrecv reports itself cancelled");

  /* The inout half. MPI_Status_set_* writes into the implementation's own
   * status and the result has to come *back* into the ABI one -- a body that
   * converted in and forgot to convert out leaves the caller's status
   * unchanged, which the queries below would not notice if they were asked of
   * a fresh status instead of this one.
   */
  MPI_Status edited = status;
  ierror            = MPI_Status_set_cancelled(&edited, 1);
  if (!unsupported(ierror, "MPI_Status_set_cancelled")) {
    CHECK(ierror == MPI_SUCCESS, "MPI_Status_set_cancelled returned %d",
          ierror);
    int flag = -1;
    CHECK_MPI(MPI_Test_cancelled(&edited, &flag));
    CHECK(flag, "MPI_Status_set_cancelled(1) did not reach the status");
  }

  edited = status;
  ierror = MPI_Status_set_elements(&edited, MPI_INT, 2);
  if (!unsupported(ierror, "MPI_Status_set_elements")) {
    CHECK(ierror == MPI_SUCCESS, "MPI_Status_set_elements returned %d", ierror);
    int n = -1;
    CHECK_MPI(MPI_Get_elements(&edited, MPI_INT, &n));
    CHECK(n == 2, "MPI_Status_set_elements(2) reads back as %d", n);
  }

  edited = status;
  ierror = MPI_Status_set_elements_c(&edited, MPI_INT, 3);
  if (!unsupported(ierror, "MPI_Status_set_elements_c")) {
    CHECK(ierror == MPI_SUCCESS, "MPI_Status_set_elements_c returned %d",
          ierror);
    int n = -1;
    CHECK_MPI(MPI_Get_elements(&edited, MPI_INT, &n));
    CHECK(n == 3, "MPI_Status_set_elements_c(3) reads back as %d", n);
  }

  edited = status;
  ierror = MPI_Status_set_elements_x(&edited, MPI_INT, 1);
  if (!unsupported(ierror, "MPI_Status_set_elements_x")) {
    CHECK(ierror == MPI_SUCCESS, "MPI_Status_set_elements_x returned %d",
          ierror);
    MPI_Count n = -1;
    CHECK_MPI(MPI_Get_elements_x(&edited, MPI_INT, &n));
    CHECK(n == 1, "MPI_Status_set_elements_x(1) reads back as %lld",
          (long long)n);
  }

  /* And the status the edits were made from is untouched, which is what says
   * the bodies copied rather than aliased.
   */
  int still = -1;
  CHECK_MPI(MPI_Get_count(&status, MPI_INT, &still));
  CHECK(still == count, "editing a copy changed the original status");
}

/* ------------------------------------------------- output string buffers ---- */

/* Every one of these must leave string[*resultlen] == '\0'. That is the half
 * of #5.8's contract that is checkable without an implementation whose
 * MPI_MAX_* exceeds the ABI's, and it is the half a body reporting the
 * implementation's length instead of the copied length gets wrong.
 */
static void check_terminated(const char *what, const char *s, int resultlen,
                             int abi_max)
{
  CHECK(resultlen >= 0 && resultlen < abi_max,
        "%s reported resultlen %d, outside [0, %d)", what, resultlen, abi_max);
  if (resultlen < 0 || resultlen >= abi_max) return;
  CHECK(s[resultlen] == '\0', "%s did not terminate at resultlen %d", what,
        resultlen);

  /* Not `== resultlen`, which is what MPI's "length in printable characters"
   * reads like and is not what implementations do: Open MPI 5.0.6's
   * MPI_Get_library_version reports 119 for a 118-character string -- measured
   * against it natively, not through this wrapper, so it is its answer and not
   * a conversion of ours. The wrapper's contract is the one #5.8 states, that
   * the string terminates at the length reported, and passing the
   * implementation's own count through unchanged is what keeps it honest.
   */
  CHECK((int)strlen(s) <= resultlen, "%s says resultlen %d, strlen %d", what,
        resultlen, (int)strlen(s));
}

static void test_output_strings(void)
{
  if (rank == 0) printf("test_output_strings\n");

  {
    char name[MPI_MAX_PROCESSOR_NAME];
    int  resultlen = -1;
    memset(name, 0x5a, sizeof name);
    CHECK_MPI(MPI_Get_processor_name(name, &resultlen));
    check_terminated("MPI_Get_processor_name", name, resultlen,
                     MPI_MAX_PROCESSOR_NAME);
  }

  {
    char version[MPI_MAX_LIBRARY_VERSION_STRING];
    int  resultlen = -1;
    memset(version, 0x5a, sizeof version);
    CHECK_MPI(MPI_Get_library_version(version, &resultlen));
    check_terminated("MPI_Get_library_version", version, resultlen,
                     MPI_MAX_LIBRARY_VERSION_STRING);
    CHECK(resultlen > 0, "MPI_Get_library_version produced an empty string");
  }

  {
    char string[MPI_MAX_ERROR_STRING];
    int  resultlen = -1;
    memset(string, 0x5a, sizeof string);
    CHECK_MPI(MPI_Error_string(MPI_ERR_TRUNCATE, string, &resultlen));
    check_terminated("MPI_Error_string", string, resultlen,
                     MPI_MAX_ERROR_STRING);
  }

  /* The three *_get_name calls, each after a *_set_name, so the answer is one
   * this test chose rather than whatever the implementation defaults to.
   */
  {
    MPI_Comm comm;
    CHECK_MPI(MPI_Comm_dup(MPI_COMM_WORLD, &comm));
    CHECK_MPI(MPI_Comm_set_name(comm, "converters"));
    char name[MPI_MAX_OBJECT_NAME];
    int  resultlen = -1;
    memset(name, 0x5a, sizeof name);
    CHECK_MPI(MPI_Comm_get_name(comm, name, &resultlen));
    check_terminated("MPI_Comm_get_name", name, resultlen,
                     MPI_MAX_OBJECT_NAME);
    CHECK(strcmp(name, "converters") == 0,
          "MPI_Comm_get_name says \"%s\", not \"converters\"", name);
    CHECK_MPI(MPI_Comm_free(&comm));
  }

  {
    MPI_Datatype datatype;
    CHECK_MPI(MPI_Type_contiguous(2, MPI_DOUBLE, &datatype));
    CHECK_MPI(MPI_Type_commit(&datatype));
    CHECK_MPI(MPI_Type_set_name(datatype, "pair"));
    char name[MPI_MAX_OBJECT_NAME];
    int  resultlen = -1;
    memset(name, 0x5a, sizeof name);
    CHECK_MPI(MPI_Type_get_name(datatype, name, &resultlen));
    check_terminated("MPI_Type_get_name", name, resultlen,
                     MPI_MAX_OBJECT_NAME);
    CHECK(strcmp(name, "pair") == 0,
          "MPI_Type_get_name says \"%s\", not \"pair\"", name);
    CHECK_MPI(MPI_Type_free(&datatype));
  }

  /* The window is a means, not the subject, so a run that cannot create one
   * skips rather than fails. That is not hypothetical:
   * ci-scripts/linux-test.sh sets OMPI_MCA_btl_vader_single_copy_mechanism=none
   * and Open MPI 4.1.6 then answers MPI_ERR_WIN to MPI_Win_create with no
   * wrapper in sight -- see test/README.md's environment quirks, and
   * abi_tools_test, which skips its window keyvals for the same reason.
   */
  {
    int      buffer[8] = {0};
    MPI_Win  win;
    const int ierror = MPI_Win_create(buffer, (MPI_Aint)sizeof buffer,
                                      (int)sizeof(int), MPI_INFO_NULL,
                                      MPI_COMM_SELF, &win);
    if (ierror != MPI_SUCCESS) {
      if (rank == 0)
        printf("  skipping MPI_Win_get_name: MPI_Win_create returned %d in "
               "this environment\n", ierror);
    } else {
      CHECK_MPI(MPI_Win_set_name(win, "window"));
      char name[MPI_MAX_OBJECT_NAME];
      int  resultlen = -1;
      memset(name, 0x5a, sizeof name);
      CHECK_MPI(MPI_Win_get_name(win, name, &resultlen));
      check_terminated("MPI_Win_get_name", name, resultlen,
                       MPI_MAX_OBJECT_NAME);
      CHECK(strcmp(name, "window") == 0,
            "MPI_Win_get_name says \"%s\", not \"window\"", name);
      CHECK_MPI(MPI_Win_free(&win));
    }
  }

  /* MPI_Info_get_nthkey is one of the four with no resultlen, so the NUL is
   * the only terminator the caller has -- and one of the four that answers
   * MPI_ERR_INTERN rather than truncating.
   */
  {
    MPI_Info info;
    CHECK_MPI(MPI_Info_create(&info));
    CHECK_MPI(MPI_Info_set(info, "abi_converters_key", "value"));
    int nkeys = 0;
    CHECK_MPI(MPI_Info_get_nkeys(info, &nkeys));
    CHECK(nkeys == 1, "a one-key info reports %d keys", nkeys);
    char key[MPI_MAX_INFO_KEY];
    memset(key, 0x5a, sizeof key);
    CHECK_MPI(MPI_Info_get_nthkey(info, 0, key));
    CHECK(strcmp(key, "abi_converters_key") == 0,
          "MPI_Info_get_nthkey says \"%s\"", key);
    CHECK_MPI(MPI_Info_free(&info));
  }

  /* MPI_File_get_view's datarep, and its three other converted parameters
   * beside it. The file is per-rank so the test needs no collective agreement
   * about a shared path.
   */
  {
    char     path[64];
    MPI_File fh;
    snprintf(path, sizeof path, "abi_converters_%d.tmp", rank);
    const int ierror =
        MPI_File_open(MPI_COMM_SELF, path,
                      MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE,
                      MPI_INFO_NULL, &fh);
    if (!unsupported(ierror, "MPI_File_open")) {
      CHECK(ierror == MPI_SUCCESS, "MPI_File_open returned %d", ierror);
      if (ierror == MPI_SUCCESS) {
        MPI_Offset   disp     = -1;
        MPI_Datatype etype    = MPI_DATATYPE_NULL;
        MPI_Datatype filetype = MPI_DATATYPE_NULL;
        char         datarep[MPI_MAX_DATAREP_STRING];
        memset(datarep, 0x5a, sizeof datarep);
        CHECK_MPI(MPI_File_get_view(fh, &disp, &etype, &filetype, datarep));
        CHECK(disp == 0, "a fresh file's view starts at %lld", (long long)disp);
        CHECK(etype == MPI_BYTE, "a fresh file's etype is not MPI_BYTE");
        CHECK(filetype == MPI_BYTE, "a fresh file's filetype is not MPI_BYTE");
        CHECK(strcmp(datarep, "native") == 0,
              "a fresh file's datarep is \"%s\", not \"native\"", datarep);
        CHECK_MPI(MPI_File_close(&fh));
      }
    }
  }
}

/* ------------------------------------------------------- MPI_Abi_* ---- */

static void test_abi_version(void)
{
  if (rank == 0) printf("test_abi_version\n");

  int major = -99, minor = -99;
  CHECK_MPI(MPI_Abi_get_version(&major, &minor));
  CHECK(major == MPI_ABI_VERSION && minor == MPI_ABI_SUBVERSION,
        "MPI_Abi_get_version says %d.%d, the header says %d.%d", major, minor,
        MPI_ABI_VERSION, MPI_ABI_SUBVERSION);
  CHECK(major != -1, "MPI_Abi_get_version reports no ABI support");
}

static void test_abi_info(void)
{
  if (rank == 0) printf("test_abi_info\n");

  MPI_Info  info   = MPI_INFO_NULL;
  const int ierror = MPI_Abi_get_info(&info);
  if (unsupported(ierror, "MPI_Abi_get_info")) return;
  CHECK(ierror == MPI_SUCCESS, "MPI_Abi_get_info returned %d", ierror);
  if (ierror != MPI_SUCCESS) return;

  static const struct {
    const char *key;
    int         want;
  } expected[] = {{"mpi_aint_size", (int)sizeof(MPI_Aint)},
                  {"mpi_count_size", (int)sizeof(MPI_Count)},
                  {"mpi_offset_size", (int)sizeof(MPI_Offset)}};

  for (size_t i = 0; i < sizeof expected / sizeof expected[0]; ++i) {
    char value[32];
    int  flag = 0;
    memset(value, 0, sizeof value);
    CHECK_MPI(info_get(info, expected[i].key, value, (int)sizeof value,
                       &flag));
    CHECK(flag, "MPI_Abi_get_info has no \"%s\"", expected[i].key);
    if (flag)
      CHECK(atoi(value) == expected[i].want, "\"%s\" is \"%s\", not %d",
            expected[i].key, value, expected[i].want);
  }
  CHECK_MPI(MPI_Info_free(&info));
}

/* The Fortran registration pair. Both setters are once-only, so the order
 * here matters: get before set, set, get again, then set again and require
 * MPI_ERR_ABI.
 */
static void test_abi_fortran(void)
{
  if (rank == 0) printf("test_abi_fortran\n");

  MPI_Info info = (MPI_Info)0x1;
  int      ierror = MPI_Abi_get_fortran_info(&info);
  if (!unsupported(ierror, "MPI_Abi_get_fortran_info")) {
    CHECK(ierror == MPI_SUCCESS, "MPI_Abi_get_fortran_info returned %d",
          ierror);
    CHECK(info == MPI_INFO_NULL,
          "MPI_Abi_get_fortran_info answered before anything was set");
  }

  int is_set = -1;
  int t = 1, f = 0;
  ierror = MPI_Abi_get_fortran_booleans(sizeof(int), &t, &f, &is_set);
  if (!unsupported(ierror, "MPI_Abi_get_fortran_booleans")) {
    CHECK(ierror == MPI_SUCCESS, "MPI_Abi_get_fortran_booleans returned %d",
          ierror);
    CHECK(is_set == 0, "the Fortran booleans report themselves already set");
  }

  /* Fortran's .TRUE. is not required to be C's 1, so the values registered
   * here are deliberately not 1 and 0 -- a body that "helpfully" normalized
   * them would be caught by reading them back.
   */
  int registered_true  = -1;
  int registered_false = 0;
  ierror = MPI_Abi_set_fortran_booleans(sizeof(int), &registered_true,
                                        &registered_false);
  if (!unsupported(ierror, "MPI_Abi_set_fortran_booleans")) {
    CHECK(ierror == MPI_SUCCESS, "MPI_Abi_set_fortran_booleans returned %d",
          ierror);

    int got_true = 0, got_false = 0;
    is_set = -1;
    CHECK_MPI(MPI_Abi_get_fortran_booleans(sizeof(int), &got_true, &got_false,
                                           &is_set));
    CHECK(is_set == 1, "the Fortran booleans did not register");
    CHECK(got_true == registered_true && got_false == registered_false,
          "the Fortran booleans read back as %d/%d, not %d/%d", got_true,
          got_false, registered_true, registered_false);

    /* A different LOGICAL size is a different question, and the honest answer
     * is that nothing is known about it.
     */
    char wide_true[8] = {0}, wide_false[8] = {0};
    int  wide_is_set  = -1;
    CHECK_MPI(MPI_Abi_get_fortran_booleans(8, wide_true, wide_false,
                                           &wide_is_set));
    CHECK(wide_is_set == 0,
          "a LOGICAL(8) query answered from a LOGICAL(4) registration");

    /* "Only the first call ... affects the state of the MPI library; all
     * subsequent calls will return the error code MPI_ERR_ABI."
     */
    const int again = MPI_Abi_set_fortran_booleans(sizeof(int),
                                                   &registered_true,
                                                   &registered_false);
    CHECK(again == MPI_ERR_ABI,
          "a second MPI_Abi_set_fortran_booleans returned %d, not MPI_ERR_ABI",
          again);
  }

  MPI_Info registered = MPI_INFO_NULL;
  CHECK_MPI(MPI_Info_create(&registered));
  CHECK_MPI(MPI_Info_set(registered, "mpi_logical_size", "4"));
  ierror = MPI_Abi_set_fortran_info(registered);
  if (!unsupported(ierror, "MPI_Abi_set_fortran_info")) {
    CHECK(ierror == MPI_SUCCESS, "MPI_Abi_set_fortran_info returned %d",
          ierror);

    /* Freed straight away, which is the point of duplicating it: the getter
     * still has to answer.
     */
    CHECK_MPI(MPI_Info_free(&registered));

    MPI_Info back = MPI_INFO_NULL;
    CHECK_MPI(MPI_Abi_get_fortran_info(&back));
    CHECK(back != MPI_INFO_NULL,
          "MPI_Abi_get_fortran_info forgot what was set");
    if (back != MPI_INFO_NULL) {
      char value[32];
      int  flag = 0;
      memset(value, 0, sizeof value);
      CHECK_MPI(info_get(back, "mpi_logical_size", value, (int)sizeof value,
                         &flag));
      CHECK(flag && strcmp(value, "4") == 0,
            "the registered Fortran info reads back as \"%s\" (flag %d)",
            value, flag);
    }

    MPI_Info second = MPI_INFO_NULL;
    CHECK_MPI(MPI_Info_create(&second));
    const int again = MPI_Abi_set_fortran_info(second);
    CHECK(again == MPI_ERR_ABI,
          "a second MPI_Abi_set_fortran_info returned %d, not MPI_ERR_ABI",
          again);
    CHECK_MPI(MPI_Info_free(&second));
  } else {
    CHECK_MPI(MPI_Info_free(&registered));
  }
}

int main(int argc, char **argv)
{
  CHECK_MPI(MPI_Init(&argc, &argv));
  CHECK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &size));
  CHECK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));

  /* Errors have to come back as return codes for any of the above to be
   * readable; the default is to abort.
   */
  CHECK_MPI(MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN));
  CHECK_MPI(MPI_Comm_set_errhandler(MPI_COMM_SELF, MPI_ERRORS_RETURN));

  if (mpiabi_expect_ranks("abi_converters_test", size, rank)) {
    MPI_Finalize();
    return 1;
  }

  if (rank == 0)
    printf("abi_converters_test: %d rank%s\n", size, size == 1 ? "" : "s");

  test_serialization();
  test_c2f();
  test_status_converters();
  test_status_consumers();
  test_output_strings();
  test_abi_version();
  test_abi_info();
  test_abi_fortran();

  int total = 0;
  CHECK_MPI(MPI_Allreduce(&failures, &total, 1, MPI_INT, MPI_SUM,
                          MPI_COMM_WORLD));
  if (rank == 0)
    printf("abi_converters_test: %d failure(s) across %d ranks\n", total, size);

  CHECK_MPI(MPI_Finalize());
  return total != 0;
}
