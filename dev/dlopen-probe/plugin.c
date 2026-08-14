/* A plugin that links libabi and is itself dlopen'ed by a host that knows nothing
 * about MPI. This is the mpi4py shape: python is the executable, MPI.so is the
 * plugin, and libabi is reached only through the plugin.
 *
 * Built twice, as libplugin1 and libplugin2, to model a process that loads a second
 * MPI-using extension module after the first has already initialized MPI.
 */

/* dladdr and Dl_info need _GNU_SOURCE on glibc. */
#define _GNU_SOURCE

#include "probe.h"

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

extern int MPI_Send(const char *msg);

int plugin_init(const char *mode)
{
  const char *err = abi_init(mode);
  if (err) {
    fprintf(stderr, "  [plugin] abi_init failed: %s\n", err);
    return -1;
  }
  return 0;
}

int plugin_run(void)
{
  fprintf(stderr, "  [plugin] -> MPI_Send\n");
  return MPI_Send("from plugin");
}

void plugin_report(const char *label)
{
  Dl_info info;
  if (dladdr((const void *)&MPI_Send, &info)) {
    const char *base = strrchr(info.dli_fname, '/');
    fprintf(stderr, "  RESOLVED %s sees MPI_Send in %s\n", label,
            base ? base + 1 : info.dli_fname);
  } else {
    fprintf(stderr, "  RESOLVED %s sees MPI_Send in (dladdr failed)\n", label);
  }
}
