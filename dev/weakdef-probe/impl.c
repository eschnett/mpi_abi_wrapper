/* Stands in for the implementation's libmpi. Mirrors the profiling-interface
 * shape every implementation ships: a (possibly weak) MPI_X whose body
 * forwards to a strong PMPI_X, plus an internal caller modelling ROMIO-style
 * traffic written against the MPI interface.
 *
 * Return-value encoding: 1 = this library, 2 = libabi. Composites add the
 * caller's offset, so every path is distinguishable from the final value.
 */
int pfoo(void) { return 1; } /* PMPI_X: strong in every measured library */

#ifdef WEAK_FOO
__attribute__((weak))
#endif
int foo(void) { return 10 + pfoo(); } /* MPI_X, forwarding to PMPI_X */

int internal(void) { return 100 + foo(); } /* a component's own MPI_X call */
