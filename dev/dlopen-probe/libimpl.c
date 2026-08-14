/* Stands in for libmpi.
 *
 * Exports MPI_Send with default visibility, as every real MPI does, and also has
 * an *internal* caller of MPI_Send -- which is what Open MPI's ROMIO and io
 * components do. That internal call is the case the whole probe exists for: if it
 * can be captured by libabi, the capture is memory-unsafe in the real system,
 * because the component passes its own smaller MPI_Status.
 */

#include <stdio.h>

#define IMPL_REACHED 1

int MPI_Send(const char *msg)
{
  fprintf(stderr, "        [impl::MPI_Send] (%s)\n", msg);
  return IMPL_REACHED;
}

/* Models a component inside the implementation calling the MPI interface. */
int impl_internal(void)
{
  fprintf(stderr, "      [impl::impl_internal] -> MPI_Send\n");
  return MPI_Send("from impl internals");
}

/* The address of MPI_Send *as this library resolves it*. Goes through the GOT
 * precisely because default-visibility function addresses must be interposable, so
 * this reports the loader's answer rather than the linker's guess.
 */
const void *impl_resolved_mpi_send(void) { return (const void *)&MPI_Send; }
