/* libmpi_abi -- the ABI side.
 *
 * Includes the ABI mpi.h and nothing else. No implementation type, constant or
 * function name appears anywhere in this library, which is why it is built once
 * and works with every wrapper. It contains no conversion logic at all: every
 * entry point is one line.
 *
 * Two parts:
 *   - the bootstrap, hand-written, in src/mpi_abi/
 *   - 1376 forwarders, generated, in gen/mpi_abi/entrypoints.c, one per slot
 *
 * MPI_Send and PMPI_Send are shown; the other 686 pairs are identical in shape.
 */

/* The ABI header: the mpi-abi-stubs header plus doc/mpi.h.patch, names untouched.
 * This is the same file the application compiles against.
 */
#include <mpi.h>

#include "mpiwrapper_vtable.h"

#include <dlfcn.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ bootstrap */

/* A plain pointer, set by the constructor below, read with no atomic and no NULL
 * check. Both omissions are deliberate.
 *
 * Safety: anything that can call MPI_Send must reference it, therefore must link
 * libmpi_abi directly or transitively, therefore *depends* on this library -- and
 * ELF and Mach-O both run constructors in dependency order, so its constructors run
 * after ours. A plugin dlopen'ed later is no exception: loading it loads
 * libmpi_abi first if it is not already present. So the window in which vt is NULL
 * contains no code that can reach these entry points.
 *
 * Cost: dev/dispatch-bench measures the alternative -- an atomic acquire load plus
 * a lazy-init branch -- at +0.55 ns per trivial call under gcc, and at 23
 * instructions per entry point instead of 4, because the possible cold call to the
 * initializer forces a stack frame. Across 1376 entry points that is 95 KB of text
 * instead of 22 KB. On a call that does real work the time difference is invisible,
 * so this is a code-size decision rather than a latency one.
 *
 * The same measurement says copying the whole vtable into our own storage to save
 * the pointer chase buys nothing: the extra load is off the dependency chain, so an
 * out-of-order core issues it in parallel. One pointer it is.
 */
static const struct mpiwrapper_vtable *vt;

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

/* Loading the wrapper is the one genuinely delicate thing this library does, and
 * the reason is symbol resolution, not the load itself.
 *
 * On ELF a dlopen'ed object resolves its *undefined* references against the global
 * scope FIRST and its own dependency subtree second. That asymmetry is why
 * RTLD_DEEPBIND exists. The application links libmpi_abi, so libmpi_abi is in the
 * global scope, so libmpiwrapper's reference to MPI_Send binds to *our* MPI_Send
 * rather than libmpi's:
 *
 *     libmpi_abi::MPI_Send -> vtable -> w_MPI_Send -> libmpi_abi::MPI_Send -> ...
 *
 * Note that RTLD_LOCAL does not fix this: LOCAL/GLOBAL controls what the loaded
 * object *exports*, not how its own references resolve. Isolation is mandatory,
 * not an optimization.
 *
 * RTLD_GLOBAL would be actively harmful, for three separate reasons:
 *
 *  - it puts libmpi's MPI_Send into the global scope, so a plugin dlopen'ed
 *    *later* binds to the native MPI (global is searched before the plugin's own
 *    local scope, where libmpi_abi lives) and is handed ABI-typed arguments;
 *  - the implementation's own internals are written against MPI in places (Open
 *    MPI's ROMIO and io components), and capturing those is not merely wrong but
 *    memory-unsafe: a component calling MPI_Recv passes a 24-byte
 *    ompi_status_public_t and our ABI MPI_Recv writes 32 bytes into it;
 *  - handles would survive such a capture *by accident* -- dynamic ones bit-cast
 *    to themselves, and predefined implementation values sit outside the ABI's
 *    0x20..0x2eb range -- which makes the failure intermittent rather than
 *    immediate.
 *
 * Calling PMPI_* internally does not save the implementation either: we export
 * those too, so both names are captured.
 *
 * Hence, per platform:
 *   macOS    RTLD_LOCAL is enough -- the two-level namespace binds libmpiwrapper's
 *            MPI_Send to libmpi at link time, so there is nothing to capture.
 *   Linux    dlmopen into a fresh namespace (correct by construction: no shared
 *            global scope at all), or dlopen with RTLD_LOCAL | RTLD_DEEPBIND.
 *   FreeBSD  RTLD_LOCAL | RTLD_DEEPBIND; dlmopen does not exist.
 *
 * Binding mode defaults to RTLD_LAZY rather than RTLD_NOW: RTLD_NOW forces every
 * undefined symbol in libmpi and its dependency closure to resolve, and real MPI
 * installations have symbols that are never called. Overridable.
 */
static void *vt_dlopen(const char *path)
{
  const int binding = getenv("MPI_ABI_WRAPPER_BIND_NOW") ? RTLD_NOW : RTLD_LAZY;

#if defined(__APPLE__)
  return dlopen(path, binding | RTLD_LOCAL);
#elif defined(__linux__)
  /* One namespace for every wrapper-side load, and glibc caps namespaces at
   * DL_NNS (16), so the id is remembered and reused rather than requesting
   * LM_ID_NEWLM again.
   */
  static Lmid_t lmid;
  static int    have_lmid;
  const char   *mode = getenv("MPI_ABI_WRAPPER_DLOPEN_MODE");
  if (!mode || strcmp(mode, "dlmopen") == 0) {
    void *h = dlmopen(have_lmid ? lmid : LM_ID_NEWLM, path, binding);
    if (h && !have_lmid && dlinfo(h, RTLD_DI_LMID, &lmid) == 0) have_lmid = 1;
    return h;
  }
  return dlopen(path, binding | RTLD_LOCAL | RTLD_DEEPBIND);
#else
  return dlopen(path, binding | RTLD_LOCAL | RTLD_DEEPBIND);
#endif
}

static const struct mpiwrapper_vtable *vt_load(void)
{
  const char *path = getenv(MPI_ABI_WRAPPER_LIB_ENV);
  if (!path || !*path) path = MPI_ABI_WRAPPER_LIB_DEFAULT;

  void *handle = vt_dlopen(path);
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
   * message, and it does so on every platform, which dlinfo(RTLD_DI_LMID) cannot:
   * that confirms the mechanism, this confirms the outcome.
   */
  const void *abi_probe = (const void *)(uintptr_t)&MPI_Send;

  const char *diagnostic = "no diagnostic";
  const struct mpiwrapper_vtable *vt =
      get(MPI_ABI_VERSION, MPI_ABI_SUBVERSION, MPIWRAPPER_LAYOUT_HASH,
          sizeof(struct mpiwrapper_vtable), abi_probe, &diagnostic);
  if (!vt) vt_fail("wrapper rejected this libmpi_abi", diagnostic);

  return vt;
}

/* Runs before main, and before the constructors of anything that depends on this
 * library -- which is everything that can call an MPI function. See the comment on
 * `vt` above for why that is sufficient and why there is no lazy fallback.
 */
__attribute__((constructor)) static void mpi_abi_ctor(void) { vt = vt_load(); }

/* Cheap insurance in development builds only: in a release build this compiles to
 * nothing and a stuck NULL would be a segfault at a small address, which is what
 * the constructor ordering argument says cannot happen.
 */
#ifndef NDEBUG
#  define VT() (assert(vt != NULL), vt)
#else
#  define VT() vt
#endif

/* --------------------------------------------------------------- entry points */

/* Generated. Note what is *not* here: no conversion, no temporary, no knowledge
 * of any implementation type, and no initialization check. The arguments pass
 * through untouched, and need no cast because the ABI header's MPI_Comm and
 * mpiabi.h's MPIABI_Comm are the same type (see mpiwrapper_vtable.h).
 *
 * MPI_* and PMPI_* are two definitions rather than an alias: macOS aliases need
 * -Wl,-alias or __asm__ labels, and at one line per body an alias saves nothing.
 * They reach *different* slots -- see mpiwrapper_vtable.h for why.
 */

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
             MPI_Comm comm)
{
  return VT()->MPI_Send(buf, count, datatype, dest, tag, comm);
}

/* Its own slot, not MPI_Send's, so that bypassing the profiling layer bypasses it
 * at the implementation level too and not merely at the ABI level.
 */
int PMPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest,
              int tag, MPI_Comm comm)
{
  return VT()->PMPI_Send(buf, count, datatype, dest, tag, comm);
}

/* Returns double, so there is no error code to map -- one of the handful of
 * entry points whose generated shape differs at all.
 */
double MPI_Wtime(void) { return VT()->MPI_Wtime(); }
double PMPI_Wtime(void) { return VT()->PMPI_Wtime(); }
