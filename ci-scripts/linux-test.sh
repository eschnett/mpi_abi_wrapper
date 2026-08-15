#!/usr/bin/env bash
# Build and test against one MPI, on Linux.
#
# Runs *inside* a Linux environment: a container started by run-linux-docker.sh,
# or a CI runner directly. It installs packages only if it is root and apt is
# present, so a prepared runner can call it unchanged.
#
#   ci-scripts/linux-test.sh mpich
#   ci-scripts/linux-test.sh openmpi
#
# Environment:
#   SRC        source tree (default: the parent of this script's directory)
#   BUILD      build directory (default: /tmp/build-$MPI; must be writable, and
#              deliberately not inside SRC, which CI mounts read-only)
#
# Gating steps exit non-zero on failure. Two steps are informational and say so,
# because they measure the environment rather than this project:
#
#   symbol binding   evidence for the claims in NOTES.md #2 about MPI_ vs PMPI_
#   dlmopen mode     known not to work with any real MPI (NOTES.md #2): PMIx
#                    dlopens components with RTLD_GLOBAL and glibc cannot add to
#                    the global scope of a namespace with no main map, so
#                    MPI_Init segfaults inside the loader. Run for the day that
#                    changes, never gated on.
set -uo pipefail

# Either a distro MPI by name, or the path to an mpicc -- the latter for an MPI
# built from a pinned tarball, which is how NOTES.md #9 wants CI to provision
# them and how anything older than the distro ships gets tested at all.
which=${1:-}
MPICC=""
case $which in
  mpich|openmpi) ;;
  */*|mpicc*)
    [ -x "$which" ] || { echo "$which: not executable" >&2; exit 2; }
    MPICC=$which
    which=$(basename "$(dirname "$(dirname "$MPICC")")")
    ;;
  *) echo "usage: $0 mpich|openmpi|/path/to/mpicc" >&2; exit 2 ;;
esac

SRC=${SRC:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}
BUILD=${BUILD:-/tmp/build-$which}
status=0

step() { printf '\n=== %s\n' "$*"; }
fail() { echo "FAILED: $*" >&2; status=1; }

# ------------------------------------------------------------------ packages

if [ "$(id -u)" = 0 ] && command -v apt-get >/dev/null; then
  export DEBIAN_FRONTEND=noninteractive
  pkgs="build-essential cmake python3 patch binutils"
  case ${MPICC:+path}$which in
    mpich)   pkgs="$pkgs libmpich-dev mpich" ;;
    openmpi) pkgs="$pkgs libopenmpi-dev openmpi-bin" ;;
  esac
  step "installing $pkgs"
  apt-get update -qq >/dev/null && apt-get install -y -qq $pkgs >/dev/null \
    || { echo "package install failed" >&2; exit 2; }
fi

# Debian installs both MPIs' wrappers under alternatives; name the one we mean.
if [ -z "$MPICC" ]; then
  MPICC=$(command -v "mpicc.$which" || command -v mpicc) || {
    echo "no mpicc for $which" >&2; exit 2; }
fi

# --------------------------------------------------------------- environment

step "$which: $("$MPICC" --version | head -1)"

cat > /tmp/mpiver.c <<'EOF'
#include <mpi.h>
#include <stdio.h>
int main(void) { printf("MPI %d.%d\n", MPI_VERSION, MPI_SUBVERSION); return 0; }
EOF
"$MPICC" -o /tmp/mpiver /tmp/mpiver.c && /tmp/mpiver

step "symbol binding (informational; NOTES.md #2)"
# Whether MPI_* are weak aliases of PMPI_* or strong definitions at the same
# address differs by implementation and platform, and the design only needs
# "both names exist and reach the same code".
for dir in $("$MPICC" -show 2>/dev/null | tr ' ' '\n' | grep '^-L' | sed 's/^-L//'); do
  for so in "$dir"/libmpi*.so*; do
    case "$so" in *.so|*.so.[0-9]*) ;; *) continue ;; esac
    [ -e "$so" ] || continue
    nm -D --defined-only "$so" 2>/dev/null |
      awk -v lib="$(basename "$so")" '
        $2 == "T" && $3 ~ /^MPI_/  { t++ } $2 == "W" && $3 ~ /^MPI_/  { w++ }
        $2 == "T" && $3 ~ /^PMPI_/ { pt++ } $2 == "W" && $3 ~ /^PMPI_/ { pw++ }
        $3 == "MPI_Send" || $3 == "PMPI_Send" { addr[$3] = $1 }
        END { if (t + w + pt + pw == 0) exit
              printf "%-28s MPI_: %4d T %4d W | PMPI_: %4d T %4d W",
                     lib, t, w, pt, pw
              if (addr["MPI_Send"] != "" && addr["MPI_Send"] == addr["PMPI_Send"])
                printf "  (MPI_Send and PMPI_Send share an address)"
              print "" }'
  done
  break
done

# ------------------------------------------------------------------ the tests

step "configure and build"
rm -rf "$BUILD"
cmake -S "$SRC" -B "$BUILD" -DMPI_C_COMPILER="$MPICC" > /tmp/cmake-$which.log 2>&1 \
  || { tail -25 /tmp/cmake-$which.log; fail "configure"; exit $status; }
grep -E 'Found MPI_C|Launching tests' /tmp/cmake-$which.log || true
cmake --build "$BUILD" -j"$(nproc)" > /tmp/build-$which.log 2>&1 \
  || { grep -E 'error|Error' /tmp/build-$which.log | head -25; fail "build"; exit $status; }

# Open MPI refuses to run as root without being told twice, which is the normal
# situation in a container. MPICH does not care.
export OMPI_ALLOW_RUN_AS_ROOT=1 OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1
export OMPI_MCA_rmaps_base_oversubscribe=1
export OMPI_MCA_btl_vader_single_copy_mechanism=none

step "ctest, default isolation (RTLD_LOCAL | RTLD_DEEPBIND)"
ctest --test-dir "$BUILD" --output-on-failure || fail "ctest"

step "dlmopen mode (informational, expected to fail; NOTES.md #2)"
MPI_ABI_WRAPPER_DLOPEN_MODE=dlmopen \
  ctest --test-dir "$BUILD" -R abi_prototype_test 2>&1 | tail -5 || true

printf '\n=== %s: %s\n' "$which" \
  "$([ $status -eq 0 ] && echo 'all gating steps passed' || echo 'FAILURES above')"
exit $status
