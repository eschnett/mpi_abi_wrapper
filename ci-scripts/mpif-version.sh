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
# **A fixed ref, never a branch.** The point of these rows is that a change here
# moves them and a change upstream does not; a floating ref would make a red row
# ambiguous between the two, which is the failure mode the pinned MPI commits
# below already avoid. This said "a tag, not a branch" while a tag was the only
# fixed ref mpif_checkout could resolve. A **commit** satisfies the rule at least
# as well -- a tag can be moved, a commit cannot -- so the function below takes
# either, and MPIF_VERSION is a commit today for the reason the next paragraph
# gives. What is still banned is `main`.

# The released version. Bump deliberately, and expect the expected-failure
# files under ci-scripts/mpif-xfail/ to need re-measuring when you do.
#
# **v1.0.0 cannot be built and is not a candidate.** Its
# ci-scripts/install-mpi-header.sh clones mpi-abi-stubs unpinned, and upstream
# has since taken two of the three hunks of its fortran/mpi.h.patch, so `patch`
# reports "Reversed (or previously applied) patch detected!", ignores 3 of 3
# hunks and exits non-zero -- after the full MPI build, since the header is
# installed last. Both `abi` rows failed that way in runs 32867153761 and
# 32868258767. mpif 40c165d, released as v1.0.1, pins the stubs to a8470014
# and drops the two retired hunks; the header dev/vendor/mpi-abi-stubs/
# VERSION.md describes is that same commit, which is why our own
# doc/mpi.h.patch faces the identical choice.
#
# v1.0.1 moves neither MPICH_COMMIT nor OMPI_COMMIT (both checked against the
# artifact: ab53493d and 003e0ca0, unchanged), so the wrap target and the
# reference are the same two MPIs they were -- the bump changes who provides
# the ABI header and nothing about what is being compared.
#
# **Why this is a commit and not v1.0.1.** Under v1.0.1 the Open MPI half of the
# pin was `OMPI_COMMIT=003e0ca0`, a direct ancestor of open-mpi/ompi's `main`
# (`main` 155 ahead of it, 0 behind), so the two `abi` rows tracked Open MPI's
# *development branch* -- the one thing the paragraph above says these rows exist
# to avoid, and it was true silently. mpif has since moved that pin to
# `a7b1e6d6`, which is `v6.0.0rc1^{}` exactly, and that is the revision these rows
# should build. Three things about the new target, each checked rather than
# assumed:
#
#   * **v6.0.0rc1 is a tag; the branch is `v6.0.x`.** `git ls-remote` shows both.
#   * **It is not a fast-forward from the old pin.** v6.0.0rc1 and 003e0ca0 have
#     *diverged* -- 685 ahead, 692 behind -- because v6.0.x forked off `main`. So
#     this is a change of line rather than a bump along one, and the
#     ci-scripts/mpif-xfail/ warning below applies with more force than usual.
#   * **v6.0.0rc1 does implement the ABI**, which is the bar install-abi-mpi.sh
#     sets and the only reason a released tarball cannot serve that role:
#     config/ompi_configure_options.m4 has `AC_ARG_ENABLE([standard-abi])` and its
#     VERSION says `mpi_standard_version=5` / `mpi_standard_subversion=0`.
#
# **And why a commit rather than a tag: mpif has not cut one.** The move landed on
# mpif's `main` (0ff0fe37, three commits ahead of v1.0.1 and zero behind -- a
# clean fast-forward) and v1.0.1 is still its newest tag. `main` itself is banned
# above, and copying OMPI_COMMIT into this repository is what install-git-mpi.sh's
# header forbids, so the remaining fixed ref is mpif's tip commit. Bump this to
# the tag when mpif tags it; nothing else needs to change, because the value is
# still read out of mpif's own installer rather than duplicated here.
#
# The delta from v1.0.1 is the Open MPI pin and its dropped fbtl patch, an i_fcoll
# row retired on 24.04, a libevent warning filter and one predefined-types test
# edit. MPICH_COMMIT is untouched at ab53493d -- checked against both revisions --
# so the MPICH `abi` rows build exactly what they built.
MPIF_VERSION=${MPIF_VERSION:-0ff0fe377230256ad89286c93f27487266662d75}
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
    # Two forms, because `git clone --branch` resolves a tag or a branch and
    # **not** a commit -- it fails with "Remote branch <sha> not found in
    # upstream origin", which is what a first attempt at pinning mpif's tip
    # produced. The fetch-then-checkout form below is the one install-git-mpi.sh
    # already uses against open-mpi/ompi and pmodels/mpich, so the dependency on
    # GitHub serving an arbitrary reachable SHA to `fetch` is one this repository
    # already has rather than a new one.
    #
    # --depth 1 either way: this needs the tree, never the history.
    if [ "${#MPIF_VERSION}" -ge 7 ] &&
           [ -z "$(printf '%s' "${MPIF_VERSION}" | tr -d '0-9a-f')" ]; then
        mkdir -p "${dir}"
        (
            cd "${dir}"
            git init --quiet .
            git remote add origin "${MPIF_REPO}"
            git fetch --quiet --depth 1 origin "${MPIF_VERSION}"
            git checkout --quiet "${MPIF_VERSION}"
        )
    else
        git clone --quiet --depth 1 --branch "${MPIF_VERSION}" \
            "${MPIF_REPO}" "${dir}"
    fi
    echo "${MPIF_VERSION}" >"${dir}/.mpif-version"
    MPIF_SRC=${dir}
}
