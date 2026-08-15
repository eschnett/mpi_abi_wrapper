/* test-consume -- the CMake-route half of S6's install-consumption check
 * (NOTES.md #9, STAGES.md's S6 exit check). An ordinary application: it knows
 * nothing about libmpiwrapper, only #include <mpi.h> and -lmpi_abi (here,
 * target_link_libraries(... mpi_abi::mpi_abi)).
 *
 * Deliberately smaller than test/abi_prototype_test.c: this is not exercising
 * argument classes, only "does a program built the ordinary way, from an
 * installed prefix, actually run" -- ci-scripts/check-install.sh is what
 * clears LD_LIBRARY_PATH/DYLD_LIBRARY_PATH and checks the binary's only MPI
 * dependency with nm/otool.
 */

#include <mpi.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
  int initialized, finalized, rank, size;

  MPI_Initialized(&initialized);
  if (initialized) { fprintf(stderr, "MPI_Initialized true before init\n"); return 1; }

  if (MPI_Init(&argc, &argv) != MPI_SUCCESS) return 1;

  MPI_Initialized(&initialized);
  if (!initialized) { fprintf(stderr, "MPI_Initialized false after init\n"); return 1; }

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  printf("hello from rank %d of %d\n", rank, size);
  fflush(stdout);

  MPI_Finalize();

  MPI_Finalized(&finalized);
  if (!finalized) { fprintf(stderr, "MPI_Finalized false after finalize\n"); return 1; }

  return 0;
}
