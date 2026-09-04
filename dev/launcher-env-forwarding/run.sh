#!/usr/bin/env bash

# Does a launcher deliver an MPI_ABI_WRAPPER_* variable set in *its own*
# environment to the ranks it starts?
#
# The question decides whether bin/mpiexec needs environment-rewriting logic
# (decision 27). The premise it tests is ci-scripts/suite/mpiexec-filter's,
# stated in that file's header: "Open MPI's launcher forwards almost nothing
# without being told", against hydra, which forwards. If that is true of
# MPI_ABI_WRAPPER_LIB too, then a user who re-points a binary at another
# wrapper gets it honoured under MPICH and silently ignored under Open MPI --
# every rank falling back to the baked-in default, which is a wrong answer
# rather than an error. If it is false, bin/mpiexec needs nothing and the
# filter's own forwarding of these names is dead weight.
#
#   dev/launcher-env-forwarding/run.sh /path/to/mpiexec [/path/to/another ...]
#
# No MPI program and no compiler: `sh -c 'echo $VAR'` is a rank as far as this
# question is concerned, since what is being measured is the launcher's
# treatment of its environment and not anything MPI does with it. That also
# makes the probe runnable against a launcher whose own mpicc does not work on
# the host, which is the case on the development laptop (conda's mpicc names a
# clang that is not installed).
#
# Results are in README.md beside this file.

set -uo pipefail

if [ $# -eq 0 ]; then
  echo "usage: $(basename "$0") /path/to/mpiexec [...]" >&2
  exit 2
fi

# Two names: one this project would want forwarded, and one control with no
# MPI-ish prefix, so that a launcher forwarding *everything* is distinguishable
# from one with a rule about our names in particular.
export MPI_ABI_WRAPPER_PROBE=wrapper-value
export UNRELATED_PROBE=control-value

for launcher in "$@"; do
  echo "=== $launcher"
  if [ ! -x "$launcher" ]; then
    echo "  not executable, skipped"
    continue
  fi
  echo "  --version: $("$launcher" --version 2>&1 | head -1)"

  for var in MPI_ABI_WRAPPER_PROBE UNRELATED_PROBE; do
    # Two ranks, so a launcher that answers -n 2 with two singletons is still
    # answering the question this probe asks (each singleton has an
    # environment either way).
    out=$("$launcher" -n 2 sh -c "echo \"\$$var\"" 2>&1)
    got=$(echo "$out" | grep -c "value")
    case $got in
      2) echo "  $var: forwarded to both ranks" ;;
      0) echo "  $var: NOT forwarded" ;;
      *) echo "  $var: forwarded to $got of 2 ranks (launcher output below)"
         echo "$out" | sed 's/^/    /' ;;
    esac
  done

  # And with the launcher told explicitly, which is what mpiexec-filter does.
  out=$("$launcher" -x MPI_ABI_WRAPPER_PROBE -n 2 sh -c 'echo "$MPI_ABI_WRAPPER_PROBE"' 2>&1)
  if [ "$(echo "$out" | grep -c wrapper-value)" = 2 ]; then
    echo "  with -x: forwarded to both ranks"
  else
    echo "  with -x: not accepted, or not forwarded:"
    echo "$out" | head -3 | sed 's/^/    /'
  fi
done
