/* libmpi_abi -- the ABI side.
 *
 * Includes the ABI mpi.h and nothing else. No implementation type, constant or
 * function name appears anywhere in this library, which is why it is built once
 * and works with every wrapper. It contains no conversion logic at all: every
 * entry point is one line.
 *
 * Two parts:
 *   - the bootstrap, hand-written, in src/mpi_abi/
 *   - 1376 forwarders, generated, in gen/mpi_abi/entrypoints.c
 *
 * MPI_Send and PMPI_Send are shown; the other 686 pairs are identical in shape.
 */

/* The ABI header: the mpi-abi-stubs header plus doc/mpi.h.patch, names untouched.
 * This is the same file the application compiles against.
 */
#include <mpi.h>

#include "mpiwrapper_vtable.h"

#include <dlfcn.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* ------------------------------------------------------------------ bootstrap */

static _Atomic(const struct mpiwrapper_vtable *) vt_cache;

/* 0 = untried, 1 = in progress, 2 = ready, 3 = failed */
static atomic_int vt_state;

#define MPI_ABI_WRAPPER_LIB_ENV "MPI_ABI_WRAPPER_LIB"

/* Baked in at build time; the environment variable overrides it. */
#ifndef MPI_ABI_WRAPPER_LIB_DEFAULT
#  define MPI_ABI_WRAPPER_LIB_DEFAULT "libmpiwrapper.so"
#endif

/* There is no way to report "the wrapper could not be loaded" through an MPI
 * return code: MPI_Init has not necessarily been called, no error handler exists
 * yet, and a wrong answer is worse than no answer. So this is one of the few
 * places where the library writes to stderr and aborts.
 */
static void vt_fail(const char *what, const char *detail)
{
  fprintf(stderr,
          "libmpi_abi: cannot initialize: %s%s%s\n"
          "libmpi_abi: set %s to the libmpiwrapper built for your MPI\n",
          what, detail ? ": " : "", detail ? detail : "",
          MPI_ABI_WRAPPER_LIB_ENV);
  abort();
}

static const struct mpiwrapper_vtable *vt_load(void)
{
  const char *path = getenv(MPI_ABI_WRAPPER_LIB_ENV);
  if (!path || !*path) path = MPI_ABI_WRAPPER_LIB_DEFAULT;

  /* RTLD_NOW: we are about to trust every slot, so a missing symbol should be
   *   reported here rather than at the first call that needs it.
   * RTLD_GLOBAL: Open MPI's components dlopen themselves and resolve libmpi's
   *   symbols out of the global namespace; RTLD_LOCAL breaks them. This is safe
   *   because the application's own MPI_* references are already bound to this
   *   library, which precedes dlopen'ed objects in the global search order.
   */
  void *handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
  if (!handle) vt_fail("dlopen failed", dlerror());

  const struct mpiwrapper_vtable *(*get)(uint32_t, uint32_t, size_t,
                                         const char **) =
      (const struct mpiwrapper_vtable *(*)(uint32_t, uint32_t, size_t,
                                           const char **))
      dlsym(handle, "mpiwrapper_get_vtable");
  if (!get) vt_fail("not an mpiwrapper library", path);

  const char *diagnostic = "no diagnostic";
  const struct mpiwrapper_vtable *vt =
      get(MPIABI_VERSION, MPIWRAPPER_LAYOUT_HASH,
          sizeof(struct mpiwrapper_vtable), &diagnostic);
  if (!vt) vt_fail("wrapper rejected this libmpi_abi", diagnostic);

  return vt;
}

static const struct mpiwrapper_vtable *vt_init(void)
{
  int expected = 0;
  if (atomic_compare_exchange_strong_explicit(&vt_state, &expected, 1,
                                              memory_order_acq_rel,
                                              memory_order_acquire)) {
    const struct mpiwrapper_vtable *vt = vt_load(); /* aborts on failure */
    atomic_store_explicit(&vt_cache, vt, memory_order_release);
    atomic_store_explicit(&vt_state, 2, memory_order_release);
    return vt;
  }

  /* Another thread got there first. In practice this never spins: the
   * constructor below runs before the application creates any thread. It exists
   * for the case where the first MPI call comes from a library that was itself
   * dlopen'ed, so that our constructor may not have run yet.
   */
  while (atomic_load_explicit(&vt_state, memory_order_acquire) == 1)
    ; /* spin */
  return atomic_load_explicit(&vt_cache, memory_order_acquire);
}

/* One predictable branch per MPI call. */
static inline const struct mpiwrapper_vtable *vt(void)
{
  const struct mpiwrapper_vtable *p =
      atomic_load_explicit(&vt_cache, memory_order_acquire);
  if (__builtin_expect(p == NULL, 0)) p = vt_init();
  return p;
}

/* Runs before main, so the branch above is never taken in the common case. It is
 * not sufficient on its own -- see vt_init -- which is why both exist.
 */
__attribute__((constructor)) static void mpi_abi_ctor(void) { (void)vt(); }

/* --------------------------------------------------------------- entry points */

/* Generated. Note what is *not* here: no conversion, no temporary, no knowledge
 * of any implementation type. The arguments are passed through untouched, and
 * they need no cast because the ABI header's MPI_Comm and mpiabi.h's MPIABI_Comm
 * are the same type (see the comment in mpiwrapper_vtable.h).
 *
 * MPI_* and PMPI_* are two definitions rather than an alias: macOS aliases need
 * -Wl,-alias or __asm__ labels, and at one line per body an alias saves nothing.
 * Both reach the same slot, so a tool that interposes MPI_Send and calls
 * PMPI_Send behaves correctly.
 */

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
             MPI_Comm comm)
{
  return vt()->MPI_Send(buf, count, datatype, dest, tag, comm);
}

int PMPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest,
              int tag, MPI_Comm comm)
{
  return vt()->MPI_Send(buf, count, datatype, dest, tag, comm);
}

/* Returns double, so there is no error code to map -- one of the handful of
 * entry points whose generated shape differs at all.
 */
double MPI_Wtime(void) { return vt()->MPI_Wtime(); }
double PMPI_Wtime(void) { return vt()->MPI_Wtime(); }
