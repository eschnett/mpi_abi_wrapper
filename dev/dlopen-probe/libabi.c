/* Stands in for libmpi_abi.
 *
 * Exports MPI_Send, loads libwrap by the mode named on the command line, and
 * forwards through the vtable. If the loader captures the wrapper's MPI_Send, this
 * function is re-entered, which the depth guard turns into a clean verdict instead
 * of a stack overflow.
 */

#define _GNU_SOURCE
#include "probe.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const struct probe_vtable *vt;
static _Thread_local int          depth;

int MPI_Send(const char *msg)
{
  fprintf(stderr, "    [abi::MPI_Send] (%s)%s\n", msg,
          depth ? "  <-- RE-ENTERED" : "");
  if (++depth > 1) {
    --depth;
    return ABI_CAPTURED;
  }
  const int r = vt ? vt->send(msg) : -1;
  --depth;
  /* Distinguish "libimpl answered because we forwarded" from "libimpl answered
   * because the caller reached it directly, bypassing us".
   */
  return r == IMPL_REACHED ? ABI_FORWARDED : r;
}

int abi_test_impl_internal(void)
{
  return vt ? vt->impl_internal() : -1;
}

/* PROBE_WRAP_LIB lets the macOS runs point at a flat-namespace build of libwrap,
 * which is how the two-level-namespace assumption gets tested rather than asserted.
 */
static const char *libwrap_path(void)
{
  const char *env = getenv("PROBE_WRAP_LIB");
  if (env && *env) return env;
#ifdef __APPLE__
  return "./libwrap.dylib";
#else
  return "./libwrap.so";
#endif
}

const char *abi_init(const char *mode)
{
  const char *path   = libwrap_path();
  void       *handle = NULL;

  if (strcmp(mode, "local") == 0) {
    handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  } else if (strcmp(mode, "global") == 0) {
    handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
#ifdef RTLD_DEEPBIND
  } else if (strcmp(mode, "deepbind") == 0) {
    handle = dlopen(path, RTLD_NOW | RTLD_LOCAL | RTLD_DEEPBIND);
#endif
#ifdef __linux__
  } else if (strcmp(mode, "dlmopen") == 0) {
    handle = dlmopen(LM_ID_NEWLM, path, RTLD_NOW);
#endif
  } else {
    return "unsupported mode on this platform";
  }

  if (!handle) return dlerror();

  const struct probe_vtable *(*get)(const void *, const char **) =
      (const struct probe_vtable *(*)(const void *, const char **))
      dlsym(handle, "wrap_get_vtable");
  if (!get) return "wrap_get_vtable not found";

  /* The address of one of our own functions, for the wrapper's isolation check. */
  const void *abi_probe   = (const void *)&MPI_Send;
  const char *diagnostic  = NULL;
  vt                      = get(abi_probe, &diagnostic);
  if (diagnostic) fprintf(stderr, "    [abi] wrapper says: %s\n", diagnostic);
  return NULL;
}

/* Which object does each library's MPI_Send actually point at? Answers the
 * question directly, without relying on a call chain.
 */
void abi_report_resolution(void)
{
  const void *addrs[3];
  const char *names[3] = {"libabi ", "libwrap", "libimpl"};
  addrs[0]             = (const void *)&MPI_Send;
  addrs[1]             = vt ? vt->wrap_resolved() : NULL;
  addrs[2]             = vt ? vt->impl_resolved() : NULL;

  for (int i = 0; i < 3; ++i) {
    Dl_info info;
    if (addrs[i] && dladdr(addrs[i], &info)) {
      const char *base = strrchr(info.dli_fname, '/');
      fprintf(stderr, "  RESOLVED %s sees MPI_Send in %s\n", names[i],
              base ? base + 1 : info.dli_fname);
    } else {
      fprintf(stderr, "  RESOLVED %s sees MPI_Send in (dladdr failed)\n", names[i]);
    }
  }
}
