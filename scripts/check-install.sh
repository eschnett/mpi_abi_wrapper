#!/usr/bin/env bash

# Local convenience wrapper around ci-scripts/check-install.sh (S6's exit
# check): unlike ci-scripts/linux-test.sh, that script needs no container --
# it only configures, builds and installs this project and then drives three
# ordinary consumer builds against the result -- so it runs directly on a
# developer's machine. This wrapper only supplies a default mpicc so the
# common case is a bare invocation.
#
#   scripts/check-install.sh                # whatever mpicc is on PATH
#   scripts/check-install.sh /path/to/mpicc  # a specific installation

set -euo pipefail

scriptdir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repodir=$(cd "$scriptdir/.." && pwd)

mpicc=${1:-$(command -v mpicc || true)}
if [ -z "$mpicc" ]; then
  echo "$(basename "$0"): no mpicc given and none on PATH" >&2
  exit 2
fi

exec "$repodir/ci-scripts/check-install.sh" "$mpicc"
