/* glibc hides dladdr, dlinfo, dlmopen, RTLD_DEEPBIND and Dl_info behind
 * _GNU_SOURCE, and it has to be defined before any header is included -- which
 * is why this sits above the file's own comment block rather than beside the
 * <dlfcn.h> that needs it. macOS declares all of them unconditionally, so this
 * was invisible until the first Linux build.
 */
#if defined(__linux__) && !defined(_GNU_SOURCE)
#  define _GNU_SOURCE
#endif

/* libmpiwrapper -- the one exported symbol.
 *
 * Hand-written and permanent. A getter rather than an exported struct, for two
 * reasons: reading a version field out of a struct means trusting the layout
 * you are trying to validate, and this is the natural place to build the
 * reverse handle map and check our own symbol resolution before anyone can call
 * a slot (NOTES.md #2, decision 4).
 */

#include "internal.h"

#include <dlfcn.h>
#include <stdatomic.h>

extern const struct mpiwrapper_vtable mpiwrapper_vtable_instance;

/* Did the loader bind our MPI_* calls outward to the implementation, or back
 * into libmpi_abi? On ELF the second is the default outcome, because a
 * dlopen'ed object searches the global scope -- where libmpi_abi lives --
 * before its own dependencies. dev/dlopen-probe measures this: plain RTLD_LOCAL
 * captures both the wrapper's calls and the implementation's own internal ones,
 * and RTLD_DEEPBIND or dlmopen fixes both; macOS is safe because of the
 * two-level namespace, and stops being safe under -flat_namespace.
 *
 * Checking the outcome rather than the mechanism matters:
 * dlinfo(RTLD_DI_LMID) confirms which namespace we got, but not that every
 * reference resolved the way that namespace was supposed to make it resolve.
 * This check does not care which mechanism was used, or whether it propagated
 * to dependencies -- which is exactly the question dev/dlopen-probe had to
 * settle for RTLD_DEEPBIND.
 */
static int resolution_is_outward(const void *abi_probe, const char **diagnostic)
{
  Dl_info abi_info, impl_info;

  if (!abi_probe) return 1; /* caller opted out */

  if (!dladdr(abi_probe, &abi_info) ||
      !dladdr((const void *)(uintptr_t)&MPI_Send, &impl_info)) {
    /* dladdr is best-effort; a statically linked implementation can defeat it.
     * Not fatal, because the alternative is refusing to run in a configuration
     * that may be perfectly correct.
     */
    return 1;
  }

  if (abi_info.dli_fbase == impl_info.dli_fbase) {
    *diagnostic =
        "symbol resolution captured: this libmpiwrapper's MPI_* calls resolve "
        "back into libmpi_abi instead of the MPI implementation, which would "
        "recurse until the stack is exhausted. Load the wrapper with dlmopen "
        "or RTLD_DEEPBIND (see MPI_ABI_WRAPPER_DLOPEN_MODE).";
    return 0;
  }
  return 1;
}

/* Every slot must be filled. A designated initializer that names a slot the
 * generator forgot leaves a NULL, and a NULL slot is a jump to address zero on
 * the first call; catching it here costs one pass over 1376 pointers, once.
 */
static int every_slot_filled(size_t size, const char **diagnostic)
{
  void *const *p = (void *const *)&mpiwrapper_vtable_instance;
  for (size_t i = 0; i < size / sizeof *p; ++i) {
    if (!p[i]) {
      *diagnostic = "libmpiwrapper has an unfilled vtable slot";
      return 0;
    }
  }
  return 1;
}

const struct mpiwrapper_vtable *
mpiwrapper_get_vtable(uint32_t abi_version, uint32_t abi_subversion,
                      uint32_t layout_hash, size_t size, const void *abi_probe,
                      const char **diagnostic)
{
  static atomic_int initialized;

  /* The ABI header carries both MPI_ABI_VERSION and MPI_ABI_SUBVERSION, so
   * check both. A differing major version is incompatible outright. A differing
   * subversion is not necessarily fatal -- subversions are meant to be additive
   * -- but the two halves must agree on which one they were generated from, and
   * the layout hash below would not catch a subversion that added no slot.
   */
  if (abi_version != MPIABI_ABI_VERSION) {
    *diagnostic =
        "MPI ABI major version mismatch between libmpi_abi and libmpiwrapper";
    return NULL;
  }
  if (abi_subversion != MPIABI_ABI_SUBVERSION) {
    *diagnostic =
        "MPI ABI subversion mismatch between libmpi_abi and libmpiwrapper";
    return NULL;
  }
  if (layout_hash != MPIWRAPPER_LAYOUT_HASH) {
    *diagnostic = "vtable layout mismatch: libmpi_abi and libmpiwrapper were "
                  "generated from different slot lists";
    return NULL;
  }
  /* A larger caller struct would read past the end of ours. A smaller one is
   * fine: it sees a prefix, and the layout hash already established that the
   * prefix agrees.
   */
  if (size > sizeof(struct mpiwrapper_vtable)) {
    *diagnostic =
        "libmpi_abi expects a larger vtable than this libmpiwrapper provides";
    return NULL;
  }

  if (!resolution_is_outward(abi_probe, diagnostic)) return NULL;
  if (!every_slot_filled(sizeof(struct mpiwrapper_vtable), diagnostic))
    return NULL;

  int expected = 0;
  if (atomic_compare_exchange_strong_explicit(&initialized, &expected, 1,
                                              memory_order_acq_rel,
                                              memory_order_acquire)) {
    if (!mpiwrapper_init_reverse_maps(diagnostic)) return NULL;
  }

  /* The maps are complete before any slot can be reached, which is the reason
   * this is a getter rather than an exported struct.
   */
  return &mpiwrapper_vtable_instance;
}
