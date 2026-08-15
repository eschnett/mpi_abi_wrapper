/* libmpiwrapper -- the attached buffer's ownership record (S4b).
 *
 * Twelve of the eighteen buffer entry points are hand-written, and this is the
 * only piece of state behind them. The other six -- the flush forms -- are
 * mechanical and the generator emits them.
 *
 * **What needs remembering, and what does not.** When the application attaches
 * a buffer of its own, nothing here is involved: the address crosses
 * unconverted and comes back out of the detach unconverted. The one case that
 * needs a record is MPI_BUFFER_AUTOMATIC over an implementation that does not
 * have it. MPI-4.1 added the mode, so an MPI-4.0 or older implementation --
 * Open MPI 5.0.6, for one -- has no way to express "MPI, provide the buffer".
 * The wrapper provides one instead, and then the detach side has a problem it
 * would not otherwise have: the implementation hands back *our* address, and
 * the standard requires the caller to be handed back MPI_BUFFER_AUTOMATIC.
 * Only a record of what we attached distinguishes the two.
 *
 * **The emulation is an approximation, and it is a documented one.** MPI-5.0
 * 3.6 says a buffer "of sufficient size is automatically used by the MPI
 * library"; a fixed block is not that, so a program that would have run
 * against a real automatic buffer can exhaust ours and see MPI_ERR_BUFFER.
 * Doing better would mean reimplementing buffered mode above the
 * implementation -- intercepting every MPI_Bsend to grow the buffer, which is
 * a second implementation of the feature and a much larger thing to get wrong.
 * The size is a tunable, and the limitation is named in gen/report.txt.
 *
 * **One record per buffering scope.** MPI-4.1 attaches buffers to the process,
 * to a communicator, or to a session, and each of the three can hold one
 * buffer at a time. The key is the implementation handle's bits, with
 * MPIWRAPPER_AUTOBUF_PROCESS for the process-wide one; a fixed-capacity,
 * lock-free table like every other here (#6.3), because the alternative is a
 * lock on a path the application may take per communicator.
 *
 * In practice only the process-wide row is reachable today: an implementation
 * that has MPI_Comm_attach_buffer has MPI_BUFFER_AUTOMATIC too, both being
 * MPI-4.1, so the communicator and session scopes reach the emulation only on
 * an implementation that adopted half of the chapter. The table is written for
 * all three anyway rather than special-casing the one that can be reached,
 * since the rule is the same and a special case is what breaks on the
 * implementation nobody tested against.
 */

#include "internal.h"

#include <stdatomic.h>
#include <stdlib.h>

#ifndef MPIWRAPPER_AUTOBUF_SLOTS
#  define MPIWRAPPER_AUTOBUF_SLOTS 64
#endif

/* 8 MiB: large enough that a program using automatic buffering for ordinary
 * messages does not notice, small enough to be an unremarkable allocation on
 * any machine that runs MPI. Only ever allocated where the application asked
 * for automatic buffering and the implementation cannot provide it.
 */
#ifndef MPIWRAPPER_AUTOBUF_BYTES
#  define MPIWRAPPER_AUTOBUF_BYTES (8u * 1024u * 1024u)
#endif

/* **The scope is the claim word, and the order matters in both directions.**
 * An empty slot has scope 0, which no key can be: the process-wide scope is
 * UINT64_MAX and every other is the bits of a live handle. So a slot is taken
 * by a CAS on the scope and the block is published after it, and released by
 * clearing the block *first* and the scope last -- which is what makes it
 * impossible for a release to free a block that a later claim has just put
 * there. The other order looks equally good and is not: a slot whose scope
 * still held the previous owner's key would be matched by that owner's
 * release, between the new claim's two stores.
 *
 * Zero as the empty marker is also why this is plain static storage with no
 * initialization step, exactly as in keyvals.c.
 */
struct autobuf_slot {
  _Atomic uint64_t scope;
  void *_Atomic    block;
};

static struct autobuf_slot autobuf[MPIWRAPPER_AUTOBUF_SLOTS];

void *mpiwrapper_autobuf_claim(uint64_t scope, size_t *bytes)
{
  /* No key can be the empty marker: the process-wide scope is UINT64_MAX and a
   * handle's bits are never zero, not even for a null handle -- MPICH numbers
   * MPI_COMM_NULL 0x04000000 and Open MPI gives it the address of an object.
   * Refusing rather than trusting that keeps a caller that found some way to
   * pass zero from corrupting the table instead of getting an error.
   */
  if (scope == 0) return NULL;

  void *const block = malloc(MPIWRAPPER_AUTOBUF_BYTES);
  if (!block) return NULL;

  for (int i = 0; i < MPIWRAPPER_AUTOBUF_SLOTS; ++i) {
    uint64_t expected = 0;
    if (atomic_compare_exchange_strong_explicit(&autobuf[i].scope, &expected,
                                                scope, memory_order_acq_rel,
                                                memory_order_acquire)) {
      atomic_store_explicit(&autobuf[i].block, block, memory_order_release);
      *bytes = MPIWRAPPER_AUTOBUF_BYTES;
      return block;
    }
  }

  free(block);
  return NULL; /* MPIWRAPPER_AUTOBUF_SLOTS automatic buffers at once */
}

int mpiwrapper_autobuf_release(uint64_t scope)
{
  for (int i = 0; i < MPIWRAPPER_AUTOBUF_SLOTS; ++i) {
    if (atomic_load_explicit(&autobuf[i].scope, memory_order_acquire) != scope)
      continue;

    /* Take the block before freeing it, so that two detaches of one scope
     * cannot both free it: the exchange picks a unique winner and the loser
     * sees NULL. Only the winner gives the slot back.
     */
    void *const block = atomic_exchange_explicit(&autobuf[i].block, NULL,
                                                 memory_order_acq_rel);
    if (!block) continue;
    atomic_store_explicit(&autobuf[i].scope, 0, memory_order_release);
    free(block);
    return 1;
  }
  return 0;
}
