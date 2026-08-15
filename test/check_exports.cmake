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
#   cmake -DABI_LIB=... -DWRAPPER_LIB=... -DTEST_BIN=... -DENTRYPOINTS=... -P this

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
# Symbols the ELF linker itself defines in every shared object. They are not
# ours and cannot be suppressed without a version script, which is S6's job;
# until then they are named rather than pattern-matched away, so that a real
# leak cannot hide behind a loose pattern.
set(LINKER_PROVIDED _init _fini _edata _end __bss_start)

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
                 "points matching the header's ${nexpected} in both "
                 "directions, no direct MPI dependency in the application")
endif()
