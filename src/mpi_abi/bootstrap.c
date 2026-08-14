/* libmpi_abi -- bootstrap: find libmpiwrapper, load it in isolation, and take
 * its vtable.
 *
 * This library includes the ABI mpi.h and nothing else. No implementation type,
 * constant or function name appears anywhere in it, which is why it is built
 * once and works with every wrapper. It contains no conversion logic at all:
 * every entry point (entrypoints.c) is one line.
 *
 * Hand-written and permanent: this file is what S2's generator does *not*
 * produce.
 */

#include <mpi.h> /* the ABI's, i.e. gen/include/mpi.h */

#include "mpiwrapper_vtable.h"

#include <assert.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* libmpi_abi is built with default visibility, because every MPI_ and PMPI_
 * entry point is meant to be exported. This is the one global that is not,
 * and it is
 * marked rather than left to a linker script: test/check_exports.cmake asserts
 * that the library's export set is exactly the entry points.
 */
#if defined(__GNUC__)
#  define MPI_ABI_HIDDEN __attribute__((visibility("hidden")))
#else
#  define MPI_ABI_HIDDEN
#endif

/* A plain pointer, set by the constructor below, read with no atomic and no
 * NULL check. Both omissions are deliberate.
 *
 * Safety: anything that can call MPI_Send must reference it, therefore must
 * link libmpi_abi directly or transitively, therefore *depends* on this library
 * -- and ELF and Mach-O both run constructors in dependency order, so its
 * constructors run after ours. A plugin dlopen'ed later is no exception:
 * loading it loads libmpi_abi first if it is not already present. So the window
 * in which vt is NULL contains no code that can reach an entry point.
 *
 * Cost of the alternative, measured in dev/dispatch-bench/: an atomic acquire
 * load plus a lazy-init branch costs +0.55 ns per trivial call under gcc, and
 * 23 instructions per entry point instead of 4, because the possible cold call
 * to the initializer forces a stack frame. Across 1376 entry points that is
 * 95 KB of text instead of 22 KB. On a call that does real work the time
 * difference is invisible, so this is a code-size decision rather than a
 * latency one. The same measurement says copying the whole vtable into our own
 * storage buys nothing: the extra load is off the dependency chain.
 */
MPI_ABI_HIDDEN const struct mpiwrapper_vtable *mpi_abi_vt;

#define MPI_ABI_WRAPPER_LIB_ENV "MPI_ABI_WRAPPER_LIB"
#define MPI_ABI_WRAPPER_MODE_ENV "MPI_ABI_WRAPPER_DLOPEN_MODE"
#define MPI_ABI_WRAPPER_BIND_NOW_ENV "MPI_ABI_WRAPPER_BIND_NOW"

/* Baked in at build time (CMake passes the installed path); the environment
 * variable overrides it. One libmpi_abi can therefore be pointed at any
 * wrapper, which is what makes the cross test possible.
 */
#ifndef MPI_ABI_WRAPPER_LIB_DEFAULT
#  if defined(__APPLE__)
#    define MPI_ABI_WRAPPER_LIB_DEFAULT "libmpiwrapper.dylib"
#  else
#    define MPI_ABI_WRAPPER_LIB_DEFAULT "libmpiwrapper.so"
#  endif
#endif

/* There is no way to report "the wrapper could not be loaded" through an MPI
 * return code: MPI_Init has not necessarily been called, no error handler
 * exists yet, and a wrong answer is worse than no answer. So this is one of the
 * few places where the library writes to stderr and aborts.
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

/* Loading the wrapper is the one genuinely delicate thing this library does,
 * and the reason is symbol resolution, not the load itself.
 *
 * On ELF a dlopen'ed object resolves its *undefined* references against the
 * global scope FIRST and its own dependency subtree second. That asymmetry is
 * why RTLD_DEEPBIND exists. The application links libmpi_abi, so libmpi_abi is
 * in the global scope, so libmpiwrapper's reference to MPI_Send binds to *our*
 * MPI_Send rather than libmpi's:
 *
 *     libmpi_abi::MPI_Send -> vtable -> w_MPI_Send -> libmpi_abi::MPI_Send ...
 *
 * RTLD_LOCAL does not fix this: LOCAL/GLOBAL controls what the loaded object
 * *exports*, not how its own references resolve. Isolation is mandatory, not an
 * optimization. RTLD_GLOBAL is actively harmful -- it also promotes libmpi's
 * MPI_Send into the global scope, where a later-dlopen'ed plugin binds to it
 * and is handed ABI-typed arguments. dev/dlopen-probe/ measures all of this;
 * NOTES.md #2 has the table.
 *
 * Per platform, as measured there:
 *   macOS    RTLD_LOCAL is enough -- the two-level namespace binds
 *            libmpiwrapper's MPI_Send to libmpi at link time.
 *   Linux    RTLD_LOCAL | RTLD_DEEPBIND by default, dlmopen(LM_ID_NEWLM)
 *            selectable (the sanitizers dislike DEEPBIND).
 *   other    RTLD_LOCAL | RTLD_DEEPBIND; dlmopen is glibc-only.
 *
 * Binding mode defaults to RTLD_LAZY rather than RTLD_NOW: RTLD_NOW forces
 * every undefined symbol in libmpi and its dependency closure to resolve, and
 * real MPI installations have symbols that are never called. Overridable.
 *
 * Whatever the loader then did, the *outcome* is checked on the far side by
 * mpiwrapper_get_vtable (see abi_probe below).
 */
static void *vt_dlopen(const char *path, const char **how)
{
  const int binding = getenv(MPI_ABI_WRAPPER_BIND_NOW_ENV) ? RTLD_NOW : RTLD_LAZY;
  const char *mode = getenv(MPI_ABI_WRAPPER_MODE_ENV);

#if defined(__APPLE__)
  (void)mode;
  *how = "dlopen(RTLD_LOCAL)";
  return dlopen(path, binding | RTLD_LOCAL);
#else
#  if defined(__GLIBC__)
  /* One namespace for every wrapper-side load, and glibc caps namespaces at
   * DL_NNS (16), so the id is remembered and reused rather than requesting
   * LM_ID_NEWLM again.
   */
  if (mode && strcmp(mode, "dlmopen") == 0) {
    static Lmid_t lmid;
    static int    have_lmid;
    *how  = "dlmopen(LM_ID_NEWLM)";
    void *h = dlmopen(have_lmid ? lmid : LM_ID_NEWLM, path, binding);
    if (h && !have_lmid && dlinfo(h, RTLD_DI_LMID, &lmid) == 0) have_lmid = 1;
    return h;
  }
#  endif
  /* MPI_ABI_WRAPPER_DLOPEN_MODE=capture exists for the tests only: it is the
   * unisolated load, which dev/dlopen-probe shows recurses until the stack is
   * exhausted. test/isolation_test.c uses it to prove that the outward-
   * resolution check below actually fires rather than being decorative.
   */
  if (mode && strcmp(mode, "capture") == 0) {
    *how = "dlopen(RTLD_LOCAL) -- no isolation, for testing only";
    return dlopen(path, binding | RTLD_LOCAL);
  }
  *how = "dlopen(RTLD_LOCAL | RTLD_DEEPBIND)";
  return dlopen(path, binding | RTLD_LOCAL | RTLD_DEEPBIND);
#endif
}

static const struct mpiwrapper_vtable *vt_load(void)
{
  const char *path = getenv(MPI_ABI_WRAPPER_LIB_ENV);
  if (!path || !*path) path = MPI_ABI_WRAPPER_LIB_DEFAULT;

  const char *how    = "dlopen";
  void       *handle = vt_dlopen(path, &how);
  if (!handle) vt_fail("dlopen failed", dlerror());

  const struct mpiwrapper_vtable *(*get)(uint32_t, uint32_t, uint32_t, size_t,
                                         const void *, const char **) =
      (const struct mpiwrapper_vtable *(*)(uint32_t, uint32_t, uint32_t, size_t,
                                           const void *, const char **))
          dlsym(handle, "mpiwrapper_get_vtable");
  if (!get) vt_fail("not an mpiwrapper library", path);

  /* The address of one of our own functions. The wrapper dladdr()s this and the
   * MPI_Send it actually resolved, and refuses if they share a base object -- a
   * positive check that the isolation above worked, whatever the loader did. It
   * catches the recursion at load rather than as a stack overflow on the first
   * message, and it does so on every platform, which dlinfo(RTLD_DI_LMID)
   * cannot: that confirms the mechanism, this confirms the outcome.
   */
  const void *abi_probe = (const void *)(uintptr_t)&MPI_Send;

  const char *diagnostic = "no diagnostic";
  const struct mpiwrapper_vtable *vt =
      get(MPI_ABI_VERSION, MPI_ABI_SUBVERSION, MPIWRAPPER_LAYOUT_HASH,
          sizeof(struct mpiwrapper_vtable), abi_probe, &diagnostic);
  if (!vt) {
    fprintf(stderr, "libmpi_abi: loaded %s with %s\n", path, how);
    vt_fail("wrapper rejected this libmpi_abi", diagnostic);
  }

  return vt;
}

/* Runs before main, and before the constructors of anything that depends on
 * this library -- which is everything that can call an MPI function. See the
 * comment on mpi_abi_vt above for why that is sufficient and why there is no
 * lazy fallback.
 */
__attribute__((constructor)) static void mpi_abi_ctor(void)
{
  mpi_abi_vt = vt_load();
}
