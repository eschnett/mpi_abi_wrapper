#!/usr/bin/env bash

# Build and install an MPI that implements the **standard ABI itself**, by
# delegating to mpif's own installer for it. This is the reference half of
# NOTES.md #10's mpif rows: the leg mpif runs against natively, so that a
# difference in the wrapper leg is attributable.
#
# Usage: install-abi-mpi.sh <mpich|openmpi> <prefix>
#
# Environment:
#   CC, CXX, FC     compilers to build the MPI with
#   MPI_SRC_DIR     where to clone and build; shared with install-git-mpi.sh,
#                   which builds the *same commit* stock. Defaults to a
#                   temporary directory removed afterwards.
#   MPIF_SRC_DIR    where to clone mpif. Defaults inside MPI_SRC_DIR.
#
# **Why mpif's installers rather than our own.** They are pinned to commits
# known to work, they carry the Fortran/C handle converters that neither
# implementation's ABI library provides yet, and they *prune* the installation
# down to the ABI -- the implementation's own mpi.h, modules and non-ABI
# libraries removed, the Forum's ABI mpi.h installed in their place. The
# resulting prefix has the same shape as one of ours (bin/mpicc, include/mpi.h,
# lib/libmpi_abi.*), which is what lets ci-scripts/test-mpif.sh take either
# without knowing which it has. Reimplementing that here would be a second
# source of truth for someone else's build recipe.
#
# **This is the first of the two documented exceptions to "pinned released
# tarballs".** ci-scripts/README.md and NOTES.md #9 say CI provisions MPI from
# released tarballs with a stock configure; that remains the rule for *wrap
# targets* -- what install-mpich.sh and install-openmpi.sh build, the second
# exception being that install-mpich.sh's own pin is a release candidate. It
# cannot hold here at all: no released Open MPI implements the ABI, so mpif pins
# a commit for both roles, and a reference has to be one that functions.
#
# That commit is no longer one off `main`. mpif moved OMPI_COMMIT to a7b1e6d6 =
# `v6.0.0rc1^{}`, the first Open MPI release candidate carrying the standard ABI,
# so the reference is now a tagged RC rather than a development-branch snapshot.
# "No *released* Open MPI implements the ABI" is still exactly true -- an rc is
# not a release -- but the gap is now one tag wide. ci-scripts/mpif-version.sh
# has the checks behind that and why this repository pins mpif by commit.
#
# MPICH's half of that reason is now retired. 5.0.1's libmpi_abi installed as
# .so.0 because -version-info never reached libtool; 5.0.2 fixes it (upstream
# 537078668) and install-mpich.sh is pinned to 5.0.2rc2, so a tarball with the
# intended soname exists. Whether the soname was the only thing missing is
# untested -- these rows also need mpif's header substitution and its pruning of
# everything the ABI does not define -- and the pin is mpif's to move regardless.

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

src_dir=${MPI_SRC_DIR:-}
cleanup_src=0
if [ -z "${src_dir}" ]; then
    src_dir=$(mktemp -d "${TMPDIR:-/tmp}/abi-mpi-src.XXXXXX")
    cleanup_src=1
else
    mkdir -p "${src_dir}"
fi
# An `if` rather than `[ ... ] && rm`: with cleanup_src=0 the test is false, the
# function returns 1, and under `set -e` a non-zero EXIT trap becomes the
# script's exit status -- the exact bug install-mpich.sh's own comment records
# having shipped once.
cleanup() {
    if [ "${cleanup_src}" = 1 ]; then
        rm -rf "${src_dir}"
    fi
}
trap cleanup EXIT

mpif_checkout "${MPIF_SRC_DIR:-${src_dir}/mpif}"

echo "=== building ABI ${which} via mpif ${MPIF_VERSION}'s installer"
echo "    prefix: ${prefix}"
echo "    source: ${src_dir}"

# mpif's installers read MPI_SRC_DIR themselves, for the clone and the autogen
# tree, and they expose MPI_PREPARE_ONLY for the prepare-then-stop step that
# install-git-mpi.sh reuses. Their prefix argument is positional.
MPI_SRC_DIR="${src_dir}" "${MPIF_SRC}/ci-scripts/install-${which}.sh" "${prefix}"

# mpif's own check, run here rather than trusted: it compiles and links a
# program against the prefix, asserts MPI_ABI_VERSION at compile time and
# checks the executable really depends on libmpi_abi. A prefix that still has
# the implementation's own mpi.h compiles that program perfectly well and is
# wrong, which is the failure it exists to catch.
"${MPIF_SRC}/ci-scripts/check-mpi-install.sh" "${prefix}"

echo "=== ABI ${which} installed to ${prefix}"
