/* Stands in for libmpiwrapper's slot bodies.
 *
 * Two callees, because the answer depends entirely on which one you have:
 *   cheap  -- models MPI_Wtime, MPI_Comm_rank, a satisfied MPI_Test
 *   real   -- burns roughly what a small MPI_Send costs, so the dispatch overhead
 *             can be seen in proportion
 */

#include <stdint.h>

int callee_cheap(int x) { return x + 1; }

/* Plain, not volatile: a volatile loop counter makes the loop itself so
 * memory-bound that it swamps the effect being measured. Set once by the harness,
 * and unknown at compile time so the loop cannot be unrolled away.
 */
int callee_spin_iters = 1;

int callee_real(int x)
{
  uint64_t s = (uint64_t)(unsigned)x + 0x9e3779b97f4a7c15u;
  for (int i = 0; i < callee_spin_iters; ++i) {
    s ^= s >> 12;
    s ^= s << 25;
    s ^= s >> 27;
    s *= 0x2545f4914f6cdd1du;
  }
  return (int)(unsigned)(s >> 33);
}

/* The vtable lives here, in the "wrapper" DSO, and is handed over through a
 * function call across the library boundary -- exactly as libmpiwrapper hands one
 * to libmpi_abi. That opacity is essential to the measurement: when the mock built
 * the vtable from a static in the same translation unit as the dispatch code, the
 * compiler proved the pointers could only hold one value each and devirtualized
 * two of the five shapes into `b _callee_cheap`, which then measured identical to
 * a direct call for the obvious wrong reason.
 */
#include "bench.h"

static const struct vtable real_vtable = {
    .cheap = callee_cheap,
    .real  = callee_real,
};

const struct vtable *dispatch_get_vtable(void) { return &real_vtable; }
