#!/usr/bin/env bash
# Build and test against one MPI, on FreeBSD.
#
# The sibling of linux-test.sh, and a separate file rather than a branch in it:
# that script is GNU/Linux throughout -- nproc, `nm -D`, .so globs, apt -- and
# none of those exist here. What the two share is the shape: gating steps decide
# the exit status, informational ones do not.
#
#   ci-scripts/freebsd-test.sh [mpich]
#
# Environment:
#   SRC    source tree (default: the parent of this script's directory)
#   BUILD  build directory (default: /tmp/build-freebsd-$MPI)
#
# It exists because the recipe cannot live in the workflow. vmactions/freebsd-vm
# hands its `run:` block to the VM over ssh, and a long line does not survive
# the trip intact: the first attempt at this row died on
#
#   CMake Error: Unknown argument: --ti
#
# which is `ctest ... --timeout 300` truncated mid-flag. Everything before it had
# already worked -- the whole project built -- so the row's first result was a
# defect in its own harness. One short line calling a script cannot be truncated
# into a different one, and a script is runnable by hand on a FreeBSD box, which
# a YAML block is not.
#
# NOTES.md #13.4 lists this platform's isolation as *unverified*: bootstrap.c
# reaches for RTLD_DEEPBIND on every non-Apple platform and whether FreeBSD's
# rtld has it was the open question. It compiles, which is now known; whether it
# *isolates* is what ctest's isolation-check answers below.
set -uo pipefail

which=${1:-mpich}
SRC=${SRC:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}
BUILD=${BUILD:-/tmp/build-freebsd-$which}
status=0

step() { printf '\n=== %s\n' "$*"; }
fail() { echo "FAILED: $*" >&2; status=1; }

# FreeBSD packages install under /usr/local, and there is no alternatives
# system to disambiguate two MPIs, so the wrapper is named directly.
MPICC=${MPICC:-/usr/local/bin/$which/mpicc}
[ -x "$MPICC" ] || MPICC=/usr/local/bin/mpicc
[ -x "$MPICC" ] || { echo "no mpicc for $which" >&2; exit 2; }

nprocs=$(sysctl -n hw.ncpu 2>/dev/null || echo 2)

step "$which: $("$MPICC" --version | head -1)"
freebsd-version
uname -a

# The gate that keeps an environment failure from being read as a wrapper
# failure. It matters more here than anywhere: on a platform nothing has run on,
# "this VM cannot launch two ranks" and "the wrapper is broken" are otherwise
# the same red ctest. The VM's hostname resolves to nothing unless the caller
# fixed /etc/hosts, and MPICH's tcp netmod resolves the host's own name to build
# its business card.
step "two ranks, with no wrapper involved"
cat > /tmp/mpi-hello.c <<'EOF'
#include <mpi.h>
#include <stdio.h>
int main(int argc, char **argv) {
  int size = -1, rank = -1;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  printf("rank %d of %d\n", rank, size);
  MPI_Finalize();
  return size == 2 ? 0 : 1;
}
EOF
mpiexec=$(dirname "$MPICC")/mpiexec
if ! "$MPICC" -o /tmp/mpi-hello /tmp/mpi-hello.c; then
  fail "compiling a wrapper-free hello"
  exit $status
fi
if ! "$mpiexec" -n 2 /tmp/mpi-hello; then
  echo "$which cannot launch two ranks in this VM; ctest below would have" \
       "blamed the wrapper" >&2
  fail "two ranks"
  exit $status
fi

step "configure and build"
rm -rf "$BUILD"
if ! cmake -S "$SRC" -B "$BUILD" -DMPI_C_COMPILER="$MPICC" \
     > /tmp/cmake-$which.log 2>&1; then
  tail -25 /tmp/cmake-$which.log
  fail "configure"
  exit $status
fi
grep -E 'Found MPI_C|Launching tests' /tmp/cmake-$which.log || true

if ! cmake --build "$BUILD" -j"$nprocs" > /tmp/build-$which.log 2>&1; then
  grep -E 'error|Error' /tmp/build-$which.log | head -25
  fail "build"
  exit $status
fi

step "ctest, default isolation (RTLD_LOCAL | RTLD_DEEPBIND)"
ctest --test-dir "$BUILD" --output-on-failure --no-tests=error --timeout 300 \
  || fail "ctest"

printf '\n=== %s: %s\n' "$which" \
  "$([ $status -eq 0 ] && echo 'all gating steps passed' || echo 'FAILURES above')"
exit $status
