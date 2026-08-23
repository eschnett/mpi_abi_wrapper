/* The self-wrap marker: the only file besides mpi.h that mpi_abi_wrapper puts
 * in an installed include directory.
 *
 * An mpi_abi_wrapper prefix is indistinguishable from a genuine
 * ABI-implementing MPI by everything find_package(MPI) looks at -- an mpi.h
 * reporting MPI_ABI_VERSION with a libmpi_abi beside it, which MPI-5.0 20.2.1
 * requires of a real implementation too. Configuring this project against
 * such a prefix builds a wrapper that forwards to itself, and the symptom is
 * a loop at startup rather than a diagnostic. CMakeLists.txt's second
 * configure-time check (NOTES.md #9) turns that into a hard error by looking
 * for this file in the found MPI's include directories.
 *
 * It is a file of its own, and declares nothing, for two reasons. mpi.h stays
 * pure -- the marker is not a macro in it. And the marker must not drag
 * mpiabi.h and mpiwrapper_vtable.h into a prefix that has no other use for
 * them: those two are internal to building this project, shared between
 * libmpi_abi and libmpiwrapper, and an installed prefix consumes neither.
 *
 * This is a hand-written file, not a generated one. It lives beside
 * FindMPI.cmake because both exist only to be installed and read by a *later*
 * CMake configure, never by this one.
 */

#ifndef MPIWRAPPER_MARKER_H
#define MPIWRAPPER_MARKER_H

#define MPIWRAPPER_MARKER 1

#endif /* MPIWRAPPER_MARKER_H */
