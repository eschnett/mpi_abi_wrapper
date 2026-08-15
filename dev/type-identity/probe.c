/* What are the ABI's integer types and the implementation's, *actually*?
 *
 * The question is whether an array of the ABI's MPI_Aint can be handed to the
 * implementation as-is. At the language level the two typedefs may be distinct
 * types, which is a constraint violation and needs a cast. At the system-ABI
 * level, which is the level this project implements, all that matters is that
 * the representation agrees. This reports both, so the difference between "the
 * spellings differ" and "the representations differ" is visible.
 */
#include <mpi.h> /* the implementation's */

#include "mpiabi.h"

#include <stdio.h>

/* _Generic tests type identity exactly, which is what "needs a cast" turns on. */
#define TYPENAME(x)                                                            \
  _Generic((x),                                                                \
      int: "int",                                                              \
      long: "long",                                                            \
      long long: "long long",                                                  \
      short: "short",                                                          \
      default: "other")

#define IDENTICAL(x, T) _Generic((x), T: 1, default: 0)

#define REPORT(abi_t, impl_t, ident)                                           \
  printf("  %-12s impl %-10s %zu bytes %s | ABI %-10s %zu bytes %s | "         \
         "same type: %s | same repr: %s\n",                                    \
         #impl_t, TYPENAME((impl_t)0), sizeof(impl_t),                         \
         (impl_t)-1 < 0 ? "signed" : "unsigned", TYPENAME((abi_t)0),           \
         sizeof(abi_t), (abi_t)-1 < 0 ? "signed" : "unsigned",                 \
         (ident) ? "YES" : "no",                                               \
         (sizeof(abi_t) == sizeof(impl_t) &&                                   \
          ((abi_t)-1 < 0) == ((impl_t)-1 < 0))                                 \
             ? "YES"                                                           \
             : "NO")

int main(void)
{
  printf("MPI %d.%d\n", MPI_VERSION, MPI_SUBVERSION);
  REPORT(MPIABI_Aint, MPI_Aint, IDENTICAL((MPIABI_Aint)0, MPI_Aint));
  REPORT(MPIABI_Count, MPI_Count, IDENTICAL((MPIABI_Count)0, MPI_Count));
  REPORT(MPIABI_Offset, MPI_Offset, IDENTICAL((MPIABI_Offset)0, MPI_Offset));
  REPORT(MPIABI_Fint, MPI_Fint, IDENTICAL((MPIABI_Fint)0, MPI_Fint));
  return 0;
}
