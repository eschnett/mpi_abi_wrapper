#!/usr/bin/env bash
# Run one of the third-implementation probes in a container, from any host with
# Docker. The same shape as ci-scripts/run-linux-docker.sh, and separate from it
# for the same reason dev/ exists at all: these are measurements behind a claim,
# not a row anything gates on.
#
#   dev/third-implementations/run.sh mvapich     # ~25 min: builds from source
#   dev/third-implementations/run.sh intelmpi    # ~5 min on amd64: apt only
#   dev/third-implementations/run.sh             # both
#
# Environment:
#   MPIABI_IMAGE     the image (default ubuntu:24.04, matching the CI rows)
#   MPIABI_PLATFORM  --platform for docker run. Defaults to the host's. Set it
#                    to linux/amd64 on an arm64 host to run the intelmpi probe,
#                    which has no other way to run at all.
#
# **The build directory must be on the container's own filesystem.** MPI_SRC_DIR
# is set to /build below rather than to a bind mount, and that is load-bearing:
# see README.md, "The failure that was the harness's". Nothing is written to the
# source tree, which is mounted read-only.

set -euo pipefail

here=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo=$(cd "$here/../.." && pwd)
image=${MPIABI_IMAGE:-ubuntu:24.04}

rows=("$@")
if [ ${#rows[@]} -eq 0 ]; then
  rows=(mvapich intelmpi)
fi

for row in "${rows[@]}"; do
  probe="$here/probe-$row.sh"
  if [ ! -x "$probe" ]; then
    echo "$row: no probe-$row.sh here" >&2
    exit 2
  fi

  # Intel MPI ships no aarch64 build, so on an arm64 host that row needs
  # emulation named explicitly. The probe refuses on a non-x86_64 machine
  # rather than failing later and obscurely.
  platform=${MPIABI_PLATFORM:-}
  if [ -z "$platform" ] && [ "$row" = intelmpi ]; then
    case $(uname -m) in
      x86_64|amd64) ;;
      *) platform=linux/amd64 ;;
    esac
  fi

  printf '\n########## %s (%s%s)\n' "$row" "$image" \
         "${platform:+, $platform}"
  docker run --rm ${platform:+--platform "$platform"} \
    -v "$repo:/repo:ro" \
    -e REPO=/repo \
    -e MPI_SRC_DIR=/build \
    "$image" \
    bash "/repo/dev/third-implementations/probe-$row.sh"
done
