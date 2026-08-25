/* Stands in for libmpi_abi: strong definitions of the same two names,
 * loaded first because the application links it. Returning 2 marks a call
 * that landed here when it should have stayed inside the implementation.
 */
int foo(void) { return 2; }
int pfoo(void) { return 2; }
