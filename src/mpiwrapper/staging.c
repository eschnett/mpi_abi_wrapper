/* libmpiwrapper -- staged temporaries.
 *
 * Hand-written and permanent. Two mechanisms:
 *
 *  - mpiwrapper_stage/unstage: a call-scoped temporary, a fixed-size caller
 *    buffer when the request fits and heap above a *byte* threshold. Arrays are
 *    always converted into temporaries and never in place, for four independent
 *    reasons (NOTES.md #5.7), of which the shortest is that
 *    `static const MPI_Datatype types[3]` lives in .rodata and writing to it
 *    crashes a legal program.
 *
 *  - the request-keyed table: temporaries that must outlive the call, which is
 *    the MPI_Ialltoallw family and the persistent _init forms. Guarded by a
 *    global atomic count, so a completion call in an application that never
 *    uses those routines pays one relaxed load and a compare against zero
 *    (NOTES.md #6.3, decision 10).
 *
 * and one policy over the second of them, mpiwrapper_staged_keep, which is what
 * the generated bodies actually call. The table itself only stores; deciding
 * whether a block belongs in it at all, and what to do when it will not fit, is
 * NOTES.md #13.2's (a), (b) and (c), and lives at the bottom of this file.
 */

#include "internal.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

void *mpiwrapper_stage(void *stackbuf, size_t stackbytes, size_t nmemb,
                       size_t size)
{
  if (nmemb == 0) return stackbuf; /* never NULL, so callers need no special
                                    * case for a zero-length array */
  if (nmemb <= stackbytes / size) return stackbuf;
  if (nmemb > SIZE_MAX / size) return NULL;
  return malloc(nmemb * size);
}

void mpiwrapper_unstage(void *p, void *stackbuf)
{
  if (p && p != stackbuf) free(p);
}

/* ------------------------------------------------- the request-keyed table */

/* Open addressing with CAS insert, fixed capacity, no mutex. Three key values
 * are reserved:
 *
 *   EMPTY (0)   never used, and no implementation handle is 0 -- MPICH's carry
 *               a nonzero kind field and Open MPI's are addresses of objects.
 *   LOCKED      a releaser owns the entry and is clearing its block.
 *   TOMBSTONE   released; reusable, and *not* EMPTY, because clearing the key
 *               would truncate the probe chain of some other key.
 *
 * The lock word is what makes reuse safe. Without it, a releaser that reads the
 * key, then has the entry stolen by a concurrent attach, would go on to free
 * the *new* owner's block. So a release first claims the entry (key -> LOCKED),
 * then clears and frees, then publishes TOMBSTONE; an attach may only take an
 * entry it observed as EMPTY or TOMBSTONE, by which point the block field is
 * already NULL.
 */

#define KEY_EMPTY 0u
#define KEY_LOCKED UINT64_MAX
#define KEY_TOMB (UINT64_MAX - 1u)

struct staged_entry {
  _Atomic uint64_t key;
  void *_Atomic   block;
};

static struct staged_entry staged_table[MPIWRAPPER_STAGED_REQUEST_SLOTS];
static atomic_int          staged_live;

static size_t staged_home(uint64_t key)
{
  /* Fibonacci hashing: the same multiply-shift as the predefined-handle map,
   * and for the same reason -- implementation request handles are dense in
   * their low bits (MPICH) or 16-byte-aligned addresses (Open MPI), so the low
   * bits alone are a poor index.
   */
  return (size_t)((key * 0x9e3779b97f4a7c15u) >> 32) %
         MPIWRAPPER_STAGED_REQUEST_SLOTS;
}

int mpiwrapper_staged_any(void)
{
  return atomic_load_explicit(&staged_live, memory_order_relaxed) != 0;
}

enum mpiwrapper_staged_fate mpiwrapper_staged_attach(MPI_Request request,
                                                     void       *block)
{
  const uint64_t key  = MPIWRAPPER_BITS(request);
  size_t         slot = staged_home(key);

  /* No implementation handle takes one of the three reserved values, so this
   * is unreachable rather than a policy -- but a handle that did would corrupt
   * the table, and DUPLICATE is the fate that leaks rather than errors.
   */
  if (key == KEY_EMPTY || key == KEY_LOCKED || key == KEY_TOMB)
    return MPIWRAPPER_STAGED_DUPLICATE;

  for (size_t probe = 0; probe < MPIWRAPPER_STAGED_REQUEST_SLOTS;
       ++probe, slot = (slot + 1) % MPIWRAPPER_STAGED_REQUEST_SLOTS) {
    struct staged_entry *e = &staged_table[slot];
    uint64_t             k = atomic_load_explicit(&e->key, memory_order_acquire);

    if (k == KEY_EMPTY || k == KEY_TOMB) {
      if (!atomic_compare_exchange_strong_explicit(&e->key, &k, key,
                                                   memory_order_acq_rel,
                                                   memory_order_acquire))
        continue; /* someone else took it; keep probing */
      atomic_store_explicit(&e->block, block, memory_order_release);
      atomic_fetch_add_explicit(&staged_live, 1, memory_order_relaxed);
      return MPIWRAPPER_STAGED_STORED;
    }
    if (k == key) {
      /* Two live operations are presenting one handle. Overwriting would lose
       * the first block and freeing it would be a guess, so the table declines
       * -- but declining is all it does; the caller must not turn this into an
       * error, because the call was legal (NOTES.md #13.2).
       */
      return MPIWRAPPER_STAGED_DUPLICATE;
    }
  }
  return MPIWRAPPER_STAGED_FULL; /* SLOTS staged operations in flight at once */
}

/* ------------------------------------------------------------ the policy */

/* Three rules, and each closes a way the table used to answer MPI_ERR_INTERN to
 * a legal call (NOTES.md #13.2, where they are (a), (b) and (c)). They are here
 * rather than in the generated bodies because they are a policy, and because
 * one place to read them is worth more than eight copies of a branch.
 */

static atomic_ulong staged_leaks;

unsigned long mpiwrapper_staged_leaked(void)
{
  return atomic_load_explicit(&staged_leaks, memory_order_relaxed);
}

/* (c) The table would not take the block, so the block is never freed.
 *
 * Whichever fate brought us here, leaking is forced: the operation is already
 * in flight, it cannot be un-started, and the implementation may still be
 * reading the block, so freeing it is a use-after-free. #6.2 already accepts
 * "never reclaimed" for the callback pools and the attribute callbacks, so
 * that much is not a new concession -- but it is worth counting, and the count
 * is the oracle the selftest uses now that the error channel is narrower.
 *
 * What differs between the two fates is only what the *caller* is told, and
 * that is decided in mpiwrapper_staged_keep, not here.
 */
static void staged_leak(void)
{
  const unsigned long n =
      1u + atomic_fetch_add_explicit(&staged_leaks, 1u, memory_order_relaxed);
#ifndef NDEBUG
  /* Once, not per occurrence: the shape this fires in is a loop. */
  if (n == 1)
    fprintf(stderr,
            "libmpiwrapper: leaked a staged array -- the request table would "
            "not take it (duplicate key, or %d slots all in use). See "
            "NOTES.md #13.2.\n",
            MPIWRAPPER_STAGED_REQUEST_SLOTS);
#else
  (void)n;
#endif
}

/* (b) Is the implementation finished with the arrays already?
 *
 * MPI-5.0 6.12 lets an implementation keep reading the counts, displacement and
 * datatype arrays *until the operation completes* -- which is the whole reason
 * the block outlives the call (#5.7). So if the operation is complete when it
 * comes back, there is nothing to keep alive, and asking is exactly MPI-2.0's
 * MPI_Request_get_status: "Sets flag = true if the operation is complete...
 * However, unlike test or wait, it does not deallocate or inactivate the
 * request."
 *
 * Only ever asked of a nonblocking form. The standard's very next sentence is
 * the trap: an inactive request answers flag = true, and 3.9 makes a persistent
 * request inactive from creation, so a fresh MPI_Alltoallw_init would say
 * "complete" and freeing on that answer is a use-after-free at the first
 * MPI_Start. That is measured on both implementations (dev/request-identity/),
 * and it is why the caller passes the kind rather than this file guessing.
 *
 * Any answer but a clean flag = true means "attach", which is always safe.
 */
static int staged_is_complete(MPI_Request request)
{
#ifdef MPIWRAPPER_HAVE_MPI_Request_get_status
  int flag = 0;
  if (PMPI_Request_get_status(request, &flag, MPI_STATUS_IGNORE) != MPI_SUCCESS)
    return 0;
  return flag;
#else
  /* MPI-2.0, so below this project's MPI-3.0 floor and not reachable in a
   * conforming implementation. The guard costs nothing and keeps the file
   * honest about what it assumes.
   */
  (void)request;
  return 0;
#endif
}

int mpiwrapper_staged_keep(MPI_Request request, void *block, size_t nstaged,
                           enum mpiwrapper_staged_kind kind)
{
  /* (a) Nothing was staged -- a degree-0 neighbourhood collective, whose send
   * and receive arrays are both empty. There is no array for the
   * implementation to read, so there is nothing to keep alive and no reason to
   * spend a slot, let alone to collide in one. malloc(0) may still have
   * returned a pointer, and it is ours to free.
   */
  if (nstaged == 0) {
    free(block);
    return 1;
  }

  /* (b) */
  if (kind == MPIWRAPPER_STAGED_NONBLOCKING && staged_is_complete(request)) {
    free(block);
    return 1;
  }

  switch (mpiwrapper_staged_attach(request, block)) {
  case MPIWRAPPER_STAGED_STORED:
    return 1;
  case MPIWRAPPER_STAGED_DUPLICATE:
    /* (c) A legal call, and nothing the caller could have done differently:
     * the implementation shared one request between two operations. Leak the
     * array and succeed. Answering MPIABI_ERR_INTERN here is what #13.2 calls
     * the conformance bug, and under the default error handler it aborted a
     * correct program.
     */
    staged_leak();
    return 1;
  case MPIWRAPPER_STAGED_FULL:
  default:
    /* Not the same thing, and deliberately still an error: this is the
     * capacity limit every fixed table in the design has (#13.2's first
     * bullet), it names a build-time constant the user can raise, and
     * degrading to a silent leak would take away both the diagnosis and the
     * only oracle the release path has. The block leaks either way.
     */
    staged_leak();
    return 0;
  }
}

void mpiwrapper_staged_release(MPI_Request request)
{
  const uint64_t key  = MPIWRAPPER_BITS(request);
  size_t         slot = staged_home(key);

  if (!mpiwrapper_staged_any()) return;

  for (size_t probe = 0; probe < MPIWRAPPER_STAGED_REQUEST_SLOTS;
       ++probe, slot = (slot + 1) % MPIWRAPPER_STAGED_REQUEST_SLOTS) {
    struct staged_entry *e = &staged_table[slot];
    uint64_t             k = atomic_load_explicit(&e->key, memory_order_acquire);

    if (k == KEY_EMPTY) return; /* never inserted: not one of ours */
    if (k != key) continue;

    if (!atomic_compare_exchange_strong_explicit(&e->key, &k, KEY_LOCKED,
                                                 memory_order_acq_rel,
                                                 memory_order_acquire))
      return; /* another thread is completing the same request */

    void *block = atomic_exchange_explicit(&e->block, NULL, memory_order_acq_rel);
    atomic_fetch_sub_explicit(&staged_live, 1, memory_order_relaxed);
    atomic_store_explicit(&e->key, KEY_TOMB, memory_order_release);
    free(block);
    return;
  }
}
