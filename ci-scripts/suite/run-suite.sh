#!/usr/bin/env bash

# S7's exit check (STAGES.md): run MPICH's own C test suite against this
# project, over one implementation per invocation, and gate the result on a
# committed expected-failure list with a reason on every line.
#
#   ci-scripts/suite/run-suite.sh mpich|openmpi|/path/to/mpicc [options]
#
# The suite is MPICH's `test/mpi`, taken from a pinned released tarball --
# the same provisioning discipline as ci-scripts/install-mpich.sh, and for
# the same reason (NOTES.md #9). It is worth being clear about what it tests
# here: the suite is an *application* of the MPI standard, so it goes through
# gen/include/mpi.h and libmpi_abi and nothing else, exactly as HDF5 or PETSc
# would in S8. Nothing in it knows this project exists.
#
# Four things this script has to arrange, none of them optional:
#
#   1. The suite is configured against the *wrapper's* prefix, not the
#      MPI's -- `--with-mpi=$prefix`, so CC is $prefix/bin/mpicc. Its own
#      probe then reports "Is the MPI derived from MPICH... no", because the
#      ABI header defines no MPICH macro, and the suite drops into its
#      generic-MPI mode. That is the correct answer and it is load-bearing:
#      it is what keeps MPICH-specific expectations out of the run.
#
#   2. --enable-strictmpi, because this project implements the standard and
#      nothing else. The suite's non-strict tests reach for MPIX_ entry
#      points and MPICH internals, which are *build* failures here rather
#      than wrong answers (STAGES.md says to expect them).
#
#   3. MPIEXEC is ci-scripts/suite/mpiexec-filter, since a wrapper prefix has
#      no launcher of its own. That file's header explains the other three
#      jobs the filter has to do.
#
#   4. `make -k` before runtests. The suite's tests are noinst_PROGRAMS, so
#      one parallel build makes runtests' own per-test `make` a no-op check;
#      -k because the build failures of point 2 are results, not accidents.
#
# Environment:
#   MPIABI_SUITE_SRC     where the tarball is downloaded and unpacked
#                        (default: build/suite-src; set it to something
#                        persistent to let CI cache it -- keyed on the
#                        suite version, which is all that identifies it)
#   MPIABI_SUITE_WORK    build and run directory (default: build/suite-<variant>)
#   MPITEST_TIMEOUT_MULTIPLIER
#                        runtests' own knob, passed straight through: every
#                        test's time limit times this. The committed xfail
#                        lists were recorded at the default of 1, and three
#                        MPICH lines in them are tests that pass alone and
#                        exceed 180 seconds inside a full run -- a slower
#                        machine wants 2, and will then see those three as
#                        "expected failure that passed", which is the gate
#                        asking for the lines to be deleted rather than a
#                        regression.
#   FI_PROVIDER          not set here, deliberately: test/README.md records
#                        that MPICH's ch4:ofi picks a VPN interface on the
#                        developers' machines and that FI_PROVIDER=tcp avoids
#                        it. That is a property of a host, not of this suite,
#                        and it is left to the host to set -- but see the
#                        note under "configure" below for what it costs when
#                        it is wrong, because the damage is silent.

set -uo pipefail

repodir=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
suitedir=$repodir/ci-scripts/suite

suite_version=4.3.1          # the pinned MPICH release the suite comes from
variant=""
prefix=""
dirs=""
np=""
jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)
with_spawn=0
with_dtp=1
gate_only=0
update_xfail=0
reconfigure=0

usage() {
  cat >&2 <<EOF
usage: $(basename "$0") mpich|openmpi|/path/to/mpicc [options]

  --prefix=DIR     an already-installed wrapper prefix; skips build+install
  --variant=NAME   which xfail list gates this run (default: the
                   implementation family, i.e. mpich or openmpi)
  --suite=VERSION  MPICH release to take test/mpi from (default $suite_version)
  --dirs=a,b,c     run only these test directories
  --np=N           default rank count (runtests -np; the suite's own is 2)
  --jobs=N         parallelism for both builds (default $jobs)
  --with-spawn     include the spawn directory. Off by default: MPI_Comm_spawn
                   hangs under hydra on macOS with no wrapper involved
                   (test/README.md), and 31 tests timing out costs an hour
  --no-dtp         leave out testlist.dtp, the datatype-pool tests: the same
                   call over hundreds of generated derived datatypes, which is
                   most of the run's value here and most of its runtime
  --reconfigure    discard an existing suite build directory first
  --update-xfail   write the run's failures into the xfail list as bare lines
                   for a human to add reasons to. Never a substitute for
                   triage: a line with no reason fails the gate
  --gate-only      re-run the comparison against an existing TAP file
EOF
  exit 2
}

which=${1:-}; shift || usage
MPICC=""
case $which in
  mpich|openmpi) family=$which ;;
  */*|mpicc*)
    [ -x "$which" ] || { echo "$which: not executable" >&2; exit 2; }
    MPICC=$which
    family=""
    ;;
  *) usage ;;
esac

for arg in "$@"; do
  case $arg in
    --prefix=*)    prefix=${arg#*=} ;;
    --variant=*)   variant=${arg#*=} ;;
    --suite=*)     suite_version=${arg#*=} ;;
    --dirs=*)      dirs=${arg#*=} ;;
    --np=*)        np=${arg#*=} ;;
    --jobs=*)      jobs=${arg#*=} ;;
    --with-spawn)  with_spawn=1 ;;
    --no-dtp)      with_dtp=0 ;;
    --reconfigure) reconfigure=1 ;;
    --update-xfail) update_xfail=1 ;;
    --gate-only)   gate_only=1 ;;
    *) usage ;;
  esac
done

status=0
step() { printf '\n=== %s\n' "$*"; }
fail() { echo "FAILED: $*" >&2; status=1; }
die()  { echo "FAILED: $*" >&2; exit 1; }

# ------------------------------------------------------- the MPI being wrapped

if [ -z "$MPICC" ]; then
  MPICC=$(command -v "mpicc.$family" || command -v mpicc) \
    || die "no mpicc for $family"
fi
mpi_bindir=$(cd "$(dirname "$MPICC")" && pwd)
launcher=$mpi_bindir/mpiexec
[ -x "$launcher" ] || die "no launcher beside $MPICC (looked for $launcher)"

# The launcher is also what says which implementation this is, when the
# caller passed a path rather than a name. Asking it beats guessing from the
# path: an MPICH derivative installed as /opt/whatever still answers hydra.
launcher_version=$("$launcher" --version 2>&1)
case $launcher_version in
  *[Hh][Yy][Dd][Rr][Aa]*)             kind=hydra; detected=mpich ;;
  *[Oo]pen*[Mm][Pp][Ii]*|*[Pp][Rr][Tt][Ee]*) kind=prte;  detected=openmpi ;;
  *) kind=other; detected=other ;;
esac
[ -n "$family" ] || family=$detected
[ -n "$variant" ] || variant=$family

xfail_file=$suitedir/xfail-$variant.txt
work=${MPIABI_SUITE_WORK:-$repodir/build/suite-$variant}
src=${MPIABI_SUITE_SRC:-$repodir/build/suite-src}
tree=$src/mpich-$suite_version/test/mpi
tap=$work/summary.tap

mkdir -p "$work" "$src"

echo "wrapped MPI:  $MPICC"
echo "launcher:     $launcher ($kind)"
echo "variant:      $variant   (gate: ${xfail_file#"$repodir"/})"
echo "suite:        MPICH $suite_version"
echo "work:         $work"

if [ "$gate_only" = 1 ]; then
  [ -f "$tap" ] || die "no TAP file at $tap to gate on"
  exec python3 "$suitedir/check-tap.py" "$tap" "$xfail_file"
fi

# ------------------------------------------------- the wrapper's own prefix

if [ -z "$prefix" ]; then
  prefix=$work/prefix
  step "building and installing this project into $prefix"
  cmake -S "$repodir" -B "$work/wrapper-build" \
        -DMPI_C_COMPILER="$MPICC" \
        -DCMAKE_INSTALL_PREFIX="$prefix" > "$work/wrapper-cmake.log" 2>&1 \
    || { tail -25 "$work/wrapper-cmake.log"; die "configure of the wrapper"; }
  cmake --build "$work/wrapper-build" -j"$jobs" > "$work/wrapper-build.log" 2>&1 \
    || { grep -iE 'error' "$work/wrapper-build.log" | head -25
         die "build of the wrapper"; }
  cmake --install "$work/wrapper-build" > "$work/wrapper-install.log" 2>&1 \
    || { cat "$work/wrapper-install.log"; die "install of the wrapper"; }
else
  step "using the installed wrapper at $prefix"
fi
[ -x "$prefix/bin/mpicc" ] || die "$prefix/bin/mpicc is not there"

# --------------------------------------------------------- the suite's source

if [ ! -d "$tree" ]; then
  tarball=$src/mpich-$suite_version.tar.gz
  if [ ! -f "$tarball" ]; then
    step "downloading mpich $suite_version (for test/mpi)"
    curl -fsSL -o "$tarball" \
      "https://www.mpich.org/static/downloads/$suite_version/mpich-$suite_version.tar.gz" \
      || die "download"
  fi
  step "unpacking test/mpi"
  tar -C "$src" -xzf "$tarball" "mpich-$suite_version/test/mpi" || die "unpack"
fi

# ------------------------------------------------------------------ configure

export MPIEXEC_FILTER_LAUNCHER=$launcher
export MPIEXEC_FILTER_KIND=$kind

# A configured suite tree remembers the prefix it was configured against in
# its Makefiles, so reusing one against a different wrapper would test the
# wrong library and say nothing about it. The stamp is what makes that an
# automatic reconfigure rather than a puzzle.
stamp=$work/build/.configured-against
if [ -f "$stamp" ] && [ "$(cat "$stamp")" != "$prefix $suite_version" ]; then
  echo "  (configured against $(cat "$stamp") before; reconfiguring)"
  reconfigure=1
fi

[ "$reconfigure" = 1 ] && rm -rf "$work/build"
mkdir -p "$work/build"

if [ ! -f "$work/build/Makefile" ]; then
  step "configuring the suite against $prefix"
  (
    cd "$work/build" &&
    "$tree/configure" \
        --with-mpi="$prefix" \
        MPIEXEC="$suitedir/mpiexec-filter" \
        --enable-strictmpi \
        --disable-cxx \
        --enable-fortran=no
  ) > "$work/suite-configure.log" 2>&1 \
    || { tail -25 "$work/suite-configure.log"; die "configure of the suite"; }
  echo "$prefix $suite_version" > "$stamp"
fi

# What configure decided, printed rather than buried: three of its answers
# change which tests exist at all, and one of them (threads) is decided by
# *running* an MPI program, so a broken environment silently removes a whole
# directory rather than failing. That is how a run can look green and have
# tested less than the last one -- FI_PROVIDER, above, is the case that bit
# this stage.
step "what the suite's configure decided"
for probe in \
    "Is the MPI derived from MPICH" \
    "whether we can compile and link MPI programs in C" \
    "whether MPI_THREAD_MULTIPLE is supported" ; do
  line=$(grep -m1 "$probe" "$work/suite-configure.log" 2>/dev/null)
  [ -n "$line" ] && echo "  $line"
done

# -------------------------------------------------------------- the testlist
#
# The suite's own top-level testlist, minus the directories that are not this
# project's to pass. Every exclusion is stated here rather than being left to
# a reader of the xfail list to infer from 30 identical reasons.
#
# It has to be written as `testlist`, the name the suite's own configure
# generated, because that name is the only one runtests looks for in every
# directory it descends into: a list under any other name is read at the top
# and then silently *skipped* in each subdirectory, which is a run of zero
# tests reported as a pass. The original is kept beside it.

configured_list=$work/build/testlist.configured
[ -f "$configured_list" ] || cp "$work/build/testlist" "$configured_list"

step "the directories this run covers"
{
  echo "# generated by ci-scripts/suite/run-suite.sh -- testlist.configured is the original"
  while read -r entry; do
    case $entry in
      ''|'#'*) continue ;;
    esac
    name=${entry%% *}
    case $name in
      impls)
        echo "# impls: MPICH's own PMI/hydra/MPIX tests, not standard MPI" ;;
      spawn)
        if [ "$with_spawn" = 1 ]; then echo "$entry"
        else echo "# spawn: --with-spawn to include; hangs under hydra on macOS with no wrapper involved (test/README.md)"; fi ;;
      *)
        if [ -n "$dirs" ]; then
          case ",$dirs," in
            *",$name,"*) echo "$entry" ;;
            *) echo "# $name: not in --dirs" ;;
          esac
        else
          echo "$entry"
        fi ;;
    esac
  done < "$configured_list"
} > "$work/build/testlist"
covered=$(grep -v '^#' "$work/build/testlist" | awk '{print $1}' | tr '\n' ' ')
grep -v '^#' "$work/build/testlist" | sed 's/^/  /'
grep '^#' "$work/build/testlist" | grep -v 'generated by' | sed 's/^/  /'

# `errors` and `threads` each have a spawn subdirectory of their own, and
# excluding the top-level one does not reach them -- those tests wait out
# their timeout on a host
# where MPI_Comm_spawn hangs, which is six minutes to learn what the
# exclusion above already said. Same mechanism: runtests reads a directory's
# `testlist` from the build tree before the source tree, so a filtered copy
# beside the generated one is the whole of it.
if [ "$with_spawn" = 0 ]; then
  for sub in errors threads; do
    [ -f "$work/build/$sub/testlist" ] || continue
    [ -f "$work/build/$sub/testlist.configured" ] \
      || cp "$work/build/$sub/testlist" "$work/build/$sub/testlist.configured"
    sed 's/^spawn$/# spawn: excluded with the top-level spawn directory/' \
        "$work/build/$sub/testlist.configured" > "$work/build/$sub/testlist"
  done
fi

# ---------------------------------------------------------------- build them
#
# -k: a test that does not compile is a result (STAGES.md expects some), so
# the build must not stop at the first one. runtests will report each as the
# failure of that one test.
#
# Only the directories this run covers, plus dtpools, which every one of them
# links against.

step "building the tests (make -k -j$jobs)"
{
  make -k -j"$jobs" -C "$work/build/dtpools"
  for d in $covered; do make -k -j"$jobs" -C "$work/build/$d"; done
} > "$work/suite-make.log" 2>&1

# A handful of tests read a data file the testlist names through the
# environment -- coll/coords-*.txt, for MPICH's topology-aware bcast -- and
# runtests runs each test in the *build* directory. In an in-tree build that
# is the same directory; here it is not, and the test aborts in MPI_Init on a
# file that is simply somewhere else. Copying is the whole fix, and it is the
# harness's job rather than an expected failure: the tests themselves pass.
for d in $covered; do
  for data in "$tree/$d"/*.txt; do
    [ -f "$data" ] && cp -f "$data" "$work/build/$d/"
  done
done 2>/dev/null
# Usually zero, and that is not the same as "everything built": with
# --enable-strictmpi the tests that use the deleted MPI-1 API are not in
# noinst_PROGRAMS at all, so they never reach this build. runtests' own
# per-test `make` is what reports those, as failures of the test.
echo "  $(grep -cE '(^make.*\bError\b|error:)' "$work/suite-make.log") build diagnostics here; full log in $work/suite-make.log"

# ------------------------------------------------------------------ run them
#
# testlist.dtp is the datatype-pool half of the suite: the same collective or
# point-to-point call over hundreds of generated derived datatypes. For a
# conversion layer that is the most valuable part of the run and also the
# longest, so --no-dtp exists for a quick pass.

#
# runtests' own `-strict` is deliberately *not* passed, though it is the exact
# counterpart of the --enable-strictmpi above: it would skip every test the
# testlists mark `strict=FALSE`, which is where the deleted MPI-1 API
# (MPI_LB, MPI_UB, MPI_Type_extent, ...) and MPICH's QMPI live. Those are the
# build failures STAGES.md expects, and a line in the xfail list saying which
# entry point the ABI header does not declare is worth more than a skip that
# says nothing. Pass it by hand for a faster run over an implementation whose
# gaps are already known.

step "running the suite"
tests=testlist
[ "$with_dtp" = 1 ] && tests=testlist,testlist.dtp
runtests_opts=(-srcdir="$tree" -tests="$tests"
               -mpiexec="$suitedir/mpiexec-filter"
               -tapfile="$tap" -xmlfile="$work/summary.xml")
[ -n "$np" ] && runtests_opts+=(-np="$np")
rm -f "$tap"
(cd "$work/build" && "$tree/runtests" "${runtests_opts[@]}") \
  > "$work/runtests.log" 2>&1
echo "  runtests exited $?; output in $work/runtests.log"
[ -f "$tap" ] || die "runtests produced no TAP file"

# ------------------------------------------------------------------ the gate

if [ "$update_xfail" = 1 ]; then
  step "writing the observed failures into ${xfail_file#"$repodir"/}"
  python3 "$suitedir/check-tap.py" "$tap" "$xfail_file" --update
  echo "  every new line needs a reason before it can gate anything"
fi

step "gate: observed results against ${xfail_file#"$repodir"/}"
python3 "$suitedir/check-tap.py" "$tap" "$xfail_file" \
        ${dirs:+--dirs="$dirs"} || fail "the suite's result does not match its list"

printf '\n=== %s\n' \
  "$([ $status -eq 0 ] && echo 'suite matches its expected-failure list' \
                       || echo 'FAILURES above')"
echo "  covered: $covered"
exit $status
