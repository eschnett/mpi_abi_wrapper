# FindMPI shim -- the third leg of NOTES.md #9's "enough of a FindMPI shim
# that a consumer written against find_package(MPI) -- HDF5, PETSc, nearly
# everyone -- works unmodified".
#
# CMake's own bundled FindMPI already has a path that can find this project on
# its own: its compiler-wrapper interrogation tries `-show`-style flags against
# whatever MPI_<lang>_COMPILER it locates, and bin/mpicc.in answers those the
# same way a real mpicc would (see that file). That path needs nothing from
# here and works the moment this prefix's bin/ is searched first (PATH, or
# -DMPI_C_COMPILER=<this prefix>/bin/mpicc).
#
# This file exists for the case that leaves open: a consumer that calls plain
# find_package(MPI) with neither on PATH nor named explicitly, and instead
# points CMAKE_MODULE_PATH at this prefix's cmake/Modules. `find_package`
# prefers a user's CMAKE_MODULE_PATH entry over CMake's own bundled module of
# the same name, so this file, once first in that list, IS the consumer's
# find_package(MPI) -- no interrogation, no guessing, and no dependence on
# CMake-version-specific parsing of a -show line. What it does is thin: hand
# the search to this package's own mpi_abiConfig.cmake, which already knows
# exactly where it installed itself, and republish the answer under the
# classic FindMPI spelling.
#
# One thing this shim deliberately does NOT provide: MPIEXEC_EXECUTABLE. A
# launcher belongs to the *wrapped* MPI, not to mpi_abi -- this package can be
# pointed at a different one at run time (decision 5), so baking in a build-time
# mpiexec would name only one of them and mislead about the rest. A consumer
# that needs to run its own tests should find the wrapped implementation's own
# mpiexec directly, the way this project's own CMakeLists.txt does.

# IN_LIST below (CMP0057) needs this: a Find module runs in the *including*
# project's policy scope, and test/check_exports.cmake (S6) is the reason this
# is not left to whatever a consumer's own cmake_minimum_required happens to
# set -- that script's own -P invocation had none at all.
if(POLICY CMP0057)
  cmake_policy(SET CMP0057 NEW)
endif()

if(MPI_FOUND)
  return()
endif()

# This file ships inside <prefix>/lib/cmake/mpi_abi/Modules/, so the package
# config is one directory up and this prefix's own bin/ is four up -- relative
# to *this file*, not to a guessed libdir name, so lib64 and multiarch layouts
# resolve the same way.
get_filename_component(_mpiabi_pkg_dir "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
get_filename_component(_mpiabi_prefix "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
find_package(mpi_abi CONFIG QUIET HINTS "${_mpiabi_pkg_dir}")
unset(_mpiabi_pkg_dir)

if(NOT mpi_abi_FOUND)
  set(MPI_FOUND FALSE)
  if(MPI_FIND_REQUIRED)
    message(FATAL_ERROR
      "FindMPI shim: mpi_abi was not found next to this module. This "
      "cmake/Modules/FindMPI.cmake is meant to be used only from inside a "
      "complete mpi_abi_wrapper installation.")
  endif()
  return()
endif()

set(MPI_FOUND TRUE)
set(MPI_C_FOUND TRUE)
get_target_property(MPI_C_INCLUDE_DIRS mpi_abi::mpi_abi
  INTERFACE_INCLUDE_DIRECTORIES)
# install(EXPORT) records the location under whichever config this package was
# built with (NOCONFIG for a single-config generator with no CMAKE_BUILD_TYPE,
# else that type's own name) -- tried in the order a consumer is most likely to
# have one.
foreach(_mpiabi_cfg NOCONFIG RELEASE RELWITHDEBINFO MINSIZEREL DEBUG)
  get_target_property(MPI_C_LIBRARIES mpi_abi::mpi_abi
    IMPORTED_LOCATION_${_mpiabi_cfg})
  if(MPI_C_LIBRARIES)
    break()
  endif()
endforeach()
unset(_mpiabi_cfg)
if(NOT MPI_C_LIBRARIES)
  get_target_property(MPI_C_LIBRARIES mpi_abi::mpi_abi IMPORTED_LOCATION)
endif()

if(NOT TARGET MPI::MPI_C)
  add_library(MPI::MPI_C INTERFACE IMPORTED)
  set_property(TARGET MPI::MPI_C APPEND PROPERTY
    INTERFACE_LINK_LIBRARIES mpi_abi::mpi_abi)
endif()

# mpicxx exists precisely when this build enabled a C++ compiler
# (CMakeLists.txt's enable_language(CXX OPTIONAL)); the same target serves
# C++ consumers since libmpi_abi carries no per-language conversion of its
# own.
if("CXX" IN_LIST MPI_FIND_COMPONENTS OR NOT MPI_FIND_COMPONENTS)
  if(EXISTS "${_mpiabi_prefix}/bin/mpicxx")
    set(MPI_CXX_FOUND TRUE)
    set(MPI_CXX_INCLUDE_DIRS "${MPI_C_INCLUDE_DIRS}")
    set(MPI_CXX_LIBRARIES "${MPI_C_LIBRARIES}")
    if(NOT TARGET MPI::MPI_CXX)
      add_library(MPI::MPI_CXX INTERFACE IMPORTED)
      set_property(TARGET MPI::MPI_CXX APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES mpi_abi::mpi_abi)
    endif()
  endif()
endif()

unset(_mpiabi_prefix)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MPI
  REQUIRED_VARS MPI_C_LIBRARIES MPI_C_INCLUDE_DIRS)
