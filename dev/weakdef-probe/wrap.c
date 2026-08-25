/* Stands in for libmpiwrapper: two-level, links the implementation, and calls
 * it by name exactly as the generated wrapper bodies do (decision 7: the
 * MPI_X slot calls MPI_X, the PMPI_X slot calls PMPI_X).
 */
extern int foo(void), pfoo(void), internal(void);
int wrap_foo(void) { return foo(); }
int wrap_pfoo(void) { return pfoo(); }
int wrap_internal(void) { return internal(); }
