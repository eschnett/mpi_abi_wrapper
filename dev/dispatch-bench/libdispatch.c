/* Stands in for libmpi_abi's entry points: the four candidate dispatch shapes.
 *
 * These are exported and called from another translation unit across a DSO
 * boundary, exactly as an application calls MPI_Send, so the dispatch cannot be
 * hoisted out of the caller's loop. That fidelity is the whole point -- measuring
 * this with everything in one TU would measure nothing.
 */

#include "bench.h"

#include <stdatomic.h>
#include <string.h>

extern int callee_cheap(int x);
extern int callee_real(int x);

/* Defined in libcallee (the "wrapper" DSO), so its value is opaque here. */
extern const struct vtable *dispatch_get_vtable(void);

/* ---- shape A: atomic acquire load + lazy-init branch (the original proposal) ---- */

static _Atomic(const struct vtable *) vt_atomic;

static const struct vtable *vt_init(void)
{
  const struct vtable *p = dispatch_get_vtable();
  atomic_store_explicit(&vt_atomic, p, memory_order_release);
  return p;
}

static inline const struct vtable *vt(void)
{
  const struct vtable *p = atomic_load_explicit(&vt_atomic, memory_order_acquire);
  if (__builtin_expect(p == NULL, 0)) p = vt_init();
  return p;
}

int disp_atomic_cheap(int x) { return vt()->cheap(x); }
int disp_atomic_real(int x) { return vt()->real(x); }

/* ---- shape B: plain global pointer, no atomic, no branch ---- */

static const struct vtable *vt_plain;

int disp_plain_cheap(int x) { return vt_plain->cheap(x); }
int disp_plain_real(int x) { return vt_plain->real(x); }

/* ---- shape C: the vtable copied into our own storage ---- */

/* Saves the pointer chase: the slot's address is PC-relative here, so there is one
 * load instead of two dependent ones.
 */
static struct vtable vt_copy;

int disp_copy_cheap(int x) { return vt_copy.cheap(x); }
int disp_copy_real(int x) { return vt_copy.real(x); }

/* ---- shape D: individual function-pointer variables (MPItrampoline's shape) ---- */

static int (*p_cheap)(int);
static int (*p_real)(int);

int disp_ptrs_cheap(int x) { return p_cheap(x); }
int disp_ptrs_real(int x) { return p_real(x); }

/* ---- shape E: direct call, the floor ---- */

int disp_direct_cheap(int x) { return callee_cheap(x); }
int disp_direct_real(int x) { return callee_real(x); }

/* ---- init ---- */

void dispatch_init(void)
{
  const struct vtable *p = vt_init();
  vt_plain               = p;
  memcpy(&vt_copy, p, sizeof vt_copy);
  p_cheap = p->cheap;
  p_real  = p->real;
}
