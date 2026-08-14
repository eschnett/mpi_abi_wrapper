#!/usr/bin/env bash
# Build and run the probe. Works on Linux and macOS; run under Docker for Linux
# results on a macOS host (see Dockerfile).
#
# Prints a per-mode verdict table for:
#   T1  the wrapper's own MPI_Send call            -> must reach libimpl
#   T2  the implementation's INTERNAL MPI_Send call -> must reach libimpl
#   T3  a second dlopen'ed plugin's MPI_Send call   -> must reach libabi
#
# T2 is the question RTLD_DEEPBIND may not answer, since libimpl is a dependency
# loaded by the same dlopen rather than the object dlopen was called on.

set -uo pipefail
cd "$(dirname "$0")"

CC=${CC:-cc}
CFLAGS=${CFLAGS:--std=c11 -Wall -Wextra -g -O0 -fPIC}
OS=$(uname -s)
rm -rf out && mkdir -p out && cd out

if [ "$OS" = "Darwin" ]; then
  SO=dylib
  mklib() { # mklib <name> <src> [extra link args...]
    local n=$1 s=$2; shift 2
    $CC $CFLAGS -dynamiclib -I.. -o "lib$n.$SO" "../$s" \
        -install_name "@rpath/lib$n.$SO" -Wl,-rpath,@loader_path "$@"
  }
  RPATH=(-Wl,-rpath,@executable_path)
  DL=()
else
  SO=so
  mklib() {
    local n=$1 s=$2; shift 2
    $CC $CFLAGS -shared -I.. -o "lib$n.$SO" "../$s" \
        -Wl,-rpath,'$ORIGIN' "$@"
  }
  RPATH=(-Wl,-rpath,'$ORIGIN')
  DL=(-ldl)
fi

echo "### building ($OS, $($CC --version 2>&1 | head -1))"
mklib impl libimpl.c                                     || exit 1
mklib wrap libwrap.c -L. -limpl                          || exit 1
mklib abi  libabi.c  "${DL[@]}"                          || exit 1
mklib plugin1 plugin.c -L. -labi                         || exit 1
mklib plugin2 plugin.c -L. -labi                         || exit 1
$CC $CFLAGS -I.. -o app     ../app.c     -L. -labi "${RPATH[@]}" || exit 1
$CC $CFLAGS -I.. -o hostapp ../hostapp.c "${DL[@]}" "${RPATH[@]}" || exit 1

# macOS only: a flat-namespace libwrap, to test rather than assume that the
# two-level namespace is what protects us there.
if [ "$OS" = "Darwin" ]; then
  $CC $CFLAGS -dynamiclib -I.. -o libwrap_flat.$SO ../libwrap.c \
      -install_name "@rpath/libwrap_flat.$SO" -Wl,-rpath,@loader_path \
      -L. -limpl -Wl,-flat_namespace -Wl,-undefined,dynamic_lookup 2>/dev/null \
      && echo "  (built flat-namespace libwrap too)"
fi

MODES=(local global)
[ "$OS" = "Linux" ] && MODES+=(deepbind dlmopen)

echo
: > results.txt
for m in "${MODES[@]}"; do
  echo "############################################################ mode=$m"
  ./app "$m" 2>&1 | tee -a trace.txt | grep '^RESULT' >> results.txt
  ./app "$m" 2>&1 | sed 's/^/  /' | grep -v '^  RESULT'
  echo
  ./hostapp "$m" 2>&1 | grep '^RESULT' >> results.txt
  ./hostapp "$m" 2>&1 | sed 's/^/  /' | grep -v '^  RESULT'
  echo
done

if [ "$OS" = "Darwin" ] && [ -f "libwrap_flat.$SO" ]; then
  echo "###################################### mode=local, FLAT-namespace libwrap"
  PROBE_WRAP_LIB="./libwrap_flat.$SO" ./app local 2>&1 \
    | sed 's/^RESULT local/RESULT flat/' | tee /dev/stderr | grep '^RESULT' >> results.txt
  echo
fi

echo "=============================================================="
printf '%-12s %-14s %-14s %-14s\n' MODE "T1 wrap->impl" "T2 impl-internal" "T3 2nd plugin"
echo "--------------------------------------------------------------"
for m in "${MODES[@]}" flat; do
  t1=$(grep -m1 "^RESULT $m T1 " results.txt | awk '{print $4}')
  t2=$(grep -m1 "^RESULT $m T2 " results.txt | awk '{print $4}')
  t3=$(grep -m1 "^RESULT $m T3 " results.txt | awk '{print $4}')
  [ -z "$t1$t2$t3" ] && continue
  printf '%-12s %-14s %-14s %-14s\n' "$m" "${t1:--}" "${t2:--}" "${t3:--}"
done
echo "=============================================================="
echo "Every cell must read OK."
echo "  CAPTURED = our own MPI_Send was re-entered: infinite recursion in the real system."
echo "  BYPASSED = the caller reached the native MPI directly, without passing through"
echo "             the ABI layer, and would be handed ABI-typed arguments."
