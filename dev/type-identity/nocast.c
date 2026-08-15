/* Does passing the ABI's array to the implementation need a cast at all?
 * Compile with -Wall -Wextra -Wpedantic and read the diagnostic (or its
 * absence). Nothing is run; this is a question about the type system.
 */
#include <mpi.h>

#include "mpiabi.h"

extern int reader(const MPI_Aint *aint, const MPI_Count *count);

int caller(const MPIABI_Aint *abi_aint, const MPIABI_Count *abi_count)
{
  return reader(abi_aint, abi_count); /* no cast, on purpose */
}
