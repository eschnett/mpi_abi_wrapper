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
 */

#include "internal.h"

#include <stdatomic.h>
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

int mpiwrapper_staged_attach(MPI_Request request, void *block)
{
  const uint64_t key  = MPIWRAPPER_BITS(request);
  size_t         slot = staged_home(key);

  if (key == KEY_EMPTY || key == KEY_LOCKED || key == KEY_TOMB) return 0;

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
      return 1;
    }
    if (k == key) {
      /* The implementation reused a request value we still hold state for.
       * That would mean a completion we did not observe, and freeing the old
       * block here would be a guess; refusing is the honest answer and the
       * caller turns it into MPIABI_ERR_INTERN.
       */
      return 0;
    }
  }
  return 0; /* full: MPIWRAPPER_STAGED_REQUEST_SLOTS in flight at once */
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
