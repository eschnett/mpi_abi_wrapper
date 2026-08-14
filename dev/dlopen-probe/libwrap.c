/* Stands in for libmpiwrapper.
 *
 * Linked directly against libimpl, calls MPI_Send by name, and exports exactly one
 * symbol for libabi to dlsym. Its call to MPI_Send must reach libimpl; if it
 * reaches libabi instead, the real system recurses until the stack is gone.
 */

/* dladdr and Dl_info need _GNU_SOURCE on glibc. */
#define _GNU_SOURCE

#include "probe.h"

#include <dlfcn.h>
#include <stdio.h>

extern int         MPI_Send(const char *msg);
extern int         impl_internal(void);
extern const void *impl_resolved_mpi_send(void);

static int wrap_send(const char *msg)
{
  fprintf(stderr, "      [wrap::wrap_send] -> MPI_Send\n");
  return MPI_Send(msg);
}

static int wrap_impl_internal(void)
{
  fprintf(stderr, "    [wrap::wrap_impl_internal] -> impl_internal\n");
  return impl_internal();
}

static const void *wrap_resolved_mpi_send(void) { return (const void *)&MPI_Send; }

static const struct probe_vtable vt = {
    .send                 = wrap_send,
    .impl_internal        = wrap_impl_internal,
    .wrap_resolved        = wrap_resolved_mpi_send,
    .impl_resolved        = impl_resolved_mpi_send,
};

const struct probe_vtable *wrap_get_vtable(const void *abi_probe,
                                           const char **diagnostic)
{
  /* The check proposed for the real getter: does the MPI_Send this library resolved
   * live in the same object as the caller? If so the loader captured us.
   *
   * Also reports whether dladdr can see across the boundary at all, which matters
   * for dlmopen: a separate namespace may make dladdr on a foreign pointer fail,
   * in which case the check silently degrades to "cannot tell".
   */
  Dl_info mine, theirs;
  const int got_mine   = dladdr((const void *)&MPI_Send, &mine) != 0;
  const int got_theirs = abi_probe ? dladdr(abi_probe, &theirs) != 0 : 0;

  fprintf(stderr, "    [wrap] dladdr(own MPI_Send)   = %s\n",
          got_mine ? mine.dli_fname : "(failed)");
  fprintf(stderr, "    [wrap] dladdr(abi probe)      = %s\n",
          got_theirs ? theirs.dli_fname : "(failed)");

  if (got_mine && got_theirs) {
    if (mine.dli_fbase == theirs.dli_fbase) {
      *diagnostic = "CAPTURED: wrapper's MPI_Send resolves into libabi";
      fprintf(stderr, "    [wrap] isolation check: FAIL (same base object)\n");
      return &vt; /* returned anyway, so the probe can continue and show the trace */
    }
    fprintf(stderr, "    [wrap] isolation check: pass\n");
  } else {
    *diagnostic = "isolation check inconclusive (dladdr could not see both)";
    fprintf(stderr, "    [wrap] isolation check: INCONCLUSIVE\n");
  }
  return &vt;
}
