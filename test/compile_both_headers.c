/* gen/include/mpi.h and gen/include/mpiabi.h, included together in one TU,
 * as libmpiwrapper's generated code will do (NOTES.md #2). Checks that no
 * tag, typedef, macro or enumerator collides between the two, and that a
 * handle value converts between MPI_X and MPIABI_X with no cast. */
#include "mpi.h"
#include "mpiabi.h"

static MPIABI_Comm mpiabi_from_mpi(MPI_Comm c) { return c; }
static MPI_Comm mpi_from_mpiabi(MPIABI_Comm c) { return c; }

int unused_both_headers_check;

void unused_both_headers_reference(void) {
  (void)mpiabi_from_mpi(MPI_COMM_WORLD);
  (void)mpi_from_mpiabi(MPIABI_COMM_WORLD);
}
