#!/usr/bin/env bash
# Build and run the probe. macOS only: the question is a Mach-O one (weak
# definitions, two-level vs flat namespaces); dev/dlopen-probe/ owns the ELF
# side, where scope order rather than symbol binding decides everything.
#
# Four variants of the implementation, everything else held fixed:
#   MPI_X strong or weak (-DWEAK_FOO), linked two-level or -flat_namespace.
# The wrapper is always two-level; libabi always defines both names strong and
# is loaded first, because the application links it.
#
#   T1  the wrapper's MPI_X call           -> must stay inside the impl (11)
#   T2  the wrapper's PMPI_X call          -> must stay inside the impl (1)
#   T3  the impl's own internal MPI_X call -> must stay inside the impl (111)

set -uo pipefail
cd "$(dirname "$0")"

if [ "$(uname -s)" != "Darwin" ]; then
  echo "Mach-O semantics only; nothing to measure here on $(uname -s)." >&2
  exit 1
fi

CC=${CC:-cc}
CFLAGS=${CFLAGS:--std=c11 -Wall -Wextra -g -O0 -fPIC}
rm -rf out && mkdir -p out && cd out

echo "### $(sw_vers -productName 2>/dev/null) $(sw_vers -productVersion 2>/dev/null), $(uname -m), $($CC --version | head -1)"

$CC $CFLAGS -dynamiclib -o abi.dylib ../abi.c \
    -install_name @rpath/abi.dylib || exit 1
$CC $CFLAGS -o app ../app.c abi.dylib -Wl,-rpath,@executable_path || exit 1

for weak in "" "-DWEAK_FOO"; do
  for ns in "" "-Wl,-flat_namespace"; do
    $CC $CFLAGS -dynamiclib $weak $ns -o impl.dylib ../impl.c \
        -install_name @rpath/impl.dylib || exit 1
    $CC $CFLAGS -dynamiclib -o wrap.dylib ../wrap.c impl.dylib \
        -install_name @rpath/wrap.dylib -Wl,-rpath,@loader_path || exit 1
    binding=strong; [ -n "$weak" ] && binding=weak
    namespace=two-level; [ -n "$ns" ] && namespace=flat
    echo "== impl: MPI_X $binding, $namespace namespace"
    ./app
  done
done
