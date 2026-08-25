#!/usr/bin/env sh
# What Mach-O's weak definitions do to this project, in the two places they
# matter and which pull in opposite directions (NOTES.md #13.2).
#
#   1. SUBSTITUTION -- the ABI's headline promise. Every other implementation
#      exports MPI_* `weak external`; this project exports them `external`.
#      Does a binary linked against one start against the other, and in which
#      direction?
#
#   2. CAPTURE -- this project's load-time isolation. HISTORY.md #2.3 measured
#      our *strong* MPI_Send beating an ABI-implementing MPI's *weak* one, so
#      the wrapper's outward call came back into libmpi_abi. If we became weak
#      to satisfy (1), who wins then?
#
# macOS only: this is a two-level-namespace and weak-coalescing question, and
# ELF's lookup does not distinguish the two (dev/symbol-versioning/ is the ELF
# counterpart).
#
#   sh probe.sh
set -e

case $(uname -s) in
  Darwin) ;;
  *) echo "macOS only -- this asks a dyld question. See dev/symbol-versioning/" >&2
     exit 2 ;;
esac

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cd "$work"

CC=${CC:-cc}

# =====================================================================
# Part 1: substitution
# =====================================================================
#
# Two libraries that differ in one attribute and nothing else, both carrying
# the install name a real ABI library carries, so that one is a drop-in for the
# other in an installed prefix (decision 21).

cat > abi.c <<'C'
#include <stdio.h>
#ifdef ABI_WEAK
__attribute__((weak))
#endif
int MPI_Init(void) { return WHICH; }
C

mkdir -p weak strong run
$CC -shared -fPIC -DABI_WEAK -DWHICH=1 -o weak/libmpi_abi.1.dylib abi.c \
    -install_name @rpath/libmpi_abi.1.dylib
$CC -shared -fPIC          -DWHICH=2 -o strong/libmpi_abi.1.dylib abi.c \
    -install_name @rpath/libmpi_abi.1.dylib

echo "=== the two libraries, as nm -m sees them"
printf '    weak-exporting:   '; nm -m weak/libmpi_abi.1.dylib   | grep ' _MPI_Init$' | sed 's/.*) //'
printf '    strong-exporting: '; nm -m strong/libmpi_abi.1.dylib | grep ' _MPI_Init$' | sed 's/.*) //'

cat > app.c <<'C'
#include <stdio.h>
int MPI_Init(void);
int main(void) { printf("MPI_Init() -> %d\n", MPI_Init()); return 0; }
C

# One client per link-time library, each looking for the library at run time in
# `run/` -- which is the swap: put one implementation's there, then the other's.
$CC -o app_linked_weak   app.c weak/libmpi_abi.1.dylib   -Wl,-rpath,"$work/run"
$CC -o app_linked_strong app.c strong/libmpi_abi.1.dylib -Wl,-rpath,"$work/run"

echo
echo "=== 1. SUBSTITUTION: link against one, run against the other"
echo "    (1 = the weak library answered, 2 = the strong one)"
for linked in weak strong; do
  for present in weak strong; do
    # rm before cp, never cp over: replacing a dylib in place keeps the inode,
    # and macOS then matches a stale code signature and kills the process with
    # no message at all. A first run of this probe reported "FAILED TO START"
    # with an empty error for a row that passes -- which is the shape of wrong
    # answer HISTORY.md #2.11 and #2.17 record twice already.
    rm -f run/libmpi_abi.1.dylib
    cp "$present/libmpi_abi.1.dylib" run/libmpi_abi.1.dylib
    printf '    linked against %-6s, %-6s present:  ' "$linked" "$present"
    if out=$(./app_linked_$linked 2>&1); then
      echo "$out"
    else
      echo "FAILED TO START -- $(echo "$out" | head -1)"
    fi
  done
done

# =====================================================================
# Part 2: capture
# =====================================================================
#
# The three-library shape of this project (NOTES.md #2), with an implementation
# that exports MPI_Send weakly -- which is what an ABI-implementing MPI, and
# Open MPI 6.1.0a1's ordinary library, both do:
#
#   app -> libmpi_abi::MPI_Send -> (dlopen) libwrap::wrapper_entry
#                                             -> MPI_Send  ... reaching whom?
#
# The wrapper's outward call must reach libimpl. If it comes back into
# libmpi_abi instead, that is HISTORY.md #2.3's capture.

cat > impl.c <<'C'
#include <stdio.h>
/* Weak, as every ABI-implementing MPI's exported MPI_* are. */
__attribute__((weak)) int MPI_Send(void) { return 10; }
C

cat > wrap.c <<'C'
int MPI_Send(void);              /* bound to libimpl at link time */
int wrapper_entry(void) { return MPI_Send(); }
C

cat > abi2.c <<'C'
#include <dlfcn.h>
#include <stdio.h>
static int (*vt)(void);
static int depth;
#ifdef ABI_WEAK
__attribute__((weak))
#endif
int MPI_Send(void)
{
  if (++depth > 1) { depth = 0; return -1; }   /* re-entered: captured */
  if (!vt) {
    void *h = dlopen(WRAPPATH, RTLD_LOCAL | RTLD_NOW);
    if (!h) { depth = 0; return -2; }
    vt = (int (*)(void))dlsym(h, "wrapper_entry");
    if (!vt) { depth = 0; return -3; }
  }
  int r = vt();
  depth = 0;
  return r;
}
C

cat > app2.c <<'C'
#include <stdio.h>
int MPI_Send(void);
int main(void)
{
  const int r = MPI_Send();
  if (r == 10) puts("reached libimpl        (isolated -- correct)");
  else if (r == -1) puts("re-entered libmpi_abi  (CAPTURED)");
  else printf("plumbing failure %d\n", r);
  return 0;
}
C

$CC -shared -fPIC -o libimpl.dylib impl.c -install_name "$work/libimpl.dylib"
$CC -shared -fPIC -o libwrap.dylib wrap.c libimpl.dylib \
    -install_name "$work/libwrap.dylib"

echo
echo "=== 2. CAPTURE: does the wrapper's outward MPI_Send reach the implementation?"
echo "    (the implementation exports MPI_Send weakly in both rows)"
for style in strong weak; do
  [ "$style" = weak ] && flag=-DABI_WEAK || flag=
  # shellcheck disable=SC2086
  $CC -shared -fPIC $flag -DWRAPPATH="\"$work/libwrap.dylib\"" \
      -o libabi2.dylib abi2.c -install_name "$work/libabi2.dylib"
  $CC -o app2 app2.c libabi2.dylib
  printf '    libmpi_abi exports %-6s:  ' "$style"
  ./app2 2>&1 | head -1
done

# Part 3 -- the shared-install-name hypothesis -- was attempted and removed
# because it did not run cleanly; README.md "What this does not settle" has
# what it showed and why it is the lead to follow, rather than a broken section
# left in the file.
