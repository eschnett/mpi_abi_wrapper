#!/usr/bin/env bash

# S6's exit check (HISTORY.md): configure, build and install this project into
# a prefix of its own, then build and run a program through each of the three
# consumption routes NOTES.md #9 requires -- bin/mpicc, find_package(mpi_abi)
# (and, separately, the FindMPI shim's plain find_package(MPI)), and
# pkg-config -- with LD_LIBRARY_PATH/DYLD_LIBRARY_PATH cleared, so a route
# that only works because the loader's search path happens to include the
# right directory cannot pass by accident.
#
#   ci-scripts/check-install.sh mpich|openmpi|/path/to/mpicc
#
# Six legs against a fresh install:
#
#   0. configure, build, install     -- into a prefix of its own, never the
#                                        wrapped MPI's (see leg 1)
#   1. prefix exclusivity            -- the wrapped MPI's own mpi.h is not
#                                        under the install prefix (NOTES.md
#                                        #9: "the prefix is exclusive")
#   2. route 1: bin/mpicc            -- compile, link, run test-consume/hello.c
#                                        with nothing but $prefix/bin/mpicc
#   3. route 2: find_package(mpi_abi) -- configure test-consume/ as its own
#                                        project against -DCMAKE_PREFIX_PATH,
#                                        build, run
#   4. route 2b: find_package(MPI)   -- the FindMPI shim: same, but with
#                                        CMAKE_MODULE_PATH pointed at the
#                                        installed Modules/ and a bare
#                                        find_package(MPI), the way HDF5 and
#                                        PETSc actually ask
#   5. route 3: pkg-config           -- `pkg-config --cflags --libs mpi_abi`,
#                                        compiled and linked by hand
#
# Every route that runs is also checked with otool/objdump for the
# 20.2.1 property oracles 1 and 2 already check in the build tree
# (test/check_exports.cmake): libmpi_abi is the *only* MPI dependency of the
# binary produced -- no libmpiwrapper (reached by dlopen, never linked) and
# no direct dependency on the wrapped implementation.
#
# Leg 5 is skipped, with a reason printed, when this host has no
# pkg-config to ask -- pkg-config is not a dependency of mpi_abi_wrapper, and
# a run that cannot ask should say so rather than fail on the tool's absence.
#
# One known source of an unrelated failure here: test/README.md documents
# MPICH's ch4:ofi picking a VPN interface (utun0 on macOS) and MPI_Finalize
# then failing with "OFI poll failed"; FI_PROVIDER=tcp works around it. That
# is an environment property of the *wrapped* MPI, not of anything this
# script or S6 built, and it is exactly as likely to hit ctest's own
# behavioural tests.

set -uo pipefail

which=${1:-}
MPICC=""
case $which in
  mpich|openmpi) ;;
  */*|mpicc*)
    [ -x "$which" ] || { echo "$which: not executable" >&2; exit 2; }
    MPICC=$which
    ;;
  *) echo "usage: $0 mpich|openmpi|/path/to/mpicc" >&2; exit 2 ;;
esac

repodir=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
if [ -z "$MPICC" ]; then
  MPICC=$(command -v "mpicc.$which" || command -v mpicc) || {
    echo "no mpicc for $which" >&2; exit 2; }
fi
mpi_incdir=$(dirname "$(dirname "$MPICC")")/include

work=$(mktemp -d "${TMPDIR:-/tmp}/mpiabi-check-install.XXXXXX")
trap 'rm -rf "$work"' EXIT
prefix="$work/prefix"
status=0

step() { printf '\n=== %s\n' "$*"; }
fail() { echo "FAILED: $*" >&2; status=1; }

# The direct-dependency check test/check_exports.cmake already uses in the
# build tree, re-run on whatever this leg just built.
check_only_mpi_abi_dependency() {
  local bin=$1
  local deps
  if [ "$(uname)" = Darwin ]; then
    deps=$(otool -L "$bin")
  else
    deps=$(objdump -p "$bin")
  fi
  if ! echo "$deps" | grep -q libmpi_abi; then
    fail "$bin: does not depend on libmpi_abi at all"
    return
  fi
  if echo "$deps" | grep -q libmpiwrapper; then
    fail "$bin: links libmpiwrapper directly (must be reached only by dlopen)"
  fi
  if echo "$deps" | grep -qE 'libmpi\.|libmpich|libpmpi|libopen_mpi'; then
    fail "$bin: depends on the wrapped MPI implementation directly"
  fi
}

# Every route is run the same way: loader search path emptied, so a route
# that only works via LD_LIBRARY_PATH/DYLD_LIBRARY_PATH cannot pass here.
run_cleared() {
  env -u LD_LIBRARY_PATH -u DYLD_LIBRARY_PATH "$@"
}

# --------------------------------------------------------- leg 0: the build

step "configure, build, install into $prefix"
cmake -S "$repodir" -B "$work/build" \
      -DMPI_C_COMPILER="$MPICC" \
      -DCMAKE_INSTALL_PREFIX="$prefix" \
      > "$work/cmake.log" 2>&1 \
  || { tail -25 "$work/cmake.log"; fail "configure"; exit $status; }
cmake --build "$work/build" -j"$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)" \
      > "$work/build.log" 2>&1 \
  || { grep -iE 'error' "$work/build.log" | head -25; fail "build"; exit $status; }
cmake --install "$work/build" > "$work/install.log" 2>&1 \
  || { cat "$work/install.log"; fail "install"; exit $status; }

# ------------------------------------------------------ leg 1: exclusivity

step "prefix exclusivity (NOTES.md #9)"
if [ -f "$mpi_incdir/mpi.h" ] && [ -f "$prefix/include/mpi.h" ]; then
  if [ "$(cd "$mpi_incdir" && pwd -P)" = "$(cd "$prefix/include" && pwd -P)" ]; then
    fail "install prefix is the wrapped MPI's own prefix -- mpi.h collides"
  fi
fi
if [ -f "$prefix/include/mpiwrapper_marker.h" ]; then
  echo "  mpiwrapper_marker.h installed -- the self-wrap marker future" \
       "configures against this prefix will see"
else
  fail "mpiwrapper_marker.h was not installed; the self-wrap check has" \
       "nothing to find"
fi

# The other side of the same rule: the include directory holds mpi.h and the
# marker, and nothing else. mpiabi.h and mpiwrapper_vtable.h are the two
# halves' private contract and no consumption route names them, so an
# installed copy could only shadow something (NOTES.md #9).
for hdr in mpiabi.h mpiwrapper_vtable.h; do
  if [ -e "$prefix/include/$hdr" ]; then
    fail "$hdr was installed; it is internal to building this project"
  fi
done

# ------------------------------------------------------- leg 2: bin/mpicc

step "route 1: bin/mpicc"
if [ -x "$prefix/bin/mpicc" ]; then
  mkdir -p "$work/route1"
  if "$prefix/bin/mpicc" -o "$work/route1/hello" \
       "$repodir/test-consume/hello.c" > "$work/route1/build.log" 2>&1; then
    check_only_mpi_abi_dependency "$work/route1/hello"
    if run_cleared "$work/route1/hello" > "$work/route1/run.log" 2>&1; then
      echo "  ran: $(cat "$work/route1/run.log")"
    else
      cat "$work/route1/run.log"
      fail "route 1: hello did not run cleanly"
    fi
  else
    cat "$work/route1/build.log"
    fail "route 1: bin/mpicc could not build test-consume/hello.c"
  fi
else
  fail "route 1: $prefix/bin/mpicc was not installed"
fi

# ------------------------------------------------ leg 3: find_package(mpi_abi)

step "route 2: find_package(mpi_abi)"
if cmake -S "$repodir/test-consume" -B "$work/route2" \
     -DCMAKE_PREFIX_PATH="$prefix" > "$work/route2-cmake.log" 2>&1; then
  if cmake --build "$work/route2" > "$work/route2-build.log" 2>&1; then
    check_only_mpi_abi_dependency "$work/route2/hello"
    if run_cleared "$work/route2/hello" > "$work/route2-run.log" 2>&1; then
      echo "  ran: $(cat "$work/route2-run.log")"
    else
      cat "$work/route2-run.log"
      fail "route 2: hello did not run cleanly"
    fi
  else
    cat "$work/route2-build.log"
    fail "route 2: test-consume did not build"
  fi
else
  cat "$work/route2-cmake.log"
  fail "route 2: find_package(mpi_abi) failed to configure test-consume/"
fi

# --------------------------------------------- leg 4: find_package(MPI) shim

step "route 2b: find_package(MPI), via the FindMPI shim"
mkdir -p "$work/route2b/src"
cat > "$work/route2b/src/CMakeLists.txt" <<'EOF'
cmake_minimum_required(VERSION 3.20)
project(mpiabi-findmpi-shim-probe LANGUAGES C)
find_package(MPI REQUIRED COMPONENTS C)
add_executable(hello hello.c)
target_link_libraries(hello PRIVATE MPI::MPI_C)
EOF
cp "$repodir/test-consume/hello.c" "$work/route2b/src/hello.c"
if cmake -S "$work/route2b/src" -B "$work/route2b/build" \
     -DCMAKE_MODULE_PATH="$prefix/lib/cmake/mpi_abi/Modules" \
     -DCMAKE_PREFIX_PATH="$prefix" > "$work/route2b-cmake.log" 2>&1; then
  if cmake --build "$work/route2b/build" > "$work/route2b-build.log" 2>&1; then
    check_only_mpi_abi_dependency "$work/route2b/build/hello"
    if run_cleared "$work/route2b/build/hello" > "$work/route2b-run.log" 2>&1; then
      echo "  ran: $(cat "$work/route2b-run.log")"
    else
      cat "$work/route2b-run.log"
      fail "route 2b: hello did not run cleanly"
    fi
  else
    cat "$work/route2b-build.log"
    fail "route 2b: test project did not build against the FindMPI shim"
  fi
else
  cat "$work/route2b-cmake.log"
  fail "route 2b: plain find_package(MPI) did not resolve to this prefix"
fi

# --------------------------------------------------------- leg 5: pkg-config

step "route 3: pkg-config"
PKG_CONFIG="${PKG_CONFIG:-pkg-config}"
if ! command -v "$PKG_CONFIG" >/dev/null 2>&1; then
  echo "  no $PKG_CONFIG on PATH, so this route is not exercised here"
else
  export PKG_CONFIG_PATH="$prefix/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
  if ! "$PKG_CONFIG" --exists mpi_abi; then
    fail "route 3: $PKG_CONFIG cannot find mpi_abi.pc under $prefix/lib/pkgconfig"
  else
    cflags=$("$PKG_CONFIG" --cflags mpi_abi)
    libs=$("$PKG_CONFIG" --libs mpi_abi)
    echo "  cflags: $cflags"
    echo "  libs:   $libs"
    mkdir -p "$work/route3"
    # shellcheck disable=SC2086
    if ${CC:-cc} $cflags -o "$work/route3/hello" "$repodir/test-consume/hello.c" $libs \
         > "$work/route3-build.log" 2>&1; then
      check_only_mpi_abi_dependency "$work/route3/hello"
      if run_cleared "$work/route3/hello" > "$work/route3-run.log" 2>&1; then
        echo "  ran: $(cat "$work/route3-run.log")"
      else
        cat "$work/route3-run.log"
        fail "route 3: hello did not run cleanly"
      fi
    else
      cat "$work/route3-build.log"
      fail "route 3: pkg-config's own flags did not build test-consume/hello.c"
    fi
  fi
fi

printf '\n=== %s\n' \
  "$([ $status -eq 0 ] && echo 'all legs passed' || echo 'FAILURES above')"
exit $status
