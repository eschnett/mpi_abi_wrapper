#!/usr/bin/env bash

# Run the MPICH C suite against one MPI, on Linux.
#
#   ci-scripts/suite/linux-suite.sh mpich|openmpi [run-suite.sh options...]
#
# Runs *inside* a Linux environment -- a container started by
# ci-scripts/run-linux-docker.sh, or a CI runner directly -- and does the two
# things run-suite.sh deliberately does not: install packages, and put every
# writable directory under /tmp, since run-linux-docker.sh mounts the source
# tree read-only (ci-scripts/README.md explains why that is worth keeping).
#
# The Open MPI row exists here and not on macOS, and when it was established
# that was not a preference: no Open MPI 5.0.x launcher would run a job on the
# development laptop, and a suite whose every test is a launcher failure would
# be a list of 900 excuses rather than a result. In a container Open MPI's
# launcher works, so the row is real. That laptop has since been fixed --
# scripts/host-env.sh, and test/README.md's third environment quirk for why it
# needed fixing -- so a macOS Open MPI row has become possible; it has not been
# run, and this one is not waiting on it.
#
#   MPIABI_LINUX_SCRIPT=/src/ci-scripts/suite/linux-suite.sh \
#     ci-scripts/run-linux-docker.sh openmpi

set -uo pipefail

which=${1:-}
case $which in
  mpich|openmpi) shift ;;
  *) echo "usage: $0 mpich|openmpi [run-suite.sh options]" >&2; exit 2 ;;
esac

SRC=${SRC:-$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)}

if [ "$(id -u)" = 0 ] && command -v apt-get >/dev/null; then
  export DEBIAN_FRONTEND=noninteractive
  # perl and curl are the suite's own needs: runtests is a perl script, and
  # run-suite.sh fetches the tarball it comes from.
  pkgs="build-essential cmake python3 patch binutils perl curl ca-certificates"
  case $which in
    mpich)   pkgs="$pkgs libmpich-dev mpich" ;;
    openmpi) pkgs="$pkgs libopenmpi-dev openmpi-bin" ;;
  esac
  echo "=== installing $pkgs"
  apt-get update -qq >/dev/null && apt-get install -y -qq $pkgs >/dev/null \
    || { echo "package install failed" >&2; exit 2; }
fi

# Debian installs both MPIs' wrappers under alternatives; name the one we mean.
MPICC=$(command -v "mpicc.$which" || command -v mpicc) \
  || { echo "no mpicc for $which" >&2; exit 2; }

# /out is run-linux-docker.sh's MPIABI_LINUX_OUT, mounted read-write. Working
# there rather than in /tmp is what keeps the TAP file and the logs after the
# container is gone; without it the only record of a failure is this script's
# own stdout.
if [ -d /out ]; then
  export MPIABI_SUITE_WORK=${MPIABI_SUITE_WORK:-/out/suite-$which}
else
  export MPIABI_SUITE_WORK=${MPIABI_SUITE_WORK:-/tmp/suite-$which}
fi
export MPIABI_SUITE_SRC=${MPIABI_SUITE_SRC:-/tmp/suite-src}

# Open MPI refuses to run as root without being told, and a container is root.
[ "$which" = openmpi ] && export OMPI_ALLOW_RUN_AS_ROOT=1 \
                                 OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1

exec "$SRC/ci-scripts/suite/run-suite.sh" "$MPICC" --variant="$which" "$@"
