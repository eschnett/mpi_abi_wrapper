#!/usr/bin/env bash
# Usage: run.sh <mpi-include-dir> [label]
set -uo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
inc=$1; label=${2:-$1}
CC=${CC:-cc}
abi=../../gen/include

echo "=== $label ==="
$CC -std=c11 -I"$inc" -I$abi -o /tmp/ti-probe probe.c && /tmp/ti-probe

echo "  -- passing the ABI array with no cast, -Wall -Wextra -Wpedantic:"
out=$($CC -std=c11 -I"$inc" -I$abi -Wall -Wextra -Wpedantic -fsyntax-only nocast.c 2>&1)
if [ -z "$out" ]; then echo "     accepted with no diagnostic (the types are identical)"
else echo "$out" | sed -n 's/^/     /p' | head -4; fi

for opt in -O0 -O2 -O3; do
  $CC -std=c11 -I"$inc" -I$abi $opt -fstrict-aliasing -o /tmp/ti-alias aliasing.c || continue
  echo "  -- cast + read at $opt -fstrict-aliasing:"
  /tmp/ti-alias | sed 's/^/  /'
done
