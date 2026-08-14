#ifndef PROBE_H
#define PROBE_H

/* Return codes from the MPI_Send stand-ins. Three, not two, because "libimpl was
 * reached" is not by itself the right answer: for T3 it means the plugin bypassed
 * the ABI layer, which is the bug. The code has to say *how* libimpl was reached.
 */
#define IMPL_REACHED  1 /* libimpl::MPI_Send ran, called directly */
#define ABI_CAPTURED  2 /* libabi::MPI_Send was re-entered -- the recursion bug */
#define ABI_FORWARDED 3 /* libabi forwarded through the wrapper, libimpl answered */

struct probe_vtable {
  int (*send)(const char *msg);
  int (*impl_internal)(void);
  const void *(*wrap_resolved)(void);
  const void *(*impl_resolved)(void);
};

/* libabi's interface. */
const char *abi_init(const char *mode); /* NULL on success, else a message */
int         abi_test_impl_internal(void);
void        abi_report_resolution(void);

#endif
