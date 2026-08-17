#!/usr/bin/env bash

# Build and install a pinned, released Open MPI from source. See
# install-mpich.sh's comment block for why this is a stock configure with no
# patches, no pruning and no header substitution -- everything mpif's
# equivalent needs and this project does not (NOTES.md #9).
#
# Usage: install-openmpi.sh <prefix> [<version>]
#
# Environment:
#   CC, CXX, FC     compilers to build Open MPI with (default: system compilers)
#   MPI_SRC_DIR     where to download and unpack. Defaults to a temporary
#                   directory removed afterwards; set it to something
#                   persistent to let CI cache the tarball and unpacked tree
#                   across runs, keyed on <version>.
#
# <version> defaults to 5.0.6, this project's primary Open MPI row (NOTES.md
# #9's version table: sessions, the current component architecture). Pass
# 4.1.x for the secondary row -- wrappable, and one of the platforms S1 ran
# on Linux.

set -euo pipefail

prefix=${1:-}
version=${2:-5.0.6}
if [ -z "$prefix" ]; then
  echo "usage: $(basename "$0") <prefix> [<version>]" >&2
  exit 1
fi
series=${version%.*}

nprocs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

src_dir=${MPI_SRC_DIR:-}
cleanup_src=0
if [ -z "$src_dir" ]; then
  src_dir=$(mktemp -d "${TMPDIR:-/tmp}/openmpi-src.XXXXXX")
  cleanup_src=1
else
  # See install-mpich.sh: mktemp creates its directory, a caller-supplied one
  # may not exist, and curl -o into a missing directory fails with exit 23,
  # which reads as a network fault.
  mkdir -p "$src_dir"
fi
cleanup() { [ "$cleanup_src" = 1 ] && rm -rf "$src_dir"; }
trap cleanup EXIT

tarball="$src_dir/openmpi-$version.tar.bz2"
tree="$src_dir/openmpi-$version"

if [ ! -f "$tarball" ]; then
  echo "downloading openmpi $version"
  curl -fsSL -o "$tarball" \
    "https://download.open-mpi.org/release/open-mpi/v$series/openmpi-$version.tar.bz2"
fi

if [ ! -d "$tree" ]; then
  tar -C "$src_dir" -xjf "$tarball"
fi

build_dir="$src_dir/build"
rm -rf "$build_dir"
mkdir -p "$build_dir"
(
  cd "$build_dir"
  # Stock configure, no --disable-mpi-fortran: install-mpich.sh's comment
  # explains why disabling Fortran support is not the free win it looks like
  # -- MPICH 3.1.4 ties three plain C entry points' implementations to it.
  # Not measured against Open MPI specifically, but there is no reason to
  # take the same risk for a build-time saving neither installer needs (this
  # project's own mpicc/mpicxx never touch Fortran; NOTES.md #9, "mpifort
  # deferring to mpif").
  "$tree/configure" --prefix="$prefix"
  make -j"$nprocs"
  make install
)

echo "openmpi $version installed to $prefix"
