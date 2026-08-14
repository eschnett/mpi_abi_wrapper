/* dladdr and Dl_info need _GNU_SOURCE on glibc. */
#define _GNU_SOURCE

/* A host that is NOT linked against libabi and reaches MPI only through plugins.
 *
 * T3  load plugin1 (which links libabi), let it initialize MPI, then load plugin2
 *     (which also links libabi) and see which MPI_Send plugin2 binds to. Under
 *     RTLD_GLOBAL the wrapper's load has put libimpl's MPI_Send into the global
 *     scope, which is searched before plugin2's own dependencies -- so plugin2 can
 *     bind straight to the native MPI and be handed ABI-typed arguments.
 */

#include "probe.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#  define SO(x) "./lib" x ".dylib"
#else
#  define SO(x) "./lib" x ".so"
#endif

static const char *verdict(int rc, int want)
{
  if (rc == want) return "OK";
  switch (rc) {
  case IMPL_REACHED:  return "BYPASSED";
  case ABI_CAPTURED:  return "CAPTURED";
  case ABI_FORWARDED: return "FORWARDED";
  default:            return "ERROR";
  }
}

int main(int argc, char **argv)
{
  const char *mode = argc > 1 ? argv[1] : "local";

  fprintf(stderr, "== hostapp (NOT linked against libabi), mode=%s ==\n", mode);

  void *p1 = dlopen(SO("plugin1"), RTLD_NOW | RTLD_LOCAL);
  if (!p1) {
    fprintf(stderr, "  dlopen plugin1: %s\n", dlerror());
    printf("RESULT %s T3 SKIP\n", mode);
    return 0;
  }

  int (*p1_init)(const char *) = (int (*)(const char *))dlsym(p1, "plugin_init");
  void (*p1_report)(const char *) =
      (void (*)(const char *))dlsym(p1, "plugin_report");
  if (!p1_init || p1_init(mode) != 0) {
    printf("RESULT %s T3 SKIP\n", mode);
    return 0;
  }
  if (p1_report) p1_report("plugin1");

  /* What is in the *global* scope now, after the wrapper has been loaded? This is
   * what a subsequently dlopen'ed plugin searches first, so it decides T3. In
   * particular: does RTLD_GLOBAL promote the dlopen'ed object's *dependencies*
   * (libimpl) into the global scope, or only the object named in the call
   * (libwrap, which defines no MPI_Send)?
   */
  {
    void  *g = dlsym(RTLD_DEFAULT, "MPI_Send");
    Dl_info info;
    if (g && dladdr(g, &info)) {
      const char *base = strrchr(info.dli_fname, '/');
      fprintf(stderr, "  GLOBAL-SCOPE MPI_Send -> %s\n",
              base ? base + 1 : info.dli_fname);
    } else {
      fprintf(stderr, "  GLOBAL-SCOPE MPI_Send -> (not in global scope)\n");
    }
  }

  /* The second extension module, loaded after MPI is already up. */
  void *p2 = dlopen(SO("plugin2"), RTLD_NOW | RTLD_LOCAL);
  if (!p2) {
    fprintf(stderr, "  dlopen plugin2: %s\n", dlerror());
    printf("RESULT %s T3 SKIP\n", mode);
    return 0;
  }
  void (*p2_report)(const char *) =
      (void (*)(const char *))dlsym(p2, "plugin_report");
  int (*p2_run)(void) = (int (*)(void))dlsym(p2, "plugin_run");
  if (p2_report) p2_report("plugin2");

  fprintf(stderr, "  -- T3: second plugin calls MPI_Send --\n");
  const int t3 = p2_run ? p2_run() : -1;

  /* The plugin must reach libimpl *through* libabi. Reaching it directly means
   * the loader let the plugin bind past us to the native MPI. */
  printf("RESULT %s T3 %s\n", mode, verdict(t3, ABI_FORWARDED));
  return 0;
}
