#!/usr/bin/env bash
# Build the MPI-3.0 floor and run this project's tests against it, on Linux.
#
#   ci-scripts/run-linux-docker.sh floor
#
# Runs *inside* a Linux environment, like linux-test.sh, which it provisions an
# MPI for and then hands over to. run-linux-docker.sh knows to give the `floor`
# row ubuntu:20.04 and this script; nothing else defaults to either.
#
# NOTES.md #9's version table makes MPI-3.0 the floor and says it is verified
# rather than declared. This is the verification, and it is a separate script
# because the row cannot use a distro package: no current distribution ships
# anything as old as MPICH 3.1.4, so the MPI has to be built, which takes some
# fifteen minutes and is why `floor` is opt-in rather than one of the two rows
# run-linux-docker.sh does by default.
#
# **The image is part of the row.** 3.1.4's configure rejects any Fortran
# compiler modern enough to warn rather than error on a mismatched-argument
# call, which rules out gcc 11's gfortran and gcc 13's -- so ubuntu:24.04 and
# debian:13 are both out, and ubuntu:20.04's gcc 9 is what is left. Disabling
# Fortran instead is not an option: it takes
# MPI_Type_create_f90_{real,complex,integer} down with it, and those are plain
# C entry points MPI-5.0 requires (ci-scripts/install-mpich.sh says more).
#
# Environment:
#   MPIABI_FLOOR_VERSION   MPICH to build (default 3.1.4)
#   MPIABI_FLOOR_PREFIX    where to install it (default /tmp/mpi-floor)
set -uo pipefail

version=${MPIABI_FLOOR_VERSION:-3.1.4}
prefix=${MPIABI_FLOOR_PREFIX:-/tmp/mpi-floor}
here=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

# run-linux-docker.sh passes the row name through; accept and ignore it, so the
# call shape is the same as linux-test.sh's.
case ${1:-floor} in
  floor|mpich) ;;
  *) echo "usage: $(basename "$0") [floor]" >&2; exit 2 ;;
esac

if [ "$(id -u)" = 0 ] && command -v apt-get >/dev/null; then
  export DEBIAN_FRONTEND=noninteractive
  # gfortran is required rather than incidental -- see the header. curl and
  # ca-certificates are for the tarball; the rest is what linux-test.sh needs
  # once it takes over, installed here so that it finds them already present.
  pkgs="build-essential gfortran cmake python3 python3-pip patch binutils"
  pkgs="$pkgs poppler-utils curl ca-certificates"
  printf '\n=== installing %s\n' "$pkgs"
  apt-get update -qq >/dev/null && apt-get install -y -qq $pkgs >/dev/null \
    || { echo "package install failed" >&2; exit 2; }

  # The image is pinned by what MPICH 3.1.4's configure accepts, and what it
  # accepts is old enough that the distro's CMake is too old for this project:
  # ubuntu:20.04 ships 3.16 and CMakeLists.txt asks for 3.20. That is a
  # property of the image rather than of the floor, so it is fixed here rather
  # than by lowering the project's requirement. pip's wheel installs to
  # /usr/local/bin, which precedes /usr/bin, so the apt cmake above stays put
  # and unused.
  if ! cmake --version | head -1 | awk '{split($3,v,"."); exit !(v[1]>3 || (v[1]==3 && v[2]>=20))}'; then
    printf '=== distro cmake is %s; installing a newer one\n' \
      "$(cmake --version | head -1 | awk '{print $3}')"
    pip3 install -q --no-cache-dir cmake \
      || { echo "cmake install failed" >&2; exit 2; }
    hash -r
  fi
  cmake --version | head -1
fi

printf '\n=== building MPICH %s (this is the slow part)\n' "$version"
if ! "$here/install-mpich.sh" "$prefix" "$version" > /tmp/floor-install.log 2>&1; then
  echo "FAILED: building MPICH $version" >&2
  tail -30 /tmp/floor-install.log >&2
  exit 2
fi
"$prefix/bin/mpichversion" | head -2

# From here it is the ordinary row: linux-test.sh takes a path to an mpicc for
# exactly this case, and everything after this point -- the build, ctest, the
# rank-count check, the two informational steps -- is shared with the distro
# rows rather than reimplemented.
printf '\n=== handing over to linux-test.sh\n'
exec "$here/linux-test.sh" "$prefix/bin/mpicc"
