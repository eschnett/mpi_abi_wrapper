cmake_minimum_required(VERSION 3.20)
project(mpi_abi_wrapper
  VERSION 1.0.0
  DESCRIPTION "An MPI-5.0 standard-ABI shim over any MPI-3.0+ implementation"
  HOMEPAGE_URL "https://github.com/eschnett/mpi_abi_wrapper"
  LANGUAGES C)

# All 688 entry points, exported under both names (1376 symbols) and reaching
# 1366 vtable slots -- the five entry points MPI-3.0 deleted are answered by
# libmpi_abi itself and have no slot (NOTES.md #3). CODE.md has the layout,
# the options and the authority behind each count.

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_VISIBILITY_PRESET hidden)

# mpicxx (NOTES.md #9's "mpicc and friends") is the only reason this project
# ever touches C++: nothing under src/ or gen/ is compiled as C++, and
# gen/include/mpi.h wraps its own declarations in extern "C" already. OPTIONAL
# so that a host with no C++ compiler still configures and builds everything
# else -- bin/mpicxx (and its FindMPI-shim COMPONENT CXX) then simply do not
# exist, which ci-scripts/check-install.sh treats as a skip rather than a
# failure.
enable_language(CXX OPTIONAL)

# Fortran is detected inside the wrapper block below, not here: the compiler it
# wants is the *wrapped MPI's own* (decision 25), which cannot be derived until
# find_package(MPI COMPONENTS C) has said where that MPI is.

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(MPIABI_GEN_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/gen/include)
set(MPIABI_GENERATE_HEADERS_SCRIPT ${CMAKE_SOURCE_DIR}/dev/generate_headers.py)
set(MPIABI_GENERATE_SCRIPT ${CMAKE_SOURCE_DIR}/dev/generate.py)
set(MPIABI_PROBE_SCRIPT ${CMAKE_SOURCE_DIR}/dev/probe_impl.py)
set(MPIABI_LAYOUT_HASH_SCRIPT ${CMAKE_SOURCE_DIR}/dev/layout_hash.py)
set(MPIABI_CHECK_PROTOTYPE_SCRIPT ${CMAKE_SOURCE_DIR}/dev/check_prototype.py)
set(MPIABI_CHECK_C_BINDINGS_SCRIPT ${CMAKE_SOURCE_DIR}/dev/check-c-bindings.py)
set(MPIABI_CHECK_OUT_PARAMS_SCRIPT ${CMAKE_SOURCE_DIR}/dev/check_out_params.py)

# `-DMPI_ABI_BUILD_WRAPPER=OFF` builds libmpi_abi alone, which needs no MPI at
# all. That is a developer option, not part of the normal flow: a user
# configures against an MPI and gets a working mpi.h, libmpi_abi, libmpiwrapper
# and (from S6) compiler wrapper and package files, without having to know that
# one half does not need MPI (NOTES.md #9).
option(MPI_ABI_BUILD_WRAPPER "Build libmpiwrapper (requires an MPI)" ON)

# **Fortran is expected, not optional.** One translation unit of it answers a
# question C cannot (decision 25, and the Fortran block below), and without one
# the two Fortran getters answer "not set" -- legal, and the only answer this
# project gave before S10, but a degradation. The default is ON because the
# common way to end up without a Fortran compiler is to have forgotten to
# install one, and a silent "not set" is exactly the accident that reaches a
# Fortran consumer as a missing feature rather than as a build error.
#
# OFF means *do not look*, not "use one if there is one". That is what makes it
# a deterministic no-Fortran configuration on any machine, which is the thing CI
# needs: before this, the no-Fortran path's only coverage was the `sanitize` row
# happening not to install gfortran (NOTES.md #9).
#
# Consulted only inside the wrapper block: the probe is compiled into
# libmpiwrapper alone, so `-DMPI_ABI_BUILD_WRAPPER=OFF` needs no Fortran
# whatever this says.
option(MPI_ABI_FORTRAN
  "Require a Fortran compiler for the wrapper's Fortran probe (decision 25)" ON)

# Needed before the mpi_abi target below, which bakes an absolute path to
# libmpiwrapper's installed name into itself (decision 5, NOTES.md #9).
if(APPLE)
  set(MPIABI_WRAPPER_SUFFIX .dylib)
else()
  set(MPIABI_WRAPPER_SUFFIX .so)
endif()

# These are GNU spellings, and were applied to whatever compiler turned up. That
# held while the compilers were gcc and clang; the NVIDIA row is what showed it
# does not generalize. nvc accepts the flags and then fails on its *own*
# <stdatomic.h>: every atomic_fetch_add_explicit here draws
#
#   error: ellipsis with no explicit parameters is nonstandard
#          [nonstd_ellipsis_only_param]
#
# which is nvc's pedantic diagnostic firing on nvc's header, turned into an
# error by our -Werror. Nothing about this project's code is involved, and no
# edit to it can help.
#
# So the strict set is asked for by name where it is known to mean what it says,
# and elsewhere the useful half is kept without -Wpedantic, whose reading of
# "nonstandard" is the compiler's own and here indicts its own headers. -Werror
# stays everywhere: a warning nobody fails on is a warning nobody reads.
if(CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang|IntelLLVM|Intel)$")
  set(MPIABI_WARNINGS -Wall -Wextra -Wpedantic -Werror)
else()
  set(MPIABI_WARNINGS -Wall -Werror)
endif()
message(STATUS "Warning flags for ${CMAKE_C_COMPILER_ID}: ${MPIABI_WARNINGS}")

# **libmpiwrapper forwards entry points the implementation has deprecated, and
# that is its job rather than an oversight.** The ABI is the whole of MPI-5.0
# and deprecated still means provided (NOTES.md #1): MPI_Get_elements_x and
# MPI_Status_set_elements_x are among the 688, so their bodies must call the
# implementation's -- which Open MPI's development branch marks
# __attribute__((deprecated)) per MPI-4.1, turning -Werror into four errors and
# failing the whole build.
#
# So the diagnostic is suppressed for the two targets that forward, and only
# those: libmpi_abi does not call an implementation at all, and the tests are
# applications, where the advice is real. Suppressing it *per call site*
# instead was the alternative, and it is the worse one -- the next entry point
# any implementation deprecates would break the build again, and the fix would
# be another pragma, which is a rule nobody stated being rediscovered one
# instance at a time.
if(CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang|IntelLLVM|Intel)$")
  set(MPIABI_FORWARDS_DEPRECATED -Wno-deprecated-declarations)
else()
  set(MPIABI_FORWARDS_DEPRECATED)
endif()

# NOTES.md #11's S9. A string rather than a bool so the set is the caller's
# choice -- `address`, `undefined`, `address,undefined`, `thread` -- and empty
# by default, which is every build that does not ask.
#
# Directory-wide rather than per-target on purpose: the instrumentation has to
# reach libmpi_abi, libmpiwrapper and the test executables alike, since the
# thing worth catching lives on the boundary between them. The wrapped MPI is
# *not* instrumented, which is the arrangement this runs in and the reason
# LeakSanitizer needs a suppression file rather than a clean slate.
set(MPI_ABI_SANITIZE "" CACHE STRING
  "Sanitizers to build with, e.g. address,undefined (empty: none)")
if(MPI_ABI_SANITIZE)
  message(STATUS "Building with -fsanitize=${MPI_ABI_SANITIZE}")
  # -g and a frame pointer so a report names lines rather than addresses; both
  # are what makes the output triage-able, and neither changes what is caught.
  add_compile_options(-fsanitize=${MPI_ABI_SANITIZE} -fno-omit-frame-pointer -g)
  add_link_options(-fsanitize=${MPI_ABI_SANITIZE})
endif()

# Generated output is committed (NOTES.md #9): `regenerate` writes it, and is
# never part of the default build. CI instead runs the check tests below, which
# fail on any drift between the committed files and a fresh generation.
add_custom_target(regenerate
  COMMAND ${Python3_EXECUTABLE} ${MPIABI_GENERATE_SCRIPT}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Regenerating all seven artifacts under gen/"
)

enable_testing()

add_test(NAME headers-up-to-date
  COMMAND ${Python3_EXECUTABLE} ${MPIABI_GENERATE_HEADERS_SCRIPT} --check
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Generated output is committed and must never be hand-edited, so the check
# that a fresh run reproduces it byte for byte is the gate (NOTES.md #3). It
# also carries the frozen tallies and the HAND_WRITTEN ledger, either of which
# failing means something was reclassified without anyone deciding to.
add_test(NAME generated-up-to-date
  COMMAND ${Python3_EXECUTABLE} ${MPIABI_GENERATE_SCRIPT} --check
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# S2's exit check: the generator reproduces the S1 prototype, or every
# difference is a named exemption that fails when it stops firing.
add_test(NAME prototype-reproduced
  COMMAND ${Python3_EXECUTABLE} ${MPIABI_CHECK_PROTOTYPE_SCRIPT}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# NOTES.md #7 decision 6's promise reaches the generated bodies too, not just
# the stubs: a caller may ignore the return code, so no early return may leave
# an out parameter undefined. This closed 49 such returns across 21 entry
# points and is here so they cannot reopen.
add_test(NAME out-params-defined
  COMMAND ${Python3_EXECUTABLE} ${MPIABI_CHECK_OUT_PARAMS_SCRIPT}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# The layout hash has to track the slot list it claims to summarize, or the
# handshake it exists for degrades into a constant nobody updates.
add_test(NAME layout-hash
  COMMAND ${Python3_EXECUTABLE} ${MPIABI_LAYOUT_HASH_SCRIPT} --check
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# S5's exit check, oracle 4 of NOTES.md #10: every one of the 688 C bindings
# in gen/include/mpi.h either matches MPI-5.0 Appendix A.3 or is a named,
# self-firing exemption. Independent of apis.json and the generator -- it
# reads only doc/mpi50-report.pdf and the committed header.
add_test(NAME c-bindings-cross-check
  COMMAND ${Python3_EXECUTABLE} ${MPIABI_CHECK_C_BINDINGS_SCRIPT}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# S0 exit check: both headers compile standalone in a TU of their own, and
# (compile_both_headers.c) together in one TU with no collision -- the
# configuration libmpiwrapper uses.
add_library(check_mpi_h_standalone OBJECT test/compile_mpi_h.c)
target_include_directories(check_mpi_h_standalone PRIVATE ${MPIABI_GEN_INCLUDE_DIR})

add_library(check_mpiabi_h_standalone OBJECT test/compile_mpiabi_h.c)
target_include_directories(check_mpiabi_h_standalone PRIVATE ${MPIABI_GEN_INCLUDE_DIR})

add_library(check_both_headers OBJECT test/compile_both_headers.c)
target_include_directories(check_both_headers PRIVATE ${MPIABI_GEN_INCLUDE_DIR})

foreach(tgt check_mpi_h_standalone check_mpiabi_h_standalone check_both_headers)
  target_compile_options(${tgt} PRIVATE ${MPIABI_WARNINGS})
endforeach()

# ---------------------------------------------------------------- libmpi_abi

# Includes the ABI mpi.h and nothing else; needs no MPI to build, which is what
# makes the cross test possible (one libmpi_abi, two wrappers, an environment
# variable between them).
add_library(mpi_abi SHARED
  src/mpi_abi/bootstrap.c
  gen/mpi_abi/entrypoints.c
)
set_target_properties(mpi_abi PROPERTIES OUTPUT_NAME mpi_abi)

# The soname, and the one number in this file that is *not* the project's.
#
# MPI-5.0 20.2.1 requires the library be named mpi_abi, says implementors "are
# expected to follow platform-specific conventions for dynamic shared library
# naming and versioning", and permits an application to depend on "mpi_abi or
# its versioned variant". The whole promise of the standard ABI is that a
# binary built against one libmpi_abi runs against another's, and that swap is
# decided by the *filename* the binary records -- so this has to agree with
# what the other implementations chose, not merely be self-consistent. Open
# MPI's ABI branch installs libmpi_abi.so.1 / libmpi_abi.1.dylib, so 1 it is.
#
# It tracks MPI_ABI_VERSION and must never be made to follow PROJECT_VERSION:
# this project's 1.1.0 or 2.0.0 would still implement ABI major 1 and must
# still answer to the same soname. Read out of the generated header rather
# than written here twice, so the two cannot drift -- the same discipline
# CLAUDE.md asks for of every other count.
file(STRINGS ${MPIABI_GEN_INCLUDE_DIR}/mpi.h MPIABI_ABI_VERSION_LINE
  REGEX "^#define[ \t]+MPI_ABI_VERSION[ \t]+[0-9]+")
if(NOT MPIABI_ABI_VERSION_LINE MATCHES "MPI_ABI_VERSION[ \t]+([0-9]+)")
  message(FATAL_ERROR
    "Could not read MPI_ABI_VERSION from ${MPIABI_GEN_INCLUDE_DIR}/mpi.h; "
    "libmpi_abi's soname is derived from it and must not be guessed.")
endif()
set(MPIABI_SOVERSION ${CMAKE_MATCH_1})
message(STATUS "libmpi_abi soname version (MPI_ABI_VERSION): ${MPIABI_SOVERSION}")

# VERSION is the project's and moves with every release; SOVERSION is the ABI's
# and does not. On ELF that gives libmpi_abi.so.${PROJECT_VERSION} with an
# soname of libmpi_abi.so.${MPIABI_SOVERSION}; on Mach-O, libmpi_abi.1.dylib
# with -compatibility_version ${MPIABI_SOVERSION} -- the same layout Open MPI's
# ABI branch installs. Both leave the unversioned name as a symlink, which is
# what bin/mpicc's -lmpi_abi resolves through at link time.
#
# libmpiwrapper deliberately gets none of this. It is not a linked dependency
# of anything: libmpi_abi dlopens it by the absolute path baked in below, and
# MPI-5.0 20.2.1 forbids the application binary naming it at all. A soname on a
# plugin located by path buys nothing and would only add symlinks.
set_target_properties(mpi_abi PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${MPIABI_SOVERSION})

# **Mach-O gates on a second number, and it is not the one in the file name.**
# dyld records the compatibility version a client was linked against and
# refuses at load any library offering a lower one -- so getting the leaf name
# right and this wrong produces exactly the failure the soname exists to
# prevent, and produces it only on macOS and only when the swap is actually
# attempted.
#
# The ecosystem's number comes from libtool: every implementation of ABI
# version 1 is built `-version-info 1:0:0`, which libtool turns into the name
# libmpi_abi.1.dylib and the compatibility version (current - age + 1) = 2.0.0.
# Open MPI's ABI branch installs exactly that, and mpif's
# ci-scripts/check-mpi-install.sh asserts both fields -- it is what caught this
# here, having found our compatibility version at CMake's default of 1.0.0.
#
# CMake derives -compatibility_version from SOVERSION, which has to stay 1 for
# the file name, so the two are decoupled with MACHO_*_VERSION. Kept as
# SOVERSION + 1 rather than written as 2, so the arithmetic that ties it to the
# ABI version survives an ABI 2 that nobody has thought about yet.
if(APPLE)
  math(EXPR MPIABI_MACHO_VERSION "${MPIABI_SOVERSION} + 1")
  set_target_properties(mpi_abi PROPERTIES
    MACHO_COMPATIBILITY_VERSION ${MPIABI_MACHO_VERSION}.0.0
    MACHO_CURRENT_VERSION ${MPIABI_MACHO_VERSION}.0.0)
endif()
target_include_directories(mpi_abi PRIVATE ${MPIABI_GEN_INCLUDE_DIR})
target_compile_options(mpi_abi PRIVATE ${MPIABI_WARNINGS})
# dlopen/dlsym/dlerror live in libdl on glibc older than 2.34 and in libc from
# 2.34 onwards, so this is empty on a current distro and "dl" on Ubuntu 20.04 --
# which is how the omission stayed invisible until an MPI-3.0 implementation
# forced an older container. CMAKE_DL_LIBS is the portable spelling.
target_link_libraries(mpi_abi PRIVATE ${CMAKE_DL_LIBS})
# Every MPI_*/PMPI_* entry point is exported and nothing else is: the ABI
# functions carry default visibility from the ABI header, and the one internal
# global is marked hidden in bootstrap.c. Visibility attributes stop there,
# though -- they cannot touch the handful of symbols the linker itself inserts
# into every shared object (_init, _fini, _edata, _end, __bss_start on ELF),
# which is what the version script / exported-symbols list below is for
# (NOTES.md #9; test/check_exports.cmake used to carry an explicit exemption
# for exactly these names).
set_target_properties(mpi_abi PROPERTIES C_VISIBILITY_PRESET default)
if(APPLE)
  target_link_options(mpi_abi PRIVATE
    "-Wl,-exported_symbols_list,${CMAKE_SOURCE_DIR}/cmake/mpi_abi.exported_symbols")
  set_target_properties(mpi_abi PROPERTIES
    LINK_DEPENDS ${CMAKE_SOURCE_DIR}/cmake/mpi_abi.exported_symbols)
else()
  target_link_options(mpi_abi PRIVATE
    "-Wl,--version-script=${CMAKE_SOURCE_DIR}/cmake/mpi_abi.version")
  set_target_properties(mpi_abi PROPERTIES
    LINK_DEPENDS ${CMAKE_SOURCE_DIR}/cmake/mpi_abi.version)
endif()

# Decision 5's "falling back to a build-time path": libmpi_abi finds
# libmpiwrapper from MPI_ABI_WRAPPER_LIB in the environment, and absent that,
# from this -- the absolute path this same prefix installs libmpiwrapper to.
# Without it, a program built the ordinary way (bin/mpicc, find_package or
# pkg-config, none of which mention libmpiwrapper -- MPI-5.0 20.2.1) would
# only start with MPI_ABI_WRAPPER_LIB set by hand, which is not "run and it
# works". src/mpi_abi/bootstrap.c's own fallback (a bare filename) is what
# runs when this is left unset, e.g. the developer build of libmpi_abi alone;
# it depends on the loader's default search path finding a same-named library,
# which an installed, exclusive prefix does not guarantee.
target_compile_definitions(mpi_abi PRIVATE
  "MPI_ABI_WRAPPER_LIB_DEFAULT=\"${CMAKE_INSTALL_FULL_LIBDIR}/libmpiwrapper${MPIABI_WRAPPER_SUFFIX}\"")

install(TARGETS mpi_abi
  EXPORT mpi_abiTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Two files, and deliberately only two. mpi.h is what an application includes;
# cmake/mpiwrapper_marker.h is check 2's self-wrap marker (CMakeLists.txt
# further down) -- a *later* configure against this prefix has to see it to
# refuse to wrap itself, so it goes in unconditionally, whether or not this
# build produced a wrapper of its own.
#
# mpiabi.h and mpiwrapper_vtable.h are *not* installed. They are the two halves'
# private contract -- the MPIABI_ view of the ABI and the vtable they share --
# and every build that needs them, including the per-MPI wrapper builds the
# README describes, is a build of this project from source with gen/include on
# its path. Installing them put a header in the public include directory that
# no consumption route names and that could only shadow something.
install(FILES
  ${MPIABI_GEN_INCLUDE_DIR}/mpi.h
  cmake/mpiwrapper_marker.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# ------------------------------------------------------------ mpicc / mpicxx

# The first of the three consumption routes (NOTES.md #9, decision 19):
# compiler wrappers naming this prefix's mpi.h and libmpi_abi, with the rpath
# set so a program they produce starts with no help. Generated once, at this
# project's own configure time, not per-MPI -- see bin/mpicc.in's comment for
# why there is exactly one.
if(APPLE AND CMAKE_OSX_SYSROOT)
  set(MPIABI_MPICC_SYSROOT_FLAG "-isysroot ${CMAKE_OSX_SYSROOT}")
else()
  set(MPIABI_MPICC_SYSROOT_FLAG "")
endif()
configure_file(bin/mpicc.in ${CMAKE_CURRENT_BINARY_DIR}/bin/mpicc @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/bin/mpicc
  DESTINATION ${CMAKE_INSTALL_BINDIR})

if(CMAKE_CXX_COMPILER)
  configure_file(bin/mpicxx.in ${CMAKE_CURRENT_BINARY_DIR}/bin/mpicxx @ONLY)
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/bin/mpicxx
    DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# ------------------------------------------------------- CMake package files

# The second consumption route: find_package(mpi_abi) plus, from
# cmake/FindMPI.cmake, plain find_package(MPI) too. mpi_abiTargets.cmake
# (written by install(EXPORT) below) carries the imported location and the
# INCLUDES DESTINATION above as usage requirements, so mpi_abiConfig.cmake.in
# itself has nothing left to compute.
set(MPIABI_PACKAGE_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/mpi_abi)

configure_package_config_file(
  cmake/mpi_abiConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/mpi_abiConfig.cmake
  INSTALL_DESTINATION ${MPIABI_PACKAGE_CMAKE_DIR})

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/mpi_abiConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/mpi_abiConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/mpi_abiConfigVersion.cmake
  DESTINATION ${MPIABI_PACKAGE_CMAKE_DIR})

install(EXPORT mpi_abiTargets
  FILE mpi_abiTargets.cmake
  NAMESPACE mpi_abi::
  DESTINATION ${MPIABI_PACKAGE_CMAKE_DIR})

# cmake/FindMPI.cmake's own comment explains why this is a second route to the
# same answer rather than a duplicate of it: it works even when a consumer
# never names mpi_abi, only find_package(MPI), by outranking CMake's bundled
# module of the same name once this directory leads CMAKE_MODULE_PATH.
install(FILES cmake/FindMPI.cmake
  DESTINATION ${MPIABI_PACKAGE_CMAKE_DIR}/Modules)

# ------------------------------------------------------------- pkg-config

configure_file(cmake/mpi_abi.pc.in ${CMAKE_CURRENT_BINARY_DIR}/mpi_abi.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mpi_abi.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

# --------------------------------------------------------------- libmpiwrapper

if(MPI_ABI_BUILD_WRAPPER)
  find_package(MPI REQUIRED COMPONENTS C)

  # Configure-time checks, all compile-only so that cross-compiling works: this
  # project may never run a program at configure time (NOTES.md #1, #9).
  include(CheckCSourceCompiles)
  set(CMAKE_REQUIRED_INCLUDES ${MPI_C_INCLUDE_DIRS})
  set(CMAKE_REQUIRED_FLAGS ${MPI_C_COMPILE_OPTIONS})

  # No self-wrapping. The marker is mpiwrapper_marker.h, a file only we install
  # and which exists for no other purpose: neither MPI_ABI_VERSION nor the
  # library name discriminates, because MPI-5.0 20.2.1 requires a genuine ABI
  # implementation to install a libmpi_abi too. The accident is easy to hit --
  # find_package(MPI) is looking for exactly the mpi.h and libmpi_abi we
  # install -- and its symptom is a startup loop rather than a diagnostic.
  find_file(MPIABI_SELF_WRAP_MARKER mpiwrapper_marker.h
    PATHS ${MPI_C_INCLUDE_DIRS} NO_DEFAULT_PATH)
  if(MPIABI_SELF_WRAP_MARKER)
    message(FATAL_ERROR
      "The MPI found at ${MPI_C_INCLUDE_DIRS} is an mpi_abi_wrapper "
      "installation (it contains ${MPIABI_SELF_WRAP_MARKER}). Wrapping "
      "ourselves would loop at startup. Point CMake at a real MPI.")
  endif()

  # Oracle 5, the identity configuration: wrapping an MPI that already implements
  # the standard ABI. Every conversion becomes an identity -- predefined handle
  # values, error codes, sentinels, MPI_MAX_*, ranks and tags all match -- so the
  # conversion tables are neutralized and any difference from native pass-through
  # is a bug in the *plumbing*: vtable handshake, bootstrap, staged temporaries,
  # trampoline pools, lifetime rules. It also lands the status assertion exactly
  # on its boundary, since an ABI implementation's private part is 20 bytes and
  # fits with nothing to spare (NOTES.md #10).
  #
  # Permitted, warned about, and never the default: the same configuration is
  # what an accidental self-wrap looks like from the compiler's point of view.
  option(MPI_ABI_WRAP_ABI_IMPL
    "Wrap an MPI that already implements the standard ABI (oracle 5)" OFF)
  if(MPI_ABI_WRAP_ABI_IMPL)
    message(WARNING
      "MPI_ABI_WRAP_ABI_IMPL is on: the MPI at ${MPI_C_INCLUDE_DIRS} is being "
      "treated as an ABI-implementing MPI, so every conversion in the wrapper "
      "becomes an identity. This is oracle 5 and it tests the plumbing; it does "
      "*not* test the conversion tables.")
  endif()

  # The implementation is *expected* to provide the MPI-4.0 API, which is what
  # makes the common case a 1:1 mapping -- but it is an expectation and not a
  # floor (NOTES.md #1). Open MPI 5.0.10 reports MPI-3.1 and has no _c entry
  # points at all, and it is a perfectly good implementation to wrap: every
  # one of them is served by narrowing onto its small twin (NOTES.md #5.10).
  # So failing this check costs a ceiling at INT_MAX rather than a missing
  # surface, which is why it is a status line and not a warning; only below
  # MPI-3.0 does this refuse, where the ABI's own surface stops being
  # expressible.
  check_c_source_compiles("
#include <mpi.h>
#if !defined(MPI_VERSION) || MPI_VERSION < 3
#error too old
#endif
int main(void) { return 0; }" MPIABI_IMPL_AT_LEAST_MPI_3)
  if(NOT MPIABI_IMPL_AT_LEAST_MPI_3)
    message(FATAL_ERROR "The MPI at ${MPI_C_INCLUDE_DIRS} is older than MPI-3.0")
  endif()

  check_c_source_compiles("
#include <mpi.h>
#if !defined(MPI_VERSION) || MPI_VERSION < 4
#error no large-count forms
#endif
int main(void) { return 0; }" MPIABI_IMPL_AT_LEAST_MPI_4)
  if(NOT MPIABI_IMPL_AT_LEAST_MPI_4)
    message(STATUS
      "The MPI at ${MPI_C_INCLUDE_DIRS} predates MPI-4.0, so it has no _c "
      "entry points. The wrapper serves them by narrowing each call onto its "
      "small twin, and refuses only a value that will not fit -- with "
      "MPI_ERR_VALUE_TOO_LARGE, not MPI_ERR_UNSUPPORTED_OPERATION. See "
      "NOTES.md #5.10 for the rule and #13.2 for what it costs.")
  endif()

  # --------------------------------------------------------------- Fortran
  #
  # Fortran, for one translation unit and one question: what LOGICAL looks like,
  # which MPI-5.0 20.4.1 requires MPI_ABI_GET_FORTRAN_BOOLEANS to answer and
  # which C cannot find out (src/mpiwrapper/fortran_probe.f90 has the argument,
  # decision 25 the rule).
  #
  # **Whose Fortran compiler answers is the whole point**, and it should be the
  # one the *wrapped MPI* was built with -- its .TRUE. is the value the getters
  # are being asked for. So the search prefers the mpifort sitting beside
  # MPI_C_COMPILER, and this block therefore lives here rather than beside
  # enable_language(CXX): MPI_C_COMPILER is only known once find_package(MPI
  # COMPONENTS C) has run, whether the user named it or FindMPI found it.
  #
  # **check_language, not enable_language(... OPTIONAL).** OPTIONAL does not
  # mean what the name suggests: with no Fortran compiler on the machine it
  # still reports the language loaded, sets CMAKE_Fortran_COMPILER to the string
  # "CMAKE_Fortran_COMPILER-NOTFOUND", and defers the failure to build time --
  #
  #   -- The Fortran compiler identification is unknown
  #   gmake[2]: CMAKE_Fortran_COMPILER-NOTFOUND: No such file or directory
  #
  # -- which is what CI's sanitize job, the only row without gfortran, reported.
  # CMAKE_Fortran_COMPILER_LOADED is TRUE there too, so it is not the test
  # either. check_language() try-compiles and leaves NOTFOUND when that fails,
  # so it answers for a missing compiler and a broken one alike, and `if()`
  # treats a *-NOTFOUND value as false.
  #
  # **ENV{FC}, not set(CMAKE_Fortran_COMPILER).** check_language is a no-op when
  # CMAKE_Fortran_COMPILER is already DEFINED (Modules/CheckLanguage.cmake) --
  # it does not validate a preset value, it declines to look. Its probe is a
  # nested cmake process that inherits this one's environment, so setting FC
  # around the call is what makes it *try-compile* the candidate. That matters
  # here because a wrapper that cannot compile is a case this actually meets:
  # measured, conda's mpifort on the author's laptop names
  # arm64-apple-darwin20.0.0-gfortran, which is not installed.
  #
  # The whole selection is guarded on CMAKE_Fortran_COMPILER being undefined, so
  # a reconfigure reuses the cache. Changing MPI_C_COMPILER in an existing build
  # directory therefore does *not* re-derive the Fortran compiler -- one build
  # directory per MPI is already the rule (CODE.md #8), and this is one more
  # reason for it.
  include(CheckLanguage)
  if(MPI_ABI_FORTRAN)
    if(MPI_C_COMPILER)
      get_filename_component(MPIABI_MPI_FORTRAN_BINDIR "${MPI_C_COMPILER}" DIRECTORY)
      get_filename_component(MPIABI_MPICC_NAME "${MPI_C_COMPILER}" NAME)
      # mpicc.mpich -> mpifort.mpich, tried before the bare names: Debian's
      # alternatives put both MPIs' wrappers in /usr/bin, so `mpifort` there can
      # belong to the other implementation -- the same mistake NO_DEFAULT_PATH
      # prevents for mpiexec, one directory further in.
      string(REPLACE "mpicc" "mpifort" MPIABI_MPIFORT_NAME "${MPIABI_MPICC_NAME}")
      find_program(MPIABI_MPIFORT
        NAMES ${MPIABI_MPIFORT_NAME} mpifort mpif90 mpifc
        HINTS ${MPIABI_MPI_FORTRAN_BINDIR} NO_DEFAULT_PATH)
    endif()

    if(NOT DEFINED CMAKE_Fortran_COMPILER AND NOT DEFINED ENV{FC} AND MPIABI_MPIFORT)
      set(ENV{FC} "${MPIABI_MPIFORT}")
      check_language(Fortran)
      unset(ENV{FC})
      if(NOT CMAKE_Fortran_COMPILER)
        # A wrapper whose backend compiler is missing. Not fatal, and not
        # silent: the wrapper build is otherwise fine, and the risk this leaves
        # -- our LOGICAL disagreeing with theirs -- is already caught at run
        # time, where hw_abi.c compares the probe's size against
        # PMPI_Type_size(MPI_LOGICAL) and answers "not set" on a mismatch.
        # Unset first, or the fallback is a no-op: check_language declines to
        # look whenever CMAKE_Fortran_COMPILER is DEFINED, and on failure it
        # leaves NOTFOUND behind as *both* a cache entry and a normal variable
        # (Modules/CheckLanguage.cmake), so both have to go.
        unset(CMAKE_Fortran_COMPILER CACHE)
        unset(CMAKE_Fortran_COMPILER)
        message(WARNING
          "${MPIABI_MPIFORT} cannot compile: the wrapped MPI's own Fortran "
          "wrapper names a backend compiler that is not installed. Falling "
          "back to whatever Fortran compiler is on PATH, which need not be the "
          "one this MPI was built with -- decision 25's run-time gate is what "
          "catches a disagreement. Set FC to silence this.")
        check_language(Fortran)
      endif()
    else()
      # An explicit choice (FC or -DCMAKE_Fortran_COMPILER) wins outright, and
      # is not second-guessed; with neither, and no sibling wrapper, this is a
      # plain PATH search. The probe includes no MPI header and links against
      # nothing, so any working Fortran compiler can answer its question -- the
      # sibling is preferred because it answers it *the wrapped MPI's way*.
      check_language(Fortran)
    endif()

    if(CMAKE_Fortran_COMPILER)
      enable_language(Fortran)
      set(MPIABI_HAVE_FORTRAN TRUE)
    else()
      # Unset the cached NOTFOUND, so that installing a compiler and running
      # cmake again in this same directory re-probes instead of re-failing.
      unset(CMAKE_Fortran_COMPILER CACHE)
      unset(CMAKE_Fortran_COMPILER)
      message(FATAL_ERROR
        "No working Fortran compiler, and MPI_ABI_FORTRAN is ON (the default). "
        "The wrapper's Fortran probe answers MPI_Abi_get_fortran_booleans/_info "
        "(decision 25), and a build that quietly loses it reaches a Fortran "
        "consumer as a missing feature rather than as this message. Install "
        "one, name it with FC, or configure with -DMPI_ABI_FORTRAN=OFF to build "
        "without it -- the getters then answer \"not set\", which is legal "
        "(MPI-5.0 20.4.1) and was this project's only answer before S10.")
    endif()

    # **The Fortran runtime is deliberately not linked**, and this is the third
    # and last mechanism it took to keep it out (the other two are at the
    # probe's own target, below). CMake adds these to any target holding a
    # Fortran object, so even with an object library and LINKER_LANGUAGE C the
    # link line still carried -lgfortran -lquadmath. With an mpifort as the
    # compiler this carries more weight than it did: its implicit link libraries
    # name -lmpifort -lmpi -lpmpi as well.
    #
    # Emptying them is safe *because of a property the probe is written to
    # have*: src/mpiwrapper/fortran_probe.f90 uses no I/O, no allocatables and
    # no derived types, and its object's only undefined symbols are malloc, free
    # and memcpy. libmpiwrapper is dlopen'd into a process that already has the
    # application's Fortran runtime -- mpif's, for the consumer this exists to
    # serve -- and a second, differently-versioned copy of one is its own class
    # of failure.
    #
    # If Fortran is ever added here that genuinely needs the runtime, this does
    # not hide it: the link fails with the undefined symbols named, which is the
    # constraint stating itself rather than a silent second libgfortran.
    set(CMAKE_Fortran_IMPLICIT_LINK_LIBRARIES "")
    set(CMAKE_Fortran_IMPLICIT_LINK_DIRECTORIES "")

    # What the wrapped MPI's *own* Fortran interface offers, from FindMPI, and
    # reported rather than consumed: nothing here becomes a compile definition.
    # The C-to-Fortran converters are guarded by the compile probe
    # (MPIWRAPPER_HAVE_MPI_*_c2f), and whether the implementation has Fortran
    # datatypes at all is decided at run time in hw_abi.c, where MPI_LOGICAL can
    # be compared and PMPI_Type_size asked. These lines are for the human
    # reading a configure log: an unexpected answer here is the mismatch that
    # MPI_ABI_FORTRAN exists to make visible.
    #
    # MPI_Fortran_COMPILER is preset, which pins FindMPI's interrogation to it
    # (MPI_PINNED_COMPILER) instead of searching PATH. Unpinned, FindMPI finds
    # whichever MPI comes first on PATH -- measured, and recorded at
    # ci-scripts/test-mpif.sh's configure step. With no sibling wrapper there is
    # nothing to pin, so the call is skipped entirely rather than let loose.
    #
    # Note MPI_DETERMINE_Fortran_CAPABILITIES is deliberately left unset: it is
    # the one part of FindMPI's Fortran component that try_run()s, and NOTES.md
    # #1 permits no configure-time run test. The HAVE_* answers below are
    # try_compile only.
    if(MPIABI_MPIFORT)
      set(MPI_Fortran_COMPILER "${MPIABI_MPIFORT}"
        CACHE FILEPATH "MPI compiler wrapper for Fortran")
      find_package(MPI OPTIONAL_COMPONENTS Fortran)
      if(MPI_Fortran_FOUND)
        # A FALSE for either module can mean two things, and the configure log
        # cannot tell them apart: the binding is absent, or our Fortran compiler
        # cannot read that MPI's .mod files. Both are worth seeing; the second
        # is another reason to prefer the sibling wrapper, which by construction
        # can read them.
        message(STATUS
          "Wrapped MPI's Fortran interface, via ${MPI_Fortran_COMPILER}: "
          "mpif.h=${MPI_Fortran_HAVE_F77_HEADER} "
          "'use mpi'=${MPI_Fortran_HAVE_F90_MODULE} "
          "'use mpi_f08'=${MPI_Fortran_HAVE_F08_MODULE}")
      else()
        message(WARNING
          "${MPIABI_MPIFORT} exists, but FindMPI could not compile an MPI "
          "Fortran program with it, so the wrapped MPI's Fortran bindings are "
          "unverified. This does not stop the build: the wrapper's own Fortran "
          "surface is the C-to-Fortran converters, which the compile probe "
          "guards, plus decision 25's probe, which needs no MPI at all.")
      endif()
    else()
      # Not an error, and an expected configuration: Open MPI built
      # --enable-standard-abi installs no Fortran wrapper at all, and its mpi.h
      # declares no MPI_Fint either (NOTES.md #10). That is oracle 5, and it is
      # a configuration this project supports on purpose -- which is why the
      # Fortran *component* is optional even while the Fortran *compiler* is
      # required.
      message(STATUS
        "No Fortran compiler wrapper beside ${MPI_C_COMPILER}, so the wrapped "
        "MPI appears to have no Fortran interface. Decision 25's probe answers "
        "from ${CMAKE_Fortran_COMPILER} instead, and the run-time gate in "
        "hw_abi.c decides whether that answer may be given.")
    endif()
  else()
    set(MPIABI_HAVE_FORTRAN FALSE)
    message(STATUS
      "MPI_ABI_FORTRAN is OFF: no Fortran compiler is looked for and no probe "
      "is built, so MPI_Abi_get_fortran_booleans/_info answer \"not set\" "
      "unless the application registers values (MPI-5.0 20.4.1)")
  endif()

  # mpiabi.h reaches the wrapper through a directory of its own, containing
  # nothing else. That is not tidiness: gen/include holds the ABI's *mpi.h*
  # beside mpiabi.h, so putting gen/include on this target's include path makes
  # `#include <mpi.h>` in internal.h liable to find the ABI header instead of
  # the implementation's. It cannot be fixed by ordering -- CMake passes an
  # imported target's includes as -isystem, and every -I is searched before
  # every -isystem, so our directory wins however the arguments are ordered.
  # The resulting build compiles and links (the ABI header is a complete,
  # valid mpi.h and the implementation exports the names) and then fails at run
  # time with the implementation rejecting comm=0x101, which is
  # MPIABI_COMM_WORLD arriving unconverted. S1 spent an hour on exactly that.
  # internal.h carries an #error for the same mistake, as a backstop for
  # anyone building this outside CMake.
  set(MPIABI_WRAPPER_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/wrapper-include)
  foreach(hdr mpiabi.h mpiwrapper_vtable.h)
    add_custom_command(
      OUTPUT ${MPIABI_WRAPPER_INCLUDE_DIR}/${hdr}
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              ${MPIABI_GEN_INCLUDE_DIR}/${hdr}
              ${MPIABI_WRAPPER_INCLUDE_DIR}/${hdr}
      DEPENDS ${MPIABI_GEN_INCLUDE_DIR}/${hdr}
      COMMENT "Staging ${hdr} where no mpi.h can shadow the implementation's")
  endforeach()

  # Every guard in the generated sources tests one of this file's
  # MPIWRAPPER_HAVE_<name>, and the only exact answer comes from asking the
  # compiler about the implementation's own header. MPI_VERSION is a proxy that
  # both over- and under-reports (Open MPI 5.0.10 says MPI-3.1 and has
  # sessions); nm misses what the header provides as a macro; and `#ifdef` on
  # the implementation's own name misses what it provides as an *enumerator*,
  # which is how MPICH provides MPI_COMBINER_* and Open MPI MPI_THREAD_*. The
  # probe is compile-only, so this stays cross-compilable, and it costs well
  # under a second because all ~600 questions go into one translation unit --
  # not one configure test per name.
  # The conversion runtime and the hand-written entry-point bodies, named once
  # rather than in each of the three targets that need them -- and named here
  # because the availability probe below reads the same files for the
  # MPIWRAPPER_HAVE_* guards they test.
  set(MPIABI_WRAPPER_RUNTIME_SOURCES
    src/mpiwrapper/handles.c
    src/mpiwrapper/status.c
    src/mpiwrapper/staging.c
    src/mpiwrapper/extents.c
    src/mpiwrapper/keyvals.c
    src/mpiwrapper/toolobj.c
    src/mpiwrapper/callbacks.c
    src/mpiwrapper/serialize.c
    src/mpiwrapper/errorcodes.c
    src/mpiwrapper/buffers.c
    src/mpiwrapper/extrastate.c
    src/mpiwrapper/toolevents.c
  )
  set(MPIABI_WRAPPER_BODY_SOURCES
    src/mpiwrapper/hw_lifecycle.c
    src/mpiwrapper/hw_callbacks.c
    src/mpiwrapper/hw_buffers.c
    src/mpiwrapper/hw_errors.c
    src/mpiwrapper/hw_spawn.c
    src/mpiwrapper/hw_pcontrol.c
    src/mpiwrapper/hw_converters.c
    src/mpiwrapper/hw_status.c
    src/mpiwrapper/hw_strings.c
    src/mpiwrapper/hw_abi.c
    src/mpiwrapper/hw_attr.c
  )

  set(MPIABI_IMPL_CONFIG ${MPIABI_WRAPPER_INCLUDE_DIR}/mpiwrapper_impl_config.h)
  set(MPIABI_PROBE_FLAGS)
  foreach(dir ${MPI_C_INCLUDE_DIRS})
    list(APPEND MPIABI_PROBE_FLAGS "--flag=-I${dir}")
  endforeach()
  foreach(opt ${MPI_C_COMPILE_OPTIONS})
    list(APPEND MPIABI_PROBE_FLAGS "--flag=${opt}")
  endforeach()
  add_custom_command(
    OUTPUT ${MPIABI_IMPL_CONFIG}
    COMMAND ${Python3_EXECUTABLE} ${MPIABI_PROBE_SCRIPT}
            --cc ${CMAKE_C_COMPILER} ${MPIABI_PROBE_FLAGS}
            --out ${MPIABI_IMPL_CONFIG}
    # The probe reads every MPIWRAPPER_HAVE_* the sources mention, and that is
    # both directories: a hand-written body is subject to decision 6 like a
    # generated one, so its guards have to be asked about too.
    DEPENDS ${MPIABI_PROBE_SCRIPT}
            ${CMAKE_SOURCE_DIR}/gen/mpiwrapper/wrappers.c
            ${CMAKE_SOURCE_DIR}/gen/mpiwrapper/constants.c
            ${MPIABI_WRAPPER_RUNTIME_SOURCES}
            ${MPIABI_WRAPPER_BODY_SOURCES}
            ${CMAKE_SOURCE_DIR}/dev/entrypoints.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Probing which MPI entry points this implementation declares")

  add_custom_target(mpiabi_h_for_wrapper
    DEPENDS ${MPIABI_WRAPPER_INCLUDE_DIR}/mpiabi.h
            ${MPIABI_WRAPPER_INCLUDE_DIR}/mpiwrapper_vtable.h
            ${MPIABI_IMPL_CONFIG})

  # The Fortran probe, when there is a compiler for it (decision 25). Its
  # object pulls in no Fortran runtime -- see the source -- so the link stays
  # a C link and the version script, visibility and rpath rules below are
  # untouched. LINKER_LANGUAGE is pinned to C for the same reason: CMake would
  # otherwise promote the link to Fortran on account of one .f90, putting
  # libgfortran on the link line of a library that must not carry one.
  #
  # **An OBJECT library, not a source in the target**, and that is not a style
  # choice: CMake adds CMAKE_Fortran_IMPLICIT_LINK_LIBRARIES to any target that
  # has a Fortran source, so naming the .f90 directly put libgfortran and
  # libquadmath into libmpiwrapper's DT_NEEDED -- measured with otool -L, and
  # exactly the second Fortran runtime in one process that the probe's own
  # header warns about, since the application's Fortran layer brings its own.
  # LINKER_LANGUAGE C does not prevent it. An object library is compiled and
  # its objects handed over, so the wrapper's link line stays a C link.
  #
  # Hidden visibility for the same reason the C half has it: bind(C) gives the
  # subroutine default visibility, and `exported-symbols` caught it beside
  # mpiwrapper_get_vtable. C_VISIBILITY_PRESET does not reach Fortran, and on
  # macOS there is no version script to catch it either.
  if(MPIABI_HAVE_FORTRAN)
    add_library(mpiwrapper_fortran_probe OBJECT src/mpiwrapper/fortran_probe.f90)
    set_target_properties(mpiwrapper_fortran_probe PROPERTIES
      POSITION_INDEPENDENT_CODE ON
      Fortran_VISIBILITY_PRESET hidden)
    set(MPIABI_WRAPPER_FORTRAN_SOURCES
      $<TARGET_OBJECTS:mpiwrapper_fortran_probe>)
    message(STATUS
      "Fortran probe: ${CMAKE_Fortran_COMPILER_ID} "
      "(${CMAKE_Fortran_COMPILER}) -- MPI_Abi_get_fortran_* will answer from it")
  else()
    set(MPIABI_WRAPPER_FORTRAN_SOURCES)
  endif()

  add_library(mpiwrapper SHARED
    gen/mpiwrapper/constants.c
    ${MPIABI_WRAPPER_RUNTIME_SOURCES}
    ${MPIABI_WRAPPER_BODY_SOURCES}
    ${MPIABI_WRAPPER_FORTRAN_SOURCES}
    gen/mpiwrapper/wrappers.c
    src/mpiwrapper/getvtable.c
  )
  # The banner src/mpiwrapper/hw_strings.c puts in front of the wrapped
  # library's MPI_Get_library_version string. PROJECT_VERSION rather than
  # MPIABI_SOVERSION: this one names *the wrapper's* release, which is what a
  # bug report needs and the one number in that banner not derivable from the
  # ABI header.
  target_compile_definitions(mpiwrapper PRIVATE
    "MPIWRAPPER_VERSION=\"${PROJECT_VERSION}\"")

  if(MPIABI_HAVE_FORTRAN)
    # Pinned, because CMake picks the "highest" language among a target's
    # objects to drive the link -- and with one Fortran object that is
    # gfortran, which puts its own runtime on the link line. Measured: the
    # generated link.txt began with gfortran-mp-15 and otool -L then showed
    # libgfortran and libquadmath. The object library alone does not prevent
    # this; the objects carry their language with them.
    set_target_properties(mpiwrapper PROPERTIES LINKER_LANGUAGE C)
    target_compile_definitions(mpiwrapper PRIVATE MPIWRAPPER_HAVE_FORTRAN)
  endif()
  target_include_directories(mpiwrapper PRIVATE
    ${MPIABI_WRAPPER_INCLUDE_DIR} src/mpiwrapper)
  add_dependencies(mpiwrapper mpiabi_h_for_wrapper)
  target_compile_options(mpiwrapper PRIVATE ${MPIABI_WARNINGS}
    ${MPIABI_FORWARDS_DEPRECATED})
  target_link_libraries(mpiwrapper PRIVATE MPI::MPI_C ${CMAKE_DL_LIBS})
  if(MPI_ABI_WRAP_ABI_IMPL)
    target_compile_definitions(mpiwrapper PRIVATE MPIWRAPPER_WRAP_ABI_IMPL)
  endif()
  # Exactly one exported symbol, enforced by -fvisibility=hidden plus the
  # explicit attribute in mpiwrapper_vtable.h, and checked by the nm test below.
  set_target_properties(mpiwrapper PROPERTIES C_VISIBILITY_PRESET hidden)
  # The other half of NOTES.md #9's rule, which this target was missing while
  # libmpi_abi had it: visibility cannot touch the symbols the linker inserts
  # into every shared object. On GNU/Linux hidden visibility alone happened to
  # be enough; on FreeBSD it was not, and `exported-symbols` reported
  # libmpiwrapper exporting _init and _fini beside mpiwrapper_get_vtable.
  # Mach-O needs no counterpart -- see the comment in the script.
  if(NOT APPLE)
    target_link_options(mpiwrapper PRIVATE
      "-Wl,--version-script=${CMAKE_SOURCE_DIR}/cmake/mpiwrapper.version")
    set_target_properties(mpiwrapper PROPERTIES
      LINK_DEPENDS ${CMAKE_SOURCE_DIR}/cmake/mpiwrapper.version)
  else()
    # There *is* a Mach-O counterpart now, and decision 25's Fortran probe is
    # why: a bind(C) subroutine is exported whatever -fvisibility=hidden says,
    # so on macOS nothing but this restricts the set. See the file's comment.
    target_link_options(mpiwrapper PRIVATE
      "-Wl,-exported_symbols_list,${CMAKE_SOURCE_DIR}/cmake/mpiwrapper.exported_symbols")
    set_target_properties(mpiwrapper PROPERTIES
      LINK_DEPENDS ${CMAKE_SOURCE_DIR}/cmake/mpiwrapper.exported_symbols)
  endif()
  # This library is never linked, only dlopen'd (MPI-5.0 20.2.1), so nothing
  # ever asks the loader to resolve *it* by rpath -- but it still has to
  # resolve its own MPI_C_LIBRARIES at the moment it is dlopen'd, and once
  # installed into a prefix of its own, well away from the MPI it wraps
  # (NOTES.md #9's "the prefix is exclusive"), the build tree's rpath no
  # longer applies. INSTALL_RPATH_USE_LINK_PATH bakes an absolute rpath to
  # every linked library's own directory -- here, just the MPI found above --
  # into the installed .so/.dylib, so LD_LIBRARY_PATH/DYLD_LIBRARY_PATH need
  # not name it either.
  set_target_properties(mpiwrapper PROPERTIES
    BUILD_WITH_INSTALL_RPATH FALSE
    INSTALL_RPATH_USE_LINK_PATH TRUE)
  install(TARGETS mpiwrapper LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

  # The wrapper the tests load, unless MPI_ABI_WRAPPER_LIB says otherwise.
  set(MPIABI_WRAPPER_PATH $<TARGET_FILE:mpiwrapper>)

  # A deliberately *unisolated* wrapper, so that the outward-resolution check can
  # be shown to fire rather than merely to exist. On macOS the way to defeat the
  # isolation is -flat_namespace, which is the one row of dev/dlopen-probe's
  # table where macOS stops being safe; on Linux the same library loaded without
  # RTLD_DEEPBIND does it, so no second target is needed there.
  if(APPLE)
    add_library(mpiwrapper_unisolated SHARED
      gen/mpiwrapper/constants.c
      ${MPIABI_WRAPPER_RUNTIME_SOURCES}
      ${MPIABI_WRAPPER_BODY_SOURCES}
      ${MPIABI_WRAPPER_FORTRAN_SOURCES}
      gen/mpiwrapper/wrappers.c
      src/mpiwrapper/getvtable.c
    )
    target_compile_definitions(mpiwrapper_unisolated PRIVATE
      "MPIWRAPPER_VERSION=\"${PROJECT_VERSION}\"")
    if(MPIABI_HAVE_FORTRAN)
      set_target_properties(mpiwrapper_unisolated PROPERTIES LINKER_LANGUAGE C)
      target_compile_definitions(mpiwrapper_unisolated PRIVATE
        MPIWRAPPER_HAVE_FORTRAN)
    endif()
    target_include_directories(mpiwrapper_unisolated PRIVATE
      ${MPIABI_WRAPPER_INCLUDE_DIR} src/mpiwrapper)
    add_dependencies(mpiwrapper_unisolated mpiabi_h_for_wrapper)
    target_compile_options(mpiwrapper_unisolated PRIVATE ${MPIABI_WARNINGS}
      ${MPIABI_FORWARDS_DEPRECATED})
    target_link_libraries(mpiwrapper_unisolated PRIVATE MPI::MPI_C ${CMAKE_DL_LIBS})
    if(MPI_ABI_WRAP_ABI_IMPL)
      target_compile_definitions(mpiwrapper_unisolated PRIVATE
        MPIWRAPPER_WRAP_ABI_IMPL)
    endif()
    target_link_options(mpiwrapper_unisolated PRIVATE
      -Wl,-flat_namespace -Wl,-undefined,dynamic_lookup)
    set(MPIABI_UNISOLATED_WRAPPER $<TARGET_FILE:mpiwrapper_unisolated>)
    set(MPIABI_CAPTURE_MODE)
  else()
    set(MPIABI_UNISOLATED_WRAPPER $<TARGET_FILE:mpiwrapper>)
    set(MPIABI_CAPTURE_MODE -DCAPTURE_MODE=1)
  endif()

  # FindMPI takes mpiexec from PATH, which is wrong the moment a second MPI is
  # installed: launching an Open MPI binary under MPICH's hydra produces two
  # *singletons* rather than a two-rank job, and the test then reports "needs
  # exactly 2 ranks, got 1" instead of anything about MPI. The launcher beside
  # the compiler wrapper we were pointed at is the one that matches it.
  get_filename_component(MPIABI_MPI_BINDIR ${MPI_C_COMPILER} DIRECTORY)
  find_program(MPIABI_MPIEXEC NAMES mpiexec mpirun
    HINTS ${MPIABI_MPI_BINDIR} NO_DEFAULT_PATH)
  if(MPIABI_MPIEXEC)
    set(MPIEXEC_EXECUTABLE ${MPIABI_MPIEXEC})
  endif()
  message(STATUS "Launching tests with ${MPIEXEC_EXECUTABLE}")

  # ------------------------------------------------------------------ tests

  # White box: the conversion runtime compiled straight into the test, so the
  # maps can be walked in both directions rather than inferred from results.
  add_executable(mpiwrapper_selftest
    test/mpiwrapper_selftest.c
    gen/mpiwrapper/constants.c
    ${MPIABI_WRAPPER_RUNTIME_SOURCES}
  )
  target_include_directories(mpiwrapper_selftest PRIVATE
    ${MPIABI_WRAPPER_INCLUDE_DIR} src/mpiwrapper)
  add_dependencies(mpiwrapper_selftest mpiabi_h_for_wrapper)
  target_compile_options(mpiwrapper_selftest PRIVATE ${MPIABI_WARNINGS})
  target_link_libraries(mpiwrapper_selftest PRIVATE MPI::MPI_C ${CMAKE_DL_LIBS})
  if(MPI_ABI_WRAP_ABI_IMPL)
    target_compile_definitions(mpiwrapper_selftest PRIVATE MPIWRAPPER_WRAP_ABI_IMPL)
  endif()

  # Normally the tests run under the implementation's own launcher. Turning that
  # off runs the binaries directly, as singletons, which is not a stylistic
  # choice: an implementation whose launcher does not work on the machine at hand
  # would otherwise contribute no behavioural coverage at all, and singleton
  # initialization is a supported MPI configuration. Open MPI 5.0.x on macOS 26
  # is exactly that case -- see test/README.md.
  option(MPI_ABI_TEST_USE_LAUNCHER "Run the tests under mpiexec" ON)
  if(MPI_ABI_TEST_USE_LAUNCHER)
    set(MPIABI_RUN1 ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 1
                    ${MPIEXEC_PREFLAGS})
    set(MPIABI_RUN2 ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2
                    ${MPIEXEC_PREFLAGS})
    set(MPIABI_RUN_POST ${MPIEXEC_POSTFLAGS})
    set(MPIABI_EXPECT_RANKS 2)
  else()
    set(MPIABI_RUN1)
    set(MPIABI_RUN2)
    set(MPIABI_RUN_POST)
    set(MPIABI_EXPECT_RANKS 1)
  endif()

  # Asking for two ranks and being handed two singletons is a green run that
  # never crossed a rank boundary, and the tests cannot tell the difference on
  # their own: one rank is a supported configuration, so each singleton simply
  # passes. The build knows what it asked the launcher for, so it says so, and
  # test/expect_ranks.h turns a mismatch into a failure. The MPICH row of the
  # Linux container had been running that way unnoticed (HISTORY.md 2.14).
  set(MPIABI_TEST_ENV
    "MPI_ABI_WRAPPER_LIB=${MPIABI_WRAPPER_PATH}"
    "MPI_ABI_EXPECT_RANKS=${MPIABI_EXPECT_RANKS}")

  add_test(NAME mpiwrapper_selftest
    COMMAND ${MPIABI_RUN1} $<TARGET_FILE:mpiwrapper_selftest> ${MPIABI_RUN_POST})

  # Black box: an ordinary MPI application against the ABI header, linking
  # libmpi_abi and nothing else.
  add_executable(abi_prototype_test test/abi_prototype_test.c)
  target_include_directories(abi_prototype_test PRIVATE ${MPIABI_GEN_INCLUDE_DIR})
  target_compile_options(abi_prototype_test PRIVATE ${MPIABI_WARNINGS})
  target_link_libraries(abi_prototype_test PRIVATE mpi_abi)

  add_test(NAME abi_prototype_test
    COMMAND ${MPIABI_RUN2} $<TARGET_FILE:abi_prototype_test> ${MPIABI_RUN_POST})
  set_tests_properties(abi_prototype_test PROPERTIES
    ENVIRONMENT "${MPIABI_TEST_ENV}")

  # The same, for S3's argument classes -- and above all for the lifetime of a
  # staged temporary, which is the property none of the generator's own
  # assertions can see.
  add_executable(abi_arrays_test test/abi_arrays_test.c)
  target_include_directories(abi_arrays_test PRIVATE ${MPIABI_GEN_INCLUDE_DIR})
  target_compile_options(abi_arrays_test PRIVATE ${MPIABI_WARNINGS})
  target_link_libraries(abi_arrays_test PRIVATE mpi_abi)

  add_test(NAME abi_arrays_test
    COMMAND ${MPIABI_RUN2} $<TARGET_FILE:abi_arrays_test> ${MPIABI_RUN_POST})
  set_tests_properties(abi_arrays_test PROPERTIES
    ENVIRONMENT "${MPIABI_TEST_ENV}")

  # The ABI's large-count half, which over an implementation lacking the `_c`
  # entry points is the narrowing fallback and over one that has them is the
  # implementation itself (NOTES.md #5.10). The test is written to pass
  # identically either way -- the `_c` form and its small twin must agree --
  # which is what makes it an oracle rather than a description, and which is
  # why it is one test rather than a per-implementation pair.
  #
  # Two ranks matter here more than elsewhere: at one rank a vector collective
  # has nothing to exchange, the nonblocking forms complete on return, and the
  # staged block that has to outlive the call is never entered into the
  # request table at all.
  add_executable(abi_large_count_test test/abi_large_count_test.c)
  target_include_directories(abi_large_count_test
    PRIVATE ${MPIABI_GEN_INCLUDE_DIR})
  target_compile_options(abi_large_count_test PRIVATE ${MPIABI_WARNINGS})
  target_link_libraries(abi_large_count_test PRIVATE mpi_abi)

  add_test(NAME abi_large_count_test
    COMMAND ${MPIABI_RUN2} $<TARGET_FILE:abi_large_count_test>
            ${MPIABI_RUN_POST})
  set_tests_properties(abi_large_count_test PROPERTIES
    ENVIRONMENT "${MPIABI_TEST_ENV}")

  # The same for S3's second half: keyvals, output-string buffers with an
  # explicit length, MPI_T's handle and enumerated classes, and the obj_handle
  # whose class comes from a query rather than from the signature. Its sharpest
  # case is the null OUT pointer MPI_T permits everywhere, which no generator
  # assertion can see either -- a body that copies back unconditionally writes
  # through it.
  add_executable(abi_tools_test test/abi_tools_test.c)
  target_include_directories(abi_tools_test PRIVATE ${MPIABI_GEN_INCLUDE_DIR})
  target_compile_options(abi_tools_test PRIVATE ${MPIABI_WARNINGS})
  target_link_libraries(abi_tools_test PRIVATE mpi_abi)

  add_test(NAME abi_tools_test
    COMMAND ${MPIABI_RUN2} $<TARGET_FILE:abi_tools_test> ${MPIABI_RUN_POST})
  set_tests_properties(abi_tools_test PROPERTIES
    ENVIRONMENT "${MPIABI_TEST_ENV}")

  # The same for S4a: the converter face. The two properties it covers that no
  # assertion in the generator can see are that _toint answers a predefined
  # handle with the ABI's own value -- which the standard requires and a
  # round trip cannot detect -- and that a status converted to the Fortran form
  # and back still answers MPI_Get_count, which is only true if the private
  # bytes travelled with it.
  add_executable(abi_converters_test test/abi_converters_test.c)
  target_include_directories(abi_converters_test PRIVATE
    ${MPIABI_GEN_INCLUDE_DIR})
  target_compile_options(abi_converters_test PRIVATE ${MPIABI_WARNINGS})
  target_link_libraries(abi_converters_test PRIVATE mpi_abi)

  add_test(NAME abi_converters_test
    COMMAND ${MPIABI_RUN2} $<TARGET_FILE:abi_converters_test> ${MPIABI_RUN_POST})
  set_tests_properties(abi_converters_test PROPERTIES
    ENVIRONMENT "${MPIABI_TEST_ENV}")

  # The same for S4b: the state the wrapper owns. What it covers that no
  # assertion in the generator can see is that a trampoline hands the
  # application *its own* handle back -- each of the four error-handler classes
  # checks the handle against the object it was set on -- that
  # MPI_COMM_DUP_FN reaches the implementation as the implementation's own
  # function rather than as the ABI's (function *)0x1, that a generalized
  # request's status survives the round trip through the query callback
  # (MPI_Get_count is what a body copying only the named fields fails), that
  # MPI_BUFFER_AUTOMATIC comes back out of a detach on an implementation that
  # has the mode and on one where the wrapper emulates it, and that a dynamic
  # error class is renumbered into the ABI's range. It is also the only test
  # that can ask MPI_Initialized before MPI_Init and MPI_Finalized after
  # MPI_Finalize.
  add_executable(abi_state_test test/abi_state_test.c)
  target_include_directories(abi_state_test PRIVATE ${MPIABI_GEN_INCLUDE_DIR})
  target_compile_options(abi_state_test PRIVATE ${MPIABI_WARNINGS})
  target_link_libraries(abi_state_test PRIVATE mpi_abi)

  # Spawn is the one case that is off by default, and the reason is measured
  # rather than cautious: MPI_Comm_spawn hangs under MPICH 4.3.1's hydra on
  # macOS 26 with no wrapper involved -- a 15-line C program does the same --
  # so a test that called it unconditionally would hang ctest instead of
  # failing it. The same program under MPICH in a Linux container returns a
  # clean error, which the test skips on. Turn this on where spawn works.
  option(MPI_ABI_TEST_SPAWN
    "Run abi_state_test's MPI_Comm_spawn case (needs a launcher that spawns)"
    OFF)
  set(MPIABI_STATE_TEST_ENV ${MPIABI_TEST_ENV})
  if(MPI_ABI_TEST_SPAWN)
    list(APPEND MPIABI_STATE_TEST_ENV "MPI_ABI_TEST_SPAWN=1")
  endif()

  add_test(NAME abi_state_test
    COMMAND ${MPIABI_RUN2} $<TARGET_FILE:abi_state_test> ${MPIABI_RUN_POST})
  set_tests_properties(abi_state_test PROPERTIES
    ENVIRONMENT "${MPIABI_STATE_TEST_ENV}")

  # 20.2.1 wants libmpi_abi to be the application's only direct MPI dependency,
  # and oracles 1 and 2 want each library's export set to be exactly what the
  # design says. Both are one nm invocation.
  # dev/dlopen-probe's conclusions, re-run on the real libraries.
  add_test(NAME isolation-check
    COMMAND ${CMAKE_COMMAND}
      -DTEST_BIN=$<TARGET_FILE:abi_prototype_test>
      -DWRAPPER=${MPIABI_UNISOLATED_WRAPPER}
      ${MPIABI_CAPTURE_MODE}
      -P ${CMAKE_SOURCE_DIR}/test/check_isolation.cmake)

  # The two legs of the export check that need a wrapper to look at. The
  # libmpi_abi leg needs no MPI and is registered outside this block, below.
  set(MPIABI_EXPORTS_WRAPPER_ARGS
    -DWRAPPER_LIB=$<TARGET_FILE:mpiwrapper>
    -DTEST_BIN=$<TARGET_FILE:abi_prototype_test>)
endif()

# NOTES.md #10 calls oracle 1 "a total completeness check as the cheapest job in
# CI", and it is only cheap if it needs no MPI. Its subject is libmpi_abi, which
# is built unconditionally and whose 1376 exports are a property of the ABI
# header alone -- so this test is registered here rather than inside the block
# above, and runs in the -DMPI_ABI_BUILD_WRAPPER=OFF developer build too. The
# two legs that do need a wrapper are passed only when there is one, and
# check_exports.cmake says in its summary which legs it actually ran, so a
# one-leg pass cannot be read as the full three-leg oracle.
add_test(NAME exported-symbols
  COMMAND ${CMAKE_COMMAND}
    -DABI_LIB=$<TARGET_FILE:mpi_abi>
    -DENTRYPOINTS=${CMAKE_SOURCE_DIR}/dev/entrypoints.txt
    ${MPIABI_EXPORTS_WRAPPER_ARGS}
    -P ${CMAKE_SOURCE_DIR}/test/check_exports.cmake)

# Only in a sanitizer build, and it is not ceremony: an uninstrumented build
# passes every other test in this file, so nothing else can tell one from a
# correctly instrumented one. Reads the object code.
if(MPI_ABI_SANITIZE)
  set(MPIABI_SANITIZE_LIBS $<TARGET_FILE:mpi_abi>)
  if(MPI_ABI_BUILD_WRAPPER)
    list(APPEND MPIABI_SANITIZE_LIBS $<TARGET_FILE:mpiwrapper>)
  endif()
  add_test(NAME sanitizer-instrumented
    COMMAND ${CMAKE_COMMAND}
      "-DLIBS=${MPIABI_SANITIZE_LIBS}"
      -DSANITIZE=${MPI_ABI_SANITIZE}
      -P ${CMAKE_SOURCE_DIR}/test/check_sanitizer.cmake)
endif()
