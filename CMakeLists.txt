cmake_minimum_required(VERSION 3.20)
project(mpi_abi_wrapper
  VERSION 0.1.0
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

# `-DMPI_ABI_BUILD_WRAPPER=OFF` builds libmpi_abi alone, which needs no MPI at
# all. That is a developer option, not part of the normal flow: a user
# configures against an MPI and gets a working mpi.h, libmpi_abi, libmpiwrapper
# and (from S6) compiler wrapper and package files, without having to know that
# one half does not need MPI (NOTES.md #9).
option(MPI_ABI_BUILD_WRAPPER "Build libmpiwrapper (requires an MPI)" ON)

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

  add_library(mpiwrapper SHARED
    gen/mpiwrapper/constants.c
    ${MPIABI_WRAPPER_RUNTIME_SOURCES}
    ${MPIABI_WRAPPER_BODY_SOURCES}
    gen/mpiwrapper/wrappers.c
    src/mpiwrapper/getvtable.c
  )
  target_include_directories(mpiwrapper PRIVATE
    ${MPIABI_WRAPPER_INCLUDE_DIR} src/mpiwrapper)
  add_dependencies(mpiwrapper mpiabi_h_for_wrapper)
  target_compile_options(mpiwrapper PRIVATE ${MPIABI_WARNINGS})
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
      gen/mpiwrapper/wrappers.c
      src/mpiwrapper/getvtable.c
    )
    target_include_directories(mpiwrapper_unisolated PRIVATE
      ${MPIABI_WRAPPER_INCLUDE_DIR} src/mpiwrapper)
    add_dependencies(mpiwrapper_unisolated mpiabi_h_for_wrapper)
    target_compile_options(mpiwrapper_unisolated PRIVATE ${MPIABI_WARNINGS})
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
