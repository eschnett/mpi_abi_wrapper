/* libmpiwrapper -- what class of MPI object an MPI_T variable binds to.
 *
 * Hand-written and permanent, and the same shape as extents.c: a question a
 * generated body has to ask the implementation before it can convert an
 * argument, asked through PMPI_ because it is the wrapper's own traffic and
 * not the application's, and answered with an implementation error code that
 * the body maps like any other.
 *
 * The question here is MPI-5.0 15.3.6's. MPI_T_cvar_handle_alloc,
 * MPI_T_pvar_handle_alloc and MPI_T_event_handle_alloc all take
 * `void *obj_handle`, "an address to a local variable that stores the object's
 * handle" -- an *ABI* handle, of a class that appears nowhere in the argument
 * list. What decides the class is the `bind` a prior get_info reported, so the
 * wrapper asks the same question, and constants.c's
 * mpiwrapper_tool_obj_fromabi then converts accordingly.
 *
 * **Every OUT argument but `bind` is a null pointer, and that is standard
 * rather than a liberty.** MPI-5.0 says so of each of these three queries in
 * turn: "if any OUT parameter is a NULL pointer, the implementation will ignore
 * the parameter and not return a value for the parameter". Passing dummy
 * buffers instead would be worse than verbose -- MPI_T_EVENT_GET_INFO's `info`
 * is required to return a *newly created* info object when it is asked for, and
 * asking for one here would leak it once per handle allocation.
 */

#include "internal.h"

/* The three are unreachable where the implementation lacks the query, because
 * the allocator that calls each is then decision 6's stub. The guard is what
 * keeps this file compiling there, exactly as extents.c's large-count envelope
 * query is guarded; MPI_ERR_OTHER is the honest answer for a call that cannot
 * be made, and MPI_ERR_UNSUPPORTED_OPERATION is deliberately not used, since
 * it is MPI-4.0 and this file must compile against the MPI-3.0 floor.
 */

int mpiwrapper_cvar_bind(int cvar_index, int *bind)
{
#ifdef MPIWRAPPER_HAVE_MPI_T_cvar_get_info
  return PMPI_T_cvar_get_info(cvar_index, NULL, NULL, NULL, NULL, NULL, NULL,
                              NULL, bind, NULL);
#else
  (void)cvar_index;
  *bind = 0;
  return MPI_ERR_OTHER;
#endif
}

int mpiwrapper_pvar_bind(int pvar_index, int *bind)
{
#ifdef MPIWRAPPER_HAVE_MPI_T_pvar_get_info
  return PMPI_T_pvar_get_info(pvar_index, NULL, NULL, NULL, NULL, NULL, NULL,
                              NULL, NULL, bind, NULL, NULL, NULL);
#else
  (void)pvar_index;
  *bind = 0;
  return MPI_ERR_OTHER;
#endif
}

int mpiwrapper_event_bind(int event_index, int *bind)
{
#ifdef MPIWRAPPER_HAVE_MPI_T_event_get_info
  return PMPI_T_event_get_info(event_index, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, bind);
#else
  (void)event_index;
  *bind = 0;
  return MPI_ERR_OTHER;
#endif
}
