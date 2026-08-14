#!/usr/bin/env bash
set -uo pipefail
cd "$(dirname "$0")"

CC=${CC:-cc}
CFLAGS=${CFLAGS:--std=c11 -Wall -Wextra -O2 -fPIC}
OS=$(uname -s)
rm -rf out && mkdir -p out && cd out

if [ "$OS" = "Darwin" ]; then
  SO=dylib
  mk() { $CC $CFLAGS -dynamiclib -I.. -o "lib$1.$SO" "../lib$1.c" \
           -install_name "@rpath/lib$1.$SO" -Wl,-rpath,@loader_path "${@:2}"; }
  RP=(-Wl,-rpath,@executable_path)
else
  SO=so
  mk() { $CC $CFLAGS -shared -I.. -o "lib$1.$SO" "../lib$1.c" \
           -Wl,-rpath,'$ORIGIN' "${@:2}"; }
  RP=(-Wl,-rpath,'$ORIGIN')
fi

mk callee                        || exit 1
mk dispatch -L. -lcallee         || exit 1
$CC $CFLAGS -I.. -o bench ../bench.c -L. -ldispatch -lcallee "${RP[@]}" || exit 1

echo "### $OS, $($CC --version 2>&1 | head -1)"
./bench "$@"
