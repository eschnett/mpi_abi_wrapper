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

  const struct mpiwrapper_vtable *(*get)(uint32_t, uint32_t, size_t,
                                         const void *, const char **) =
      (const struct mpiwrapper_vtable *(*)(uint32_t, uint32_t, size_t,
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
      get(MPIABI_VERSION, MPIWRAPPER_LAYOUT_HASH,
          sizeof(struct mpiwrapper_vtable), abi_probe, &diagnostic);
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
