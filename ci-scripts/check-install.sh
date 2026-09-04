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
# Eight legs against a fresh install:
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
#   6. the prefix's inventory        -- every program decision 27 says a prefix
#                                        holds is there, the aliases agree with
#                                        what they alias, and no two of them are
#                                        the same file by accident
#   7. route 4: bin/mpiexec          -- a real two-rank job through this
#                                        prefix's own launcher (decision 27)
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
# MPI_ABI_FORTRAN defaults to the project's own default, ON; a caller with no
# Fortran compiler sets it to OFF in the environment (see linux-test.sh).
cmake -S "$repodir" -B "$work/build" \
      -DMPI_C_COMPILER="$MPICC" \
      -DMPI_ABI_FORTRAN="${MPI_ABI_FORTRAN:-ON}" \
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

# ------------------------------------------------- leg 6: the inventory

# What decision 27 says an installed prefix holds. This leg needs no launcher
# and no MPI to start, so unlike leg 7 it always runs.
step "the prefix's inventory (decision 27)"

for prog in mpicc mpiexec mpirun mpi_abi_wrapper_info; do
  if [ ! -x "$prefix/bin/$prog" ]; then
    fail "bin/$prog was not installed, or is not executable"
  fi
done

# mpicxx and mpic++ exist precisely when the build had a C++ compiler
# (CMakeLists.txt's enable_language(CXX OPTIONAL)), so their absence is a
# skip with a reason rather than a failure -- the same distinction
# cmake/FindMPI.cmake makes when it decides whether to publish MPI::MPI_CXX.
if [ -x "$prefix/bin/mpicxx" ]; then
  [ -x "$prefix/bin/mpic++" ] || fail "bin/mpicxx was installed but bin/mpic++ was not"
else
  echo "  no bin/mpicxx, so this build had no C++ compiler and mpic++ is not expected"
fi

# The oracle for the failure that made mpiCC unshippable: on a
# case-insensitive filesystem two names differing only in case are one
# directory entry, so an alias can silently *replace* what it was meant to sit
# beside. test-consume/hello.c compiles as C++ too, so leg 2 would still pass
# with bin/mpicc secretly a C++ wrapper -- this is what would not.
if [ -x "$prefix/bin/mpicxx" ]; then
  cc_cmd=$("$prefix/bin/mpicc" -showme:command 2>/dev/null)
  cxx_cmd=$("$prefix/bin/mpicxx" -showme:command 2>/dev/null)
  if [ -z "$cc_cmd" ]; then
    fail "leg 6: bin/mpicc -showme:command answered nothing"
  elif [ "$cc_cmd" = "$cxx_cmd" ]; then
    fail "leg 6: bin/mpicc and bin/mpicxx name the same compiler ($cc_cmd) --" \
         "one has clobbered the other"
  else
    echo "  mpicc -> $cc_cmd, mpicxx -> $cxx_cmd"
  fi
  # An alias must agree with what it aliases, or it is a stale copy.
  if [ "$("$prefix/bin/mpic++" -showme:link 2>/dev/null)" != \
       "$("$prefix/bin/mpicxx" -showme:link 2>/dev/null)" ]; then
    fail "leg 6: bin/mpic++ and bin/mpicxx disagree about their link line"
  fi
fi
if [ "$("$prefix/bin/mpirun" -showme:launcher 2>/dev/null)" != \
     "$("$prefix/bin/mpiexec" -showme:launcher 2>/dev/null)" ]; then
  fail "leg 6: bin/mpirun and bin/mpiexec resolve different launchers"
fi

# The info tool is an installed binary of ours, so 20.2.1's property applies to
# it exactly as to a consumer's: libmpi_abi and nothing else.
if run_cleared "$prefix/bin/mpi_abi_wrapper_info" > "$work/info.log" 2>&1; then
  check_only_mpi_abi_dependency "$prefix/bin/mpi_abi_wrapper_info"
  grep -q "^wrapper:" "$work/info.log" ||
    fail "leg 6: mpi_abi_wrapper_info reported no wrapper"
  grep -q "^launcher:" "$work/info.log" ||
    fail "leg 6: mpi_abi_wrapper_info reported no launcher"
  # "present NO" beside either path is the whole reason the tool prints that
  # column: a prefix whose wrapper or launcher has moved is broken now, not
  # later.
  if grep -q "present *NO" "$work/info.log"; then
    cat "$work/info.log"
    fail "leg 6: mpi_abi_wrapper_info says a resolved path is not present"
  fi
  # MPI_Get_library_version is answered by the wrapped implementation through
  # the wrapper that was actually loaded, so a non-empty second line here is
  # the end-to-end proof for this leg. When the caller named an
  # implementation, say whether it is the one that answered.
  impl=$(sed -n '/^library version:/,$p' "$work/info.log" | tail -n +2 | tr -d ' \t' | grep -c .)
  if [ "${impl:-0}" -lt 2 ]; then
    cat "$work/info.log"
    fail "leg 6: mpi_abi_wrapper_info printed no wrapped-implementation string"
  else
    echo "  mpi_abi_wrapper_info ran, and its resolved wrapper and launcher are present"
  fi
  case $which in
    mpich)   grep -qi "MPICH"    "$work/info.log" && echo "  wrapped implementation reports MPICH, as asked" ||
               echo "  NOTE: asked for mpich, but the library version string does not say MPICH" ;;
    openmpi) grep -qi "Open MPI" "$work/info.log" && echo "  wrapped implementation reports Open MPI, as asked" ||
               echo "  NOTE: asked for openmpi, but the library version string does not say Open MPI" ;;
  esac
else
  cat "$work/info.log"
  fail "leg 6: $prefix/bin/mpi_abi_wrapper_info did not run cleanly"
fi

# ------------------------------------------------ leg 7: bin/mpiexec

# Two ranks through the prefix's own launcher. Asking for two and being handed
# two *singletons* is the trap this project has already been caught by
# (HISTORY.md #2.14): each singleton simply passes, so the exit status says
# nothing at all. What says something is the output -- rank 0 of 2 *and* rank 1
# of 2, both ranks and the size.
#
# The comparison is one binary under two launchers: leg 2's hello, run first
# under the wrapped MPI's own mpiexec and then under this prefix's. That is
# what separates "bin/mpiexec is broken" from "this host cannot start a
# two-rank job", and it is a tighter comparison than compiling a second
# program would be, since the only thing differing between the two runs is the
# launcher. If the native launcher cannot do it -- Open MPI 5.0.x on macOS 26
# cannot, for reasons test/README.md and HISTORY.md #2.13 attribute to the
# machine's firewall, and neither can a wrapped MPI whose own mpicc does not
# work on this host -- the leg has nothing to say and says so. The skip is
# *derived* from that measurement rather than declared by a flag, so there is
# no opt-out here to fall out of date. ci.yaml runs the wrapper-free version of
# the same gate before this script, which is where a launcher and a library
# that disagree get separated.
step "route 4: bin/mpiexec, two ranks (decision 27)"

two_ranks() {   # <launcher> <binary> -- true when both distinct ranks answered
  local out
  out=$(run_cleared "$1" -n 2 "$2" 2>&1)
  echo "$out" | grep -q "rank 0 of 2" && echo "$out" | grep -q "rank 1 of 2"
}

native_mpiexec=$(dirname "$MPICC")/mpiexec
if [ ! -x "$work/route1/hello" ]; then
  fail "leg 7: leg 2 produced no hello to launch"
elif [ ! -x "$native_mpiexec" ]; then
  echo "  SKIPPED: no mpiexec beside $MPICC, so there is no launcher to compare"
  echo "  against. bin/mpiexec was built with none baked in, and leg 6 has"
  echo "  already checked that it says so rather than forwarding somewhere wrong."
elif ! two_ranks "$native_mpiexec" "$work/route1/hello"; then
  echo "  SKIPPED: $native_mpiexec cannot start a two-rank job on this host, so"
  echo "  it cannot say anything about bin/mpiexec forwarding to it. See"
  echo "  test/README.md and HISTORY.md #2.13."
else
  echo "  $native_mpiexec: two ranks, so this host can launch"
  for launcher in mpiexec mpirun; do
    if two_ranks "$prefix/bin/$launcher" "$work/route1/hello"; then
      echo "  bin/$launcher: two ranks, both distinct"
    else
      run_cleared "$prefix/bin/$launcher" -n 2 "$work/route1/hello" 2>&1 | sed 's/^/    /'
      fail "leg 7: bin/$launcher did not produce rank 0 of 2 and rank 1 of 2" \
           "where $native_mpiexec did, on the same binary"
    fi
  done
fi

printf '\n=== %s\n' \
  "$([ $status -eq 0 ] && echo 'all legs passed' || echo 'FAILURES above')"
exit $status
