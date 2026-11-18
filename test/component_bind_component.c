/* component_bind_component -- a stand-in for one of the implementation's own
 * components, for component_bind_test.c.
 *
 * Shaped like HPC-X's mca_io_romio321.so: it calls a public MPI function and
 * links no MPI library, so its reference is resolved wherever the loader finds
 * the name. The prototype is written out rather than taken from any mpi.h,
 * because the component belongs to neither side -- MPI_Get_library_version is
 * one of the few entry points whose signature is the same in the ABI and in
 * every implementation, which is why it was chosen: a call that crosses to the
 * wrong side still returns, and says which side it reached. Built six times,
 * as six files, so that each load is a new object.
 */

#include <dlfcn.h>

#define COMPONENT_EXPORT __attribute__((visibility("default")))

extern int MPI_Get_library_version(char *version, int *resultlen);

/* Through the PLT: a JUMP_SLOT relocation, bound lazily or at load. */
COMPONENT_EXPORT int component_library_version(char *version, int *resultlen)
{
  return MPI_Get_library_version(version, resultlen);
}

/* Through the GOT: a GLOB_DAT relocation, always bound at load. */
COMPONENT_EXPORT void *component_library_version_address(void)
{
  return (void *)&MPI_Get_library_version;
}

/* A component that loads plugins of its own, with its own dlopen: what it
 * loads must land on the same side as the component itself.
 */
COMPONENT_EXPORT void *component_load(const char *file)
{
  return dlopen(file, RTLD_LAZY | RTLD_LOCAL);
}
