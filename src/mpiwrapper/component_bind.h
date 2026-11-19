/* libmpiwrapper -- binding the implementation's own components (Linux).
 *
 * component_bind.c has the mechanism and NOTES.md #2, "Components the
 * implementation loads later", the reason.
 */

#ifndef MPIWRAPPER_COMPONENT_BIND_H
#define MPIWRAPPER_COMPONENT_BIND_H

#if defined(__linux__)
/* Called once, from mpiwrapper_get_vtable once the wrapper's own MPI_* calls
 * are known to reach the implementation: abi_probe is an address in
 * libmpi_abi, impl_probe one in the implementation's libmpi. Until it has run,
 * and if either is NULL, the dlopen hook only forwards.
 */
void mpiwrapper_component_bind_init(const void *abi_probe,
                                    const void *impl_probe);
#else
static inline void mpiwrapper_component_bind_init(const void *abi_probe,
                                                  const void *impl_probe)
{
  (void)abi_probe;
  (void)impl_probe;
}
#endif

#endif
