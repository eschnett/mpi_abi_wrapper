#!/usr/bin/env bash

# Build and install a pinned, released MPICH from source. NOTES.md #9
# ("Provisioning MPI in CI"): unlike mpif, which has to fetch an ABI-
# implementing MPICH from `main`, patch out its own mpi.h and prune what
# does not belong on the ABI, this project wraps *any* MPI-3.0+ -- stock
# configure, no patches, no pruning, no header substitution. What sits in
# the resulting prefix is exactly what upstream ships.
#
# Usage: install-mpich.sh <prefix> [<version>]
#
# Environment:
#   CC, CXX, FC     compilers to build MPICH with (default: system compilers)
#   MPI_SRC_DIR     where to download and unpack. Defaults to a temporary
#                   directory removed afterwards; set it to something
#                   persistent to let CI cache the tarball and unpacked tree
#                   across runs (keyed on <version>, since that is the whole
#                   of what identifies the source here -- ci-scripts/README.md
#                   explains why this file, not ci-scripts/suite/, is what a
#                   cache key should hash).
#
# <version> defaults to 4.3.1: MPICH >= 4.0 is this project's primary row
# (NOTES.md #9's version table), since it is the only implementation that
# actually provides the ABI's `_c` large-count entry points. Pass 3.1.4 for
# the MPI-3.0 floor row instead -- verified, not just declared, per the same
# table -- but that row needs an older toolchain pinned by hand: 3.1.4's own
# configure rejects any Fortran compiler modern enough to warn rather than
# error on a mismatched-argument call (measured here: gcc 13's gfortran, and
# gcc 11's). gcc 9, which ubuntu:20.04 ships, is accepted, and
# ci-scripts/linux-floor.sh is the row wired up end to end. Both of those are
# this release's own limitations rather than this script's; the out-of-tree
# one below is the third.

set -euo pipefail

prefix=${1:-}
version=${2:-4.3.1}
if [ -z "$prefix" ]; then
  echo "usage: $(basename "$0") <prefix> [<version>]" >&2
  exit 1
fi

nprocs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

src_dir=${MPI_SRC_DIR:-}
cleanup_src=0
if [ -z "$src_dir" ]; then
  src_dir=$(mktemp -d "${TMPDIR:-/tmp}/mpich-src.XXXXXX")
  cleanup_src=1
else
  # mktemp creates its directory and a caller-supplied one may not exist, so
  # the two branches disagreed about whose job this was and the answer was
  # nobody's: curl -o into a missing directory fails with "Failure writing
  # output to destination" (exit 23), which reads like a network fault rather
  # than a missing mkdir. Only ever hit on the path this script's own header
  # invites CI to take, which is why it survived until a workflow took it.
  mkdir -p "$src_dir"
fi
cleanup() { [ "$cleanup_src" = 1 ] && rm -rf "$src_dir"; }
trap cleanup EXIT

tarball="$src_dir/mpich-$version.tar.gz"
tree="$src_dir/mpich-$version"

if [ ! -f "$tarball" ]; then
  echo "downloading mpich $version"
  curl -fsSL -o "$tarball" \
    "https://www.mpich.org/static/downloads/$version/mpich-$version.tar.gz"
fi

if [ ! -d "$tree" ]; then
  tar -C "$src_dir" -xzf "$tarball"
fi

# 4.x builds out of tree, which keeps a cached MPI_SRC_DIR clean and lets one
# unpacked source serve more than one prefix. **3.1.4 cannot be built out of
# tree at all**, and fails in a way that reads like something else: the
# generated rule for src/binding/fortran/use_mpi/mpi_c_interface_types.mod
# names its input relative to the *build* directory --
# src/binding/fortran/use_mpi_f08/mpi_c_interface_types.F90, which exists only
# in the source tree -- so gfortran stops with "no input files". It looks like a
# parallel-make race and is not one: -j1 fails identically, and the file is
# present in the tarball the whole time. So the floor row configures in its own
# source tree, and pays for it by leaving build artifacts there when
# MPI_SRC_DIR is a cache.
case $version in
  3.*) build_dir=$tree ;;
  *)   build_dir="$src_dir/build"; rm -rf "$build_dir"; mkdir -p "$build_dir" ;;
esac
(
  cd "$build_dir"
  # Not --disable-fortran: measured against 3.1.4, disabling it takes
  # MPI_Type_create_f90_{real,complex,integer} down with it -- those three are
  # plain C entry points MPI-5.0 requires, declared in mpi.h unconditionally,
  # but this release's build ties their *implementation* to the Fortran
  # support layer. dev/probe_impl.py's compile-only probe sees the
  # declaration and reports them available; only the link step in
  # ci-scripts/check-install.sh caught the mismatch. Stock configure avoids
  # the question entirely.
  "$tree/configure" --prefix="$prefix"
  make -j"$nprocs"
  make install
)

echo "mpich $version installed to $prefix"
