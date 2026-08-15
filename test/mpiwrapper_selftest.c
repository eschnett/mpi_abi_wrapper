/* mpiwrapper_selftest -- in-process, single rank, no launcher needed.
 *
 * The white-box half of S1's testing: it compiles the conversion runtime's own
 * sources into the test binary and calls them directly, so it can walk the maps
 * in both directions rather than inferring them from MPI results. libmpiwrapper
 * itself still exports exactly one symbol; nothing here changes that.
 *
 * What it checks (NOTES.md #10, "Behavioural tests"):
 *
 *  - every predefined handle of every class, ABI -> implementation -> ABI, and
 *    the implementation -> ABI direction separately, since that one goes
 *    through the perfect hash rather than the switch;
 *  - the rank, tag, error-code and mode maps, round trip;
 *  - the status blob, round trip, including that the implementation's private
 *    bytes survive;
 *  - staging, at and above the stack threshold;
 *  - the **dynamic-handle collision probe**: create many objects of each class
 *    and assert that none of them bit-casts into the ABI's predefined range.
 *    That probe is specifically the runtime replacement for the configure-time
 *    test cross-compiling forbids (NOTES.md #5.1).
 */

#include "internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

#define CHECK(cond, ...)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      ++failures;                                                              \
      printf("FAIL %s:%d: ", __FILE__, __LINE__);                              \
      printf(__VA_ARGS__);                                                     \
      printf("\n");                                                            \
    }                                                                          \
  } while (0)

/* ------------------------------------------------------ predefined handles */

static void test_predefined(void)
{
  struct mpiwrapper_predef predef[128];
  uint64_t                 lo = UINT64_MAX, hi = 0;
  size_t                   total = 0, aliased = 0;

  for (const struct mpiwrapper_predef_class *c = mpiwrapper_predef_classes;
       c->name; ++c) {
    const size_t n = c->fill(predef, sizeof predef / sizeof *predef);
    CHECK(n <= sizeof predef / sizeof *predef, "%s: %zu entries overflow the "
          "test's buffer", c->name, n);
    total += n;

    for (size_t i = 0; i < n; ++i) {
      if (predef[i].abi < lo) lo = predef[i].abi;
      if (predef[i].abi > hi) hi = predef[i].abi;

      /* ABI -> implementation is the generated switch. */
      const uint64_t impl = c->fromabi_bits(predef[i].abi);
      CHECK(impl == predef[i].impl,
            "%s: %s converts to 0x%llx, table says 0x%llx", c->name,
            predef[i].name, (unsigned long long)impl,
            (unsigned long long)predef[i].impl);

      /* implementation -> ABI is the perfect hash. An implementation may give
       * two ABI-distinct predefined handles the same value -- an unsupported
       * optional datatype answering MPI_DATATYPE_NULL is the usual way -- and
       * then only the first can come back. That is not a failure; it is
       * counted and reported.
       */
      const uint64_t abi = c->toabi_bits(predef[i].impl);
      if (abi != predef[i].abi) {
        int alias = 0;
        for (size_t j = 0; j < i; ++j)
          if (predef[j].impl == predef[i].impl) alias = 1;
        if (alias) {
          ++aliased;
        } else {
          ++failures;
          printf("FAIL %s: %s (impl 0x%llx) reverses to 0x%llx, not 0x%llx\n",
                 c->name, predef[i].name, (unsigned long long)predef[i].impl,
                 (unsigned long long)abi, (unsigned long long)predef[i].abi);
        }
      }
    }
  }

  /* The compile-time range that both directions rely on has to cover what the
   * generated tables actually contain, or the checks that use it are narrower
   * than they look.
   */
  CHECK(lo >= MPIWRAPPER_PREDEF_FIRST && hi <= MPIWRAPPER_PREDEF_LAST,
        "predefined handles span 0x%llx..0x%llx, outside the assumed "
        "0x%x..0x%x", (unsigned long long)lo, (unsigned long long)hi,
        MPIWRAPPER_PREDEF_FIRST, MPIWRAPPER_PREDEF_LAST);

  printf("  predefined handles: %zu mapped, %zu aliased by this "
         "implementation\n", total, aliased);
}

/* ------------------------------------------------------- integer constants */

static void test_integers(void)
{
  const int ranks[] = {MPIABI_ANY_SOURCE, MPIABI_PROC_NULL, MPIABI_ROOT,
                       MPIABI_UNDEFINED,   0,               1,
                       4095};
  for (size_t i = 0; i < sizeof ranks / sizeof *ranks; ++i)
    CHECK(mpiwrapper_rank_toabi(mpiwrapper_rank_fromabi(ranks[i])) == ranks[i],
          "rank %d does not round trip", ranks[i]);

  const int tags[] = {MPIABI_ANY_TAG, MPIABI_UNDEFINED, 0, 1, 32767};
  for (size_t i = 0; i < sizeof tags / sizeof *tags; ++i)
    CHECK(mpiwrapper_tag_toabi(mpiwrapper_tag_fromabi(tags[i])) == tags[i],
          "tag %d does not round trip", tags[i]);

  /* The two classes really are distinct: MPICH gives MPI_PROC_NULL and
   * MPI_ANY_TAG the same implementation value, so a single mapper could not
   * have produced both of these.
   */
  CHECK(mpiwrapper_rank_fromabi(MPIABI_PROC_NULL) == MPI_PROC_NULL,
        "MPI_PROC_NULL");
  CHECK(mpiwrapper_tag_fromabi(MPIABI_ANY_TAG) == MPI_ANY_TAG, "MPI_ANY_TAG");

  const int codes[] = {MPIABI_SUCCESS,  MPIABI_ERR_BUFFER, MPIABI_ERR_COUNT,
                       MPIABI_ERR_TYPE, MPIABI_ERR_TRUNCATE,
                       MPIABI_ERR_INTERN, MPIABI_ERR_IN_STATUS,
                       MPIABI_ERR_OTHER};
  for (size_t i = 0; i < sizeof codes / sizeof *codes; ++i)
    CHECK(mpiwrapper_errorcode_toabi(mpiwrapper_errorcode_fromabi(codes[i])) ==
              codes[i],
          "error class %d does not round trip", codes[i]);

  /* The bitmasks are decompositions, so the interesting case is a combination
   * rather than any single bit.
   */
  const int amode = MPIABI_MODE_CREATE | MPIABI_MODE_RDWR |
                    MPIABI_MODE_DELETE_ON_CLOSE;
  CHECK(mpiwrapper_filemode_toabi(mpiwrapper_filemode_fromabi(amode)) == amode,
        "file amode 0x%x does not round trip", amode);
  CHECK(mpiwrapper_filemode_fromabi(MPIABI_MODE_RDONLY) == MPI_MODE_RDONLY,
        "MPI_MODE_RDONLY");

  const int assertion = MPIABI_MODE_NOCHECK | MPIABI_MODE_NOSTORE;
  CHECK(mpiwrapper_winassert_toabi(mpiwrapper_winassert_fromabi(assertion)) ==
            assertion,
        "window assert 0x%x does not round trip", assertion);

  /* And the two families are genuinely separate classes, not a stylistic
   * split: on Open MPI they share bit values, so a single mapper would answer
   * MPI_MODE_CREATE for a window assert of MPI_MODE_NOCHECK. Asserting that the
   * bits collide would be asserting a property of Open MPI; what has to hold
   * here is that each family round-trips *through its own mapper* while the
   * wrong mapper does not reproduce it.
   */
  const int nocheck_as_file =
      mpiwrapper_filemode_toabi(mpiwrapper_winassert_fromabi(MPIABI_MODE_NOCHECK));
  CHECK(nocheck_as_file == 0 || nocheck_as_file != MPIABI_MODE_NOCHECK,
        "the file-mode mapper reproduced a window assert, which means the two "
        "families were not separated after all");
}

/* ------------------------------------------------------------------ status */

static void test_status(void)
{
  MPI_Status    st;
  MPIABI_Status abi;

  memset(&st, 0xa5, sizeof st);
  st.MPI_SOURCE = MPI_ANY_SOURCE;
  st.MPI_TAG    = MPI_ANY_TAG;
  st.MPI_ERROR  = MPI_ERR_TRUNCATE;

  mpiwrapper_status_toabi(&st, &abi);
  CHECK(abi.MPI_SOURCE == MPIABI_ANY_SOURCE, "status MPI_SOURCE");
  CHECK(abi.MPI_TAG == MPIABI_ANY_TAG, "status MPI_TAG");
  CHECK(abi.MPI_ERROR == MPIABI_ERR_TRUNCATE, "status MPI_ERROR");

  MPI_Status back;
  mpiwrapper_status_fromabi(&abi, &back);
  CHECK(back.MPI_SOURCE == st.MPI_SOURCE && back.MPI_TAG == st.MPI_TAG &&
            back.MPI_ERROR == st.MPI_ERROR,
        "status named fields do not round trip");
  CHECK(memcmp(&back, &st, sizeof st) == 0,
        "status private bytes do not round trip");

  /* A real status, so the blob carries what the implementation itself put
   * there rather than a pattern we invented, and MPI_Get_count can be asked to
   * read it back out of the ABI status -- which is the whole point of the blob
   * having no validity marker and no synthesis path.
   */
  MPI_Status real;
  memset(&real, 0, sizeof real);
  char inbuf[8];
  CHECK(PMPI_Sendrecv("hello", 5, MPI_CHAR, 0, 7, inbuf, (int)sizeof inbuf,
                      MPI_CHAR, 0, 7, MPI_COMM_SELF, &real) == MPI_SUCCESS,
        "self send/recv failed");

  MPIABI_Status abi_real;
  mpiwrapper_status_toabi(&real, &abi_real);

  MPI_Status restored;
  mpiwrapper_status_fromabi(&abi_real, &restored);

  int count = -1;
  CHECK(PMPI_Get_count(&restored, MPI_CHAR, &count) == MPI_SUCCESS,
        "MPI_Get_count on a status restored from its ABI blob");
  CHECK(count == 5, "restored status reports %d elements, not 5", count);
}

/* ----------------------------------------------------------------- staging */

static void test_staging(void)
{
  int  stack[MPIWRAPPER_STAGE_BYTES / sizeof(int)];
  void *p;

  p = mpiwrapper_stage(stack, sizeof stack, 0, sizeof(int));
  CHECK(p == stack, "a zero-length staging must still return the buffer");

  p = mpiwrapper_stage(stack, sizeof stack, 4, sizeof(int));
  CHECK(p == stack, "a small staging must use the stack buffer");
  mpiwrapper_unstage(p, stack);

  const size_t big = sizeof stack / sizeof(int) + 1;
  p                = mpiwrapper_stage(stack, sizeof stack, big, sizeof(int));
  CHECK(p != NULL && p != stack, "a large staging must go to the heap");
  if (p) {
    memset(p, 0, big * sizeof(int)); /* writable, and the right size */
    mpiwrapper_unstage(p, stack);
  }

  p = mpiwrapper_stage(stack, sizeof stack, SIZE_MAX / 2, sizeof(int) * 4);
  CHECK(p == NULL, "an overflowing staging must fail rather than wrap");
}

/* ------------------------------------------- the dynamic-handle collision probe */

/* Bit-casting a dynamically created implementation handle into an ABI handle is
 * only correct if it cannot land in 0x20..0x2eb. Neither implementation does
 * today, but cross-compiling forbids proving it at configure time, so it is
 * proved here at run time instead -- by making a few hundred of each kind of
 * object and looking at where they land.
 */
static void probe_one(const char *what, uint64_t bits)
{
  CHECK(!mpiwrapper_in_predef_range(bits),
        "%s: a dynamically created handle landed at 0x%llx, inside the ABI's "
        "predefined range -- the bit-cast in the toabi direction is unsound on "
        "this implementation", what, (unsigned long long)bits);
}

#define PROBE_COUNT 256

static void test_dynamic_handles(void)
{
  MPI_Comm     comms[PROBE_COUNT];
  MPI_Datatype types[PROBE_COUNT];
  MPI_Group    groups[PROBE_COUNT];
  MPI_Info     infos[PROBE_COUNT];
  MPI_Request  requests[PROBE_COUNT];

  for (int i = 0; i < PROBE_COUNT; ++i) {
    CHECK(PMPI_Comm_dup(MPI_COMM_SELF, &comms[i]) == MPI_SUCCESS, "Comm_dup");
    probe_one("MPI_Comm", MPIWRAPPER_BITS(comms[i]));

    CHECK(PMPI_Type_contiguous(i + 1, MPI_INT, &types[i]) == MPI_SUCCESS,
          "Type_contiguous");
    probe_one("MPI_Datatype", MPIWRAPPER_BITS(types[i]));

    CHECK(PMPI_Comm_group(MPI_COMM_SELF, &groups[i]) == MPI_SUCCESS,
          "Comm_group");
    probe_one("MPI_Group", MPIWRAPPER_BITS(groups[i]));

    CHECK(PMPI_Info_create(&infos[i]) == MPI_SUCCESS, "Info_create");
    probe_one("MPI_Info", MPIWRAPPER_BITS(infos[i]));

    CHECK(PMPI_Isend(NULL, 0, MPI_INT, MPI_PROC_NULL, 0, MPI_COMM_SELF,
                     &requests[i]) == MPI_SUCCESS,
          "Isend to MPI_PROC_NULL");
    probe_one("MPI_Request", MPIWRAPPER_BITS(requests[i]));
  }

  for (int i = 0; i < PROBE_COUNT; ++i) {
    PMPI_Wait(&requests[i], MPI_STATUS_IGNORE);
    PMPI_Info_free(&infos[i]);
    PMPI_Group_free(&groups[i]);
    PMPI_Type_free(&types[i]);
    PMPI_Comm_free(&comms[i]);
  }
}

/* ------------------------------------------------- staged-request bookkeeping */

static void test_staged_requests(void)
{
  enum { N = 8 };
  MPI_Request requests[N];
  int         buf[N];

  CHECK(!mpiwrapper_staged_any(), "the staged-request table starts empty");

  /* Real point-to-point requests on MPI_COMM_SELF, deliberately: MPICH answers
   * every operation on MPI_PROC_NULL with one shared built-in request handle
   * (0x6c000001 here, for all of them), so a table keyed on the implementation
   * handle would see one key and eight attaches. Refusing the duplicate is the
   * right behaviour and is checked below -- but it is not the case this test is
   * about, and using it here would have tested the refusal by accident.
   */
  for (int i = 0; i < N; ++i) {
    buf[i] = i;
    CHECK(PMPI_Send_init(&buf[i], 1, MPI_INT, 0, i, MPI_COMM_SELF,
                         &requests[i]) == MPI_SUCCESS,
          "Send_init");
    CHECK(mpiwrapper_staged_attach(requests[i], malloc(16)),
          "attaching a block to request %d (0x%llx)", i,
          (unsigned long long)MPIWRAPPER_BITS(requests[i]));
  }
  CHECK(mpiwrapper_staged_any(), "the table reports itself non-empty");

  /* A second block for a request that already has one is refused rather than
   * silently replacing it: replacing would leak the first block, and freeing it
   * would be a use-after-free if the implementation is still reading it.
   */
  void *duplicate = malloc(16);
  CHECK(!mpiwrapper_staged_attach(requests[0], duplicate),
        "a second block for the same request must be refused");
  free(duplicate);

  for (int i = 0; i < N; ++i) {
    mpiwrapper_staged_release(requests[i]);
    /* Releasing twice must be harmless: a completion function cannot always
     * tell whether it is the one that completed the request.
     */
    mpiwrapper_staged_release(requests[i]);
    PMPI_Request_free(&requests[i]);
  }
  CHECK(!mpiwrapper_staged_any(), "the table is empty again");

  /* And the table must survive being cycled far past its capacity, since
   * released entries leave tombstones rather than holes.
   */
  for (int round = 0; round < MPIWRAPPER_STAGED_REQUEST_SLOTS / N + 4;
       ++round) {
    for (int i = 0; i < N; ++i) {
      PMPI_Send_init(&buf[i], 1, MPI_INT, 0, i, MPI_COMM_SELF, &requests[i]);
      CHECK(mpiwrapper_staged_attach(requests[i], malloc(16)),
            "attach in round %d", round);
    }
    for (int i = 0; i < N; ++i) {
      mpiwrapper_staged_release(requests[i]);
      PMPI_Request_free(&requests[i]);
    }
  }
  CHECK(!mpiwrapper_staged_any(), "the table is empty after cycling");
}

/* ------------------------------------------------------ the keyval registry */

/* The dynamic half of the keyval family (NOTES.md #5.6), which no black-box
 * test can reach yet: the only way an application obtains a dynamic keyval is
 * MPI_Comm_create_keyval, and that one is in the ledger for S4. Here the
 * registry is called directly, which is the whole reason this test compiles
 * the runtime in rather than loading it.
 *
 * Three properties, and the third is the one a plausible implementation gets
 * wrong.
 */
static void test_keyvals(void)
{
  int abi_a = 0, abi_b = 0, abi_c = 0;

  /* 1. The predefined half still goes through the generated switch, and the
   * ABI's 501 is not the implementation's MPI_TAG_UB on either implementation
   * tried.
   */
  CHECK(mpiwrapper_keyval_fromabi(MPIABI_TAG_UB) == MPI_TAG_UB,
        "MPI_TAG_UB does not convert");
  CHECK(mpiwrapper_keyval_toabi(MPI_TAG_UB) == MPIABI_TAG_UB,
        "MPI_TAG_UB does not convert back");
  CHECK(mpiwrapper_keyval_fromabi(MPIABI_WIN_SIZE) == MPI_WIN_SIZE,
        "MPI_WIN_SIZE does not convert");
  CHECK(mpiwrapper_keyval_toabi(MPI_KEYVAL_INVALID) == MPIABI_KEYVAL_INVALID,
        "MPI_KEYVAL_INVALID does not convert back");

  /* 2. A registered keyval round-trips, and the ABI-side value it is given
   * lands outside every predefined key -- which is what keeps the two halves
   * of the family from colliding by construction rather than by luck.
   *
   * The value registered is MPI_TAG_UB's *ABI* number, 501, because an
   * implementation that handed that out as a dynamic keyval is exactly the
   * collision #5.6 describes. It only means anything where the two numberings
   * differ, though: an implementation never issues a dynamic keyval equal to
   * one of its own predefined ones, so in the identity configuration -- where
   * the implementation *is* the ABI -- 501 is MPI_TAG_UB on both sides and
   * there is nothing to collide. Asking the map is how that is detected,
   * rather than assuming a number.
   */
  {
    int probe = MPIABI_TAG_UB;

    if (mpiwrapper_keyval_toabi(probe) != MPIABI_KEYVAL_INVALID) {
      printf("note: this implementation numbers its keyvals as the ABI does, "
             "so no dynamic value can collide with a predefined one; the "
             "registry is exercised on a plain value instead\n");
      probe = 0x5eed;
    }

    CHECK(mpiwrapper_keyval_add(probe, &abi_a),
          "the keyval table refused an add");
    CHECK(abi_a > MPIABI_WIN_MODEL,
          "a dynamic keyval was issued as %d, inside the predefined range",
          abi_a);
    CHECK(mpiwrapper_keyval_fromabi(abi_a) == probe,
          "a registered keyval does not convert back to the implementation's");
    CHECK(mpiwrapper_keyval_toabi(probe) == abi_a,
          "a registered keyval does not convert to the ABI value issued for it");
    /* And the predefined side is unmoved by it: the ABI's 501 is still
     * MPI_TAG_UB, not the dynamic keyval that may have that value on the
     * implementation's side.
     */
    CHECK(mpiwrapper_keyval_fromabi(MPIABI_TAG_UB) == MPI_TAG_UB,
          "registering a dynamic keyval disturbed a predefined one");
  }

  CHECK(mpiwrapper_keyval_add(7001, &abi_b), "the keyval table refused an add");
  CHECK(abi_b != abi_a, "two keyvals were issued the same ABI value");
  CHECK(mpiwrapper_keyval_fromabi(abi_b) == 7001, "the second keyval");

  /* 3. Recycling. An implementation is free to hand out the number of a
   * keyval that has been freed, and when it does, the entry that matters is
   * the most recent registration -- so the reverse direction has to answer
   * with the new ABI value, not the stale one. A registry that searched
   * forwards would return abi_b here and resolve a live key to one the
   * application no longer holds.
   */
  CHECK(mpiwrapper_keyval_add(7001, &abi_c), "the keyval table refused an add");
  CHECK(abi_c != abi_b, "a re-registered keyval reused its ABI value");
  CHECK(mpiwrapper_keyval_toabi(7001) == abi_c,
        "a recycled implementation keyval resolves to the stale registration");
  CHECK(mpiwrapper_keyval_fromabi(abi_b) == 7001,
        "the stale ABI keyval stopped converting");

  /* Anything this library never issued is not translatable, in either
   * direction, and says so rather than passing a number through.
   */
  CHECK(mpiwrapper_keyval_fromabi(abi_c + 1000) == MPI_KEYVAL_INVALID,
        "an ABI keyval we never issued was translated anyway");
  CHECK(mpiwrapper_keyval_toabi(123456) == MPIABI_KEYVAL_INVALID,
        "an implementation keyval we never registered was translated anyway");
}

/* -------------------------------------------------- the error-code registry */

/* errorcodes.c, the other half of #5.6. abi_state_test exercises it from
 * outside -- a class added, seen by MPI_Error_class and MPI_Error_string, and
 * removed -- but two of its properties are only reachable from in here: what a
 * full table does, which has no error channel of its own, and the recycling
 * rule, which needs an implementation that reuses a number and neither of ours
 * obliges on demand.
 */
static void test_error_codes(void)
{
  int abi_a = 0, abi_b = 0, abi_c = 0;

  /* 1. The predefined half still goes through the generated switch, and the
   * two sides' numbering really does differ (MPI_ERR_TRUNCATE is 15 in the
   * ABI, and MPICH and Open MPI each have their own).
   */
  CHECK(mpiwrapper_errorcode_toabi(MPI_SUCCESS) == MPIABI_SUCCESS,
        "MPI_SUCCESS does not convert");
  CHECK(mpiwrapper_errorcode_toabi(MPI_ERR_TRUNCATE) == MPIABI_ERR_TRUNCATE,
        "MPI_ERR_TRUNCATE does not convert");
  CHECK(mpiwrapper_errorcode_fromabi(MPIABI_ERR_TRUNCATE) == MPI_ERR_TRUNCATE,
        "MPI_ERR_TRUNCATE does not convert back");

  /* 2. A registered code is renumbered *above* the ABI's MPI_ERR_LASTCODE,
   * which is where MPI-5.0 9.5 puts a dynamic class and where an application
   * comparing against MPI_ERR_LASTCODE expects to find one. Passing the
   * implementation's own number through would put it five orders of magnitude
   * higher on MPICH (0x3fffffff) and inside the predefined range on an
   * implementation that numbered its classes densely.
   */
  CHECK(mpiwrapper_errorcode_add(0x5eed, &abi_a),
        "the error-code table refused an add");
  CHECK(abi_a > MPIABI_ERR_LASTCODE,
        "a dynamic error code was issued as %d, not above MPI_ERR_LASTCODE",
        abi_a);
  CHECK(mpiwrapper_errorcode_fromabi(abi_a) == 0x5eed,
        "a registered error code does not convert back");
  CHECK(mpiwrapper_errorcode_toabi(0x5eed) == abi_a,
        "a registered error code does not convert to the value issued for it");
  CHECK(mpiwrapper_errorcode_toabi(MPI_ERR_TRUNCATE) == MPIABI_ERR_TRUNCATE,
        "registering a dynamic error code disturbed a predefined one");

  /* 3. A code the implementation invented is interned on sight, which is what
   * keeps MPICH's instance-specific codes -- it answers essentially every
   * error with one -- from all arriving as MPI_ERR_OTHER.
   */
  const int interned = mpiwrapper_errorcode_toabi(0x600d);
  CHECK(interned > MPIABI_ERR_LASTCODE,
        "an implementation error code came back as %d rather than interned",
        interned);
  CHECK(mpiwrapper_errorcode_fromabi(interned) == 0x600d,
        "an interned error code does not convert back to the implementation's");
  CHECK(mpiwrapper_errorcode_toabi(0x600d) == interned,
        "the same implementation error code interned twice");

  /* 4. Recycling, the same rule as the keyval registry above and for the same
   * reason: the newest registration of a number is the live one.
   */
  CHECK(mpiwrapper_errorcode_add(0x5eed, &abi_b),
        "the error-code table refused an add");
  CHECK(abi_b != abi_a, "a re-registered error code reused its ABI value");
  CHECK(mpiwrapper_errorcode_toabi(0x5eed) == abi_b,
        "a recycled implementation error code resolves to the stale entry");
  CHECK(mpiwrapper_errorcode_fromabi(abi_a) == 0x5eed,
        "the stale ABI error code stopped converting");

  /* 5. An ABI code this library never issued is not translatable, and says so
   * with MPI_ERR_OTHER rather than passing a number down that the
   * implementation would reject in some other way.
   */
  CHECK(mpiwrapper_errorcode_fromabi(MPIABI_ERR_LASTCODE - 1) == MPI_ERR_OTHER,
        "an ABI error code we never issued was translated anyway");

  /* 6. Overflow: the table degrades to the answer it gave before it existed,
   * and what was already in it still converts. `add` reports, which is how
   * MPI_Add_error_class turns a full table into MPIABI_ERR_INTERN; the toabi
   * direction has nowhere to report and so falls back to MPI_ERR_OTHER.
   */
  for (int i = 0; i < MPIWRAPPER_ERRORCODE_SLOTS + 16; ++i)
    (void)mpiwrapper_errorcode_add(0x100000 + i, &abi_c);

  int overflow = 0;
  CHECK(!mpiwrapper_errorcode_add(0x7fff0001, &overflow),
        "a full error-code table issued a value anyway");
  CHECK(mpiwrapper_errorcode_toabi(0x7fff0002) == MPIABI_ERR_OTHER,
        "a full error-code table did not fall back to MPI_ERR_OTHER");
  CHECK(mpiwrapper_errorcode_fromabi(abi_b) == 0x5eed,
        "filling the table disturbed a code issued earlier");
  CHECK(mpiwrapper_errorcode_toabi(MPI_ERR_TRUNCATE) == MPIABI_ERR_TRUNCATE,
        "a full table stopped answering for predefined codes");
}

/* ------------------------------------------------- handle serialization */

/* serialize.c, which abi_converters_test exercises from outside but cannot
 * push to its limits: the table is 4096 entries and the only way to fill it is
 * from in here.
 *
 * What the black-box test cannot see at all is the *capacity* behaviour. A
 * full table has nowhere to report a failure -- MPI_Comm_toint returns an int,
 * not an error code -- so the contract is that it answers 0, which no
 * _fromint accepts, and the caller finds the handle invalid rather than being
 * handed someone else's.
 */
static void test_serialization(void)
{
  /* 1. A predefined ABI handle serializes to its own value and is not
   * interned. MPI-5.0 20.4.5 requires the first half; the second is what
   * makes the requirement survive a table that has run out of room.
   */
  CHECK(mpiwrapper_handle_toint(MPIWRAPPER_BITS(MPIABI_COMM_WORLD))
            == (int)(uintptr_t)MPIABI_COMM_WORLD,
        "a predefined handle does not serialize to its ABI value");
  CHECK(mpiwrapper_handle_toint(MPIWRAPPER_BITS(MPIABI_DATATYPE_NULL))
            == (int)(uintptr_t)MPIABI_DATATYPE_NULL,
        "a predefined null handle does not serialize to its ABI value");

  uint64_t bits = 0;
  CHECK(mpiwrapper_handle_fromint((int)(uintptr_t)MPIABI_COMM_WORLD, &bits)
            && bits == MPIWRAPPER_BITS(MPIABI_COMM_WORLD),
        "a predefined handle does not deserialize to itself");

  /* 2. A dynamic handle is interned, stably, outside the predefined range,
   * and a repeat of the same bits reuses its entry rather than a second slot.
   */
  const uint64_t a = UINT64_C(0x00007fff12345678); /* an Open MPI-shaped one */
  const uint64_t b = UINT64_C(0x000000004c000123); /* an MPICH-shaped one */

  const int ia = mpiwrapper_handle_toint(a);
  const int ib = mpiwrapper_handle_toint(b);
  CHECK(ia != 0 && ib != 0, "the serialization table refused two handles");
  CHECK(ia != ib, "two handles serialized to one integer");
  CHECK(mpiwrapper_handle_toint(a) == ia,
        "serializing the same handle twice gave two integers");
  CHECK(!mpiwrapper_in_predef_range((uint64_t)(unsigned)ia),
        "a dynamic handle serialized into the predefined range");

  CHECK(mpiwrapper_handle_fromint(ia, &bits) && bits == a,
        "a 64-bit handle did not survive the round trip");
  CHECK(mpiwrapper_handle_fromint(ib, &bits) && bits == b,
        "an int-shaped handle did not survive the round trip");

  /* 3. Nothing this library never issued is accepted. 1 is below the
   * predefined range and 0 is what a full table answers with.
   */
  CHECK(!mpiwrapper_handle_fromint(1, &bits),
        "an integer below the predefined range was accepted");
  CHECK(!mpiwrapper_handle_fromint(0, &bits),
        "zero was accepted as a serialized handle");
  CHECK(!mpiwrapper_handle_fromint(-1, &bits),
        "a negative integer was accepted as a serialized handle");

  /* 4. Overflow. Fill the table and check that it degrades the documented way
   * rather than wrapping into a valid index -- and that what was already in it
   * still converts, since an application holding an integer from before the
   * overflow must not have it silently repointed.
   */
  for (int i = 0; i < MPIWRAPPER_SERIAL_SLOTS + 16; ++i)
    (void)mpiwrapper_handle_toint(UINT64_C(0x0000700000000000) + (uint64_t)i);

  CHECK(mpiwrapper_handle_toint(UINT64_C(0x0000600000000001)) == 0,
        "a full serialization table issued an integer anyway");
  CHECK(mpiwrapper_handle_fromint(ia, &bits) && bits == a,
        "filling the table disturbed an integer issued earlier");
  CHECK(mpiwrapper_handle_toint(MPIWRAPPER_BITS(MPIABI_COMM_WORLD))
            == (int)(uintptr_t)MPIABI_COMM_WORLD,
        "a full table stopped answering for predefined handles");
}

/* ------------------------------------------------- MPI_T's handle classes */

/* Six classes with at most two predefined values each, so the sentinel shape
 * of #5.3 rather than the perfect-hash shape of #5.1. What is under test is
 * that the sentinels are *translated* and not bit-cast, which is where the ABI
 * and both implementations genuinely disagree: MPI_T_PVAR_ALL_HANDLES is 1 in
 * the ABI, -1 in Open MPI, and an extern object in MPICH.
 */
static void test_tool_handles(void)
{
#ifdef MPIWRAPPER_HAVE_MPI_T_enum
  CHECK(mpiwrapper_t_enum_fromabi(MPIABI_T_ENUM_NULL) == MPI_T_ENUM_NULL,
        "MPI_T_ENUM_NULL does not convert");
  CHECK(mpiwrapper_t_enum_toabi(MPI_T_ENUM_NULL) == MPIABI_T_ENUM_NULL,
        "MPI_T_ENUM_NULL does not convert back");
#endif
#ifdef MPIWRAPPER_HAVE_MPI_T_pvar_handle
  CHECK(mpiwrapper_t_pvar_handle_fromabi(MPIABI_T_PVAR_HANDLE_NULL) ==
            MPI_T_PVAR_HANDLE_NULL,
        "MPI_T_PVAR_HANDLE_NULL does not convert");
  CHECK(mpiwrapper_t_pvar_handle_fromabi(MPIABI_T_PVAR_ALL_HANDLES) ==
            MPI_T_PVAR_ALL_HANDLES,
        "MPI_T_PVAR_ALL_HANDLES does not convert");
  CHECK(mpiwrapper_t_pvar_handle_toabi(MPI_T_PVAR_ALL_HANDLES) ==
            MPIABI_T_PVAR_ALL_HANDLES,
        "MPI_T_PVAR_ALL_HANDLES does not convert back");
  /* The bits are what makes this worth asserting rather than assuming: if the
   * two spellings had the same value the round trip above would pass with no
   * conversion at all, and the test would be vacuous. It is not vacuous on
   * either implementation tried, and this records which.
   */
  if (MPIWRAPPER_BITS(MPI_T_PVAR_ALL_HANDLES) ==
      MPIWRAPPER_BITS(MPIABI_T_PVAR_ALL_HANDLES))
    printf("note: this implementation spells MPI_T_PVAR_ALL_HANDLES as the "
           "ABI does, so its translation is untested here\n");

  /* An ordinary handle keeps its bits, and one that would come back out as a
   * sentinel is refused rather than silently becoming MPI_T_PVAR_ALL_HANDLES.
   */
  {
    const MPI_T_pvar_handle fake = MPIWRAPPER_HANDLE(MPI_T_pvar_handle,
                                                     0x40001000u);
    CHECK(MPIWRAPPER_BITS(mpiwrapper_t_pvar_handle_toabi(fake)) == 0x40001000u,
          "a dynamic MPI_T handle did not keep its bits");
    CHECK(!mpiwrapper_take_handle_error(),
          "a dynamic MPI_T handle outside the sentinel range set the flag");
  }
#endif
}

int main(int argc, char **argv)
{
  const char *diagnostic = "none";

  PMPI_Init(&argc, &argv);

  if (!mpiwrapper_init_reverse_maps(&diagnostic)) {
    printf("FAIL could not build the reverse handle maps: %s\n", diagnostic);
    ++failures;
  } else {
    test_predefined();
    test_integers();
    test_status();
    test_staging();
    test_dynamic_handles();
    test_staged_requests();
    test_keyvals();
    test_error_codes();
    test_serialization();
    test_tool_handles();
  }

  PMPI_Finalize();

  printf("mpiwrapper_selftest: %d failure(s)\n", failures);
  return failures != 0;
}
