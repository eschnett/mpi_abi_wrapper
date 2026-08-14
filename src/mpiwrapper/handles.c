/* libmpiwrapper -- the implementation -> ABI handle direction.
 *
 * Hand-written and permanent. The ABI -> implementation direction is a dense
 * switch and lives in the generated constants.c; this is the hard direction,
 * because predefined *implementation* handle values are not compile-time
 * constants in general -- Open MPI's are the addresses of static objects -- so
 * the map cannot be a switch and cannot be built at compile time. It is built
 * once, inside mpiwrapper_get_vtable, before any slot can be called.
 */

#include "internal.h"

/* A *perfect* hash, not open addressing. The whole key set is known at
 * initialization, so a multiplier can be searched for until no two keys
 * collide, which removes the probe loop -- and the probe loop is the only
 * data-dependent part. dev/handle-map-bench measures the difference: 1.09 ns
 * flat, against 1.36-1.53 ns for open addressing whenever the datatype varies
 * from call to call, and 3.9 ns for a sorted array with binary search.
 */
int mpiwrapper_rmap_build(struct mpiwrapper_rmap *m, const uint64_t *keys,
                          const uint64_t *abis, size_t n)
{
  unsigned shift = 64;
  for (size_t t = m->nslots; t > 1; t >>= 1) --shift;

  uint64_t mul = 0x9e3779b97f4a7c15u;
  for (int attempt = 0; attempt < 4096; ++attempt, mul += 0x2545f4914f6cdd1du) {
    for (size_t i = 0; i < m->nslots; ++i)
      m->slots[i].key = MPIWRAPPER_RMAP_EMPTY;

    size_t stored = 0;
    int    ok     = 1;
    for (size_t i = 0; i < n && ok; ++i) {
      const size_t slot = (size_t)((keys[i] * mul) >> shift);
      if (m->slots[slot].key == keys[i]) {
        /* An alias, not a collision: two ABI-distinct predefined handles that
         * this implementation gives the same value. The commonest case is an
         * optional datatype answering MPI_DATATYPE_NULL. The reverse direction
         * has to pick one and the ABI's own order is the canonical one, so the
         * first entry wins and this one is dropped -- silently, because it is
         * not an error, and loudly enough in that the selftest walks the same
         * tables and reports what did not round-trip.
         */
        continue;
      }
      if (m->slots[slot].key != MPIWRAPPER_RMAP_EMPTY) {
        ok = 0;
      } else {
        m->slots[slot].key = keys[i];
        m->slots[slot].abi = abis[i];
        ++stored;
      }
    }
    if (ok) {
      m->mul   = mul;
      m->shift = shift;
      m->nkeys = stored;
      return 1;
    }
  }
  /* Bounded, and reported rather than degraded: falling back to probing at run
   * time would put back the branch this exists to remove, so the failure has to
   * be loud and at initialization (NOTES.md #5.1).
   */
  return 0;
}

/* A dynamically created implementation handle becomes an ABI handle by
 * preserving its bits. That is only correct if it cannot land inside the ABI's
 * predefined range, and it does not for either implementation today: MPICH's
 * handles carry a kind field in the high bits so all real ones are
 * >= 0x04000000, and Open MPI's are object addresses.
 *
 * That cannot be proved at configure time -- cross-compiling forbids running a
 * probe -- and it cannot be tagged around, because ABI handles are
 * pointer-sized and a 32-bit target has no spare high bits. So the check is
 * here, at run time, in the toabi direction only: that is object *creation*,
 * not every MPI_Send. test/mpiwrapper_selftest.c probes it too, by creating
 * many objects of each class and checking where they land.
 *
 * The flag is thread-local so that a collision hit by one thread is not
 * reported by another's call, and it is *taken* rather than read, so a caller
 * that forgets to check cannot leave it set for the next call to blame.
 */
static _Thread_local int handle_error;

int mpiwrapper_take_handle_error(void)
{
  const int e = handle_error;
  handle_error = 0;
  return e;
}

#define MPIWRAPPER_TOABI(key, abitype, impltype, abinull)                      \
  abitype mpiwrapper_##key##_toabi(impltype h)                                 \
  {                                                                            \
    const uint64_t bits = MPIWRAPPER_BITS(h);                                  \
    uint64_t       abi;                                                        \
    if (mpiwrapper_rmap_lookup(&mpiwrapper_rmap_##key, bits, &abi))            \
      return MPIWRAPPER_HANDLE(abitype, abi);                                  \
    if (mpiwrapper_in_predef_range(bits)) {                                    \
      handle_error = 1;                                                        \
      return abinull;                                                          \
    }                                                                          \
    return MPIWRAPPER_HANDLE(abitype, bits);                                   \
  }

MPIWRAPPER_TOABI(comm, MPIABI_Comm, MPI_Comm, MPIABI_COMM_NULL)
MPIWRAPPER_TOABI(datatype, MPIABI_Datatype, MPI_Datatype, MPIABI_DATATYPE_NULL)
MPIWRAPPER_TOABI(errhandler, MPIABI_Errhandler, MPI_Errhandler,
                 MPIABI_ERRHANDLER_NULL)
MPIWRAPPER_TOABI(file, MPIABI_File, MPI_File, MPIABI_FILE_NULL)
MPIWRAPPER_TOABI(group, MPIABI_Group, MPI_Group, MPIABI_GROUP_NULL)
MPIWRAPPER_TOABI(info, MPIABI_Info, MPI_Info, MPIABI_INFO_NULL)
MPIWRAPPER_TOABI(message, MPIABI_Message, MPI_Message, MPIABI_MESSAGE_NULL)
MPIWRAPPER_TOABI(op, MPIABI_Op, MPI_Op, MPIABI_OP_NULL)
MPIWRAPPER_TOABI(request, MPIABI_Request, MPI_Request, MPIABI_REQUEST_NULL)
MPIWRAPPER_TOABI(session, MPIABI_Session, MPI_Session, MPIABI_SESSION_NULL)
MPIWRAPPER_TOABI(win, MPIABI_Win, MPI_Win, MPIABI_WIN_NULL)
