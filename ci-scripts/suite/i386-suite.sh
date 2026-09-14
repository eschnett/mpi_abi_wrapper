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

version=${MPIABI_I386_MPICH_VERSION:-5.0.2rc2}
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
      ca-certificates ccache file >/dev/null \
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
# **MPICH 5.0.x's embedded libfabric does not compile on ILP32**, and this is
# what it costs to have the row at all. `ofi_cma.h`'s cma_copy passes
# `unsigned long *` where `ofi_consume_iov` takes `size_t *`; on LP64 those are
# the same type, and on 32-bit `size_t` is `unsigned int` -- same width, same
# signedness, a different type -- so gcc 14, which promoted
# -Wincompatible-pointer-types to an error, stops the build. It is upstream's bug
# in a vendored module, the mismatch is harmless at this width, and the
# alternative would be changing the device away from ch4:ofi -- which would make
# this row differ from the 64-bit ones in two variables instead of the one it
# exists to isolate. Drop the flag when a libfabric that compiles here lands.
#
# **It rides in CFLAGS again as of 5.0.2rc2**, which is where it belongs and where
# it was until 5.0.2rc1. For one pin it had to ride in CC instead: upstream
# b99300bad emptied the USER_* snapshot that PAC_RESET_ALL_FLAGS restores before
# each embedded module's configure, so CFLAGS reached src/mpl, src/pmi, romio and
# hydra but not modules/libfabric, and all four ILP32 legs of run 34371453201
# died at ofi_cma.h:67. That is fixed -- pmodels/mpich#7959, fixed by #7960,
# shipped in 5.0.2rc2 as e8de23b0b, and verified in dev/mpich-user-cflags/ to
# repair it without reintroducing the mpicc leak #7921 existed to stop. CFLAGS is
# the narrower place for the flag: CC is baked into mpicc, so the workaround
# followed the row into the wrapper's own build and had to be stripped back out
# again.
#
# -g -O2 is restated because setting CFLAGS at all replaces autoconf's default,
# and an unoptimised MPI would make an already long row much longer.
export CFLAGS="${CFLAGS:--g -O2} -Wno-error=incompatible-pointer-types"

if [ -x "$prefix/bin/mpicc" ]; then
  step "MPICH $version is already installed at $prefix"
else
  step "building MPICH $version for i386 (this is the slow half)"
  MPI_SRC_DIR=$OUT/mpi-src "$SRC/ci-scripts/install-mpich.sh" "$prefix" "$version" \
    || { echo "MPICH build failed" >&2; exit 1; }
fi
command -v ccache >/dev/null && ccache --show-stats 2>/dev/null | head -5

# **The workaround flag must not reach the wrapper's own build, and that is now
# checked rather than arranged.** While the flag rode in CC it was baked into
# mpicc and had to be stripped back out; with it in CFLAGS, MPICH 5.0.2 does not
# put build flags into the compiler wrappers at all -- that is what #7921 fixed,
# and #7960 preserved. So this is a property to verify, not a step to perform.
#
# It is worth verifying rather than assuming, because MPICH has had this wrong in
# both directions inside two releases: 5.0.1 leaked build CFLAGS into mpicc, and
# 5.0.2rc1 then dropped them from the embedded modules. If a future release leaks
# again, -Wincompatible-pointer-types -- a diagnostic about *our* generated
# conversion code, which must stay an error -- would be quietly demoted for
# everything this row builds. Failing here is the right outcome; the fix would be
# to strip it, as this script did at the 5.0.2rc1 pin (git log).
#
# `mpicc -show` is the assertion, not a grep: it prints the command line the
# wrapper would actually run, so it answers the question wherever MPICH chose to
# keep the value. A wrapper that cannot even -show is itself a failure here.
step "checking that no compiler wrapper passes the workaround flag to user code"
for w in mpicc mpicxx mpifort; do
  [ -x "$prefix/bin/$w" ] || continue
  shown=$("$prefix/bin/$w" -show 2>&1) \
    || { echo "$0: $prefix/bin/$w -show failed:" >&2; echo "$shown" >&2; exit 1; }
  case $shown in
    *Wno-error=incompatible-pointer-types*)
      echo "$0: $w passes the libfabric workaround flag through to user code:" >&2
      echo "  $shown" >&2
      exit 1 ;;
  esac
done
echo "  no wrapper passes -Wno-error=incompatible-pointer-types to user code"

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
