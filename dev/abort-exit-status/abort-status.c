/* What exit status does the invoking environment see for MPI_Abort(comm, N)?
 *
 * MPI-5.0 11.8 calls `errorcode` the "error code to return to the invoking
 * environment", and both MPICH and Open MPI hand it to the OS as the job's
 * exit status. That is a different question from whether an MPI error code
 * survives a conversion, and this program is the only way to tell the two
 * apart: it aborts with the number given on the command line and says nothing
 * else, so `echo $?` is the whole answer.
 *
 *   abort-status N
 *
 * Built twice -- once with the wrapper's mpicc, once with the wrapped MPI's own
 * -- the second build is the control. See README.md.
 */

#include <mpi.h>

#include <stdlib.h>

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);
  MPI_Abort(MPI_COMM_WORLD, argc > 1 ? atoi(argv[1]) : 0);
  return 0; /* not reached */
}
