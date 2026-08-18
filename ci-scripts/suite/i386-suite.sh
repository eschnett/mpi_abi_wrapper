#!/usr/bin/env bash

# Run the MPICH C suite against this project on **32-bit x86**, from inside a
# linux/386 container.
#
#   MPIABI_IMAGE=i386/debian:trixie-slim \
#   MPIABI_LINUX_SCRIPT=/src/ci-scripts/suite/i386-suite.sh \
#   MPIABI_LINUX_OUT=$PWD/build/suite-i386 \
#     ci-scripts/run-linux-docker.sh mpich
#
# Why this is a container and not a matrix row. NOTES.md #4.1 makes 32-bit
# load-bearing rather than routine -- an ABI handle is *pointer-sized*, so ILP32
# is the only place "no spare high bits to tag a handle with" is visible, and
# #4.2's status layouts shrink there too. But a GitHub Actions job cannot be
# 32-bit: the runner injects its own x86_64 node to execute JS actions, and a
# 32-bit userspace has no 64-bit libc to run it against, so `container:
# i386/debian` fails before the first step. docker/mpich-i386.dockerfile solved
# that by making the image build *be* the test; this file solves it the other
# way, with `docker run`, because a suite run has to hand a TAP file back and a
# build cannot.
#
# `linux/386` is native on an x86_64 kernel, so this costs about what the 64-bit
# rows cost. On an arm64 development machine it is emulated and very slow, which
# is a property of that machine and not of this script.
#
# Everything writable is under /out, which run-linux-docker.sh mounts from the
# host: /src is read-only, and the MPI prefix has to outlive the container for
# CI to cache it. The prefix path is /out/opt/... *inside* the container
# regardless of where the host mounted it, which is what makes it cacheable at
# all -- an installed MPI bakes absolute paths into mpicc, its RPATHs and its
# libtool files, and those have to mean the same thing on the next run.

set -uo pipefail

which=${1:-mpich}
case $which in
  mpich) ;;
  *) echo "$0: only mpich is built here (got '$which')" >&2; exit 2 ;;
esac
shift || true

SRC=${SRC:-/src}
OUT=/out
[ -d "$OUT" ] || { echo "$0: /out is not mounted; MPIABI_LINUX_OUT is required" >&2; exit 2; }

version=${MPIABI_I386_MPICH_VERSION:-5.0.1}
prefix=$OUT/opt/mpich-$version

step() { printf '\n=== %s\n' "$*"; }

# ccache is here for the same reason mpif's CI has it: this build is the
# expensive half of the row, and a cache miss on the prefix should not also mean
# recompiling every object from scratch. The cache directory is under /out so the
# host can keep it.
if [ "$(id -u)" = 0 ] && command -v apt-get >/dev/null; then
  export DEBIAN_FRONTEND=noninteractive
  step "installing the toolchain"
  apt-get update -qq >/dev/null
  # gfortran is not optional: NOTES.md #9 forbids --disable-fortran, because it
  # silently drops the *implementations* of MPI_Type_create_f90_{real,complex,
  # integer}, which are plain C entry points MPI-5.0 requires -- the
  # compile-only probe then reports them available and only the wrapper's link
  # fails. perl is runtests, curl fetches both tarballs.
  apt-get install -y -qq --no-install-recommends \
      build-essential gfortran cmake python3 patch binutils perl curl \
      ca-certificates ccache >/dev/null \
    || { echo "package install failed" >&2; exit 2; }
fi

export CCACHE_DIR=$OUT/ccache
if command -v ccache >/dev/null; then
  export PATH=/usr/lib/ccache:$PATH
  ccache --zero-stats >/dev/null 2>&1
fi

# Build MPICH only when the prefix is not already there from the host's cache.
# `mpicc` existing is the test, and the two-rank check below is what says the
# restored copy actually works -- mpif's check-mpi-install.sh makes the same
# point, that a cache hit is exactly when nothing has verified the installation.
if [ -x "$prefix/bin/mpicc" ]; then
  step "MPICH $version is already installed at $prefix"
else
  step "building MPICH $version for i386 (this is the slow half)"
  MPI_SRC_DIR=$OUT/mpi-src "$SRC/ci-scripts/install-mpich.sh" "$prefix" "$version" \
    || { echo "MPICH build failed" >&2; exit 1; }
fi
command -v ccache >/dev/null && ccache --show-stats 2>/dev/null | head -5

# 32 bits is the claim this row exists to make, so it is checked rather than
# assumed: a 64-bit MPICH restored from a mislabelled cache would otherwise run
# the whole suite and report a result about the wrong architecture.
step "confirming this really is a 32-bit build"
cat >"$OUT/bits.c" <<'EOF'
#include <mpi.h>
#include <stdio.h>
int main(void) { printf("%zu\n", sizeof(MPI_Aint) == sizeof(void *) ? sizeof(void *) : 0); return 0; }
EOF
bits=$("$prefix/bin/mpicc" -o "$OUT/bits" "$OUT/bits.c" && "$OUT/bits")
case $bits in
  4) echo "  pointers are 4 bytes, as this row requires" ;;
  *) echo "::error::this is not an ILP32 build (pointer size reported: ${bits:-unknown})" >&2
     exit 1 ;;
esac

step "two ranks, with no wrapper involved"
cat >"$OUT/hello.c" <<'EOF'
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
"$prefix/bin/mpicc" -o "$OUT/hello" "$OUT/hello.c" || exit 1
"$prefix/bin/mpiexec" -n 2 "$OUT/hello" || {
  echo "::error::MPICH $version cannot launch two ranks in this container; the suite would have blamed the wrapper" >&2
  exit 1
}

# hydra, stated rather than derived. run-suite.sh would ask the launcher and
# get this right, but the cost of a wrong answer is a whole category of the
# suite failing to launch on a row that takes an hour.
export MPIEXEC_FILTER_KIND=hydra

export MPIABI_SUITE_WORK=${MPIABI_SUITE_WORK:-$OUT/suite}
export MPIABI_SUITE_SRC=${MPIABI_SUITE_SRC:-$OUT/suite-src}

exec "$SRC/ci-scripts/suite/run-suite.sh" "$prefix/bin/mpicc" \
     --variant=ci-mpich-i386 "$@"
