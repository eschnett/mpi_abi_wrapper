// The MPI implementation
// All MPI functions have been renamed, e.g. via `#define`. Only constants and types remain visible.
#include <mpi.h>

// The MPI ABI
// All MPI constants and types have been renamed: `MPI_*` -> to `MPI_ABI_*`. Only function prototypes remain visible.
#include <mpi_abi.h>



// The following identifiers are now available:
// Constants (e.g. `MPI_COMM_WORLD`):       MPI_COMM_WORLD:       MPI implementation
//                                          MPI_ABI_COMM_WORLD:   MPI ABI
// Types (e.g. `MPI_Comm`):                 MPI_Comm:             MPI implementation
//                                          MPI_ABI_Comm:         MPI ABI
// Function prototypes (e.g. `MPI_Send`):   MPI_Send              MPI ABI
//                                          MPI_hidden_Send       MPI implementation (unused)



// Pointer to the wrapped `MPI_Send` function
extern int (*mpiwrapper_MPI_Send)(const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);

int MPI_Send(const void* abi_buf, int abi_count, MPI_ABI_Datatype abi_datatype, int abi_dest, int abi_tag, MPI_ABI_Comm abi_comm)
{
  // Convert inputs
  const void* buf = mpiwrapper_buffer_fromabi(abi_buf);
  const int count = mpiwrapper_count_fromabi(abi_count);
  const MPI_Datatype datatype = mpiwrapper_datatype_fromabi(abi_datatype);
  const int dest = mpiwrapper_rank_fromabi(abi_dest);
  const int tag = mpiwrapper_tag_fromabi(abi_tag);
  const MPI_Comm comm = mpiwrapper_comm_fromabi(abi_comm);

  // MPI functions which takes arrays as input/output can either
  // rewrite the existing arrays (if allowed by the MPI standard, and
  // if there is sufficient space) or need to allocate temporaries

  // Forward the call
  const int ierror = (*mpiwrapper_MPI_Send)(buf, count, datatype, dest, tag, comm);

  // Convert outputs
  const int abi_ierror = mpiwrapper_errorcode_toabi(ierror);
  return abi_ierror;
}



extern MPI_Comm *mpiwrapper_MPI_COMM_WORLD;

MPI_ABI_Comm mpiwrapper_comm_toabi(MPI_Comm comm) {
  if (comm == *mpiwrapper_MPI_COMM_WORLD) return MPIABI_COMM_WORLD;

  // ... check for other predefined handles ...

  // Not a predefined handle, just preserve the bits:
  return (MPI_ABI_Comm)comm;
}

// Sentinels (e.g. `MPI_BOTTOM`) are converted in the same way as handles.

// Regular constants (e.g. error codes) can be converted via a switch statement.



// Initialize `mpiwrapper`
void initialize() {
  // Add error checks

  void *handle = dlopen("/path/libmpiwrapper.so");
  
  // We know the function names
  mpiwrapper_MPI_Send = dlsym(handle, "MPI_Send");

  // We defined names for all the predefined handles
  mpiwrapper_MPI_COMM_WORLD = dlsym(handle, "mpiwrapper_MPI_COMM_WORLD");
}
