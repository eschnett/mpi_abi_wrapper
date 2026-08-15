#!/usr/bin/env bash

# Local convenience wrapper around ci-scripts/install-mpich.sh and
# install-openmpi.sh, installing into build/mpi/<name>-<version> so it sits
# beside this project's own build/ (which .gitignore already excludes) rather
# than in a temporary directory that vanishes when the shell does.
#
#   scripts/install-mpi.sh mpich            # 4.3.1, this project's primary row
#   scripts/install-mpi.sh mpich 3.1.4      # the MPI-3.0 floor row
#   scripts/install-mpi.sh openmpi          # 5.0.6, the primary Open MPI row
#   scripts/install-mpi.sh openmpi 4.1.6    # the secondary row

set -euo pipefail

scriptdir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repodir=$(cd "$scriptdir/.." && pwd)

which=${1:-}
version=${2:-}
case $which in
  mpich|openmpi) ;;
  *) echo "usage: $(basename "$0") mpich|openmpi [<version>]" >&2; exit 2 ;;
esac

prefix="$repodir/build/mpi/$which-${version:-default}"
mkdir -p "$repodir/build/mpi"
exec "$scriptdir/../ci-scripts/install-$which.sh" "$prefix" ${version:+"$version"}
