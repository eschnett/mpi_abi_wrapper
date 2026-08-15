/* libmpiwrapper -- MPI_Pcontrol, the one genuinely variadic entry point
 * (NOTES.md #8, S4b).
 *
 * A file for one function, because it belongs to no family and its one
 * question is not a conversion.
 *
 * **The extra arguments cannot be forwarded, and nothing in C makes that
 * fixable.** `MPI_Pcontrol(const int level, ...)` has no `va_list` form to
 * hand on to, and the types of what follows `level` are not knowable: the
 * whole point of the interface is that a profiling library and the application
 * agree on them privately. libmpi_abi's own entry point has the same shape and
 * makes the same choice, calling the slot with the level alone.
 *
 * That is not a gap the standard leaves us to fill, though; it is what the
 * standard says happens. MPI-5.0 14.2.2: "MPI libraries themselves make no use
 * of this routine and simply return immediately to the user code", and the
 * rationale beside it is explicit that the arguments exist for a *profiling*
 * library, which is a library interposed on our MPI_* names and therefore
 * above this one. So an implementation that is handed the level alone is
 * handed everything it is entitled to act on.
 *
 * What is left is the level and the return code, and both convert: the level
 * is a plain int the standard fixes the meaning of, and the return code is
 * mapped like every other.
 */

#include "internal.h"

#ifdef MPIWRAPPER_HAVE_MPI_Pcontrol
#  define BODY_MPI_Pcontrol(TARGET)                                            \
    {                                                                          \
      const int level = abi_level;                                             \
                                                                               \
      const int ierror = TARGET(level);                                        \
      return mpiwrapper_errorcode_toabi(ierror);                               \
    }
#else
#  define BODY_MPI_Pcontrol(TARGET)                                            \
    {                                                                          \
      (void)abi_level;                                                         \
      return MPIABI_ERR_UNSUPPORTED_OPERATION;                                 \
    }
#endif

/* Declared variadic to match the vtable slot, which matches the ABI's own
 * declaration. The trailing arguments are never read, so no va_start appears
 * here -- reading them would require knowing their types, which is exactly
 * what nobody at this layer does.
 */
int mpiwrapper_w_MPI_Pcontrol(const int abi_level, ...)
    BODY_MPI_Pcontrol(MPI_Pcontrol)
int mpiwrapper_w_PMPI_Pcontrol(const int abi_level, ...)
    BODY_MPI_Pcontrol(PMPI_Pcontrol)
