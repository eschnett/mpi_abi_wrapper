#!/usr/bin/env bash

# Build and install a pinned, released MVAPICH from source. See
# install-mpich.sh's comment block for why this is a stock configure with no
# patches, no pruning and no header substitution (NOTES.md #9).
#
# Usage: install-mvapich.sh <prefix> [<version>]
#
# Environment:
#   CC, CXX, FC     compilers to build MVAPICH with (default: system compilers)
#   MPI_SRC_DIR     where to download and unpack. Defaults to a temporary
#                   directory removed afterwards; set it to something
#                   persistent to let CI cache the tarball and unpacked tree
#                   across runs, keyed on <version>.
#
# **Why this row exists at all**, since MVAPICH is MPICH-derived and the
# conversion tables are therefore already exercised: NOTES.md #9's matrix says
# it, and the answer is not the conversion tables. What a third implementation
# adds is a third *installation* shape -- its own library naming, its own
# mpicc, its own launcher -- which is where decision 19's "all three
# consumption routes must build and run a program" gets tested against
# something nobody tuned it for. The same paragraph extends the argument to
# Intel MPI, which ci.yaml's `linux-oneapi` job is; Cray MPICH is the third
# name there and cannot be a public CI row.
#
# <version> defaults to 4.1, the current release of the MVAPICH (MVP) series.
# Four things about it were measured from the unpacked tarball rather than read
# off a page, because the vendor's own 4.1 quickstart still says "integrated and
# ABI compatible with MPICH-3.4.3" and that is stale by two major versions:
#
#   1. src/include/mpi.h.in declares MPI_VERSION 4 / MPI_SUBVERSION 1.
#   2. src/include/mpi_proto.h, which mpi.h includes, holds 387 `_c`
#      prototypes. So this is the **second implementation in CI carrying the
#      ABI's large-count surface** -- NOTES.md #9's version table has MPICH
#      >= 4.0 as the only one, and Open MPI 5.0.10 still has no `_c` entry
#      point at all. Decision 6's stubs are as nearly-empty here as over MPICH.
#   3. configure.ac has --enable-mpi-abi (default no) and the tree has
#      src/binding/abi/mpi_abi.h, which places the base at MPICH 4.3-or-later,
#      not 3.4.3.
#   4. src/pm/hydra is the process manager, so `mpiexec` is Hydra and this row
#      has the same launcher shape as the from-source MPICH row rather than a
#      new one to diagnose.
#
# **This deliberately does not pass --enable-mpi-abi**, for exactly the reason
# install-mpich.sh spells out at length: wrapping a library that already exports
# the standard ABI is a different oracle from this one (NOTES.md #10's fifth).
# Point 3 above is why the sentence has to be repeated here rather than assumed
# absent -- MVAPICH 4.1 carries the flag, so a stock configure is a choice.
#
# **No --with-device either, and that is the load-bearing one.** MVAPICH is an
# InfiniBand-first implementation and its documented configurations name
# ch4:ucx for IB/RoCE and ch4:ofi for Slingshot/OPX/PSM3, none of which a
# GitHub runner has. A stock configure needs none of them: configure.ac
# defaults with_device=ch4, ch4's default netmod is ofi over the libfabric
# bundled in modules/libfabric, and configure's own epilogue for that case says
# it "should work for TCP networks". So the fabric question that looked like
# this row's main risk is answered by taking no position on it -- the same
# stock-configure rule that covers the other two installers, not an exception
# to it.
#
# **Two packages are prerequisites, though, and this is where "MPICH-derived so
# its build is MPICH's" stops being true.** MVAPICH's bundled libfabric carries
# two providers MPICH's does not -- prov/mverbs and prov/ucr -- and both include
# <infiniband/ib.h> unconditionally, so a stock `configure && make` gets all the
# way through configure and every module before dying at
# prov/ucr/src/ucr_domain.lo with "infiniband/ib.h: No such file or directory".
# Measured on ubuntu:24.04/aarch64 (dev/third-implementations/). The caller must
# provide:
#
#   libibverbs-dev librdmacm-dev
#
# and that is all -- headers only, no hardware, no kernel module, no runtime
# dependency on a fabric existing, and above all no configure flag. This script
# deliberately does not install them itself: the other two installers touch no
# package manager either, and ci.yaml installs them on the MVAPICH legs alone
# rather than on all six. That last point is not tidiness -- the MPI cache key
# hashes these scripts and not the runner's package list, so adding IB headers
# to the MPICH and Open MPI legs would change what their configures find with
# nothing to invalidate their caches.

set -euo pipefail

prefix=${1:-}
version=${2:-4.1}
if [ -z "$prefix" ]; then
  echo "usage: $(basename "$0") <prefix> [<version>]" >&2
  exit 1
fi

nprocs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

src_dir=${MPI_SRC_DIR:-}
cleanup_src=0
if [ -z "$src_dir" ]; then
  src_dir=$(mktemp -d "${TMPDIR:-/tmp}/mvapich-src.XXXXXX")
  cleanup_src=1
else
  # See install-mpich.sh: mktemp creates its directory, a caller-supplied one
  # may not exist, and curl -o into a missing directory fails with exit 23,
  # which reads as a network fault.
  mkdir -p "$src_dir"
fi
# See install-mpich.sh: `[ ... ] && rm` as the whole body returns 1 when the
# test is false, and under `set -e` that EXIT trap status becomes the script's,
# so a successful install exited 1 whenever MPI_SRC_DIR was set.
cleanup() {
  if [ "$cleanup_src" = 1 ]; then
    rm -rf "$src_dir"
  fi
}
trap cleanup EXIT

tarball="$src_dir/mvapich-$version.tar.gz"
tree="$src_dir/mvapich-$version"

# The `mv2` in the path is not a typo and not a version to keep in step with
# <version>: it is MVAPICH2's old download directory, which the 4.x series is
# still published into. Checked against the downloads page rather than guessed
# -- mvapich-4.1.tar.gz and mvapich-4.0.tar.gz both resolve there, and there is
# no `mvapich/mv4/` or bare `mvapich/`.
if [ ! -f "$tarball" ]; then
  echo "downloading mvapich $version"
  curl -fsSL -o "$tarball" \
    "https://mvapich.cse.ohio-state.edu/download/mvapich/mv2/mvapich-$version.tar.gz"
fi

if [ ! -d "$tree" ]; then
  tar -C "$src_dir" -xzf "$tarball"
fi

# Out of tree, as install-mpich.sh does for MPICH 4.x and for the same reasons:
# it keeps a cached MPI_SRC_DIR clean and lets one unpacked source serve more
# than one prefix. There is no 3.1.4-style exception to make here -- that bug
# is specific to that release's generated Fortran module rule -- but note that
# the MVAPICH2 2.3.x series is a different codebase from this one and has not
# been tried out of tree by anything here.
build_dir="$src_dir/build"
rm -rf "$build_dir"
mkdir -p "$build_dir"
(
  cd "$build_dir"
  # Not --disable-fortran, and not --disable-mpi-fortran: install-mpich.sh's
  # comment has the measurement that makes disabling Fortran support look
  # cheaper than it is.
  "$tree/configure" --prefix="$prefix"
  make -j"$nprocs"
  make install
)

echo "mvapich $version installed to $prefix"
