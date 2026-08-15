/* Does MPI_Type_get_contents touch the whole of max_datatypes?
 *
 * MPI-5.0 makes `max_datatypes` an *upper bound*: it "must be at least as
 * large as" the count MPI_TYPE_GET_ENVELOPE reports, and the call writes that
 * many entries. So a caller may legally pass an array larger than it needs,
 * and a wrapper may legally hand the implementation whatever the caller did.
 *
 * S3 found that it may not, and this is the measurement. Build and run against
 * each implementation; a crash *is* the result.
 *
 *   cc -I$MPI/include -L$MPI/lib -o probe probe.c -lmpi
 *   ./probe
 */

#include <mpi.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);

  const int          blocklengths[2]  = {1, 1};
  const MPI_Aint     displacements[2] = {0, (MPI_Aint)sizeof(int)};
  const MPI_Datatype types[2]         = {MPI_INT, MPI_DOUBLE};
  MPI_Datatype       newtype          = MPI_DATATYPE_NULL;

  MPI_Type_create_struct(2, blocklengths, displacements, types, &newtype);
  MPI_Type_commit(&newtype);

  int ni = 0, na = 0, nd = 0, combiner = 0;
  MPI_Type_get_envelope(newtype, &ni, &na, &nd, &combiner);
  printf("envelope: %d integers, %d addresses, %d datatypes\n", ni, na, nd);

  int      got_int[8];
  MPI_Aint got_addr[8];

  /* 1. max_datatypes exactly the envelope's count: the shape every
   *    implementation must accept. */
  MPI_Datatype exact[8];
  for (int i = 0; i < 8; ++i) exact[i] = (MPI_Datatype)0x7f;
  printf("max_datatypes == %d ...\n", nd);
  fflush(stdout);
  int e = MPI_Type_get_contents(newtype, ni, na, nd, got_int, got_addr, exact);
  printf("  returned %d, and left entry %d at %#llx -- untouched\n", e, nd,
         (unsigned long long)(uintptr_t)exact[nd]);
  fflush(stdout);

  /* 2. max_datatypes larger, with the tail zeroed. Legal, and Open MPI 5.0.6
   *    walks the whole of max_datatypes and dereferences each entry, so this
   *    segfaults at address 0x10 -- offset 16 of the NULL it found. Filling
   *    the tail with anything else moves the address and nothing else.
   */
  MPI_Datatype spare[8];
  memset(spare, 0, sizeof spare);
  printf("max_datatypes == 8, tail zeroed ...\n");
  fflush(stdout);
  e = MPI_Type_get_contents(newtype, ni, na, 8, got_int, got_addr, spare);
  printf("  returned %d -- the whole of max_datatypes is safe here\n", e);
  fflush(stdout);

  MPI_Type_free(&newtype);
  MPI_Finalize();
  return 0;
}
