cmake_minimum_required(VERSION 3.20)
project(mpi_abi_wrapper C)

# S0 built the two headers; S1 adds the prototype -- libmpi_abi, libmpiwrapper
# and the tests for 28 entry points (STAGES.md). The dev/ generator core is S2,
# packaging is S6.

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_VISIBILITY_PRESET hidden)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(MPIABI_GEN_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/gen/include)
set(MPIABI_SRC_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/include)
set(MPIABI_GENERATE_HEADERS_SCRIPT ${CMAKE_SOURCE_DIR}/dev/generate_headers.py)
set(MPIABI_LAYOUT_HASH_SCRIPT ${CMAKE_SOURCE_DIR}/dev/layout_hash.py)

# `-DMPI_ABI_BUILD_WRAPPER=OFF` builds libmpi_abi alone, which needs no MPI at
# all. That is a developer option, not part of the normal flow: a user
# configures against an MPI and gets a working mpi.h, libmpi_abi, libmpiwrapper
# and (from S6) compiler wrapper and package files, without having to know that
# one half does not need MPI (NOTES.md #9).
option(MPI_ABI_BUILD_WRAPPER "Build libmpiwrapper (requires an MPI)" ON)

set(MPIABI_WARNINGS -Wall -Wextra -Wpedantic -Werror)

# Generated output is committed (NOTES.md #9): `regenerate` writes it, and is
# never part of the default build. CI instead runs the check tests below, which
# fail on any drift between the committed files and a fresh generation.
add_custom_target(regenerate
  COMMAND ${Python3_EXECUTABLE} ${MPIABI_GENERATE_HEADERS_SCRIPT}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Regenerating gen/include/mpi.h, gen/include/mpiabi.h, dev/entrypoints.txt"
)

enable_testing()

add_test(NAME headers-up-to-date
  COMMAND ${Python3_EXECUTABLE} ${MPIABI_GENERATE_HEADERS_SCRIPT} --check
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# The layout hash has to track the slot list it claims to summarize, or the
# handshake it exists for degrades into a constant nobody updates.
add_test(NAME layout-hash
  COMMAND ${Python3_EXECUTABLE} ${MPIABI_LAYOUT_HASH_SCRIPT} --check
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
  src/mpi_abi/entrypoints.c
)
set_target_properties(mpi_abi PROPERTIES OUTPUT_NAME mpi_abi)
target_include_directories(mpi_abi PRIVATE
  ${MPIABI_GEN_INCLUDE_DIR} ${MPIABI_SRC_INCLUDE_DIR})
target_compile_options(mpi_abi PRIVATE ${MPIABI_WARNINGS})
# Every MPI_*/PMPI_* entry point is exported and nothing else is: the ABI
# functions carry default visibility from the ABI header, and the one internal
# global is marked hidden in bootstrap.c.
set_target_properties(mpi_abi PROPERTIES C_VISIBILITY_PRESET default)

if(APPLE)
  set(MPIABI_WRAPPER_SUFFIX .dylib)
else()
  set(MPIABI_WRAPPER_SUFFIX .so)
endif()

# --------------------------------------------------------------- libmpiwrapper

if(MPI_ABI_BUILD_WRAPPER)
  find_package(MPI REQUIRED COMPONENTS C)

  # Configure-time checks, all compile-only so that cross-compiling works: this
  # project may never run a program at configure time (NOTES.md #1, #9).
  include(CheckCSourceCompiles)
  set(CMAKE_REQUIRED_INCLUDES ${MPI_C_INCLUDE_DIRS})
  set(CMAKE_REQUIRED_FLAGS ${MPI_C_COMPILE_OPTIONS})

  # No self-wrapping. The marker is mpiwrapper_vtable.h, a file only we install:
  # neither MPI_ABI_VERSION nor the library name discriminates, because MPI-5.0
  # 20.2.1 requires a genuine ABI implementation to install a libmpi_abi too.
  # The accident is easy to hit -- find_package(MPI) is looking for exactly the
  # mpi.h and libmpi_abi we install -- and its symptom is a startup loop rather
  # than a diagnostic.
  find_file(MPIABI_SELF_WRAP_MARKER mpiwrapper_vtable.h
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
  # makes the common case a 1:1 mapping with no large-count fallback -- but it
  # is an expectation and not a floor (NOTES.md #1). Open MPI 5.0.10 reports
  # MPI-3.1 and has no _c entry points at all, and it is a perfectly good
  # implementation to wrap: the _c slots become the runtime-reporting stubs
  # decision 6 describes. So this warns rather than refuses, and refuses only
  # below MPI-3.0, where the ABI's own surface stops being expressible.
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
    message(WARNING
      "The MPI at ${MPI_C_INCLUDE_DIRS} predates MPI-4.0, so it has no _c "
      "entry points. Those ABI slots will return "
      "MPI_ERR_UNSUPPORTED_OPERATION; everything else works normally.")
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
  add_custom_command(
    OUTPUT ${MPIABI_WRAPPER_INCLUDE_DIR}/mpiabi.h
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${MPIABI_GEN_INCLUDE_DIR}/mpiabi.h
            ${MPIABI_WRAPPER_INCLUDE_DIR}/mpiabi.h
    DEPENDS ${MPIABI_GEN_INCLUDE_DIR}/mpiabi.h
    COMMENT "Staging mpiabi.h where no mpi.h can shadow the implementation's")
  add_custom_target(mpiabi_h_for_wrapper
    DEPENDS ${MPIABI_WRAPPER_INCLUDE_DIR}/mpiabi.h)

  add_library(mpiwrapper SHARED
    src/mpiwrapper/constants.c
    src/mpiwrapper/handles.c
    src/mpiwrapper/status.c
    src/mpiwrapper/staging.c
    src/mpiwrapper/callbacks.c
    src/mpiwrapper/handwritten.c
    src/mpiwrapper/wrappers.c
    src/mpiwrapper/getvtable.c
  )
  target_include_directories(mpiwrapper PRIVATE
    ${MPIABI_WRAPPER_INCLUDE_DIR} ${MPIABI_SRC_INCLUDE_DIR} src/mpiwrapper)
  add_dependencies(mpiwrapper mpiabi_h_for_wrapper)
  target_compile_options(mpiwrapper PRIVATE ${MPIABI_WARNINGS})
  target_link_libraries(mpiwrapper PRIVATE MPI::MPI_C)
  if(MPI_ABI_WRAP_ABI_IMPL)
    target_compile_definitions(mpiwrapper PRIVATE MPIWRAPPER_WRAP_ABI_IMPL)
  endif()
  # Exactly one exported symbol, enforced by -fvisibility=hidden plus the
  # explicit attribute in mpiwrapper_vtable.h, and checked by the nm test below.
  set_target_properties(mpiwrapper PROPERTIES C_VISIBILITY_PRESET hidden)

  # The wrapper the tests load, unless MPI_ABI_WRAPPER_LIB says otherwise.
  set(MPIABI_WRAPPER_PATH $<TARGET_FILE:mpiwrapper>)

  # A deliberately *unisolated* wrapper, so that the outward-resolution check can
  # be shown to fire rather than merely to exist. On macOS the way to defeat the
  # isolation is -flat_namespace, which is the one row of dev/dlopen-probe's
  # table where macOS stops being safe; on Linux the same library loaded without
  # RTLD_DEEPBIND does it, so no second target is needed there.
  if(APPLE)
    add_library(mpiwrapper_unisolated SHARED
      src/mpiwrapper/constants.c
      src/mpiwrapper/handles.c
      src/mpiwrapper/status.c
      src/mpiwrapper/staging.c
      src/mpiwrapper/callbacks.c
      src/mpiwrapper/handwritten.c
      src/mpiwrapper/wrappers.c
      src/mpiwrapper/getvtable.c
    )
    target_include_directories(mpiwrapper_unisolated PRIVATE
      ${MPIABI_WRAPPER_INCLUDE_DIR} ${MPIABI_SRC_INCLUDE_DIR} src/mpiwrapper)
    add_dependencies(mpiwrapper_unisolated mpiabi_h_for_wrapper)
    target_compile_options(mpiwrapper_unisolated PRIVATE ${MPIABI_WARNINGS})
    target_link_libraries(mpiwrapper_unisolated PRIVATE MPI::MPI_C)
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
    src/mpiwrapper/constants.c
    src/mpiwrapper/handles.c
    src/mpiwrapper/status.c
    src/mpiwrapper/staging.c
    src/mpiwrapper/callbacks.c
  )
  target_include_directories(mpiwrapper_selftest PRIVATE
    ${MPIABI_WRAPPER_INCLUDE_DIR} ${MPIABI_SRC_INCLUDE_DIR} src/mpiwrapper)
  add_dependencies(mpiwrapper_selftest mpiabi_h_for_wrapper)
  target_compile_options(mpiwrapper_selftest PRIVATE ${MPIABI_WARNINGS})
  target_link_libraries(mpiwrapper_selftest PRIVATE MPI::MPI_C)
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
  else()
    set(MPIABI_RUN1)
    set(MPIABI_RUN2)
    set(MPIABI_RUN_POST)
  endif()

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
    ENVIRONMENT "MPI_ABI_WRAPPER_LIB=${MPIABI_WRAPPER_PATH}")

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

  add_test(NAME exported-symbols
    COMMAND ${CMAKE_COMMAND}
      -DABI_LIB=$<TARGET_FILE:mpi_abi>
      -DWRAPPER_LIB=$<TARGET_FILE:mpiwrapper>
      -DTEST_BIN=$<TARGET_FILE:abi_prototype_test>
      -P ${CMAKE_SOURCE_DIR}/test/check_exports.cmake)
endif()
