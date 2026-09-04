#!/usr/bin/env bash

# Build mpif against an ABI prefix and run **mpif's own ctest** against it.
# NOTES.md #10's consumer oracle, wired up.
#
# Usage: test-mpif.sh <abi-prefix> [<launcher-prefix>]
#
#   <abi-prefix>       what provides mpi.h and libmpi_abi. Either an ABI MPI
#                      built by install-abi-mpi.sh, or an installation of *this
#                      project* over an ordinary MPI.
#   <launcher-prefix>  what provides mpiexec, and the MPI the tests actually
#                      run on. Defaults to <abi-prefix>.
#
# Environment:
#   CC, FC                  compilers to build mpif with
#   MPIF_SRC_DIR            where to clone mpif
#   MPIF_TEST_MPI_LIBRARY   substring MPI_Get_library_version must contain,
#                           e.g. MPICH or "Open MPI". Defaults to "mpi", which
#                           matches both and asserts less.
#   MPIF_XFAIL              a file of test names expected to fail
#   MPIEXEC_PREFLAGS        extra mpiexec flags (Open MPI needs them on a
#                           runner; MPICH wants none rather than an empty list)
#
# **Two prefixes, because the launcher belongs to the wrapped MPI.** NOTES.md
# #9 makes this project's prefix exclusive -- it holds mpi.h, libmpi_abi,
# libmpiwrapper and the compiler wrappers, and deliberately not a second MPI.
# The launcher is the wrapped implementation's, and the second argument is
# where it comes from. For a native ABI prefix the two are the same directory
# and it can be left off.
#
# What changed with decision 27 is what happens when it is left off for a
# *wrapper* prefix: that prefix now installs a bin/mpiexec of its own, which
# forwards to the wrapped launcher, so the default below resolves instead of
# naming a file that is not there. Both CI legs still pass both prefixes
# explicitly, so nothing here behaves differently -- but the failure mode the
# second argument existed to avoid is gone.
#
# **ctest only.** mpif's MPICH Fortran suite (its ci-scripts/suite/) is a much
# larger, separately-triaged thing and is deliberately not run here.

set -euo pipefail

abi_prefix=${1:-}
launcher_prefix=${2:-${abi_prefix}}
if [ -z "${abi_prefix}" ]; then
    echo "usage: $(basename "$0") <abi-prefix> [<launcher-prefix>]" >&2
    exit 1
fi
for d in "${abi_prefix}" "${launcher_prefix}"; do
    if [ ! -d "${d}" ]; then
        echo "no such prefix: ${d}" >&2
        exit 1
    fi
done

scriptdir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
# shellcheck source=ci-scripts/mpif-version.sh
. "${scriptdir}/mpif-version.sh"

abi_prefix=$(cd "${abi_prefix}" && pwd -P)
launcher_prefix=$(cd "${launcher_prefix}" && pwd -P)

work=$(mktemp -d "${TMPDIR:-/tmp}/mpif-test.XXXXXX")
cleanup_work=1
cleanup() {
    if [ "${cleanup_work}" = 1 ]; then
        rm -rf "${work}"
    fi
}
trap cleanup EXIT

mpif_checkout "${MPIF_SRC_DIR:-${work}/mpif}"

case $(uname -s) in
    Darwin) shlib_ext=dylib ;;
    *)      shlib_ext=so ;;
esac

abi_lib=${abi_prefix}/lib/libmpi_abi.${shlib_ext}
if [ ! -e "${abi_lib}" ]; then
    echo "no libmpi_abi.${shlib_ext} under ${abi_prefix}/lib -- not an ABI" \
         "prefix" >&2
    ls -l "${abi_prefix}/lib" >&2 || true
    exit 1
fi

mpiexec=${launcher_prefix}/bin/mpiexec
if [ ! -x "${mpiexec}" ]; then
    echo "no mpiexec under ${launcher_prefix}/bin -- pass the wrapped MPI's" \
         "prefix as the second argument" >&2
    exit 1
fi

mpif_prefix=${work}/mpif-install
: "${CC:=cc}"
: "${FC:=gfortran}"

echo "=== mpif ${MPIF_VERSION}"
echo "    ABI:      ${abi_prefix}"
echo "    launcher: ${launcher_prefix}"
echo "    CC=${CC} FC=${FC}"

# mpif's own check on the prefix, before anything is built against it: it
# asserts MPI_ABI_VERSION at compile time and that the executable really
# depends on libmpi_abi. A prefix carrying an implementation's own mpi.h
# compiles that program perfectly well and is not an ABI prefix.
"${MPIF_SRC}/ci-scripts/check-mpi-install.sh" "${abi_prefix}"

# ------------------------------------------------------------------ build ---
#
# MPI_C_COMPILER is passed explicitly rather than left to find_package(MPI):
# CMake's FindMPI searches the PATH, and on a machine with any other MPI
# installed it finds that one and mpif stops with "MPI_C_LIBRARIES ... names no
# libmpi_abi". Measured, on a laptop with conda MPICH ahead on PATH.
echo "=== configuring mpif"
cmake \
    -S "${MPIF_SRC}" \
    -B "${work}/build" \
    -DBUILD_SHARED_LIBS=ON \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_C_COMPILER="${CC}" \
    -DCMAKE_Fortran_COMPILER="${FC}" \
    -DCMAKE_INSTALL_PREFIX="${mpif_prefix}" \
    -DMPI_HOME="${abi_prefix}" \
    -DMPI_C_COMPILER="${abi_prefix}/bin/mpicc"
cmake --build "${work}/build" --parallel
cmake --install "${work}/build" >/dev/null

# What the installation *remembers*, read back rather than respecified: this is
# what a user would get, and mpif's own CI reads it the same way.
remembered=$("${mpif_prefix}/bin/mpifort" -showme:mpiprefix)
if [ "${remembered}" != "${abi_prefix}" ]; then
    echo "mpifort remembers ${remembered}, not ${abi_prefix}" >&2
    exit 1
fi

# ------------------------------------------------------------------ tests ---
echo "=== configuring mpif's tests"
cmake_args=()
if [ -n "${MPIEXEC_PREFLAGS:-}" ]; then
    # A guard rather than an unconditional -D: MPICH wants no preflags at all
    # rather than an empty list.
    cmake_args+=("-DMPIEXEC_PREFLAGS=${MPIEXEC_PREFLAGS}")
fi

cmake \
    -S "${MPIF_SRC}/test" \
    -B "${work}/build-tests" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_C_COMPILER="${CC}" \
    -DCMAKE_Fortran_COMPILER="${FC}" \
    -DMPI_C_COMPILER="${abi_prefix}/bin/mpicc" \
    -DMPI_Fortran_COMPILER="${mpif_prefix}/bin/mpifort" \
    -DMPI_C_HEADER_DIR="${abi_prefix}/include" \
    -DMPI_C_LIB_NAMES=mpi_abi \
    -DMPI_mpi_abi_LIBRARY="${abi_lib}" \
    -DMPIEXEC_EXECUTABLE="${mpiexec}" \
    -DMPIF_TEST_MPI_LIBRARY="${MPIF_TEST_MPI_LIBRARY:-mpi}" \
    "${cmake_args[@]}"
cmake --build "${work}/build-tests" --parallel

echo "=== running mpif's ctest"
set +e
ctest --test-dir "${work}/build-tests" --output-on-failure
ctest_status=$?
set -e

# ------------------------------------------------------------ expectations ---
#
# The list is normally empty, and an entry needs a reason beside it. What these
# rows exist to surface is a wrapper-leg failure that the native leg of the
# same implementation does not have; an xfail file that grows without anyone
# comparing the two turns that signal off.
xfail=${MPIF_XFAIL:-}
if [ "${ctest_status}" != 0 ] && [ -n "${xfail}" ] && [ -f "${xfail}" ]; then
    # `ctest --print-failed` does not exist, so the failed set comes from a
    # rerun-failed listing, which is cheap because it runs nothing.
    failed=$(ctest --test-dir "${work}/build-tests" --rerun-failed -N 2>/dev/null |
                 sed -n 's/^ *Test *#[0-9]*: *\(.*\)$/\1/p' | sort -u)
    expected=$(grep -vE '^\s*(#|$)' "${xfail}" | sed 's/[[:space:]]*$//' | sort -u)
    unexpected=$(comm -23 <(echo "${failed}") <(echo "${expected}"))
    if [ -z "${unexpected}" ]; then
        echo "=== all failures are in $(basename "${xfail}"):"
        echo "${failed}" | sed 's/^/    /'
        ctest_status=0
    else
        echo "=== failures NOT in $(basename "${xfail}"):" >&2
        echo "${unexpected}" | sed 's/^/    /' >&2
    fi
fi

if [ "${ctest_status}" != 0 ]; then
    # The build tree is what triaging a failure needs, and it is inside the
    # directory the trap removes.
    cleanup_work=0
    echo "=== mpif tests FAILED; build tree kept at ${work}" >&2
    exit "${ctest_status}"
fi

echo "=== mpif ${MPIF_VERSION} passed against ${abi_prefix}"
