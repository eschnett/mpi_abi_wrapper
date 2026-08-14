#!/usr/bin/env bash
# Compile the examples. They are illustrative, but an illustration that has never
# been through a compiler is a guess -- and this project's whole argument is that
# the compilers do half the verification.
#
# Needs two *different* mpi.h: the ABI one for the mpi_abi side, an
# implementation's for the wrapper side. Compiling the wrapper side against the ABI
# header is the self-wrapping mistake the real CMakeLists refuses.

set -euo pipefail
cd "$(dirname "$0")"

CC=${CC:-cc}
CFLAGS=${CFLAGS:--std=c11 -Wall -Wextra -Wno-unused-parameter}

# Defaults point into mpif's build tree, which is where both kinds of header
# happen to exist side by side on the machine this was written on.
: "${ABI_INCLUDE:=$HOME/src/mpif/build/mpi/mpich-gcc/include}"
: "${MPI_INCLUDE:=$HOME/src/mpif/build/mpi-src/mpich-gcc/mpich/src/include}"

fail=0

check() {
  local label=$1 inc=$2 src=$3
  if [ ! -r "$inc/mpi.h" ]; then
    printf 'SKIP  %-28s no mpi.h in %s\n' "$src" "$inc"
    return
  fi
  if $CC $CFLAGS -fsyntax-only -I. -I"$inc" "$src" 2>&1; then
    printf 'ok    %-28s (%s)\n' "$src" "$label"
  else
    printf 'FAIL  %-28s (%s)\n' "$src" "$label"
    fail=1
  fi
}

check "ABI header"            "$ABI_INCLUDE" mpi_abi_side.c
check "implementation header" "$MPI_INCLUDE" mpiwrapper_wrappers.c
check "implementation header" "$MPI_INCLUDE" mpiwrapper_convert.c

exit $fail
