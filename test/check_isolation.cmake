# The outward-resolution check must *fire*, not merely be present.
#
# dev/dlopen-probe/ established, on a mock-up with no MPI in it, that a wrapper
# loaded without isolation has its own MPI_Send captured by the ABI library --
# infinite recursion -- and that the load-time check catches it. This runs the
# same experiment on the real libraries, which is the half of that conclusion a
# mock-up cannot establish.
#
# The unisolated build is arranged per platform:
#   macOS   a second libmpiwrapper linked -Wl,-flat_namespace, which is exactly
#           the row of the probe's table where macOS stops being safe.
#   Linux   the same libmpiwrapper, loaded with MPI_ABI_WRAPPER_DLOPEN_MODE=capture,
#           i.e. plain RTLD_LOCAL with no RTLD_DEEPBIND and no dlmopen.
#
# Either way the expected outcome is identical and specific: the process aborts
# during the constructor, before MPI_Init, naming the capture. A test binary that
# ran to completion would mean the check had silently stopped working -- which is
# the failure mode this exists to prevent, since everything else about the
# wrapper would keep passing.
#
# Invoked as: cmake -DTEST_BIN=... -DWRAPPER=... [-DCAPTURE_MODE=1] -P this

set(env "MPI_ABI_WRAPPER_LIB=${WRAPPER}")
if(CAPTURE_MODE)
  list(APPEND env "MPI_ABI_WRAPPER_DLOPEN_MODE=capture")
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E env ${env} ${TEST_BIN}
  OUTPUT_VARIABLE out ERROR_VARIABLE err RESULT_VARIABLE rc)

set(all "${out}${err}")

if(rc EQUAL 0)
  message(FATAL_ERROR
    "the test binary ran to completion against an unisolated wrapper. Either "
    "the loader is isolating it anyway -- in which case this test needs a "
    "different way to defeat the isolation -- or the outward-resolution check "
    "in mpiwrapper_get_vtable has stopped working.\n${all}")
endif()

if(NOT all MATCHES "symbol resolution captured")
  message(FATAL_ERROR
    "the test binary failed against an unisolated wrapper, but not with the "
    "capture diagnostic, so this may be failing for an unrelated reason:\n${all}")
endif()

message(STATUS "isolation: capture detected at load, as dev/dlopen-probe predicts")
