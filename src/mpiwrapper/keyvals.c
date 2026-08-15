/* libmpiwrapper -- the dynamic half of the keyval family (NOTES.md #5.6).
 *
 * Hand-written and permanent. The thirteen predefined attribute keys convert
 * through the generated switch in constants.c, like every other mapped integer
 * family. A keyval the implementation handed out at run time cannot: it is an
 * int with no structure, and it can land anywhere. The ABI puts its
 * communicator keys at 501-507 and its window keys at 601-605, and Open MPI
 * hands out small sequential ints, so nothing but arithmetic stops a dynamic
 * keyval from arriving as MPI_TAG_UB. No pointer slack is available to tag
 * around it -- a keyval is an `int` on both sides -- so the mapping has to be a
 * table.
 *
 * Hence the two rules this file implements:
 *
 *  - **The ABI-side value is ours to choose**, and it is drawn from a base far
 *    above any predefined key. So "predefined or dynamic" is decided by the
 *    value itself and the two halves cannot collide by construction. The
 *    generated switch tries the predefined cases first and falls through to
 *    here, which is why the base has to be outside every case label there.
 *  - **Only what this library issued is translatable.** Every dynamic keyval
 *    an application can hold came back from one of the four
 *    MPI_*_create_keyval calls, which is the only caller of
 *    mpiwrapper_keyval_add. A value that is neither predefined nor in this
 *    table is one we never issued, and both directions answer
 *    MPI_KEYVAL_INVALID rather than invent a mapping -- which is what the
 *    implementation would answer for it anyway.
 *
 * Nothing is ever removed. MPI-5.0 7.7 makes a freed key still usable by
 * attributes that have not been deleted yet -- "the actual free does not
 * transpire until after all references have been freed" -- so there is no
 * point at which an entry is known dead (NOTES.md #6.2). What that costs is
 * one slot per key ever created, which is why the table is a documented
 * fixed-capacity limit like every other here.
 */

#include "internal.h"

#include <stdatomic.h>

#ifndef MPIWRAPPER_KEYVAL_SLOTS
#  define MPIWRAPPER_KEYVAL_SLOTS 1024
#endif

/* Above the ABI's last predefined key (605) by a margin no future one will
 * cross, and small enough that base + MPIWRAPPER_KEYVAL_SLOTS cannot overflow
 * an int on any platform.
 */
#define MPIWRAPPER_KEYVAL_ABI_BASE 0x40000000

/* Lock-free append, like the other shared tables (NOTES.md #6.3): one relaxed
 * fetch_add to claim an index, one release store to publish the value.
 *
 * A reader may see a claimed-but-unpublished slot, which reads as 0 --
 * MPI_KEYVAL_INVALID, and never a keyval any implementation hands out, so it
 * simply does not match. That is not a race a caller can hit with its own
 * keyval: the thread that was given an ABI keyval published it before
 * MPI_*_create_keyval returned, and every later use is ordered after that.
 */
static _Atomic int keyval_impl[MPIWRAPPER_KEYVAL_SLOTS];
static atomic_int  keyval_count;

int mpiwrapper_keyval_add(int keyval, int *abi_keyval)
{
  const int slot = atomic_fetch_add_explicit(&keyval_count, 1,
                                             memory_order_relaxed);
  if (slot >= MPIWRAPPER_KEYVAL_SLOTS) {
    /* Do not let the counter run away from the table; a later add must still
     * see itself as out of room rather than wrap into a valid index.
     */
    atomic_store_explicit(&keyval_count, MPIWRAPPER_KEYVAL_SLOTS,
                          memory_order_relaxed);
    return 0;
  }
  atomic_store_explicit(&keyval_impl[slot], keyval, memory_order_release);
  *abi_keyval = MPIWRAPPER_KEYVAL_ABI_BASE + slot;
  return 1;
}

/* The ABI value *is* the index, so this direction needs no search. */
int mpiwrapper_keyval_dynamic_fromabi(int abi_keyval)
{
  const int slot  = abi_keyval - MPIWRAPPER_KEYVAL_ABI_BASE;
  const int count = atomic_load_explicit(&keyval_count, memory_order_relaxed);

  if (slot < 0 || slot >= count || slot >= MPIWRAPPER_KEYVAL_SLOTS)
    return MPI_KEYVAL_INVALID;
  return atomic_load_explicit(&keyval_impl[slot], memory_order_acquire);
}

/* Newest first, and that is the whole reason this is a scan rather than a
 * second index: an implementation is free to reuse the number of a keyval that
 * has been freed, and when it does, the entry that matters is the most recent
 * registration of that number. Searching forwards would resolve it to a key
 * the application no longer holds.
 */
int mpiwrapper_keyval_dynamic_toabi(int keyval)
{
  const int count = atomic_load_explicit(&keyval_count, memory_order_relaxed);
  const int last  = count < MPIWRAPPER_KEYVAL_SLOTS ? count
                                                    : MPIWRAPPER_KEYVAL_SLOTS;

  if (keyval == MPI_KEYVAL_INVALID) return MPIABI_KEYVAL_INVALID;
  for (int slot = last - 1; slot >= 0; --slot)
    if (atomic_load_explicit(&keyval_impl[slot], memory_order_acquire) == keyval)
      return MPIWRAPPER_KEYVAL_ABI_BASE + slot;
  return MPIABI_KEYVAL_INVALID;
}
