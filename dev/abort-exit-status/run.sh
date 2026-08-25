#!/usr/bin/env bash

# Does `errors/comm/intercomm_abort` fail because of this project, or because of
# the launcher? See README.md. Both halves of the answer are exit statuses, so
# the probe is a loop and a control rather than a program.
#
#   dev/abort-exit-status/run.sh /path/to/wrapper-prefix/bin/mpicc \
#                               /path/to/impl/bin/mpicc [iterations]
#
# The first mpicc is the wrapper's -- the one `run-suite.sh` installs into
# $work/prefix/bin -- and the second is the wrapped implementation's own, which is
# the control: the same source, the same launcher, no wrapper anywhere.

set -uo pipefail

wrapper_cc=${1:?usage: run.sh WRAPPER_MPICC IMPL_MPICC [iterations]}
impl_cc=${2:?usage: run.sh WRAPPER_MPICC IMPL_MPICC [iterations]}
iters=${3:-30}

repodir=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
suite_version=${MPIABI_SUITE_VERSION:-5.0.1}
src=${MPIABI_SUITE_SRC:-$repodir/build/suite-src}/mpich-$suite_version/test/mpi
prog=$src/errors/comm/intercomm_abort.c
[ -r "$prog" ] || { echo "no $prog -- run ci-scripts/suite/run-suite.sh first" >&2; exit 2; }

launcher=$(cd "$(dirname "$impl_cc")" && pwd)/mpiexec
[ -x "$launcher" ] || { echo "no launcher beside $impl_cc" >&2; exit 2; }

work=$(mktemp -d) || exit 1
trap 'rm -rf "$work"' EXIT

# The conda MPICH on the development laptop names a compiler that is not
# installed, so the control build says which one to use rather than trusting the
# wrapper script's baked-in default (CLAUDE.md, "This host").
cc_for_impl=${MPIABI_PROBE_CC:-clang}

echo "=== building"
"$wrapper_cc" -o "$work/wrapped" "$prog" || exit 1
"$impl_cc" -cc="$cc_for_impl" -o "$work/native" "$prog" \
  || "$impl_cc" -o "$work/native" "$prog" || exit 1

# --- part 1: the raw exit status ---------------------------------------------
#
# This is the whole question. runtests' `resultTest=TestStatusNoErrors` demands
# that mpiexec exit *nonzero*, because rank 0 aborts; a zero here is the failure
# CI reports as "returned a zero status but the program returned a nonzero
# status".
for build in wrapped native; do
  echo "=== $build: $iters raw launcher runs, exit status each"
  for _ in $(seq "$iters"); do
    "$launcher" -disable-auto-cleanup -n 6 "$work/$build" >/dev/null 2>&1
    printf '%s ' "$?"
  done
  echo
done

# --- part 2: the same thing as runtests sees it ------------------------------
#
# A minimal build tree with a one-line testlist, so the verdict is runtests' own
# words rather than an interpretation of part 1.
mkdir -p "$work/t/errors/comm"
echo errors >"$work/t/testlist"
echo comm >"$work/t/errors/testlist"
grep '^intercomm_abort' "$src/errors/comm/testlist" >"$work/t/errors/comm/testlist"

export MPIEXEC_FILTER_LAUNCHER=$launcher MPIEXEC_FILTER_KIND=hydra
export MPITEST_RUN_INDIVIDUAL=1
for build in wrapped native; do
  echo "=== $build: runtests"
  cp "$work/$build" "$work/t/errors/comm/intercomm_abort"
  (cd "$work/t" && "$src/runtests" -srcdir="$src" -tests=testlist \
       -mpiexec="$repodir/ci-scripts/suite/mpiexec-filter" \
       -tapfile="$work/$build.tap") 2>&1 | grep -v '^Load tests'
  grep '^##' "$work/$build.tap"
done
