/* component_bind_test -- the implementation's components reach the
 * implementation; the application's plugins reach libmpi_abi (Linux).
 *
 * NOTES.md #2, "Components the implementation loads later". RTLD_DEEPBIND
 * keeps the wrapper's and the implementation's own MPI_* references inside
 * the wrapper's group, but an object the implementation dlopen's later --
 * Open MPI's ROMIO component, measured on HPC-X -- searches the global scope
 * first, where only libmpi_abi defines MPI_*. libmpiwrapper's dlopen
 * (src/mpiwrapper/component_bind.c) rebinds what the implementation loads.
 *
 * The implementation's own dlopen is the wrapper's dlopen, so this test calls
 * that one directly, by dlsym on the wrapper's handle, and calls the
 * application's own for comparison. What a call reached is visible in its
 * answer: libmpi_abi's MPI_Get_library_version begins with the wrapper's
 * banner (NOTES.md #7, decision 26), the implementation's does not.
 *
 *   argv[1]  a component, loaded by the application: must reach libmpi_abi
 *   argv[2]  a second copy, by absolute path through the wrapper, RTLD_LAZY
 *   argv[3]  a third copy's file name, loaded as "$ORIGIN/<name>", RTLD_NOW
 *   argv[4]  a fourth copy's file name, loaded bare, found by this program's
 *            DT_RUNPATH -- which only works if the wrapper searched on this
 *            program's behalf rather than its own
 *   argv[5]  a fifth copy, loaded by the second with its *own* dlopen: a
 *            component's plugin, which must reach the implementation too
 *   argv[6]  a sixth copy, loaded by the first with its own dlopen: an
 *            application plugin's plugin, which must still reach libmpi_abi
 */

#define _GNU_SOURCE
#include <mpi.h>

#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

#define CHECK(cond, ...)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      ++failures;                                                              \
      printf("FAIL %s:%d: ", __FILE__, __LINE__);                              \
      printf(__VA_ARGS__);                                                     \
      printf("\n");                                                            \
      fflush(stdout);                                                          \
    }                                                                          \
  } while (0)

static const char banner[] = "mpi_abi_wrapper ";

typedef int (*version_fn)(char *, int *);
typedef void *(*address_fn)(void);
typedef void *(*dlopen_fn)(const char *, int);
typedef void *(*load_fn)(const char *);

/* Which library a function address lies in. */
static const char *library_of(const void *address)
{
  Dl_info info;
  return dladdr(address, &info) && info.dli_fname ? info.dli_fname : "?";
}

/* Which side a loaded component's two references reached: 1 for libmpi_abi,
 * 0 for the implementation, -1 if it did not load.
 */
static int side_of(void *handle, const char *what, const char *file)
{
  CHECK(handle, "%s: %s did not load: %s", what, file, dlerror());
  if (!handle) return -1;

  version_fn version = (version_fn)(uintptr_t)dlsym(handle,
                                                    "component_library_version");
  address_fn address = (address_fn)(uintptr_t)dlsym(
      handle, "component_library_version_address");
  CHECK(version && address, "%s: component symbols missing", what);
  if (!version || !address) return -1;

  static char text[8192];
  int len = 0;
  CHECK(version(text, &len) == 0, "%s: MPI_Get_library_version failed", what);
  const int call_abi = strncmp(text, banner, sizeof banner - 1) == 0;

  const char *abi_lib  = library_of((const void *)(uintptr_t)&MPI_Get_library_version);
  const char *got_lib  = library_of(address());
  const int address_abi = strcmp(abi_lib, got_lib) == 0;

  CHECK(call_abi == address_abi,
        "%s: the call reached %s but the address is in %s", what,
        call_abi ? "libmpi_abi" : "the implementation", got_lib);
  printf("%-34s call -> %-18s address -> %s\n", what,
         call_abi ? "libmpi_abi" : "the implementation", got_lib);
  return call_abi;
}

/* Load one component with `open` and report which side it reached. */
static int reached_abi(dlopen_fn open, const char *what, const char *file,
                       int mode, void **handle)
{
  *handle = open(file, mode);
  return side_of(*handle, what, file);
}

/* Have a loaded component load another with its own dlopen, and report which
 * side that one reached.
 */
static int nested_reached_abi(void *parent, const char *what, const char *file)
{
  load_fn load = parent ? (load_fn)(uintptr_t)dlsym(parent, "component_load")
                        : NULL;
  CHECK(load, "%s: component_load missing", what);
  if (!load) return -1;
  return side_of(load(file), what, file);
}

int main(int argc, char **argv)
{
  if (argc != 7) {
    fprintf(stderr,
            "usage: %s component1 component2 name3 name4 component5 component6\n",
            argv[0]);
    return 2;
  }
  MPI_Init(&argc, &argv);

  /* The application's own call, for the record: libmpi_abi answers it. */
  static char text[8192];
  int len = 0;
  MPI_Get_library_version(text, &len);
  CHECK(strncmp(text, banner, sizeof banner - 1) == 0,
        "the application's MPI_Get_library_version lacks the banner");

  /* The application's plugin binds to libmpi_abi, and must go on doing so. */
  void *app_plugin = NULL;
  CHECK(reached_abi(dlopen, "application dlopen", argv[1], RTLD_LAZY,
                    &app_plugin) == 1,
        "the application's plugin did not reach libmpi_abi");
  CHECK(nested_reached_abi(app_plugin, "application plugin's own dlopen",
                           argv[6]) == 1,
        "the application plugin's plugin did not reach libmpi_abi");

  /* The wrapper's dlopen, which is the one the implementation calls. */
  const char *wrapper = getenv("MPI_ABI_WRAPPER_LIB");
  CHECK(wrapper, "MPI_ABI_WRAPPER_LIB is not set");
  void *self = wrapper ? dlopen(wrapper, RTLD_LAZY | RTLD_NOLOAD) : NULL;
  CHECK(self, "the wrapper is not loaded: %s", wrapper ? wrapper : "?");
  dlopen_fn hook = self ? (dlopen_fn)(uintptr_t)dlsym(self, "dlopen") : NULL;
  CHECK(hook && hook != dlopen, "libmpiwrapper does not define dlopen");

  if (hook && hook != dlopen) {
    char origin[4096];
    snprintf(origin, sizeof origin, "$ORIGIN/%s", argv[3]);
    void *component = NULL, *other = NULL;
    CHECK(reached_abi(hook, "wrapper dlopen, path, lazy", argv[2], RTLD_LAZY,
                      &component) == 0,
          "a component loaded by path was not rebound");
    CHECK(reached_abi(hook, "wrapper dlopen, $ORIGIN, now", origin, RTLD_NOW,
                      &other) == 0,
          "a component loaded by $ORIGIN was not rebound");
    CHECK(reached_abi(hook, "wrapper dlopen, bare name, lazy", argv[4],
                      RTLD_LAZY, &other) == 0,
          "a component loaded by bare name was not rebound");
    CHECK(nested_reached_abi(component, "component's own dlopen", argv[5]) == 0,
          "a plugin a component loaded with its own dlopen was not rebound");
  }

  MPI_Finalize();
  printf("%s\n", failures ? "FAILED" : "PASSED");
  return failures ? 1 : 0;
}
