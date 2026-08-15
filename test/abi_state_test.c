/* abi_state_test -- the black-box half of S4b.
 *
 * The same shape as the other three: an ordinary MPI application over the ABI
 * header, linking libmpi_abi and nothing else. What it covers is the half of
 * the hand-written set that owns state -- the initialization state machine,
 * the six trampoline pools, the three extra-state callback families, the
 * attached buffer, the dynamic error-code registry, spawn and MPI_Pcontrol.
 *
 * Each group is written against what a plausible-but-wrong body gets wrong,
 * not against the happy path:
 *
 *  - **MPI_Initialized before MPI_Init.** These two are answered from the
 *    wrapper's own state (NOTES.md #8), and the only place that is observable
 *    is before initialization and after finalization -- so this test asks
 *    both questions in the two places no other test can reach.
 *  - **the handle a trampoline hands back.** Every error-handler callback
 *    receives the object it fired for, and the implementation gives our
 *    trampoline *its* handle. A trampoline that forwarded the bits
 *    unconverted still runs, still prints, and hands the application a
 *    communicator it cannot use -- so each of the four checks the handle
 *    against the one it set the handler on.
 *  - **MPI_COMM_DUP_FN.** The ABI spells it (function *)0x1 and both
 *    implementations spell it as a real function, so a body that passed the
 *    value through would hand the implementation a function pointer of 1, and
 *    one that wrapped it in a trampoline would call through it. The test
 *    installs it as the copy callback and then checks that MPI_Comm_dup
 *    actually copied the attribute -- which only happens if the
 *    implementation's own function ran.
 *  - **a generalized request's status.** The user's query callback fills an
 *    *ABI* status and the implementation reads its own, so the blob has to
 *    cross in both directions inside the trampoline. MPI_Get_count on the
 *    status MPI_Wait returns is what a body that copied only the named fields
 *    fails.
 *  - **MPI_BUFFER_AUTOMATIC.** It is (void *)2 in the ABI and (void *)-2 in
 *    MPICH, and Open MPI 5.0.6 does not have it at all -- so the same two
 *    calls exercise a sentinel translation on one implementation and the
 *    wrapper's own emulation on the other, and both must answer
 *    MPI_BUFFER_AUTOMATIC from the detach.
 *  - **a dynamic error class.** MPI_ERR_LASTCODE is 16383 in the ABI and
 *    0x3fffffff in MPICH, so a class that was passed through rather than
 *    renumbered arrives as a number the ABI header says cannot be an error
 *    code. The test asks for the class, then asks MPI_Error_class and
 *    MPI_Error_string about the code -- which is the registry's round trip
 *    through two entry points that never heard of it.
 *
 * Two rows are gaps rather than passes, and they are named here so that a
 * green run is not read as more than it is:
 *
 *  - **spawn needs a launcher**, so it runs where one works (MPICH here) and
 *    is skipped otherwise. STAGES.md S4b says so; test/README.md repeats it.
 *  - **MPI_T's event callbacks have no oracle here.** Open MPI 5.0.6 has no
 *    event interface, and MPICH 4.3.1 declares every entry point and reports
 *    zero event types, so no registration handle exists to key the map in
 *    src/mpiwrapper/toolevents.c with. What is tested is that the entry
 *    points are reachable and report properly.
 */

#include <mpi.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* ------------------------------------------------------------- lifecycle -- */

/* Called before MPI_Init_thread, so nothing here may use a communicator --
 * and CHECK's own `rank` is still 0, which is what we want for a message from
 * a process that does not yet have one.
 */
static void test_before_init(void)
{
  int flag = 1;
  CHECK_MPI(MPI_Initialized(&flag));
  CHECK(!flag, "MPI_Initialized answered true before MPI_Init");

  flag = 1;
  CHECK_MPI(MPI_Finalized(&flag));
  CHECK(!flag, "MPI_Finalized answered true before MPI_Init");
}

static void test_after_init(int provided)
{
  if (rank == 0) printf("test_lifecycle\n");

  int flag = 0;
  CHECK_MPI(MPI_Initialized(&flag));
  CHECK(flag, "MPI_Initialized answered false after MPI_Init_thread");

  flag = 1;
  CHECK_MPI(MPI_Finalized(&flag));
  CHECK(!flag, "MPI_Finalized answered true before MPI_Finalize");

  CHECK(provided == MPI_THREAD_SINGLE || provided == MPI_THREAD_FUNNELED
            || provided == MPI_THREAD_SERIALIZED
            || provided == MPI_THREAD_MULTIPLE,
        "MPI_Init_thread provided %d, which is no ABI thread level", provided);

  /* The level the wrapper reports is the implementation's own, converted --
   * there is no remembered copy (NOTES.md #8) -- so MPI_Query_thread, which
   * is generated and goes through the same table, must agree with it.
   */
  int queried = -1;
  CHECK_MPI(MPI_Query_thread(&queried));
  CHECK(queried == provided,
        "MPI_Query_thread says %d where MPI_Init_thread provided %d", queried,
        provided);
}

/* -------------------------------------------------------- error handlers -- */

static MPI_Comm    seen_comm;
static MPI_File    seen_file;
static MPI_Win     seen_win;
static MPI_Session seen_session;
static int         seen_code[4];
static int         errh_calls[4];

static void comm_errh(MPI_Comm *comm, int *code, ...)
{
  seen_comm    = *comm;
  seen_code[0] = *code;
  ++errh_calls[0];
}

static void file_errh(MPI_File *file, int *code, ...)
{
  seen_file    = *file;
  seen_code[1] = *code;
  ++errh_calls[1];
}

static void win_errh(MPI_Win *win, int *code, ...)
{
  seen_win     = *win;
  seen_code[2] = *code;
  ++errh_calls[2];
}

static void session_errh(MPI_Session *session, int *code, ...)
{
  seen_session = *session;
  seen_code[3] = *code;
  ++errh_calls[3];
}

/* A file of this rank's own: two ranks opening one path would be two
 * processes' error handlers firing on one object.
 */
static int open_scratch_file(MPI_File *fh)
{
  char name[64];
  snprintf(name, sizeof name, "abi_state_test.%d.tmp", rank);
  return MPI_File_open(MPI_COMM_SELF, name,
                       MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE,
                       MPI_INFO_NULL, fh);
}

static void test_errhandlers(void)
{
  if (rank == 0) printf("test_errhandlers\n");

  /* --- the communicator pool, which is S1's and the template for the rest */
  MPI_Comm       dup = MPI_COMM_NULL;
  MPI_Errhandler eh  = MPI_ERRHANDLER_NULL;
  CHECK_MPI(MPI_Comm_dup(MPI_COMM_SELF, &dup));
  CHECK_MPI(MPI_Comm_create_errhandler(comm_errh, &eh));
  CHECK_MPI(MPI_Comm_set_errhandler(dup, eh));
  CHECK_MPI(MPI_Comm_call_errhandler(dup, MPI_ERR_TOPOLOGY));
  CHECK(errh_calls[0] == 1, "the communicator error handler ran %d times",
        errh_calls[0]);
  CHECK(seen_comm == dup,
        "the communicator error handler was given a handle that is not the "
        "communicator it was set on");
  CHECK(seen_code[0] == MPI_ERR_TOPOLOGY,
        "the communicator error handler saw error code %d, not "
        "MPI_ERR_TOPOLOGY (%d)",
        seen_code[0], MPI_ERR_TOPOLOGY);
  CHECK_MPI(MPI_Errhandler_free(&eh));
  CHECK_MPI(MPI_Comm_free(&dup));

  /* --- the file pool */
  MPI_File fh     = MPI_FILE_NULL;
  int      ierror = open_scratch_file(&fh);
  if (ierror != MPI_SUCCESS) {
    if (rank == 0)
      printf("  skipping the file error handler: MPI_File_open returned %d\n",
             ierror);
  } else {
    MPI_Errhandler feh = MPI_ERRHANDLER_NULL;
    ierror             = MPI_File_create_errhandler(file_errh, &feh);
    if (!unsupported(ierror, "MPI_File_create_errhandler")) {
      CHECK_MPI(ierror);
      CHECK_MPI(MPI_File_set_errhandler(fh, feh));
      CHECK_MPI(MPI_File_call_errhandler(fh, MPI_ERR_AMODE));
      CHECK(errh_calls[1] == 1, "the file error handler ran %d times",
            errh_calls[1]);
      CHECK(seen_file == fh,
            "the file error handler was given a handle that is not the file "
            "it was set on");
      CHECK(seen_code[1] == MPI_ERR_AMODE,
            "the file error handler saw error code %d, not MPI_ERR_AMODE (%d)",
            seen_code[1], MPI_ERR_AMODE);
      CHECK_MPI(MPI_Errhandler_free(&feh));
    }
    CHECK_MPI(MPI_File_close(&fh));
  }

  /* --- the window pool. The window is a means here, not the subject, so a
   * machine where MPI_Win_create does not work skips rather than fails (the
   * quirk test/README.md records for Open MPI 4.1 with vader).
   */
  int      base = 0;
  MPI_Win  win  = MPI_WIN_NULL;
  ierror = MPI_Win_create(&base, (MPI_Aint)sizeof base, (int)sizeof base,
                          MPI_INFO_NULL, MPI_COMM_SELF, &win);
  if (ierror != MPI_SUCCESS) {
    if (rank == 0)
      printf("  skipping the window error handler: MPI_Win_create returned "
             "%d\n",
             ierror);
  } else {
    MPI_Errhandler weh = MPI_ERRHANDLER_NULL;
    ierror             = MPI_Win_create_errhandler(win_errh, &weh);
    if (!unsupported(ierror, "MPI_Win_create_errhandler")) {
      CHECK_MPI(ierror);
      CHECK_MPI(MPI_Win_set_errhandler(win, weh));
      CHECK_MPI(MPI_Win_call_errhandler(win, MPI_ERR_RMA_SYNC));
      CHECK(errh_calls[2] == 1, "the window error handler ran %d times",
            errh_calls[2]);
      CHECK(seen_win == win,
            "the window error handler was given a handle that is not the "
            "window it was set on");
      CHECK(seen_code[2] == MPI_ERR_RMA_SYNC,
            "the window error handler saw error code %d, not "
            "MPI_ERR_RMA_SYNC (%d)",
            seen_code[2], MPI_ERR_RMA_SYNC);
      CHECK_MPI(MPI_Errhandler_free(&weh));
    }
    CHECK_MPI(MPI_Win_free(&win));
  }

  /* --- the session pool, which also exercises MPI_Session_init/_finalize */
  MPI_Session session = MPI_SESSION_NULL;
  ierror = MPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_RETURN, &session);
  if (!unsupported(ierror, "MPI_Session_init")) {
    CHECK_MPI(ierror);
    MPI_Errhandler seh = MPI_ERRHANDLER_NULL;
    ierror             = MPI_Session_create_errhandler(session_errh, &seh);
    if (!unsupported(ierror, "MPI_Session_create_errhandler")) {
      CHECK_MPI(ierror);
      CHECK_MPI(MPI_Session_set_errhandler(session, seh));
      CHECK_MPI(MPI_Session_call_errhandler(session, MPI_ERR_SESSION));
      CHECK(errh_calls[3] == 1, "the session error handler ran %d times",
            errh_calls[3]);
      CHECK(seen_session == session,
            "the session error handler was given a handle that is not the "
            "session it was set on");
      CHECK(seen_code[3] == MPI_ERR_SESSION,
            "the session error handler saw error code %d, not "
            "MPI_ERR_SESSION (%d)",
            seen_code[3], MPI_ERR_SESSION);
      CHECK_MPI(MPI_Errhandler_free(&seh));
    }
    CHECK_MPI(MPI_Session_finalize(&session));
    CHECK(session == MPI_SESSION_NULL,
          "MPI_Session_finalize left the handle set");
  }
}

/* ------------------------------------------------------------ user ops ---- */

static void sum_op(void *invec, void *inoutvec, int *len,
                   MPI_Datatype *datatype)
{
  int *in = invec, *inout = inoutvec;
  /* The datatype is the reason a user op needs a trampoline at all: the
   * implementation hands us its own and the callback is written against the
   * ABI's.
   */
  if (*datatype != MPI_INT) {
    ++failures;
    printf("[%d] FAIL: a user reduction was given a datatype that is not "
           "MPI_INT\n",
           rank);
    return;
  }
  for (int i = 0; i < *len; ++i) inout[i] += in[i];
}

static void sum_op_c(void *invec, void *inoutvec, MPI_Count *len,
                     MPI_Datatype *datatype)
{
  int *in = invec, *inout = inoutvec;
  if (*datatype != MPI_INT) {
    ++failures;
    printf("[%d] FAIL: a large-count user reduction was given a datatype that "
           "is not MPI_INT\n",
           rank);
    return;
  }
  for (MPI_Count i = 0; i < *len; ++i) inout[i] += in[i];
}

static void test_user_ops(void)
{
  if (rank == 0) printf("test_user_ops\n");

  const int in = rank + 1;
  int       out;

  MPI_Op op = MPI_OP_NULL;
  CHECK_MPI(MPI_Op_create(sum_op, 1, &op));
  out = 0;
  CHECK_MPI(MPI_Allreduce(&in, &out, 1, MPI_INT, op, MPI_COMM_WORLD));
  CHECK(out == size * (size + 1) / 2, "a user reduction summed to %d, not %d",
        out, size * (size + 1) / 2);
  CHECK_MPI(MPI_Op_free(&op));

  MPI_Op    op_c   = MPI_OP_NULL;
  const int ierror = MPI_Op_create_c(sum_op_c, 1, &op_c);
  if (!unsupported(ierror, "MPI_Op_create_c")) {
    CHECK_MPI(ierror);
    out = 0;
    CHECK_MPI(MPI_Allreduce_c(&in, &out, 1, MPI_INT, op_c, MPI_COMM_WORLD));
    CHECK(out == size * (size + 1) / 2,
          "a large-count user reduction summed to %d, not %d", out,
          size * (size + 1) / 2);
    CHECK_MPI(MPI_Op_free(&op_c));
  }
}

/* --------------------------------------------------------------- keyvals -- */

static int  attr_payload = 0xabc;
static int  attr_state   = 0xdef;
static int  copy_calls, delete_calls;
static int  copy_keyval, delete_keyval;
static void *copy_extra, *delete_extra;

static int comm_copy(MPI_Comm oldcomm, int keyval, void *extra_state,
                     void *attribute_val_in, void *attribute_val_out, int *flag)
{
  (void)oldcomm;
  ++copy_calls;
  copy_keyval = keyval;
  copy_extra  = extra_state;
  *(void **)attribute_val_out = attribute_val_in;
  *flag                       = 1;
  return MPI_SUCCESS;
}

static int comm_delete(MPI_Comm comm, int keyval, void *attribute_val,
                       void *extra_state)
{
  (void)comm;
  (void)attribute_val;
  ++delete_calls;
  delete_keyval = keyval;
  delete_extra  = extra_state;
  return MPI_SUCCESS;
}

static int type_copy(MPI_Datatype oldtype, int keyval, void *extra_state,
                     void *attribute_val_in, void *attribute_val_out, int *flag)
{
  (void)oldtype;
  ++copy_calls;
  copy_keyval = keyval;
  copy_extra  = extra_state;
  *(void **)attribute_val_out = attribute_val_in;
  *flag                       = 1;
  return MPI_SUCCESS;
}

static int type_delete(MPI_Datatype datatype, int keyval, void *attribute_val,
                       void *extra_state)
{
  (void)datatype;
  (void)attribute_val;
  ++delete_calls;
  delete_keyval = keyval;
  delete_extra  = extra_state;
  return MPI_SUCCESS;
}

static void test_keyvals(void)
{
  if (rank == 0) printf("test_keyvals\n");

  /* --- a communicator key with callbacks of our own */
  int keyval = MPI_KEYVAL_INVALID;
  CHECK_MPI(MPI_Comm_create_keyval(comm_copy, comm_delete, &keyval,
                                   &attr_state));
  CHECK(keyval != MPI_KEYVAL_INVALID, "MPI_Comm_create_keyval gave no keyval");

  MPI_Comm first = MPI_COMM_NULL, second = MPI_COMM_NULL;
  CHECK_MPI(MPI_Comm_dup(MPI_COMM_SELF, &first));
  CHECK_MPI(MPI_Comm_set_attr(first, keyval, &attr_payload));

  copy_calls = delete_calls = 0;
  CHECK_MPI(MPI_Comm_dup(first, &second));
  CHECK(copy_calls == 1, "the copy callback ran %d times on MPI_Comm_dup",
        copy_calls);
  CHECK(copy_keyval == keyval,
        "the copy callback was given keyval %d, not the %d it was created as",
        copy_keyval, keyval);
  CHECK(copy_extra == &attr_state,
        "the copy callback was given the wrong extra_state");

  void *back = NULL;
  int   flag = 0;
  CHECK_MPI(MPI_Comm_get_attr(second, keyval, &back, &flag));
  CHECK(flag && back == &attr_payload,
        "the duplicated communicator's attribute is %p (flag %d)", back, flag);

  CHECK_MPI(MPI_Comm_free(&second));
  CHECK_MPI(MPI_Comm_free(&first));
  CHECK(delete_calls == 2, "the delete callback ran %d times, not twice",
        delete_calls);
  CHECK(delete_keyval == keyval,
        "the delete callback was given keyval %d, not %d", delete_keyval,
        keyval);
  CHECK(delete_extra == &attr_state,
        "the delete callback was given the wrong extra_state");
  CHECK_MPI(MPI_Comm_free_keyval(&keyval));
  CHECK(keyval == MPI_KEYVAL_INVALID,
        "MPI_Comm_free_keyval left the keyval set");

  /* --- the predefined attribute functions, which are *sentinels*: the ABI
   * spells MPI_COMM_DUP_FN as (function *)0x1 and both implementations spell
   * it as a real function. What proves the substitution happened is that the
   * attribute survives MPI_Comm_dup without any callback of ours running.
   */
  int dupkey = MPI_KEYVAL_INVALID;
  CHECK_MPI(MPI_Comm_create_keyval(MPI_COMM_DUP_FN, MPI_COMM_NULL_DELETE_FN,
                                   &dupkey, NULL));
  MPI_Comm a = MPI_COMM_NULL, b = MPI_COMM_NULL;
  CHECK_MPI(MPI_Comm_dup(MPI_COMM_SELF, &a));
  CHECK_MPI(MPI_Comm_set_attr(a, dupkey, &attr_payload));
  copy_calls = 0;
  CHECK_MPI(MPI_Comm_dup(a, &b));
  CHECK(copy_calls == 0,
        "a callback of ours ran for MPI_COMM_DUP_FN, which is the "
        "implementation's own function");
  back = NULL;
  flag = 0;
  CHECK_MPI(MPI_Comm_get_attr(b, dupkey, &back, &flag));
  CHECK(flag && back == &attr_payload,
        "MPI_COMM_DUP_FN did not copy the attribute (%p, flag %d)", back, flag);
  CHECK_MPI(MPI_Comm_free(&b));
  CHECK_MPI(MPI_Comm_free(&a));
  CHECK_MPI(MPI_Comm_free_keyval(&dupkey));

  /* --- a datatype key, which is the same mechanism against another class */
  int typekey = MPI_KEYVAL_INVALID;
  CHECK_MPI(MPI_Type_create_keyval(type_copy, type_delete, &typekey,
                                   &attr_state));
  MPI_Datatype t1 = MPI_DATATYPE_NULL, t2 = MPI_DATATYPE_NULL;
  CHECK_MPI(MPI_Type_dup(MPI_INT, &t1));
  CHECK_MPI(MPI_Type_set_attr(t1, typekey, &attr_payload));
  copy_calls = delete_calls = 0;
  CHECK_MPI(MPI_Type_dup(t1, &t2));
  CHECK(copy_calls == 1, "the datatype copy callback ran %d times", copy_calls);
  CHECK(copy_keyval == typekey,
        "the datatype copy callback was given keyval %d, not %d", copy_keyval,
        typekey);
  back = NULL;
  flag = 0;
  CHECK_MPI(MPI_Type_get_attr(t2, typekey, &back, &flag));
  CHECK(flag && back == &attr_payload,
        "the duplicated datatype's attribute is %p (flag %d)", back, flag);
  CHECK_MPI(MPI_Type_free(&t2));
  CHECK_MPI(MPI_Type_free(&t1));
  CHECK(delete_calls == 2, "the datatype delete callback ran %d times",
        delete_calls);
  CHECK_MPI(MPI_Type_free_keyval(&typekey));

  /* --- a window key. Only the sentinel form, because the window is a means
   * here as it is above, and a machine without working RMA should skip rather
   * than fail.
   */
  int     base   = 0;
  MPI_Win win    = MPI_WIN_NULL;
  int     ierror = MPI_Win_create(&base, (MPI_Aint)sizeof base,
                                  (int)sizeof base, MPI_INFO_NULL,
                                  MPI_COMM_SELF, &win);
  if (ierror != MPI_SUCCESS) {
    if (rank == 0)
      printf("  skipping the window keyval: MPI_Win_create returned %d\n",
             ierror);
    return;
  }
  int winkey = MPI_KEYVAL_INVALID;
  ierror = MPI_Win_create_keyval(MPI_WIN_NULL_COPY_FN, MPI_WIN_NULL_DELETE_FN,
                                 &winkey, NULL);
  if (!unsupported(ierror, "MPI_Win_create_keyval")) {
    CHECK_MPI(ierror);
    CHECK(winkey != MPI_KEYVAL_INVALID, "MPI_Win_create_keyval gave no keyval");
    CHECK_MPI(MPI_Win_set_attr(win, winkey, &attr_payload));
    back = NULL;
    flag = 0;
    CHECK_MPI(MPI_Win_get_attr(win, winkey, &back, &flag));
    CHECK(flag && back == &attr_payload,
          "the window attribute reads back as %p (flag %d)", back, flag);
    CHECK_MPI(MPI_Win_free_keyval(&winkey));
  }
  CHECK_MPI(MPI_Win_free(&win));
}

/* --------------------------------------------------- generalized requests -- */

static int   gq_query_calls, gq_free_calls, gq_cancel_calls;
static void *gq_extra_seen;

static int gq_query(void *extra_state, MPI_Status *status)
{
  ++gq_query_calls;
  gq_extra_seen = extra_state;

  /* The callback fills an *ABI* status, which the trampoline has to convert
   * back into the implementation's before MPI reads it.
   */
  status->MPI_SOURCE = 7;
  status->MPI_TAG    = 11;
  status->MPI_ERROR  = MPI_SUCCESS;
  CHECK_MPI(MPI_Status_set_elements(status, MPI_INT, 3));
  CHECK_MPI(MPI_Status_set_cancelled(status, 0));
  return MPI_SUCCESS;
}

static int gq_free(void *extra_state)
{
  ++gq_free_calls;
  CHECK(extra_state == &attr_state,
        "the generalized request's free callback got the wrong extra_state");
  return MPI_SUCCESS;
}

static int gq_cancel(void *extra_state, int complete)
{
  (void)extra_state;
  (void)complete;
  ++gq_cancel_calls;
  return MPI_SUCCESS;
}

static void test_grequest(void)
{
  if (rank == 0) printf("test_grequest\n");

  MPI_Request request = MPI_REQUEST_NULL;
  const int   ierror =
      MPI_Grequest_start(gq_query, gq_free, gq_cancel, &attr_state, &request);
  if (unsupported(ierror, "MPI_Grequest_start")) return;
  CHECK_MPI(ierror);
  CHECK(request != MPI_REQUEST_NULL, "MPI_Grequest_start gave no request");

  CHECK_MPI(MPI_Grequest_complete(request));

  MPI_Status status;
  memset(&status, 0, sizeof status);
  CHECK_MPI(MPI_Wait(&request, &status));

  CHECK(gq_query_calls == 1, "the query callback ran %d times", gq_query_calls);
  CHECK(gq_free_calls == 1, "the free callback ran %d times", gq_free_calls);
  CHECK(gq_cancel_calls == 0, "the cancel callback ran %d times",
        gq_cancel_calls);
  CHECK(gq_extra_seen == &attr_state,
        "the query callback got the wrong extra_state");

  CHECK(status.MPI_SOURCE == 7 && status.MPI_TAG == 11,
        "the status the callback filled arrived as source %d, tag %d",
        status.MPI_SOURCE, status.MPI_TAG);

  /* The one check a body that copied only the named fields fails: the count
   * lives in the implementation's private bytes, so it survives only if the
   * whole blob crossed in both directions.
   */
  int count = -1;
  CHECK_MPI(MPI_Get_count(&status, MPI_INT, &count));
  CHECK(count == 3, "MPI_Get_count on the callback's status is %d, not 3",
        count);
}

/* ------------------------------------------------------ attached buffers -- */

static void bounce_a_bsend(const char *what)
{
  const int payload = 0x5eed;
  int       got     = 0;
  CHECK_MPI(MPI_Bsend(&payload, 1, MPI_INT, 0, 99, MPI_COMM_SELF));
  CHECK_MPI(MPI_Recv(&got, 1, MPI_INT, 0, 99, MPI_COMM_SELF,
                     MPI_STATUS_IGNORE));
  CHECK(got == payload, "a buffered send through %s delivered 0x%x", what, got);
}

static void test_buffers(void)
{
  if (rank == 0) printf("test_buffers\n");

  /* --- a buffer of the application's own, which must come back unchanged */
  const int   bytes  = 4096 + MPI_BSEND_OVERHEAD;
  void *const buffer = malloc((size_t)bytes);
  CHECK(buffer != NULL, "out of memory");
  if (!buffer) return;

  int ierror = MPI_Buffer_attach(buffer, bytes);
  if (unsupported(ierror, "MPI_Buffer_attach")) {
    free(buffer);
    return;
  }
  CHECK_MPI(ierror);
  bounce_a_bsend("an explicit buffer");

  void *back     = NULL;
  int   backsize = 0;
  CHECK_MPI(MPI_Buffer_detach(&back, &backsize));
  CHECK(back == buffer, "MPI_Buffer_detach answered %p, not the %p attached",
        back, buffer);
  CHECK(backsize == bytes, "MPI_Buffer_detach answered size %d, not %d",
        backsize, bytes);
  free(buffer);

  /* --- MPI_BUFFER_AUTOMATIC. A sentinel translation where the
   * implementation has the mode (MPICH spells it (void *)-2 against the ABI's
   * (void *)2) and the wrapper's own emulation where it does not; the detach
   * must answer MPI_BUFFER_AUTOMATIC either way, which is what MPI-5.0 3.6
   * requires.
   */
  ierror = MPI_Buffer_attach(MPI_BUFFER_AUTOMATIC, 0);
  if (!unsupported(ierror, "MPI_Buffer_attach(MPI_BUFFER_AUTOMATIC)")) {
    CHECK_MPI(ierror);
    bounce_a_bsend("an automatic buffer");

    back     = NULL;
    backsize = 0;
    CHECK_MPI(MPI_Buffer_detach(&back, &backsize));
    CHECK(back == MPI_BUFFER_AUTOMATIC,
          "detaching an automatic buffer answered %p, not "
          "MPI_BUFFER_AUTOMATIC (%p)",
          back, MPI_BUFFER_AUTOMATIC);
  }

  /* --- the large-count form, which is the same body against MPI_Count */
  void *const cbuffer = malloc((size_t)bytes);
  CHECK(cbuffer != NULL, "out of memory");
  if (cbuffer) {
    ierror = MPI_Buffer_attach_c(cbuffer, (MPI_Count)bytes);
    if (!unsupported(ierror, "MPI_Buffer_attach_c")) {
      CHECK_MPI(ierror);
      bounce_a_bsend("an explicit large-count buffer");
      back = NULL;
      MPI_Count csize = 0;
      CHECK_MPI(MPI_Buffer_detach_c(&back, &csize));
      CHECK(back == cbuffer, "MPI_Buffer_detach_c answered %p, not %p", back,
            cbuffer);
      CHECK(csize == (MPI_Count)bytes,
            "MPI_Buffer_detach_c answered size %lld, not %d", (long long)csize,
            bytes);
    }
    free(cbuffer);
  }

  /* --- the communicator's own buffer (MPI-4.1) */
  MPI_Comm dup = MPI_COMM_NULL;
  CHECK_MPI(MPI_Comm_dup(MPI_COMM_SELF, &dup));
  ierror = MPI_Comm_attach_buffer(dup, MPI_BUFFER_AUTOMATIC, 0);
  if (!unsupported(ierror, "MPI_Comm_attach_buffer")) {
    CHECK_MPI(ierror);
    back     = NULL;
    backsize = 0;
    CHECK_MPI(MPI_Comm_detach_buffer(dup, &back, &backsize));
    CHECK(back == MPI_BUFFER_AUTOMATIC,
          "detaching a communicator's automatic buffer answered %p", back);
  }
  CHECK_MPI(MPI_Comm_free(&dup));
}

/* ---------------------------------------------------- dynamic error codes -- */

static void test_error_codes(void)
{
  if (rank == 0) printf("test_error_codes\n");

  int       errorclass = 0;
  const int ierror     = MPI_Add_error_class(&errorclass);
  if (unsupported(ierror, "MPI_Add_error_class")) return;
  CHECK_MPI(ierror);

  /* The renumbering, in one line: MPI-5.0 9.5 puts a dynamic class above
   * MPI_ERR_LASTCODE, and the ABI's MPI_ERR_LASTCODE is 16383 where MPICH's
   * is 0x3fffffff. A class that was passed through would be neither.
   */
  CHECK(errorclass > MPI_ERR_LASTCODE,
        "a dynamic error class came back as %d, which is not above the ABI's "
        "MPI_ERR_LASTCODE (%d)",
        errorclass, MPI_ERR_LASTCODE);

  int second = 0;
  CHECK_MPI(MPI_Add_error_class(&second));
  CHECK(second != errorclass, "two error classes were given the same value");

  int errorcode = 0;
  CHECK_MPI(MPI_Add_error_code(errorclass, &errorcode));
  CHECK(errorcode > MPI_ERR_LASTCODE,
        "a dynamic error code came back as %d", errorcode);
  CHECK(errorcode != errorclass,
        "the error code and its class were given the same value");

  static const char *const text = "abi_state_test: a code of our own";
  CHECK_MPI(MPI_Add_error_string(errorcode, text));

  /* Both of these are *generated* bodies that never heard of the registry:
   * they convert through the same tables every other body does, so this is
   * the registry's round trip through the ordinary path.
   */
  int back = 0;
  CHECK_MPI(MPI_Error_class(errorcode, &back));
  CHECK(back == errorclass,
        "MPI_Error_class of a dynamic code answered %d, not the class %d it "
        "was added to",
        back, errorclass);

  char string[MPI_MAX_ERROR_STRING];
  int  resultlen = 0;
  memset(string, 0, sizeof string);
  CHECK_MPI(MPI_Error_string(errorcode, string, &resultlen));
  CHECK(strcmp(string, text) == 0,
        "MPI_Error_string of a dynamic code answered \"%s\"", string);

  /* And through a callback, which is the direction an error handler takes:
   * the implementation raises its own code and the trampoline maps it back.
   */
  MPI_Comm       dup = MPI_COMM_NULL;
  MPI_Errhandler eh  = MPI_ERRHANDLER_NULL;
  CHECK_MPI(MPI_Comm_dup(MPI_COMM_SELF, &dup));
  CHECK_MPI(MPI_Comm_create_errhandler(comm_errh, &eh));
  CHECK_MPI(MPI_Comm_set_errhandler(dup, eh));
  errh_calls[0] = 0;
  CHECK_MPI(MPI_Comm_call_errhandler(dup, errorcode));
  CHECK(errh_calls[0] == 1, "the error handler ran %d times", errh_calls[0]);
  CHECK(seen_code[0] == errorcode,
        "an error handler was given %d for the dynamic code %d", seen_code[0],
        errorcode);
  CHECK_MPI(MPI_Errhandler_free(&eh));
  CHECK_MPI(MPI_Comm_free(&dup));

  /* --- and the other half of the same registry: a code the *implementation*
   * invented. MPICH answers essentially every error with an
   * instance-specific code rather than with a class -- 604597509 for this one
   * -- which the ABI cannot represent and which was reaching applications as
   * MPI_ERR_OTHER before S4b interned it. What proves the interning is that
   * the class comes back through the round trip, since MPI_Error_class has to
   * reach the implementation's own code to answer.
   */
  int       ignored = 0;
  const int raised  = MPI_Comm_rank(MPI_COMM_NULL, &ignored);
  CHECK(raised != MPI_SUCCESS, "MPI_Comm_rank(MPI_COMM_NULL) succeeded");
  if (raised != MPI_SUCCESS) {
    int cls = MPI_SUCCESS;
    CHECK_MPI(MPI_Error_class(raised, &cls));
    CHECK(cls == MPI_ERR_COMM,
          "the class of the implementation's own error code %d is %d, not "
          "MPI_ERR_COMM (%d)",
          raised, cls, MPI_ERR_COMM);

    resultlen = 0;
    memset(string, 0, sizeof string);
    CHECK_MPI(MPI_Error_string(raised, string, &resultlen));
    CHECK(resultlen > 0 && string[0] != '\0',
          "MPI_Error_string of the implementation's own error code is empty");
  }

  /* The MPI-5.0 removal forms. Newer than either implementation is obliged
   * to be, so a skip here is a fact about the implementation.
   */
  const int removed = MPI_Remove_error_string(errorcode);
  if (!unsupported(removed, "MPI_Remove_error_class/_code/_string")) {
    CHECK_MPI(removed);
    CHECK_MPI(MPI_Remove_error_code(errorcode));
    CHECK_MPI(MPI_Remove_error_class(errorclass));
  }
}

/* ---------------------------------------------------------------- datarep -- */

static int  datarep_extent_calls, datarep_read_calls, datarep_write_calls;
static int  datarep_saw_wrong_type;
static void *datarep_extra_seen;

static int datarep_extent(MPI_Datatype datatype, MPI_Aint *extent,
                          void *extra_state)
{
  ++datarep_extent_calls;
  datarep_extra_seen = extra_state;
  if (datatype != MPI_INT) datarep_saw_wrong_type = 1;
  *extent = (MPI_Aint)sizeof(int);
  return MPI_SUCCESS;
}

/* The conversion is the identity, byte for byte: what is under test is that
 * the callbacks are reached with a converted datatype, not the arithmetic.
 */
static int datarep_read(void *userbuf, MPI_Datatype datatype, int count,
                        void *filebuf, MPI_Offset position, void *extra_state)
{
  (void)position;
  ++datarep_read_calls;
  datarep_extra_seen = extra_state;
  if (datatype != MPI_INT) datarep_saw_wrong_type = 1;
  memcpy(userbuf, filebuf, (size_t)count * sizeof(int));
  return MPI_SUCCESS;
}

static int datarep_write(void *userbuf, MPI_Datatype datatype, int count,
                         void *filebuf, MPI_Offset position, void *extra_state)
{
  (void)position;
  ++datarep_write_calls;
  datarep_extra_seen = extra_state;
  if (datatype != MPI_INT) datarep_saw_wrong_type = 1;
  memcpy(filebuf, userbuf, (size_t)count * sizeof(int));
  return MPI_SUCCESS;
}

static void test_datarep(void)
{
  if (rank == 0) printf("test_datarep\n");

  char name[64];
  snprintf(name, sizeof name, "abi_state_test_rep_%d", rank);

  int ierror = MPI_Register_datarep(name, datarep_read, datarep_write,
                                    datarep_extent, &attr_state);
  if (unsupported(ierror, "MPI_Register_datarep")) return;
  if (ierror != MPI_SUCCESS) {
    if (rank == 0)
      printf("  skipping the datarep: MPI_Register_datarep returned %d\n",
             ierror);
    return;
  }

  MPI_File fh = MPI_FILE_NULL;
  ierror      = open_scratch_file(&fh);
  if (ierror != MPI_SUCCESS) {
    if (rank == 0)
      printf("  skipping the datarep: MPI_File_open returned %d\n", ierror);
    return;
  }

  const int out[4] = {10, 20, 30, 40};
  int       in[4]  = {0};

  ierror = MPI_File_set_view(fh, 0, MPI_INT, MPI_INT, name, MPI_INFO_NULL);
  if (ierror != MPI_SUCCESS) {
    if (rank == 0)
      printf("  skipping the datarep: MPI_File_set_view returned %d\n", ierror);
    CHECK_MPI(MPI_File_close(&fh));
    return;
  }

  CHECK_MPI(MPI_File_write_at(fh, 0, out, 4, MPI_INT, MPI_STATUS_IGNORE));
  CHECK_MPI(MPI_File_read_at(fh, 0, in, 4, MPI_INT, MPI_STATUS_IGNORE));
  CHECK_MPI(MPI_File_close(&fh));

  for (int i = 0; i < 4; ++i)
    CHECK(in[i] == out[i], "the datarep round trip gave in[%d] = %d, not %d", i,
          in[i], out[i]);

  CHECK(datarep_write_calls > 0 && datarep_read_calls > 0,
        "the datarep's conversion functions ran %d times writing and %d "
        "reading",
        datarep_write_calls, datarep_read_calls);
  CHECK(datarep_extent_calls > 0, "the datarep's extent function never ran");
  CHECK(!datarep_saw_wrong_type,
        "a datarep callback was given a datatype that is not MPI_INT");
  CHECK(datarep_extra_seen == &attr_state,
        "a datarep callback was given the wrong extra_state");
}

/* ------------------------------------------------------------ MPI_Pcontrol */

static void test_pcontrol(void)
{
  if (rank == 0) printf("test_pcontrol\n");

  /* The trailing arguments are dropped rather than forwarded, which MPI-5.0
   * 14.2.2 allows because they are a profiling library's business and this
   * library is below one. What must hold is that the call reaches the
   * implementation and reports.
   */
  CHECK_MPI(MPI_Pcontrol(0));
  CHECK_MPI(MPI_Pcontrol(1));
  CHECK_MPI(MPI_Pcontrol(1, 2, 3));
}

/* ---------------------------------------------------------------- spawn --- */

static const char *self_path;

static void test_spawn(void)
{
  if (rank == 0) printf("test_spawn\n");

  /* Off unless MPI_ABI_TEST_SPAWN says otherwise, and that is a measurement
   * rather than caution: MPI_Comm_spawn *hangs* under MPICH 4.3.1's hydra on
   * macOS 26 with no wrapper involved at all -- a 15-line C program does the
   * same -- so a test that called it would hang ctest rather than fail it.
   * The same program under MPICH in a Linux container returns a clean error
   * instead, which the skip below handles. Where spawn works, -D
   * MPI_ABI_TEST_SPAWN=ON turns this on.
   */
  const char *const enabled = getenv("MPI_ABI_TEST_SPAWN");
  if (!enabled || strcmp(enabled, "1") != 0) {
    if (rank == 0)
      printf("  skipping spawn: MPI_ABI_TEST_SPAWN is not set (see "
             "test/README.md)\n");
    return;
  }

  char *argv_[] = {(char *)"--child", NULL};
  int   errcodes[1];
  errcodes[0] = -1;

  MPI_Comm  intercomm = MPI_COMM_NULL;
  const int ierror    = MPI_Comm_spawn(self_path, argv_, 1, MPI_INFO_NULL, 0,
                                       MPI_COMM_SELF, &intercomm, errcodes);
  if (unsupported(ierror, "MPI_Comm_spawn")) return;
  if (ierror != MPI_SUCCESS) {
    /* No launcher, or one that cannot spawn: a documented gap rather than a
     * failure (STAGES.md S4b).
     */
    if (rank == 0)
      printf("  skipping spawn: MPI_Comm_spawn returned %d\n", ierror);
    return;
  }

  CHECK(errcodes[0] == MPI_SUCCESS,
        "the spawned process's error code is %d, not MPI_SUCCESS",
        errcodes[0]);
  CHECK(intercomm != MPI_COMM_NULL, "MPI_Comm_spawn gave no intercommunicator");

  int remote = 0;
  CHECK_MPI(MPI_Comm_remote_size(intercomm, &remote));
  CHECK(remote == 1, "the spawn intercommunicator has %d remote ranks",
        remote);

  CHECK_MPI(MPI_Barrier(intercomm));
  CHECK_MPI(MPI_Comm_disconnect(&intercomm));
}

/* The other half of the same test, in the spawned process. */
static int run_as_child(void)
{
  MPI_Comm parent = MPI_COMM_NULL;
  if (MPI_Comm_get_parent(&parent) != MPI_SUCCESS
      || parent == MPI_COMM_NULL) {
    printf("abi_state_test child: no parent communicator\n");
    MPI_Finalize();
    return 1;
  }
  const int ok = MPI_Barrier(parent) == MPI_SUCCESS
                 && MPI_Comm_disconnect(&parent) == MPI_SUCCESS;
  MPI_Finalize();
  return ok ? 0 : 1;
}

/* ------------------------------------------------------------ MPI_T events */

static void event_cb(MPI_T_event_instance instance,
                     MPI_T_event_registration registration,
                     MPI_T_cb_safety cb_safety, void *user_data)
{
  (void)instance;
  (void)registration;
  (void)cb_safety;
  (void)user_data;
}

static void event_free_cb(MPI_T_event_registration registration,
                          MPI_T_cb_safety cb_safety, void *user_data)
{
  (void)registration;
  (void)cb_safety;
  (void)user_data;
}

static void test_tool_events(void)
{
  if (rank == 0) printf("test_tool_events\n");

  int provided = 0;
  if (MPI_T_init_thread(MPI_THREAD_SINGLE, &provided) != MPI_SUCCESS) {
    if (rank == 0) printf("  skipping MPI_T: MPI_T_init_thread failed\n");
    return;
  }

  int       num    = 0;
  const int ierror = MPI_T_event_get_num(&num);
  if (unsupported(ierror, "the MPI_T event interface")) {
    MPI_T_finalize();
    return;
  }
  if (ierror != MPI_SUCCESS || num == 0) {
    /* MPICH 4.3.1 declares every event entry point and reports zero event
     * types, so there is no registration handle to key the map with. Named
     * rather than passed over: the callbacks in src/mpiwrapper/toolevents.c
     * have no oracle on any implementation available here.
     */
    if (rank == 0)
      printf("  skipping the event callbacks: this implementation reports %d "
             "event types (MPI_T_event_get_num returned %d)\n",
             num, ierror);
    MPI_T_finalize();
    return;
  }

  MPI_T_event_registration registration;
  int                      bind = 0, nelem = 0, verbosity = 0, namelen = 0;
  char                     name[128];
  namelen = (int)sizeof name;
  if (MPI_T_event_get_info(0, name, &namelen, &verbosity, NULL, NULL, &nelem,
                           NULL, NULL, NULL, NULL, &bind)
      != MPI_SUCCESS) {
    MPI_T_finalize();
    return;
  }
  if (MPI_T_event_handle_alloc(0, NULL, MPI_INFO_NULL, &registration)
      != MPI_SUCCESS) {
    if (rank == 0)
      printf("  skipping the event callbacks: no handle could be allocated\n");
    MPI_T_finalize();
    return;
  }

  CHECK_MPI(MPI_T_event_register_callback(registration,
                                          MPI_T_CB_REQUIRE_NONE, MPI_INFO_NULL,
                                          &attr_state, event_cb));
  CHECK_MPI(MPI_T_event_handle_free(registration, &attr_state, event_free_cb));
  MPI_T_finalize();
}

/* -------------------------------------------------------------------- main */

int main(int argc, char **argv)
{
  self_path = argv[0];

  if (argc > 1 && strcmp(argv[1], "--child") == 0) {
    int provided = 0;
    if (MPI_Init_thread(&argc, &argv, MPI_THREAD_SINGLE, &provided)
        != MPI_SUCCESS)
      return 1;
    return run_as_child();
  }

  /* Before anything else, and this is the only place it can be asked. */
  test_before_init();

  int provided = 0;
  CHECK_MPI(MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided));
  CHECK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &size));
  CHECK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));

  /* Errors have to come back as return codes for any of the above to be
   * readable; the default is to abort.
   */
  CHECK_MPI(MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN));
  CHECK_MPI(MPI_Comm_set_errhandler(MPI_COMM_SELF, MPI_ERRORS_RETURN));

  if (rank == 0)
    printf("abi_state_test: %d rank%s\n", size, size == 1 ? "" : "s");

  test_after_init(provided);
  test_errhandlers();
  test_user_ops();
  test_keyvals();
  test_grequest();
  test_buffers();
  test_error_codes();
  test_datarep();
  test_pcontrol();
  test_tool_events();
  test_spawn();

  int total = 0;
  CHECK_MPI(MPI_Allreduce(&failures, &total, 1, MPI_INT, MPI_SUM,
                          MPI_COMM_WORLD));
  if (rank == 0)
    printf("abi_state_test: %d failure(s) across %d ranks\n", total, size);

  CHECK_MPI(MPI_Finalize());

  /* The other end of test_before_init, and the other place no other test can
   * reach: after MPI_Finalize, MPI_Finalized must answer true and
   * MPI_Initialized must still answer true (MPI-5.0 11.4.1).
   */
  int flag = 0;
  if (MPI_Finalized(&flag) != MPI_SUCCESS || !flag) {
    printf("[%d] FAIL: MPI_Finalized answered false after MPI_Finalize\n",
           rank);
    ++total;
  }
  flag = 0;
  if (MPI_Initialized(&flag) != MPI_SUCCESS || !flag) {
    printf("[%d] FAIL: MPI_Initialized answered false after MPI_Finalize\n",
           rank);
    ++total;
  }

  return total != 0;
}
