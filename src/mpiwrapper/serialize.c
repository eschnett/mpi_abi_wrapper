/* libmpiwrapper -- handle serialization, the table behind MPI_<class>_toint
 * and _fromint (MPI-5.0 20.4.5).
 *
 * Hand-written and permanent, and the closest relative in this directory is
 * keyvals.c rather than handles.c: like a dynamic keyval, a serialized handle
 * is an `int` the ABI side gets to choose, and choosing it from a high base
 * puts it beyond the reserved predefined range by construction rather than by
 * luck.
 *
 * **Why a table and not a cast.** The standard says a predefined handle must
 * serialize to the value Annex A lists for it, and the ABI's predefined handle
 * values are 0x20..0x2eb -- so those are answered by a cast and must not be
 * interned. A *dynamic* ABI handle, though, is the implementation's own handle
 * bits, and on Open MPI that is the address of an object: 64 bits, which does
 * not fit in an int, and which no truncation can recover. The rationale beside
 * the standard's own text names the remedy -- "using a lookup table or hash
 * function" -- and this is the lookup table.
 *
 * **Why dynamic handles are interned even where they would fit.** MPICH's
 * handles are ints and would survive a cast. Interning them anyway keeps one
 * code path rather than two, so the table is exercised on every implementation
 * instead of only the one that needs it (the discipline of NOTES.md #5.8), and
 * it removes a collision that is otherwise real: an implementation handle
 * whose bits happened to land in the interned range would be indistinguishable
 * from an index.
 *
 * **Why nothing is removed.** MPI-5.0 20.4.5 makes it erroneous to pass an
 * integer whose handle has been freed, so a stale entry is never *consulted*
 * legally; and when an implementation reuses a freed handle value for a new
 * object, the existing entry describes the new object correctly, which is why
 * a repeat interning of the same bits returns the same integer rather than
 * consuming a second slot. What that costs is one slot per distinct handle
 * value ever serialized -- a documented fixed-capacity limit like every other
 * table here (NOTES.md #6.2).
 */

#include "internal.h"

#include <stdatomic.h>

/* Above the ABI's predefined range (0x2eb) by a margin no future ABI revision
 * will cross, and small enough that base + MPIWRAPPER_SERIAL_SLOTS cannot
 * overflow an int on any platform.
 */
#define MPIWRAPPER_SERIAL_ABI_BASE 0x40000000

/* Zero is the empty marker, which is what lets the table be plain static
 * storage with no initialization step and therefore no initialization race: a
 * claimed-but-not-yet-published slot reads as zero and simply does not match,
 * exactly as in keyvals.c.
 *
 * Zero is available as a marker because it is not a handle that can reach
 * here. Every ABI predefined handle is in 0x20..0x2eb and is answered above
 * without interning, and a *dynamic* handle is an object the implementation
 * created -- an address on Open MPI, a kind-tagged int on MPICH, never null in
 * either. The one caller that could break that rule is one converting a null
 * handle it had not recognized, so the entry point rejects zero rather than
 * trusting the argument.
 */
static _Atomic uint64_t serial_bits[MPIWRAPPER_SERIAL_SLOTS];
static atomic_int       serial_count;

int mpiwrapper_handle_toint(uint64_t abi_bits)
{
  /* Requirement one: a predefined handle serializes to its own ABI value, so
   * that an application built against the ABI header can compare the result
   * with the constant it knows.
   */
  if (mpiwrapper_in_predef_range(abi_bits)) return (int)abi_bits;
  if (abi_bits == 0) return 0; /* not a handle; see the marker note above */

  /* Requirement two: the same handle serializes to the same integer. Searched
   * backwards for the same reason keyvals.c does -- when an implementation
   * recycles a handle value the newest registration is the live one -- though
   * here the two agree, since a repeat of the same bits is not appended.
   */
  const int count = atomic_load_explicit(&serial_count, memory_order_acquire);
  const int last  = count < MPIWRAPPER_SERIAL_SLOTS ? count
                                                    : MPIWRAPPER_SERIAL_SLOTS;
  for (int slot = last - 1; slot >= 0; --slot)
    if (atomic_load_explicit(&serial_bits[slot], memory_order_acquire)
        == abi_bits)
      return MPIWRAPPER_SERIAL_ABI_BASE + slot;

  /* Lock-free append, as everywhere else here: one relaxed fetch_add to claim
   * an index, one release store to publish it. Two threads serializing the
   * same handle at the same time may each claim a slot and so hand out two
   * integers for one handle; both convert back to that handle, so the round
   * trip stays exact, and the standard's requirement holds for every sequence
   * of calls a single thread can observe.
   */
  const int slot = atomic_fetch_add_explicit(&serial_count, 1,
                                             memory_order_relaxed);
  if (slot >= MPIWRAPPER_SERIAL_SLOTS) {
    /* Do not let the counter run away from the table; a later call must still
     * see itself as out of room rather than wrap into a valid index.
     */
    atomic_store_explicit(&serial_count, MPIWRAPPER_SERIAL_SLOTS,
                          memory_order_relaxed);
    return 0; /* MPIWRAPPER_SERIAL_SLOTS handles serialized in one process */
  }
  atomic_store_explicit(&serial_bits[slot], abi_bits, memory_order_release);
  return MPIWRAPPER_SERIAL_ABI_BASE + slot;
}

int mpiwrapper_handle_fromint(int value, uint64_t *abi_bits)
{
  if (value >= 0 && mpiwrapper_in_predef_range((uint64_t)value)) {
    *abi_bits = (uint64_t)value;
    return 1;
  }

  const int slot = value - MPIWRAPPER_SERIAL_ABI_BASE;
  if (slot < 0 || slot >= MPIWRAPPER_SERIAL_SLOTS) return 0;

  const uint64_t bits = atomic_load_explicit(&serial_bits[slot],
                                             memory_order_acquire);
  if (bits == 0) return 0; /* an integer this library never issued */
  *abi_bits = bits;
  return 1;
}
