/* libmpiwrapper -- the dynamic half of the error-code family (NOTES.md #5.6,
 * S4b).
 *
 * Hand-written and permanent, and deliberately the same shape as keyvals.c:
 * both families have a closed predefined half that the generated switch in
 * constants.c converts case by case, and an open half that the implementation
 * hands out at run time and that no switch can name.
 *
 * **Why the value cannot be passed through.** MPI_ERR_LASTCODE is a constant
 * the *header* fixes, and the two headers disagree by five orders of
 * magnitude: 16383 in the ABI against MPICH's 0x3fffffff. MPI-5.0 9.5 puts
 * every error class the implementation issues at run time above its own
 * MPI_ERR_LASTCODE, so a code from MPI_Add_error_class arrives here as a
 * number the ABI header says cannot be one. An application that compares it
 * against MPI_ERR_LASTCODE -- which is exactly what the standard tells it to
 * do -- would then be told the code is invalid.
 *
 * So the ABI-side value is ours to choose, and it is drawn from just above the
 * ABI's own MPI_ERR_LASTCODE: above every predefined class, above MPI_T's
 * 1001-1018, and where an application looking for a user-defined class expects
 * to find one.
 *
 * **The implementation's own dynamic codes are interned too, and that is a
 * measurement rather than a precaution.** MPICH 4.3.1 answers essentially
 * every error with an instance-specific code rather than with a class:
 * MPI_Comm_rank(MPI_COMM_NULL) is 604597509, a bad count is 807037698, a
 * missing file is 874557989, each encoding an error stack whose class is
 * recovered with MPI_Error_class. Open MPI 5.0.6 returns the plain class in
 * all three. So a default arm that answered MPIABI_ERR_OTHER for anything it
 * did not recognize -- which is what this switch did until S4b -- turned
 * *every* MPICH error into "other", and an application asking
 * MPI_Error_class(err) == MPI_ERR_NO_SUCH_FILE could never be told yes.
 *
 * Interning fixes both halves at once, because the ABI value converts back
 * down to the implementation's own code: MPI_Error_class then reaches MPICH's
 * class for it (which is predefined, so the switch names it), and
 * MPI_Error_string reaches MPICH's full message rather than a generic one.
 * A code the application created with MPI_Add_error_class and a code the
 * implementation invented sit in the same table, and the composition is
 * exact -- asking for the class of an interned instance code yields the
 * implementation's class, which is itself either predefined or interned.
 *
 * **What a full table answers.** MPIABI_ERR_OTHER, which is what this arm
 * answered before and is a legal class for an error this ABI cannot name.
 * That is the one place the fidelity above degrades, and it degrades to
 * exactly the old behaviour.
 *
 * **Nothing is removed, even by MPI_Remove_error_class.** The removal makes
 * the implementation's code invalid, not our record of what it once meant, and
 * an implementation is free to hand the same number out again afterwards. So
 * the reverse direction scans newest-first, exactly as keyvals.c does and for
 * exactly the same reason: the newest registration of a recycled number is the
 * live one.
 */

#include "internal.h"

#include <stdatomic.h>

/* Just above the ABI's last predefined code. MPI-5.0 9.5 is explicit that
 * MPI_ERR_LASTCODE "is a constant value and is not affected by new
 * user-defined error codes and classes", so this base cannot collide with a
 * later ABI revision's predefined set either.
 *
 * MPI_LASTUSEDCODE, the attribute that reports the current maximum, is a
 * different matter and is not this file's to answer: it comes back through
 * MPI_Comm_get_attr as an int the implementation owns, and neither the
 * generator nor this registry can see that it is an error class rather than
 * any other attribute value. That is recorded in NOTES.md #5.6 as a known gap
 * rather than papered over here.
 */
#define MPIWRAPPER_ERRORCODE_ABI_BASE (MPIABI_ERR_LASTCODE + 1)

_Static_assert(MPIWRAPPER_ERRORCODE_ABI_BASE + MPIWRAPPER_ERRORCODE_SLOTS
                   > MPIWRAPPER_ERRORCODE_ABI_BASE,
               "the dynamic error-code range overflows an int");

/* Lock-free append (#6.3): one relaxed fetch_add to claim an index, one
 * release store to publish the value. Zero is the empty marker and is not a
 * value any implementation issues here -- 0 is MPI_SUCCESS everywhere -- so a
 * claimed-but-unpublished slot simply does not match.
 */
static _Atomic int errorcode_impl[MPIWRAPPER_ERRORCODE_SLOTS];
static atomic_int  errorcode_count;

int mpiwrapper_errorcode_add(int ierror, int *abi_ierror)
{
  if (ierror == MPI_SUCCESS) return 0; /* not a code anyone can register */

  const int slot = atomic_fetch_add_explicit(&errorcode_count, 1,
                                             memory_order_relaxed);
  if (slot >= MPIWRAPPER_ERRORCODE_SLOTS) {
    /* Do not let the counter run away from the table; a later add must still
     * see itself as out of room rather than wrap into a valid index.
     */
    atomic_store_explicit(&errorcode_count, MPIWRAPPER_ERRORCODE_SLOTS,
                          memory_order_relaxed);
    return 0; /* MPIWRAPPER_ERRORCODE_SLOTS codes in one process */
  }
  atomic_store_explicit(&errorcode_impl[slot], ierror, memory_order_release);
  *abi_ierror = MPIWRAPPER_ERRORCODE_ABI_BASE + slot;
  return 1;
}

/* The ABI value *is* the index, so this direction needs no search. */
int mpiwrapper_errorcode_dynamic_fromabi(int abi_ierror)
{
  const int slot  = abi_ierror - MPIWRAPPER_ERRORCODE_ABI_BASE;
  const int count = atomic_load_explicit(&errorcode_count,
                                         memory_order_relaxed);

  if (slot < 0 || slot >= count || slot >= MPIWRAPPER_ERRORCODE_SLOTS)
    return MPI_ERR_OTHER;
  return atomic_load_explicit(&errorcode_impl[slot], memory_order_acquire);
}

int mpiwrapper_errorcode_dynamic_toabi(int ierror)
{
  const int count = atomic_load_explicit(&errorcode_count,
                                         memory_order_relaxed);
  const int last  = count < MPIWRAPPER_ERRORCODE_SLOTS
                        ? count
                        : MPIWRAPPER_ERRORCODE_SLOTS;

  for (int slot = last - 1; slot >= 0; --slot)
    if (atomic_load_explicit(&errorcode_impl[slot], memory_order_acquire)
        == ierror)
      return MPIWRAPPER_ERRORCODE_ABI_BASE + slot;

  /* Not one we have seen, so it is the implementation's own -- intern it, and
   * the round trip through MPI_Error_class and MPI_Error_string works on it
   * exactly as it does on a class the application added. Two threads racing on
   * the same code may each claim a slot; both convert back to it, so the round
   * trip stays exact, as in serialize.c.
   */
  int abi_ierror;
  if (mpiwrapper_errorcode_add(ierror, &abi_ierror)) return abi_ierror;
  return MPIABI_ERR_OTHER; /* the table is full; the pre-S4b answer */
}
