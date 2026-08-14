#include <mpi.h>

// Define extern variables for all predefined handles and sentinels.
// This ensures that we know the symbol names which the linker sees.

// All handles
const MPI_Comm mpiwrapper_MPI_COMM_WORLD = MPI_COMM_WORLD;
// ... all others ...

// All sentinel values
const void* mpiwrapper_MPI_BOTTOM = MPI_BOTTOM;
// ... all others ...
