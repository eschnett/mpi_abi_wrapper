#!/usr/bin/env bash
# The pinned mpif, and the one place its version is written down.
#
# Sourced by install-abi-mpi.sh, install-git-mpi.sh and test-mpif.sh, all three
# of which need the same checkout: the first two build the MPIs *from mpif's
# own installers*, and the third builds mpif itself. A version skew between
# them would be a test of nothing in particular.
#
# mpif is NOTES.md #1's fourth consumer and #10's fifth oracle, and the only
# one that reaches the 26 Fortran converters, the six MPI_Abi_* entry points
# and the status f2c/c2f paths. Running it found three defects in this library
# on its first outing (HISTORY.md #2.18 and decisions 24 and 25), which is the
# argument for the rows it feeds.
#
# **A tag, not a branch.** The point of these rows is that a change here moves
# them and a change upstream does not; a floating ref would make a red row
# ambiguous between the two, which is the failure mode the pinned MPI commits
# below already avoid.

# The released version. Bump deliberately, and expect the expected-failure
# files under ci-scripts/mpif-xfail/ to need re-measuring when you do.
MPIF_VERSION=${MPIF_VERSION:-v1.0.0}
MPIF_REPO=${MPIF_REPO:-https://github.com/eschnett/mpif}

# Clone the pinned mpif into $1, or leave an existing checkout of the right
# version alone. Sets MPIF_SRC to the checkout.
mpif_checkout() {
    local dir=$1
    if [ -z "${dir}" ]; then
        echo "mpif_checkout: no directory given" >&2
        return 1
    fi

    # The stamp is the version, so a cached directory from a previous version
    # is replaced rather than reused. Without it a bump to MPIF_VERSION would
    # silently keep testing the old one wherever the directory is cached.
    if [ -f "${dir}/.mpif-version" ] &&
           [ "$(cat "${dir}/.mpif-version")" = "${MPIF_VERSION}" ]; then
        MPIF_SRC=${dir}
        return 0
    fi

    rm -rf "${dir}"
    # --depth 1 against a tag: this needs the tree, never the history.
    git clone --quiet --depth 1 --branch "${MPIF_VERSION}" \
        "${MPIF_REPO}" "${dir}"
    echo "${MPIF_VERSION}" >"${dir}/.mpif-version"
    MPIF_SRC=${dir}
}
