/* libmpiwrapper -- status conversion.
 *
 * Hand-written and permanent. The ABI status is 32 bytes: three named ints plus
 * 20 bytes of scratch. The implementation's is smaller, and its private part is
 * the complement of the named-field block -- at the *front* in MPICH
 * (count_lo, count_hi_and_cancelled, then the named fields) and at the *back*
 * in Open MPI (named fields, then _cancelled and _ucount).
 *
 * So the private part is a head range and a tail range, both of compile-time
 * constant length, and one of the two is empty in each known implementation:
 * this compiles to a single 8-byte copy for MPICH and a single 12-byte copy for
 * Open MPI.
 *
 * There is deliberately no validity marker and no synthesis fallback
 * (NOTES.md #5.2, decision 2). In the generalized-request flow the blob is
 * always implementation-backed; for a genuinely uninitialized status, garbage
 * out is what the native implementation does for the same user error. MPI
 * already requires statuses to be freely copyable, which forces these bytes to
 * be position-independent and self-contained -- that is what makes the scheme
 * sound in general and not merely for these two implementations.
 */

#include "internal.h"

#include <string.h>

#define IMPL_NAMED_OFF offsetof(MPI_Status, MPI_SOURCE)
#define IMPL_NAMED_LEN (3 * sizeof(int))
#define IMPL_HEAD_LEN  IMPL_NAMED_OFF
#define IMPL_TAIL_OFF  (IMPL_NAMED_OFF + IMPL_NAMED_LEN)
#define IMPL_TAIL_LEN  (sizeof(MPI_Status) - IMPL_TAIL_OFF)
#define ABI_SCRATCH    sizeof(((MPIABI_Status *)0)->MPI_internal)

/* The three named fields must be contiguous and in order. True for both known
 * implementations; one that interleaved private bytes between them would fail
 * the build here rather than being silently mishandled.
 */
_Static_assert(offsetof(MPI_Status, MPI_TAG) == IMPL_NAMED_OFF + sizeof(int),
               "MPI_Status: MPI_TAG does not follow MPI_SOURCE");
_Static_assert(offsetof(MPI_Status, MPI_ERROR) == IMPL_NAMED_OFF + 2 * sizeof(int),
               "MPI_Status: MPI_ERROR does not follow MPI_TAG");

/* This one has no runtime recourse: there would be nowhere to put the overflow,
 * and side storage keyed on a status address is unsound because applications
 * copy statuses freely. Hence a build failure rather than a check
 * (NOTES.md #5.9).
 */
_Static_assert(IMPL_HEAD_LEN + IMPL_TAIL_LEN <= ABI_SCRATCH,
               "MPI_Status: private part exceeds the ABI's 20 scratch bytes");

/* The one shape both directions of the out-conversion share. `with_error` is
 * the whole difference, and it is MPI-5.0 3.2.5: "the error field ... is never
 * modified" except by the multiple-completion calls of 3.7.5, which are
 * exactly the ones taking an array of statuses. A single-status body therefore
 * must not touch the field, and the reason it is a *problem* here rather than
 * free is that the implementation is handed a temporary of ours: whatever it
 * leaves in that temporary's error field -- zero, since the generated bodies
 * clear it -- would otherwise be copied over the caller's value.
 *
 * Found by MPICH's own pt2pt/mprobe and its threaded relatives, which preset
 * the field to MPI_ERR_DIMS and require it back (S7, NOTES.md #3).
 */
static void status_toabi(const MPI_Status *st, MPIABI_Status *abi,
                         int with_error)
{
  const unsigned char *src = (const unsigned char *)st;

  /* Zeroed first, so the bytes we do not use are reproducible and no
   * implementation stack garbage reaches the application. Not needed for
   * correctness; it keeps valgrind and MSan quiet, which matters because this
   * shim will be debugged under both. _ucount is size_t-aligned while
   * MPI_internal is int-aligned, which is why this goes through an aligned
   * local and memcpy rather than a struct assignment.
   */
  unsigned char blob[ABI_SCRATCH];
  memset(blob, 0, sizeof blob);
  memcpy(blob, src, IMPL_HEAD_LEN);
  memcpy(blob + IMPL_HEAD_LEN, src + IMPL_TAIL_OFF, IMPL_TAIL_LEN);

  /* MPI_SOURCE can be MPI_PROC_NULL or MPI_ANY_SOURCE and MPI_TAG can be
   * MPI_ANY_TAG, so both go through their own role-specific conversion here
   * too -- the out direction of NOTES.md #5.4.
   */
  abi->MPI_SOURCE = mpiwrapper_rank_toabi(st->MPI_SOURCE);
  abi->MPI_TAG    = mpiwrapper_tag_toabi(st->MPI_TAG);

  /* The *empty* status is the exception to 3.2.5, and it is one the standard
   * spells out: MPI-5.0 3.7.3 defines an empty status as MPI_SOURCE =
   * MPI_ANY_SOURCE, MPI_TAG = MPI_ANY_TAG, MPI_ERROR = MPI_SUCCESS and a
   * count of zero, which MPI_Wait on MPI_REQUEST_NULL returns. So the error
   * field is part of *that* status's value and has to be copied, while for
   * an ordinary completion it must not be.
   *
   * The two cases are told apart by the status the implementation filled in,
   * and MPI_ANY_SOURCE is what does it: a completed receive always reports a
   * definite rank, and a receive from MPI_PROC_NULL -- which does have
   * MPI_TAG = MPI_ANY_TAG (3.11) and whose error field MPICH's own
   * pt2pt/mprobe requires *preserved* -- reports MPI_PROC_NULL rather than
   * MPI_ANY_SOURCE. Found by pt2pt/rqstatus, which is the test for the empty
   * status and the one that fails if this rule is written as the simple one.
   */
  const int empty = st->MPI_SOURCE == MPI_ANY_SOURCE
                    && st->MPI_TAG == MPI_ANY_TAG;
  if (with_error || empty)
    abi->MPI_ERROR = mpiwrapper_errorcode_toabi(st->MPI_ERROR);
  memcpy(abi->MPI_internal, blob, sizeof blob);
}

/* The whole status, error field included: the multiple-completion calls, and
 * the four Fortran status converters, where the field is part of the object
 * being converted rather than an output of a call.
 */
void mpiwrapper_status_toabi(const MPI_Status *st, MPIABI_Status *abi)
{
  status_toabi(st, abi, 1);
}

/* Everything but the error field, for a single OUT status -- except when the
 * implementation filled in an empty status, whose error field is part of its
 * definition. See status_toabi above.
 */
void mpiwrapper_status_toabi_keep_error(const MPI_Status *st,
                                        MPIABI_Status *abi)
{
  status_toabi(st, abi, 0);
}

void mpiwrapper_status_fromabi(const MPIABI_Status *abi, MPI_Status *st)
{
  unsigned char       *dst  = (unsigned char *)st;
  const unsigned char *blob = (const unsigned char *)abi->MPI_internal;

  memset(st, 0, sizeof *st);
  memcpy(dst, blob, IMPL_HEAD_LEN);
  memcpy(dst + IMPL_TAIL_OFF, blob + IMPL_HEAD_LEN, IMPL_TAIL_LEN);

  st->MPI_SOURCE = mpiwrapper_rank_fromabi(abi->MPI_SOURCE);
  st->MPI_TAG    = mpiwrapper_tag_fromabi(abi->MPI_TAG);
  st->MPI_ERROR  = mpiwrapper_errorcode_fromabi(abi->MPI_ERROR);
}
