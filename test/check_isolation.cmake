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

# The property under test is that an unisolated wrapper does not silently work.
# There are two honest ways for that to show, and they are not equally good:
#
#   detected  the load-time checks refuse, naming the capture. What we want.
#   crashed   the capture is real but the checks did not see it, and the process
#             dies of unbounded recursion instead. Still safe -- nothing wrong
#             was computed -- but the diagnosis is the stack, not a sentence.
#
# Only a *successful* run is a failure of the guarantee, so only that is fatal.
# The crash case is reported rather than hidden, because which one occurs is a
# property of the implementation being wrapped: Open MPI main's flat-namespace
# build captures some entry points and not others, and MPI_Get_version -- the
# probe's own call -- happens to be one of the ones that resolve outward.
if(rc EQUAL 0)
  message(FATAL_ERROR
    "the test binary ran to completion against an unisolated wrapper. Either "
    "the loader is isolating it anyway -- in which case this test needs a "
    "different way to defeat the isolation -- or the outward-resolution checks "
    "in libmpi_abi and mpiwrapper_get_vtable have stopped working.\n${all}")
endif()

if(all MATCHES "symbol resolution captured|come back into libmpi_abi")
  message(STATUS "isolation: capture detected at load, as dev/dlopen-probe predicts")
else()
  message(STATUS
    "isolation: the unisolated wrapper failed (exit ${rc}) without being "
    "detected at load -- a partial capture the probe's own call does not hit. "
    "Safe but undiagnosed; see NOTES.md #2.")
endif()
