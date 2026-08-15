/* abi_tools_test -- the black-box half of S3's second part.
 *
 * The same shape as abi_arrays_test: an ordinary MPI application over the ABI
 * header, linking libmpi_abi and nothing else. What it adds is the classes
 * S3b taught the generator -- keyvals, output-string buffers with an explicit
 * length, MPI_T's six handle classes and six enumerated families, and the
 * obj_handle whose class is not in its own argument list.
 *
 * Several of the tests below are written against the shape a
 * plausible-but-wrong body gets wrong rather than against the happy path:
 *
 *  - **a null OUT pointer.** MPI-5.0 15.3.6-15.3.9 let a caller pass NULL for
 *    any OUT parameter of the MPI_T query functions to say it does not want
 *    that answer. A generated body converts through a local and copies back,
 *    and a body that copied back unconditionally would write through the null
 *    the caller passed. Every MPI_T query here is called twice, once asking
 *    for everything and once for one field, and the second call is the test.
 *  - **a predefined keyval read back.** MPI_TAG_UB is 501 in the ABI and
 *    0x64000001 in MPICH, so a body that passed the int through unconverted
 *    would ask the implementation for attribute 501 -- which is not an error,
 *    merely a different question, and answers flag=0 instead of failing.
 *  - **MPI_T_PVAR_ALL_HANDLES.** It is 1 in the ABI, -1 in Open MPI and an
 *    `extern ... * const` object in MPICH, so a wrapper that bit-cast MPI_T
 *    handles instead of translating their sentinels stops one rank short of a
 *    wrong answer: MPI_T_pvar_reset(session, ALL_HANDLES) silently resets
 *    nothing, or the implementation rejects a handle of 1.
 *  - **a truncating string buffer.** MPI_T's 15.3.3 convention makes the
 *    length INOUT: in it is the buffer size, out it is the string's length
 *    plus one, and both directions have to survive the wrapper. A one-byte
 *    buffer is what catches a body that forwarded the caller's length but
 *    reported the implementation's.
 *
 * The MPI_T interface is optional in full and unevenly implemented -- Open MPI
 * 5.0.6 has cvars, pvars and enums but no events at all -- so every test here
 * skips rather than fails when the entry point reports
 * MPI_ERR_UNSUPPORTED_OPERATION, which is decision 6's promise that the slot
 * exists either way.
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

/* ------------------------------------------------------ predefined keyvals */

/* The keyval family's predefined half, which is the half testable without
 * MPI_Comm_create_keyval -- that one is in the ledger and S4 writes it, and
 * the dynamic half of the registry is exercised by mpiwrapper_selftest, which
 * can call it directly.
 *
 * MPI_TAG_UB is the sharp one. The standard requires it to be set on
 * MPI_COMM_WORLD, so flag must come back true; a wrapper that forwarded the
 * ABI's 501 unconverted asks MPICH about a keyval it never issued and gets
 * flag=0 and MPI_SUCCESS -- a wrong answer that looks like a legitimate
 * "attribute not set".
 */
static void test_predefined_keyvals(void)
{
  void *value = NULL;
  int   flag  = 0;

  if (rank == 0) printf("test_predefined_keyvals\n");

  CHECK_MPI(MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_TAG_UB, &value, &flag));
  CHECK(flag, "MPI_TAG_UB is required to be set on MPI_COMM_WORLD");
  if (flag) {
    const int tag_ub = *(int *)value;
    CHECK(tag_ub >= 32767, "MPI_TAG_UB is %d, below the required 32767",
          tag_ub);
  }

  /* Not required to be set, so only the call is under test -- but a wrapper
   * that mapped the keyval to something else entirely would be as likely to
   * fail here as to answer flag=0.
   */
  flag = 0;
  CHECK_MPI(MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_WTIME_IS_GLOBAL, &value,
                              &flag));

  /* The window keys live in the ABI's second range (601+) and are set by
   * MPI_Win_create rather than by the implementation's startup, so these are
   * the only ones of the thirteen whose values we know independently.
   *
   * The window is a means, not the subject, so a run that cannot create one
   * skips rather than fails -- and it has to ask rather than abort, hence the
   * errhandler. That is not hypothetical: `ci-scripts/linux-test.sh` sets
   * OMPI_MCA_btl_vader_single_copy_mechanism=none, and Open MPI 4.1.6 then
   * answers MPI_ERR_WIN to MPI_Win_create with no wrapper in sight (measured
   * both ways; see test/README.md's environment quirks).
   */
  {
    MPI_Win        win;
    MPI_Errhandler saved = MPI_ERRHANDLER_NULL;
    int            buffer[4] = {0, 0, 0, 0};
    int            ierror;

    CHECK_MPI(MPI_Comm_get_errhandler(MPI_COMM_WORLD, &saved));
    CHECK_MPI(MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN));
    ierror = MPI_Win_create(buffer, (MPI_Aint)sizeof buffer, (int)sizeof(int),
                            MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    CHECK_MPI(MPI_Comm_set_errhandler(MPI_COMM_WORLD, saved));
    CHECK_MPI(MPI_Errhandler_free(&saved));
    if (ierror != MPI_SUCCESS) {
      if (rank == 0)
        printf("  skipping the window keyvals: MPI_Win_create returned %d in "
               "this environment\n",
               ierror);
      return;
    }
    flag  = 0;
    value = NULL;
    CHECK_MPI(MPI_Win_get_attr(win, MPI_WIN_SIZE, &value, &flag));
    CHECK(flag, "MPI_WIN_SIZE is required to be set on a window");
    if (flag)
      CHECK(*(MPI_Aint *)value == (MPI_Aint)sizeof buffer,
            "MPI_WIN_SIZE is %lld, expected %zu",
            (long long)*(MPI_Aint *)value, sizeof buffer);

    flag  = 0;
    value = NULL;
    CHECK_MPI(MPI_Win_get_attr(win, MPI_WIN_DISP_UNIT, &value, &flag));
    CHECK(flag, "MPI_WIN_DISP_UNIT is required to be set on a window");
    if (flag)
      CHECK(*(int *)value == (int)sizeof(int), "MPI_WIN_DISP_UNIT is %d",
            *(int *)value);

    CHECK_MPI(MPI_Win_free(&win));
  }
}

/* --------------------------------- output string buffers with a length */

/* NOTES.md #5.8's safe half: the caller passes the buffer's size, so nothing
 * is staged and nothing converts. What is under test is that the length
 * survives the wrapper in *both* directions, which a body that dropped the
 * INOUT half would get wrong only when the buffer is too small.
 */
static void test_info_strings(void)
{
  MPI_Info info;
  char     value[64];
  int      buflen, flag;

  if (rank == 0) printf("test_info_strings\n");

  CHECK_MPI(MPI_Info_create(&info));
  CHECK_MPI(MPI_Info_set(info, "shape", "rectangular"));

  /* Big enough: the whole value comes back, and buflen is its length plus the
   * terminator.
   *
   * MPI_Info_get_string is MPI-4.0, so this is one of the places decision 6's
   * promise gets exercised for real: Open MPI 4.1 has no such entry point, its
   * slot reports at run time, and the test has to skip rather than fail. That
   * is the whole Linux row's value -- macOS has only Open MPI 5.0.6 here, which
   * does have it.
   */
  memset(value, 'x', sizeof value);
  buflen = (int)sizeof value;
  flag   = 0;
  {
    const int ierror = MPI_Info_get_string(info, "shape", &buflen, value,
                                           &flag);
    if (unsupported(ierror, "MPI_Info_get_string")) {
      CHECK_MPI(MPI_Info_free(&info));
      return;
    }
    CHECK(ierror == MPI_SUCCESS, "MPI_Info_get_string returned %d", ierror);
  }
  CHECK(flag, "the key we just set is not there");
  if (flag) {
    CHECK(strcmp(value, "rectangular") == 0, "value is %s", value);
    CHECK(buflen == (int)strlen("rectangular") + 1, "buflen is %d, expected %zu",
          buflen, strlen("rectangular") + 1);
  }

  /* Too small: truncated to buflen-1 characters plus a terminator, and buflen
   * still reports what the whole string would need. A wrapper that reported
   * the buffer size back instead would pass the first case and fail here.
   */
  memset(value, 'x', sizeof value);
  buflen = 4;
  flag   = 0;
  CHECK_MPI(MPI_Info_get_string(info, "shape", &buflen, value, &flag));
  if (flag) {
    CHECK(strcmp(value, "rec") == 0, "truncated value is %s, expected \"rec\"",
          value);
    CHECK(buflen == (int)strlen("rectangular") + 1,
          "buflen after truncation is %d, expected %zu", buflen,
          strlen("rectangular") + 1);
  }

  /* Zero length: nothing is written and only the length comes back, which is
   * what makes a null buffer legal here.
   */
  buflen = 0;
  flag   = 0;
  CHECK_MPI(MPI_Info_get_string(info, "shape", &buflen, NULL, &flag));
  if (flag)
    CHECK(buflen == (int)strlen("rectangular") + 1,
          "buflen with no buffer is %d, expected %zu", buflen,
          strlen("rectangular") + 1);

  /* The deprecated MPI_Info_get, whose `value` apis.json spells as an out
   * array of STRING rather than as a scalar -- a different path through
   * classify() to the same passthrough.
   */
  {
    int ierror;
    memset(value, 'x', sizeof value);
    flag   = 0;
    ierror = MPI_Info_get(info, "shape", (int)sizeof value - 1, value, &flag);
    if (!unsupported(ierror, "MPI_Info_get")) {
      CHECK(ierror == MPI_SUCCESS, "MPI_Info_get returned %d", ierror);
      CHECK(flag, "MPI_Info_get did not find the key");
      if (flag) CHECK(strcmp(value, "rectangular") == 0, "value is %s", value);
    }
  }

  CHECK_MPI(MPI_Info_free(&info));
}

/* ------------------------------------------------------- MPI_T: the basics */

/* MPI_T has its own initialization, separate from MPI's, and its own error
 * classes. Everything below runs between this and MPI_T_finalize.
 */
static int tools_ready;

static void test_tools_init(void)
{
  int provided = -1;
  int ierror   = MPI_T_init_thread(MPI_THREAD_SINGLE, &provided);

  if (rank == 0) printf("test_tools_init\n");
  if (unsupported(ierror, "MPI_T_init_thread")) return;
  if (ierror != MPI_SUCCESS) {
    if (rank == 0) printf("  skipping MPI_T: init returned %d\n", ierror);
    return;
  }
  /* The thread level comes back through the same mapper MPI_Init_thread's
   * does, so an unmapped value shows up as a level outside the four.
   */
  CHECK(provided == MPI_THREAD_SINGLE || provided == MPI_THREAD_FUNNELED ||
            provided == MPI_THREAD_SERIALIZED ||
            provided == MPI_THREAD_MULTIPLE,
        "MPI_T_init_thread provided %d, which is no thread level", provided);
  tools_ready = 1;
}

/* ------------------------------------------------------ MPI_T control vars */

/* MPI_T_cvar_get_info is the densest of the new bodies: two output-string
 * buffers, three enumerated families, a datatype handle and an MPI_T_enum
 * handle, and every one of them nullable. It is called twice for that reason.
 */
static void test_cvars(void)
{
  int num_cvar = 0;

  if (!tools_ready) return;
  if (rank == 0) printf("test_cvars\n");

  if (unsupported(MPI_T_cvar_get_num(&num_cvar), "MPI_T_cvar_get_num")) return;
  CHECK(num_cvar >= 0, "MPI_T_cvar_get_num returned %d", num_cvar);
  if (num_cvar == 0) {
    if (rank == 0) printf("  no control variables to inspect\n");
    return;
  }

  for (int index = 0; index < num_cvar && index < 8; ++index) {
    char         name[128], desc[256];
    int          name_len = (int)sizeof name, desc_len = (int)sizeof desc;
    int          verbosity = -1, bind = -1, scope = -1;
    MPI_Datatype datatype = MPI_DATATYPE_NULL;
    MPI_T_enum   enumtype = MPI_T_ENUM_NULL;

    const int ierror = MPI_T_cvar_get_info(index, name, &name_len, &verbosity,
                                           &datatype, &enumtype, desc,
                                           &desc_len, &bind, &scope);
    if (unsupported(ierror, "MPI_T_cvar_get_info")) return;
    CHECK(ierror == MPI_SUCCESS, "MPI_T_cvar_get_info(%d) returned %d", index,
          ierror);
    if (ierror != MPI_SUCCESS) continue;

    /* The standard requires a name of at least length one, so name_len is at
     * least two counting the terminator.
     */
    CHECK(name_len >= 2, "cvar %d has name_len %d", index, name_len);
    CHECK(name[0] != '\0', "cvar %d has an empty name", index);

    /* Each of these is a switch family, so a value outside the ABI's own
     * enumerators is either an unmapped implementation value or a case that
     * dropped out of the switch -- the exact failure NOTES.md #3's guard rule
     * exists to prevent.
     */
    CHECK(verbosity >= MPI_T_VERBOSITY_USER_BASIC &&
              verbosity <= MPI_T_VERBOSITY_MPIDEV_ALL,
          "cvar %d has verbosity %d, outside the ABI's range", index,
          verbosity);
    CHECK(bind >= MPI_T_BIND_NO_OBJECT && bind <= MPI_T_BIND_MPI_SESSION,
          "cvar %d has bind %d, outside the ABI's range", index, bind);
    CHECK(scope >= MPI_T_SCOPE_CONSTANT && scope <= MPI_T_SCOPE_ALL_EQ,
          "cvar %d has scope %d, outside the ABI's range", index, scope);

    /* The datatype came back through the reverse handle map, so it is an ABI
     * handle and comparable against the ABI's own predefined ones.
     */
    CHECK(datatype != MPI_DATATYPE_NULL, "cvar %d has no datatype", index);

    /* And now the same call asking for one field. A body that copied back
     * unconditionally writes through nine null pointers here.
     */
    {
      int only_bind = -1;
      CHECK_MPI(MPI_T_cvar_get_info(index, NULL, NULL, NULL, NULL, NULL, NULL,
                                    NULL, &only_bind, NULL));
      CHECK(only_bind == bind,
            "cvar %d reports bind %d when asked alone and %d when asked with "
            "everything else",
            index, only_bind, bind);
    }

    /* An enumeration, where the variable has one: MPI_T_enum is one of the six
     * handle classes S3b added, and its only predefined value is the null
     * handle -- so a body that failed to translate it hands the
     * implementation an ABI pointer.
     */
    if (enumtype != MPI_T_ENUM_NULL) {
      char ename[128];
      int  ename_len = (int)sizeof ename, num = -1;

      CHECK_MPI(MPI_T_enum_get_info(enumtype, &num, ename, &ename_len));
      CHECK(num >= 0, "enum of cvar %d has %d items", index, num);
      if (num > 0) {
        char iname[128];
        int  iname_len = (int)sizeof iname, value = 0;
        CHECK_MPI(MPI_T_enum_get_item(enumtype, 0, &value, iname, &iname_len));
        CHECK(iname_len >= 1, "enum item 0 has name_len %d", iname_len);
      }
    }
  }

  /* A handle over the first control variable, which is where obj_handle goes
   * through mpiwrapper_tool_obj_fromabi. Only a variable bound to no object is
   * exercised unconditionally, because a bound one needs an object of exactly
   * the class the implementation names, and that class differs per build.
   */
  for (int index = 0; index < num_cvar && index < 32; ++index) {
    MPI_T_cvar_handle handle = MPI_T_CVAR_HANDLE_NULL;
    int               count = -1, bind = -1, ierror;

    if (MPI_T_cvar_get_info(index, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                            &bind, NULL) != MPI_SUCCESS)
      continue;
    if (bind != MPI_T_BIND_NO_OBJECT) continue;

    ierror = MPI_T_cvar_handle_alloc(index, NULL, &handle, &count);
    if (unsupported(ierror, "MPI_T_cvar_handle_alloc")) return;
    if (ierror != MPI_SUCCESS) continue;
    CHECK(handle != MPI_T_CVAR_HANDLE_NULL,
          "MPI_T_cvar_handle_alloc succeeded and produced a null handle");
    CHECK(count >= 0, "cvar %d handle has count %d", index, count);
    CHECK_MPI(MPI_T_cvar_handle_free(&handle));
    CHECK(handle == MPI_T_CVAR_HANDLE_NULL,
          "MPI_T_cvar_handle_free left the handle non-null");
    break;
  }

  /* The same allocator with a *bound* variable, where obj_handle is really
   * read: a communicator-bound cvar is the common case, and this is the one
   * place the class of an argument comes from a query rather than from the
   * signature.
   */
  for (int index = 0; index < num_cvar && index < 32; ++index) {
    MPI_T_cvar_handle handle = MPI_T_CVAR_HANDLE_NULL;
    MPI_Comm          comm   = MPI_COMM_WORLD;
    int               count = -1, bind = -1;

    if (MPI_T_cvar_get_info(index, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                            &bind, NULL) != MPI_SUCCESS)
      continue;
    if (bind != MPI_T_BIND_MPI_COMM) continue;

    if (MPI_T_cvar_handle_alloc(index, &comm, &handle, &count) == MPI_SUCCESS) {
      CHECK(handle != MPI_T_CVAR_HANDLE_NULL,
            "a comm-bound cvar handle came back null");
      CHECK_MPI(MPI_T_cvar_handle_free(&handle));
    }
    break;
  }
}

/* ------------------------------------------------- MPI_T performance vars */

/* The pvar half adds a session handle and MPI_T_PVAR_ALL_HANDLES, which is the
 * one MPI_T sentinel whose implementation value differs from the ABI's on
 * every implementation tried.
 */
static void test_pvars(void)
{
  MPI_T_pvar_session session = MPI_T_PVAR_SESSION_NULL;
  int                num_pvar = 0, ierror;

  if (!tools_ready) return;
  if (rank == 0) printf("test_pvars\n");

  ierror = MPI_T_pvar_session_create(&session);
  if (unsupported(ierror, "MPI_T_pvar_session_create")) return;
  CHECK(ierror == MPI_SUCCESS, "MPI_T_pvar_session_create returned %d", ierror);
  if (ierror != MPI_SUCCESS) return;
  CHECK(session != MPI_T_PVAR_SESSION_NULL,
        "MPI_T_pvar_session_create produced a null session");

  if (MPI_T_pvar_get_num(&num_pvar) == MPI_SUCCESS && num_pvar > 0) {
    for (int index = 0; index < num_pvar && index < 8; ++index) {
      char name[128], desc[256];
      int  name_len = (int)sizeof name, desc_len = (int)sizeof desc;
      int  verbosity = -1, var_class = -1, bind = -1;
      int  readonly = -1, continuous = -1, atomic = -1;
      MPI_Datatype datatype = MPI_DATATYPE_NULL;
      MPI_T_enum   enumtype = MPI_T_ENUM_NULL;

      if (MPI_T_pvar_get_info(index, name, &name_len, &verbosity, &var_class,
                              &datatype, &enumtype, desc, &desc_len, &bind,
                              &readonly, &continuous, &atomic) != MPI_SUCCESS)
        continue;

      CHECK(var_class >= MPI_T_PVAR_CLASS_STATE &&
                var_class <= MPI_T_PVAR_CLASS_GENERIC,
            "pvar %d has class %d, outside the ABI's range", index, var_class);
      CHECK(bind >= MPI_T_BIND_NO_OBJECT && bind <= MPI_T_BIND_MPI_SESSION,
            "pvar %d has bind %d, outside the ABI's range", index, bind);

      /* The same call for one field only, as for cvars. */
      {
        int only_class = -1;
        CHECK_MPI(MPI_T_pvar_get_info(index, NULL, NULL, NULL, &only_class,
                                      NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                      NULL));
        CHECK(only_class == var_class, "pvar %d reports class %d alone and %d "
                                       "with everything else",
              index, only_class, var_class);
      }

      /* MPI_T_pvar_get_index takes the class in the *in* direction, which is
       * the only place any of MPI_T's enumerated families is converted that
       * way -- every other one is an OUT parameter.
       */
      if (name_len <= (int)sizeof name) {
        int found = -1;
        if (MPI_T_pvar_get_index(name, var_class, &found) == MPI_SUCCESS)
          CHECK(found == index,
                "MPI_T_pvar_get_index(%s, %d) answered %d, expected %d", name,
                var_class, found, index);
      }

      /* A handle, and through it MPI_T_PVAR_ALL_HANDLES: the ABI fixes it at
       * 1, Open MPI spells it -1 and MPICH makes it an extern object, so a
       * body that bit-cast MPI_T handles reaches the implementation with a
       * value that is not its ALL_HANDLES at all.
       */
      if (bind == MPI_T_BIND_NO_OBJECT) {
        MPI_T_pvar_handle handle = MPI_T_PVAR_HANDLE_NULL;
        int               count  = -1;

        if (MPI_T_pvar_handle_alloc(session, index, NULL, &handle, &count) ==
            MPI_SUCCESS) {
          CHECK(handle != MPI_T_PVAR_HANDLE_NULL,
                "MPI_T_pvar_handle_alloc produced a null handle");
          if (!readonly) MPI_T_pvar_reset(session, handle);
          if (continuous == 0) MPI_T_pvar_start(session, handle);

          /* ALL_HANDLES is legal at reset and stop whatever the variable's
           * own attributes are, and it is the value under test rather than
           * the outcome -- an implementation may decline, but it must not see
           * the ABI's 1.
           */
          {
            const int all = MPI_T_pvar_reset(session, MPI_T_PVAR_ALL_HANDLES);
            CHECK(all == MPI_SUCCESS || all == MPI_T_ERR_INVALID_HANDLE ||
                      all == MPI_T_ERR_PVAR_NO_WRITE,
                  "MPI_T_pvar_reset with ALL_HANDLES returned %d, which is "
                  "neither success nor a documented refusal",
                  all);
          }

          if (continuous == 0) MPI_T_pvar_stop(session, handle);
          CHECK_MPI(MPI_T_pvar_handle_free(session, &handle));
          CHECK(handle == MPI_T_PVAR_HANDLE_NULL,
                "MPI_T_pvar_handle_free left the handle non-null");
          break;
        }
      }
    }
  }

  CHECK_MPI(MPI_T_pvar_session_free(&session));
  CHECK(session == MPI_T_PVAR_SESSION_NULL,
        "MPI_T_pvar_session_free left the session non-null");
}

/* ------------------------------------------------------- MPI_T categories */

/* Categories have no converted OUT parameter at all -- every one of them is an
 * int the implementation and the ABI agree about -- so what this covers is the
 * output-string pair, and that a body of eight parameters with two string
 * buffers among them still passes them all in the right order.
 */
static void test_categories(void)
{
  int num_cat = 0;

  if (!tools_ready) return;
  if (rank == 0) printf("test_categories\n");

  if (unsupported(MPI_T_category_get_num(&num_cat), "MPI_T_category_get_num"))
    return;
  if (num_cat <= 0) return;

  for (int index = 0; index < num_cat && index < 4; ++index) {
    char name[128], desc[256];
    int  name_len = (int)sizeof name, desc_len = (int)sizeof desc;
    int  num_cvars = -1, num_pvars = -1, num_categories = -1;

    if (MPI_T_category_get_info(index, name, &name_len, desc, &desc_len,
                                &num_cvars, &num_pvars,
                                &num_categories) != MPI_SUCCESS)
      continue;
    CHECK(name_len >= 2, "category %d has name_len %d", index, name_len);
    CHECK(num_cvars >= 0 && num_pvars >= 0 && num_categories >= 0,
          "category %d reports %d cvars, %d pvars, %d categories", index,
          num_cvars, num_pvars, num_categories);

    /* The nullable path again, this time where nothing converts: the null has
     * to reach the implementation rather than be intercepted.
     */
    {
      int only_cvars = -1;
      CHECK_MPI(MPI_T_category_get_info(index, NULL, NULL, NULL, NULL,
                                        &only_cvars, NULL, NULL));
      CHECK(only_cvars == num_cvars,
            "category %d reports %d cvars alone and %d with everything else",
            index, only_cvars, num_cvars);
    }
  }
}

/* ----------------------------------------------------------- MPI_T events */

/* Optional even by MPI_T's standards: Open MPI 5.0.6 declares neither
 * MPI_T_event_registration nor MPI_T_event_instance, which is what the
 * generated bodies' type guards are for. Where events do exist, the two
 * handle classes and the callback-safety family are what this covers.
 */
static void test_events(void)
{
  int num_events = 0, ierror;

  if (!tools_ready) return;
  if (rank == 0) printf("test_events\n");

  ierror = MPI_T_event_get_num(&num_events);
  if (unsupported(ierror, "MPI_T_event_get_num")) return;
  if (ierror != MPI_SUCCESS || num_events <= 0) {
    if (rank == 0) printf("  no event types to inspect\n");
    return;
  }

  for (int index = 0; index < num_events && index < 4; ++index) {
    char         name[128], desc[256];
    int          name_len = (int)sizeof name, desc_len = (int)sizeof desc;
    int          verbosity = -1, bind = -1, num_elements = 8;
    MPI_Datatype datatypes[8];
    MPI_Aint     displacements[8];
    MPI_T_enum   enumtype = MPI_T_ENUM_NULL;
    MPI_Info     info     = MPI_INFO_NULL;

    for (int i = 0; i < 8; ++i) datatypes[i] = MPI_DATATYPE_NULL;

    ierror = MPI_T_event_get_info(index, name, &name_len, &verbosity,
                                  datatypes, displacements, &num_elements,
                                  &enumtype, &info, desc, &desc_len, &bind);
    if (unsupported(ierror, "MPI_T_event_get_info")) return;
    if (ierror != MPI_SUCCESS) continue;

    CHECK(name_len >= 2, "event %d has name_len %d", index, name_len);
    CHECK(bind >= MPI_T_BIND_NO_OBJECT && bind <= MPI_T_BIND_MPI_SESSION,
          "event %d has bind %d, outside the ABI's range", index, bind);
    /* Only the elements the implementation actually described are converted,
     * and the rest of the array must be untouched -- converting the tail
     * would turn uninitialized handles into ABI ones.
     */
    for (int i = 0; i < num_elements && i < 8; ++i)
      CHECK(datatypes[i] != MPI_DATATYPE_NULL,
            "event %d element %d came back as MPI_DATATYPE_NULL", index, i);
    if (info != MPI_INFO_NULL) CHECK_MPI(MPI_Info_free(&info));

    /* The array is what the null-OUT rule matters most for here: a body that
     * wrote back unconditionally would copy into the null array_of_datatypes
     * this call passes.
     */
    {
      int only_bind = -1;
      CHECK_MPI(MPI_T_event_get_info(index, NULL, NULL, NULL, NULL, NULL, NULL,
                                     NULL, NULL, NULL, NULL, &only_bind));
      CHECK(only_bind == bind,
            "event %d reports bind %d alone and %d with everything else",
            index, only_bind, bind);
    }
  }
}

static void test_tools_finalize(void)
{
  if (!tools_ready) return;
  CHECK_MPI(MPI_T_finalize());
  tools_ready = 0;
}

int main(int argc, char **argv)
{
  CHECK_MPI(MPI_Init(&argc, &argv));
  CHECK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &size));
  CHECK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));

  if (rank == 0)
    printf("abi_tools_test: %d rank%s\n", size, size == 1 ? "" : "s");

  test_predefined_keyvals();
  test_info_strings();

  test_tools_init();
  test_cvars();
  test_pvars();
  test_categories();
  test_events();
  test_tools_finalize();

  int total = 0;
  CHECK_MPI(MPI_Allreduce(&failures, &total, 1, MPI_INT, MPI_SUM,
                          MPI_COMM_WORLD));
  if (rank == 0)
    printf("abi_tools_test: %d failure(s) across %d ranks\n", total, size);

  CHECK_MPI(MPI_Finalize());
  return total != 0;
}
