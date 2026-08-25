#!/usr/bin/env bash

# Does `MPI_Abort(comm, N)` reach the invoking environment as N? See README.md,
# "The exit-status measurement". The whole answer is an exit status, so the probe
# is a loop over N and a control rather than a program that prints anything.
#
#   dev/abort-exit-status/exit-status.sh BUILD_DIR IMPL_MPIEXEC [IMPL_MPICC]
#
# BUILD_DIR is a configured wrapper build tree (it supplies libmpi_abi and the
# libmpiwrapper for *this* implementation); IMPL_MPIEXEC is the wrapped MPI's own
# launcher. The control is the same source built with IMPL_MPICC -- no wrapper
# anywhere -- which is what makes each row a comparison rather than a number.
#
# On the development laptop, put `scripts/host-env.sh` in front of it as usual.

set -uo pipefail

build=${1:?usage: exit-status.sh BUILD_DIR IMPL_MPIEXEC [IMPL_MPICC]}
launcher=${2:?usage: exit-status.sh BUILD_DIR IMPL_MPIEXEC [IMPL_MPICC]}
impl_cc=${3:-$(dirname "$launcher")/mpicc}

repodir=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
build=$(cd "$build" && pwd)
prog=$repodir/dev/abort-exit-status/abort-status.c

work=$(mktemp -d) || exit 1
trap 'rm -rf "$work"' EXIT

# The wrapper build is linked against by hand rather than through the build
# tree's bin/mpicc, which bakes in the *install* prefix's paths.
sysroot=
[ "$(uname -s)" = Darwin ] && sysroot="-isysroot $(xcrun --show-sdk-path)"
# shellcheck disable=SC2086
${CC:-cc} $sysroot -I "$repodir/gen/include" -o "$work/wrapped" "$prog" \
    -L "$build" -Wl,-rpath,"$build" -lmpi_abi || exit 1

# The conda MPICH on the development laptop names a compiler that is not
# installed, so the control build says which one to use rather than trusting the
# wrapper script's baked-in default (CLAUDE.md, "This host").
# Open MPI's wrapper has no -cc= and says so, hence the quiet first attempt.
cc_for_impl=${MPIABI_PROBE_CC:-clang}
"$impl_cc" -cc="$cc_for_impl" -o "$work/native" "$prog" 2>/dev/null \
  || "$impl_cc" -o "$work/native" "$prog" || exit 1

export MPI_ABI_WRAPPER_LIB=$build/libmpiwrapper.$(
    [ "$(uname -s)" = Darwin ] && echo dylib || echo so)

# 1, 8, 16 and 42 are predefined ABI classes, and are the rows that show a
# *renumbering* -- MPIABI_ERR_ROOT is 8 where MPICH's MPI_ERR_ROOT is 7. From 62
# up nothing is a predefined class any more, which is the case that matters:
# those are ordinary exit statuses an application picked for its own reasons.
# 1001 is in MPI_T's block, 256 and 1000000 are past what wait(2) can carry, and
# both belong here so the rows show POSIX truncation rather than a conversion.
printf '%-10s %-10s %-10s %s\n' code wrapped native verdict
for n in 1 8 16 42 62 63 100 137 200 255 256 1001 16384 1000000; do
  "$launcher" -n 1 "$work/wrapped" "$n" >/dev/null 2>&1; w=$?
  "$launcher" -n 1 "$work/native"  "$n" >/dev/null 2>&1; v=$?
  [ "$w" = "$v" ] && verdict=same || verdict="DIFFERS"
  printf '%-10s %-10s %-10s %s\n' "$n" "$w" "$v" "$verdict"
done
