# Oracles 1 and 2, and MPI-5.0 20.2.1, in one nm pass.
#
#  - libmpiwrapper exports exactly one symbol, mpiwrapper_get_vtable. Anything
#    else it exported would be a symbol the loader could bind someone to.
#  - libmpi_abi exports only MPI_*/PMPI_* entry points, and every slot in the
#    vtable has one. (S1 checks the 56 the prototype covers; S2 raises this to
#    the full 1376 by extracting the list from the ABI header instead.)
#  - the test binary's only MPI dependency is libmpi_abi: no libmpi, no
#    libmpiwrapper, since the wrapper is reached by dlopen.
#
# Invoked as: cmake -DABI_LIB=... -DWRAPPER_LIB=... -DTEST_BIN=... -P this

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

# --- libmpiwrapper: exactly one symbol -------------------------------------
exported_symbols(wrapper_syms ${WRAPPER_LIB})
if(NOT "${wrapper_syms}" STREQUAL "mpiwrapper_get_vtable")
  message(SEND_ERROR
    "libmpiwrapper must export exactly mpiwrapper_get_vtable, but exports: "
    "${wrapper_syms}")
  math(EXPR errors "${errors} + 1")
endif()

# --- libmpi_abi: MPI_*/PMPI_* only -----------------------------------------
exported_symbols(abi_syms ${ABI_LIB})
set(bad)
set(nmpi 0)
foreach(sym ${abi_syms})
  if(sym MATCHES "^P?MPI_")
    math(EXPR nmpi "${nmpi} + 1")
  else()
    list(APPEND bad ${sym})
  endif()
endforeach()
if(bad)
  message(SEND_ERROR "libmpi_abi exports non-MPI symbols: ${bad}")
  math(EXPR errors "${errors} + 1")
endif()
if(NOT nmpi EQUAL 56)
  message(SEND_ERROR
    "libmpi_abi exports ${nmpi} MPI entry points; the S1 prototype has 56 "
    "(28 entry points, MPI_ and PMPI_ each)")
  math(EXPR errors "${errors} + 1")
endif()

# --- the application's direct dependencies ---------------------------------
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

if(errors EQUAL 0)
  message(STATUS "exports: libmpiwrapper 1 symbol, libmpi_abi ${nmpi} entry "
                 "points, no direct MPI dependency in the application")
endif()
