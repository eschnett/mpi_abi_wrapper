#!/usr/bin/env bash

# Build and install an ordinary, **unpruned, non-ABI** MPI from the same commit
# that ci-scripts/install-abi-mpi.sh builds with the ABI enabled. This is the
# wrap target of NOTES.md #10's mpif rows.
#
# Usage: install-git-mpi.sh <mpich|openmpi> <prefix>
#
# Environment:
#   CC, CXX, FC     compilers to build the MPI with
#   MPI_SRC_DIR     where to clone and build; share it with install-abi-mpi.sh
#                   and the clone and autogen happen once for both roles.
#   MPIF_SRC_DIR    where to find or clone mpif, whose installers carry the pin.
#
# **Why two builds of one commit.** The wrapper cannot be pointed at an ABI
# prefix: mpif's installers prune away the implementation's own mpi.h and
# libmpi, which are exactly what libmpiwrapper links. So the reference leg and
# the wrapper leg need different prefixes -- and building both from *one*
# commit is what makes the comparison mean something, since the two legs then
# differ in one thing only, which is who provides the ABI.
#
# **Why not a released tarball.** ci-scripts/install-mpich.sh and
# install-openmpi.sh do exactly that and are the right thing for every other
# row: a stock release is what an ordinary user wraps. Here the wrap target has
# to match the reference, and the reference cannot be a release -- MPICH
# 5.0.1's standard-ABI implementation does not work, and no released Open MPI
# has one. Pinning the same commit for both is the whole point of this script.
#
# **The pin is read out of mpif's installer, never copied.** mpif already uses
# this trick on itself, to keep MPICH_VERSION in one file; a hash copied here
# would be a second source of truth that drifts silently, and the symptom would
# be two legs quietly testing different MPIs -- which is the one thing these
# rows must not do.

set -euo pipefail

which=${1:-}
prefix=${2:-}
case ${which} in
    mpich|openmpi) ;;
    *) echo "usage: $(basename "$0") <mpich|openmpi> <prefix>" >&2; exit 1 ;;
esac
if [ -z "${prefix}" ]; then
    echo "usage: $(basename "$0") <mpich|openmpi> <prefix>" >&2
    exit 1
fi

scriptdir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
# shellcheck source=ci-scripts/mpif-version.sh
. "${scriptdir}/mpif-version.sh"

nprocs=$(getconf _NPROCESSORS_ONLN 2>/dev/null ||
             sysctl -n hw.ncpu 2>/dev/null || echo 4)

src_dir=${MPI_SRC_DIR:-}
cleanup_src=0
if [ -z "${src_dir}" ]; then
    src_dir=$(mktemp -d "${TMPDIR:-/tmp}/git-mpi-src.XXXXXX")
    cleanup_src=1
else
    mkdir -p "${src_dir}"
fi
cleanup() {
    if [ "${cleanup_src}" = 1 ]; then
        rm -rf "${src_dir}"
    fi
}
trap cleanup EXIT

mpif_checkout "${MPIF_SRC_DIR:-${src_dir}/mpif}"

# The pin, by name, out of the file that owns it. A miss is fatal rather than
# an empty ref: `git checkout ""` would silently leave the clone on its default
# branch, which is the drift this reads the file to avoid.
case ${which} in
    mpich)
        pin_var=MPICH_COMMIT
        upstream=https://github.com/pmodels/mpich.git
        ;;
    openmpi)
        pin_var=OMPI_COMMIT
        upstream=https://github.com/open-mpi/ompi.git
        ;;
esac
commit=$(sed -n "s/^${pin_var}=\\([0-9a-f]\\{7,\\}\\).*/\\1/p" \
             "${MPIF_SRC}/ci-scripts/install-${which}.sh" | head -1)
if [ -z "${commit}" ]; then
    echo "could not read ${pin_var} from mpif ${MPIF_VERSION}'s" \
         "ci-scripts/install-${which}.sh -- it has moved or been renamed," \
         "and this script must not guess" >&2
    exit 1
fi

tree=${src_dir}/${which}-stock
# The stamp covers the commit *and* this file, so that a change to the
# configure flags below rebuilds rather than reusing a tree configured with the
# old ones. Same discipline as mpif's own installers.
stamp=${tree}/.prepared-${commit}-$(cksum <"${BASH_SOURCE[0]}" | cut -d' ' -f1)

echo "=== stock ${which} at ${commit} (pinned by mpif ${MPIF_VERSION})"
echo "    prefix: ${prefix}"

if [ ! -f "${stamp}" ]; then
    rm -rf "${tree}"
    mkdir -p "${tree}"
    (
        cd "${tree}"
        git init --quiet .
        git remote add origin "${upstream}"
        # Fetching one commit rather than cloning: these repositories are large
        # and the history is not wanted.
        git fetch --quiet --depth 1 origin "${commit}"
        git checkout --quiet "${commit}"
        # **Both implementations, and without --depth.** MPICH needs its
        # submodules as much as Open MPI does -- modules/hwloc, json-c and
        # yaksa -- and its autogen.sh stops with "Submodule modules/hwloc is
        # not checked out" rather than trying to continue. Doing this only for
        # Open MPI is what a first attempt did.
        #
        # No --depth 1: a shallow submodule fetch gets the branch tip, which
        # need not be the commit the superproject records. mpif's installers
        # spell it exactly this way and this script exists to differ from them
        # in the configure flags and nothing else.
        git submodule update --init --recursive --quiet
        if [ "${which}" = openmpi ]; then
            ./autogen.pl
        else
            ./autogen.sh
        fi
    )
    touch "${stamp}"
fi

build_dir=${src_dir}/${which}-stock-build
rm -rf "${build_dir}"
mkdir -p "${build_dir}"
(
    cd "${build_dir}"
    # **Stock, and in particular no ABI flag.** --enable-mpi-abi /
    # --enable-standard-abi is what makes the *other* prefix; this one is what
    # an ordinary user has, and what libmpiwrapper is designed to wrap.
    #
    # Not --disable-fortran, for NOTES.md #9's reason: it silently drops the
    # implementations of MPI_Type_create_f90_{real,complex,integer}, which are
    # plain C entry points MPI-5.0 requires, so the compile-only probe reports
    # them available and only the wrapper's link step fails. It would also take
    # MPI_LOGICAL down with it, which decision 25's second gate reads.
    "${tree}/configure" --prefix="${prefix}"
    make -j"${nprocs}"
    make install
)

echo "=== stock ${which} installed to ${prefix}"
