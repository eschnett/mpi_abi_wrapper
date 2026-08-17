# Oracles 1 and 2, and MPI-5.0 20.2.1, in one nm pass.
#
#  - libmpiwrapper exports exactly one symbol, mpiwrapper_get_vtable. Anything
#    else it exported would be a symbol the loader could bind someone to.
#  - libmpi_abi's exports are exactly the ABI's entry points, MPI_ and PMPI_
#    for each -- checked in *both* directions against dev/entrypoints.txt, so
#    that an entry point the generator dropped fails here as loudly as a symbol
#    it invented.
#  - the test binary's only MPI dependency is libmpi_abi: no libmpi, no
#    libmpiwrapper, since the wrapper is reached by dlopen.
#
# Invoked as:
#   cmake -DABI_LIB=... -DENTRYPOINTS=... [-DWRAPPER_LIB=...] [-DTEST_BIN=...] -P this
#
# The libmpi_abi leg is the one oracle 1 calls the cheapest check in CI, and it
# needs no MPI: its subject is built in the -DMPI_ABI_BUILD_WRAPPER=OFF build
# too, and what it compares against is the ABI header's own entry-point list.
# So WRAPPER_LIB and TEST_BIN are optional, and their legs are skipped when
# there is no wrapper to look at. The summary at the end names the legs that
# actually ran, because a one-leg pass read as a three-leg one would be exactly
# the "compiles, therefore checked" mistake HISTORY.md #5 collects.
#
# -P script mode does not inherit the main configure's policy scope, so
# IN_LIST below needs its own cmake_minimum_required -- found the hard way, by
# adding cmake/mpi_abi.version (S6) and hitting "Unknown arguments specified"
# on a version of CMake this had never been run against before.
cmake_minimum_required(VERSION 3.20)

if(APPLE)
  set(NM_ARGS -gU)          # defined, external
  set(STRIP_UNDERSCORE ON)  # Mach-O prefixes C symbols with _
else()
  set(NM_ARGS --defined-only --extern-only)
  set(STRIP_UNDERSCORE OFF)
endif()

function(exported_symbols out lib)
  execute_process(COMMAND nm ${NM_ARGS} ${lib}
                  OUTPUT_VARIABLE raw RESULT_VARIABLE rc)
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "nm failed on ${lib}")
  endif()
  string(REPLACE "\n" ";" lines "${raw}")
  set(names)
  foreach(line ${lines})
    if(line MATCHES "^[0-9a-fA-F]+ +[A-Za-z] +([^ ]+)$")
      set(name ${CMAKE_MATCH_1})
      if(STRIP_UNDERSCORE)
        string(REGEX REPLACE "^_" "" name ${name})
      endif()
      list(APPEND names ${name})
    endif()
  endforeach()
  list(SORT names)
  set(${out} ${names} PARENT_SCOPE)
endfunction()

set(errors 0)

# The one name a version script always adds to the dynamic table: an absolute
# symbol for its own version node, not a leak of anything this project wrote.
# One per script, so one per library that has one, and neither appears on macOS,
# which has no version scripts. cmake/mpiwrapper.version is newer than
# cmake/mpi_abi.version -- it was added when FreeBSD showed libmpiwrapper
# exporting _init and _fini -- and it brought MPIWRAPPER_1 with it.
set(LINKER_PROVIDED MPIABI_1 MPIWRAPPER_1)

# --- libmpiwrapper: exactly one symbol -------------------------------------
if(WRAPPER_LIB)
  exported_symbols(wrapper_syms ${WRAPPER_LIB})
  list(REMOVE_ITEM wrapper_syms ${LINKER_PROVIDED})
  if(NOT "${wrapper_syms}" STREQUAL "mpiwrapper_get_vtable")
    message(SEND_ERROR
      "libmpiwrapper must export exactly mpiwrapper_get_vtable, but exports: "
      "${wrapper_syms}")
    math(EXPR errors "${errors} + 1")
  endif()
endif()

# --- libmpi_abi: MPI_*/PMPI_* only -----------------------------------------
exported_symbols(abi_syms ${ABI_LIB})
# cmake/mpi_abi.version (S6) makes _init/_fini/_edata/_end/__bss_start local
# instead of naming them here as an exemption -- the linker's own symbols do
# not survive to be seen at all now, on any ELF linker or toolchain, rather
# than being tolerated by a list this test has to keep in sync with whichever
# ones a given libc/binutils happens to emit.
#
# LINKER_PROVIDED is set once above, both libraries' version nodes together.

file(STRINGS ${ENTRYPOINTS} bases)
set(expected)
foreach(base ${bases})
  if(base)
    list(APPEND expected MPI_${base} PMPI_${base})
  endif()
endforeach()
list(SORT expected)
list(LENGTH expected nexpected)

set(bad)
set(exported)
foreach(sym ${abi_syms})
  if(sym MATCHES "^P?MPI_")
    list(APPEND exported ${sym})
  elseif(sym IN_LIST LINKER_PROVIDED)
    # not ours
  else()
    list(APPEND bad ${sym})
  endif()
endforeach()
if(bad)
  message(SEND_ERROR "libmpi_abi exports non-MPI symbols: ${bad}")
  math(EXPR errors "${errors} + 1")
endif()

set(missing ${expected})
list(REMOVE_ITEM missing ${exported})
set(extra ${exported})
list(REMOVE_ITEM extra ${expected})
if(missing)
  list(LENGTH missing n)
  message(SEND_ERROR
    "libmpi_abi is missing ${n} entry point(s) the ABI header declares: "
    "${missing}")
  math(EXPR errors "${errors} + 1")
endif()
if(extra)
  message(SEND_ERROR
    "libmpi_abi exports MPI names the ABI header does not declare: ${extra}")
  math(EXPR errors "${errors} + 1")
endif()
list(LENGTH exported nmpi)

# --- the application's direct dependencies ---------------------------------
if(TEST_BIN)
  if(APPLE)
    execute_process(COMMAND otool -L ${TEST_BIN} OUTPUT_VARIABLE deps)
  else()
    execute_process(COMMAND objdump -p ${TEST_BIN} OUTPUT_VARIABLE deps)
  endif()
  if(deps MATCHES "libmpiwrapper")
    message(SEND_ERROR
      "the test binary links libmpiwrapper directly; it must be reached only by "
      "dlopen (MPI-5.0 20.2.1)")
    math(EXPR errors "${errors} + 1")
  endif()
  if(deps MATCHES "libmpi\\.|libmpich|libpmpi|libopen_mpi")
    message(SEND_ERROR
      "the test binary depends on an MPI implementation directly; libmpi_abi "
      "must be its only MPI dependency (MPI-5.0 20.2.1)")
    math(EXPR errors "${errors} + 1")
  endif()
endif()

# The summary names the legs that ran, not the legs this file can run: in a
# build with no wrapper only the first clause is true, and saying the rest
# anyway would make the cheap job look like the whole oracle.
if(errors EQUAL 0)
  string(CONCAT summary
    "libmpi_abi ${nmpi} entry points matching the header's "
    "${nexpected} in both directions")
  if(WRAPPER_LIB)
    string(APPEND summary "; libmpiwrapper 1 symbol")
  endif()
  if(TEST_BIN)
    string(APPEND summary "; no direct MPI dependency in the application")
  endif()
  if(NOT WRAPPER_LIB AND NOT TEST_BIN)
    string(APPEND summary " (no wrapper in this build; its two legs skipped)")
  endif()
  message(STATUS "exports: ${summary}")
endif()
