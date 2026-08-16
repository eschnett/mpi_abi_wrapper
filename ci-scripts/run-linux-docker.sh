#!/usr/bin/env bash
# Run linux-test.sh in a container, from any host that has Docker.
#
#   ci-scripts/run-linux-docker.sh              # both distro MPIs, each on its own image
#   ci-scripts/run-linux-docker.sh mpich        # one
#   ci-scripts/run-linux-docker.sh floor        # the MPI-3.0 floor, built from source
#   MPIABI_IMAGE=ubuntu:24.04 ci-scripts/run-linux-docker.sh   # override the image
#
# MPIABI_LINUX_SCRIPT names what runs inside; it defaults to linux-test.sh
# (build and ctest), the `floor` row uses linux-floor.sh, and S7's suite runner
# is the third:
#
#   MPIABI_LINUX_SCRIPT=/src/ci-scripts/suite/linux-suite.sh \
#     MPIABI_LINUX_OUT=$PWD/build/linux-out \
#     ci-scripts/run-linux-docker.sh openmpi
#
# MPIABI_LINUX_OUT, if set, is mounted read-write at /out and is where the
# inner script puts anything worth keeping -- the suite's TAP file and its
# logs, which are otherwise destroyed with the container and are exactly what
# triaging a failure needs.
#
# This exists because the developers' machines are macOS and the failure modes
# are not the same: the first Linux build of this project needed four fixes that
# macOS could not have shown, from _GNU_SOURCE to a GNU-vs-BSD difference in
# `patch`. Reaching Linux early, from the machine the work happens on, is worth
# a small script.
#
# The source tree is mounted **read-only** on purpose. That is what caught the
# `patch -o -` bug -- GNU patch writes a temporary file into the current
# directory -- and it keeps a Linux build from leaving artifacts in a macOS
# checkout. All build output goes to /tmp inside the container.
set -euo pipefail

# The default image is per-MPI, and neither default is arbitrary. MPIABI_IMAGE
# overrides both.
#
#   mpich -> debian:13. **Ubuntu 24.04's MPICH cannot run a multi-rank job at
#     all**, and does not say so: its libmpi is built --with-pmix and imports
#     PMIx_Init and no other PMI entry point, while the hydra it ships is a
#     PMI-1 server setting PMI_FD/PMI_RANK/PMI_SIZE that this libmpi never
#     reads. PMIx_Init finds no server, MPICH falls back to singleton
#     initialization, and `mpiexec -n 2` returns two processes that each report
#     "rank 0 of 1" and exit 0. There is no workaround inside that image --
#     Ubuntu 24.04 packages no PMIx launcher (no prrte, no prte, no prun) -- so
#     the fix is an image whose MPICH is coherent. Debian 13's MPICH 4.2.1 links
#     no libpmix and hydra's PMI-1 is what its libmpi speaks (HISTORY.md 2.14).
#
#   openmpi -> ubuntu:24.04, and this one must not drift: S7's expected-failure
#     list is calibrated against **the Open MPI 4.1.6 that Ubuntu 24.04 ships**
#     (ci-scripts/suite/xfail-openmpi.txt says so in its header, and its 168
#     lines are version-specific). Moving this row to another image invalidates
#     that file.
#   floor -> ubuntu:20.04, and this one is the image as much as it is the MPI:
#     MPICH 3.1.4's configure rejects gcc 11's gfortran and gcc 13's, so gcc 9
#     is what the row can be built with. See ci-scripts/linux-floor.sh.
image_for() {
  case ${MPIABI_IMAGE:-} in ?*) printf '%s' "$MPIABI_IMAGE"; return ;; esac
  case $1 in
    mpich) printf 'debian:13' ;;
    floor) printf 'ubuntu:20.04' ;;
    *)     printf 'ubuntu:24.04' ;;
  esac
}

# `floor` provisions its own MPI before running the same tests, since no current
# distribution packages anything as old as MPI-3.0's floor.
inner_for() {
  case ${MPIABI_LINUX_SCRIPT:-} in ?*) printf '%s' "$MPIABI_LINUX_SCRIPT"; return ;; esac
  case $1 in
    floor) printf '/src/ci-scripts/linux-floor.sh' ;;
    *)     printf '/src/ci-scripts/linux-test.sh' ;;
  esac
}

src=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
mpis=("$@")
# `floor` is not in the default set: it builds an MPI from source and takes
# some fifteen minutes, against under two for each of the rows below.
[ ${#mpis[@]} -gt 0 ] || mpis=(mpich openmpi)

command -v docker >/dev/null || { echo "docker not found" >&2; exit 2; }

status=0
for mpi in "${mpis[@]}"; do
  image=$(image_for "$mpi")
  inner=$(inner_for "$mpi")
  printf '\n########## %s on %s ##########\n' "$mpi" "$image"
  docker run --rm \
    -v "$src:/src:ro" \
    ${MPIABI_LINUX_OUT:+-v "$MPIABI_LINUX_OUT:/out"} \
    -e SRC=/src \
    "$image" bash "$inner" "$mpi" ${MPIABI_LINUX_ARGS:-} || status=1
done

printf '\n########## %s\n' \
  "$([ $status -eq 0 ] && echo 'all MPIs passed' || echo 'FAILURES above')"
exit $status
