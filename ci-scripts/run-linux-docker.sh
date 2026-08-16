#!/usr/bin/env bash
# Run linux-test.sh in a container, from any host that has Docker.
#
#   ci-scripts/run-linux-docker.sh              # both MPIs
#   ci-scripts/run-linux-docker.sh mpich        # one
#   MPIABI_IMAGE=debian:13 ci-scripts/run-linux-docker.sh
#
# MPIABI_LINUX_SCRIPT names what runs inside; it defaults to linux-test.sh
# (build and ctest) and S7's suite runner is the other one:
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

image=${MPIABI_IMAGE:-ubuntu:24.04}
inner=${MPIABI_LINUX_SCRIPT:-/src/ci-scripts/linux-test.sh}
src=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
mpis=("$@")
[ ${#mpis[@]} -gt 0 ] || mpis=(mpich openmpi)

command -v docker >/dev/null || { echo "docker not found" >&2; exit 2; }

status=0
for mpi in "${mpis[@]}"; do
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
