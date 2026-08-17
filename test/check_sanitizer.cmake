# A sanitizer build whose flags never reached the compiler builds, links, and
# passes all thirteen tests -- which is exactly what a correct one does on clean
# code. No behavioural test can tell the two apart, so this reads the object
# code instead.
#
# It is registered only when MPI_ABI_SANITIZE is non-empty, and it fails if the
# libraries that were supposed to be instrumented carry none of the runtime's
# symbols. The failure it exists for is silent: a typo in the option name, a
# flag dropped by a toolchain that does not support the sanitizer asked for, or
# a target that acquired its own compile options and stopped inheriting the
# directory's.
#
# Invoked as:
#   cmake -DLIBS=a;b -DSANITIZE=address,undefined -P this
cmake_minimum_required(VERSION 3.20)

if(APPLE)
  set(NM_ARGS -gU)
else()
  set(NM_ARGS --defined-only --extern-only)
endif()

# What each sanitizer leaves behind in an instrumented object. Interface
# symbols the instrumentation calls into, not the runtime library itself, so
# these are present whether the runtime is linked statically or shared.
#
# `undefined` alone emits no *defined* symbol of its own into the library --
# its calls are to the runtime and stay undefined here -- so the undefined
# table is what has to be read for it. Hence two nm passes rather than one.
set(address_MARKER "__asan_")
set(undefined_MARKER "__ubsan_")
set(thread_MARKER "__tsan_")
set(memory_MARKER "__msan_")

string(REPLACE "," ";" wanted "${SANITIZE}")
set(errors 0)

foreach(lib ${LIBS})
  # Both tables: instrumentation shows up as defined symbols for asan/tsan/msan
  # and as undefined ones for ubsan.
  execute_process(COMMAND nm ${NM_ARGS} ${lib} OUTPUT_VARIABLE defined_syms)
  execute_process(COMMAND nm ${lib} OUTPUT_VARIABLE all_syms)
  set(syms "${defined_syms}${all_syms}")

  foreach(san ${wanted})
    if(NOT DEFINED ${san}_MARKER)
      message(SEND_ERROR "check_sanitizer: no marker known for '${san}'")
      math(EXPR errors "${errors} + 1")
      continue()
    endif()
    if(NOT syms MATCHES "${${san}_MARKER}")
      message(SEND_ERROR
        "${lib} was built without -fsanitize=${san}: no '${${san}_MARKER}' "
        "symbol in it. The build claimed MPI_ABI_SANITIZE=${SANITIZE}.")
      math(EXPR errors "${errors} + 1")
    endif()
  endforeach()
endforeach()

if(errors EQUAL 0)
  list(LENGTH LIBS nlibs)
  message(STATUS
    "sanitizer: ${nlibs} librar(ies) carry the instrumentation for ${SANITIZE}")
endif()
