#!/usr/bin/env bash

# S7's exit check (HISTORY.md): run MPICH's own C test suite against this
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
#      than wrong answers (HISTORY.md says to expect them).
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
#   MPITEST_RUN_INDIVIDUAL
#                        exported below, not read: one MPI job per test rather
#                        than the 5.0.1 suite's batched default. See the export
#                        for the three reasons, of which the per-process capacity
#                        limits of NOTES.md #6.2 is the one that matters.
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

suite_version=5.0.1          # the pinned MPICH release the suite comes from
variant=""
prefix=""
dirs=""
skip_dirs=""
exclude_args=()
timelimit_args=()
flaky_args=()
np=""
jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)
with_spawn=0
with_dtp=1
gate_only=0
update_xfail=0
xfail_args=()
reconfigure=0

usage() {
  cat >&2 <<EOF
usage: $(basename "$0") mpich|openmpi|/path/to/mpicc [options]

  --prefix=DIR     an already-installed wrapper prefix; skips build+install
  --variant=NAME   which xfail list gates this run (default: the
                   implementation family, i.e. mpich or openmpi)
  --xfail=FILE     gate against this list instead of the variant's. Repeatable,
                   and the files are read as one, so a shared per-implementation
                   list can carry a small per-environment one beside it. A bare
                   filename is looked for in ci-scripts/suite/
  --suite=VERSION  MPICH release to take test/mpi from (default $suite_version)
  --dirs=a,b,c     run only these test directories
  --skip-dirs=a,b,c
                   run every directory *except* these -- the complement of
                   --dirs, and what the last shard of a sharded run wants: a
                   directory a later suite release adds lands in it by
                   construction rather than by somebody remembering to add it
  --np=N           default rank count (runtests -np; the suite's own is 2)
  --jobs=N         parallelism for both builds (default $jobs)
  --with-spawn     include the spawn directory. Off by default: MPI_Comm_spawn
                   hangs under hydra on macOS with no wrapper involved
                   (test/README.md), and 31 tests timing out costs an hour
  --flaky=FILE     tests in FILE may fail or pass without failing the gate.
                   Repeatable. For a test that genuinely flaps: an xfail line
                   fails the runs it passes, and no line fails the runs it
                   fails, so neither category fits. FILE must exist; a variant
                   with no flaky tests simply does not pass this.
  --exclude=FILE   drop individual test lines matching the patterns in FILE.
                   Repeatable. For a test that takes the host down rather than
                   failing -- an xfail list cannot cover one, because a killed
                   runner leaves no TAP line to expect. See
                   exclude-ci-openmpi.txt for the format and the reasoning.
  --time-limit=FILE
                   cap the per-test time limit on the test lines matching the
                   patterns in FILE. Repeatable. For a test that *hangs*: it
                   still runs, still reports and still gates, but it costs the
                   cap rather than the suite's 180-second default. See
                   timelimit-ci-openmpi.txt for the format and the measurement.
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
    --xfail=*)     xfail_args+=("${arg#*=}") ;;
    --suite=*)     suite_version=${arg#*=} ;;
    --dirs=*)      dirs=${arg#*=} ;;
    --skip-dirs=*) skip_dirs=${arg#*=} ;;
    --exclude=*)   exclude_args+=("${arg#*=}") ;;
    --time-limit=*) timelimit_args+=("${arg#*=}") ;;
    --flaky=*)     flaky_args+=("${arg#*=}") ;;
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
#
# A caller that already knows may say so, and CI does: MPIEXEC_FILTER_KIND set
# in the environment is taken as given rather than re-derived. Getting this wrong
# is expensive and quiet -- `prte` is what makes mpiexec-filter add
# --oversubscribe, and without it Open MPI refuses to start any test asking for
# more ranks than the runner has cores, which on a four-vCPU runner is a whole
# category of the suite failing to launch rather than failing.
#
# The patterns cover the spellings actually seen rather than the ones that would
# be tidy: Open MPI 4.1.8's mpiexec answers "mpiexec (OpenRTE) 4.1.8", which
# matches neither "Open MPI" nor "prte" -- measured here, and the reason
# OpenRTE and PRRTE are named explicitly.
launcher_version=$("$launcher" --version 2>&1)
case ${MPIEXEC_FILTER_KIND:-} in
  hydra)  kind=hydra; detected=mpich ;;
  prte)   kind=prte;  detected=openmpi ;;
  ?*)     kind=$MPIEXEC_FILTER_KIND; detected=other ;;
  *)
    case $launcher_version in
      *[Hh][Yy][Dd][Rr][Aa]*)  kind=hydra; detected=mpich ;;
      *[Oo]pen*[Mm][Pp][Ii]*|*[Oo]pen[Rr][Tt][Ee]*|*[Pp][Rr][Tt][Ee]*|*[Pp][Rr][Rr][Tt][Ee]*)
        kind=prte; detected=openmpi ;;
      *) kind=other; detected=other ;;
    esac
    ;;
esac
[ -n "$family" ] || family=$detected
[ -n "$variant" ] || variant=$family

# One list or several. Several is the case a shared list plus an environment
# delta needs; check-tap.py reads them as one and treats a test listed in two of
# them as an error rather than an override.
xfail_files=()
if [ ${#xfail_args[@]} -eq 0 ]; then
  xfail_files=("$suitedir/xfail-$variant.txt")
else
  for f in "${xfail_args[@]}"; do
    case $f in
      */*) xfail_files+=("$f") ;;
      *)   xfail_files+=("$suitedir/$f") ;;
    esac
  done
fi
# Resolved like the xfail lists -- a bare name means beside this script -- and
# required to exist for the same reason: a --flaky that silently did nothing
# would excuse whatever it was pointed at by mistake.
flaky_files=()
for f in "${flaky_args[@]+"${flaky_args[@]}"}"; do
  case $f in
    */*) flaky_files+=("$f") ;;
    *)   flaky_files+=("$suitedir/$f") ;;
  esac
done
# Resolved the same way, and checked here rather than where they are read: the
# reading happens after the wrapper build and the suite's configure, so a
# mistyped filename would otherwise cost five minutes before saying so.
timelimit_files=()
for f in "${timelimit_args[@]+"${timelimit_args[@]}"}"; do
  case $f in
    */*) timelimit_files+=("$f") ;;
    *)   timelimit_files+=("$suitedir/$f") ;;
  esac
  [ -f "${timelimit_files[${#timelimit_files[@]}-1]}" ] \
    || die "no time-limit list at ${timelimit_files[${#timelimit_files[@]}-1]}"
done
timelimit_rules=()
for tf in "${timelimit_files[@]+"${timelimit_files[@]}"}"; do
  while IFS= read -r line; do
    case $line in ''|'#'*) continue ;; esac
    secs=${line%% *}
    re=${line#* }
    case $secs in
      ''|*[!0-9]*) die "$tf: '$line' does not begin with a number of seconds" ;;
    esac
    [ "$secs" != 0 ] || die "$tf: '$line' caps at zero seconds"
    [ "$re" != "$line" ] || die "$tf: '$line' has seconds but no pattern"
    timelimit_rules+=("$secs $re")
  done < "$tf"
done

work=${MPIABI_SUITE_WORK:-$repodir/build/suite-$variant}
src=${MPIABI_SUITE_SRC:-$repodir/build/suite-src}
tree=$src/mpich-$suite_version/test/mpi
tap=$work/summary.tap

mkdir -p "$work" "$src"

echo "wrapped MPI:  $MPICC"
echo "launcher:     $launcher ($kind)"
gate_names=""
for f in "${xfail_files[@]}"; do gate_names="$gate_names ${f#"$repodir"/}"; done
flaky_opts=()
for f in "${flaky_files[@]+"${flaky_files[@]}"}"; do
  [ -f "$f" ] || die "no flaky list at $f"
  flaky_opts+=("--flaky=$f")
  gate_names="$gate_names +flaky:${f#"$repodir"/}"
done
echo "variant:      $variant   (gate:$gate_names)"
echo "suite:        MPICH $suite_version"
echo "work:         $work"

if [ "$gate_only" = 1 ]; then
  [ -f "$tap" ] || die "no TAP file at $tap to gate on"
  exec python3 "$suitedir/check-tap.py" "$tap" "${xfail_files[@]}" \
       "${flaky_opts[@]+"${flaky_opts[@]}"}"
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

# **One MPI job per test, which is not what the 5.0.1 suite does by default.**
# MPICH 5.0's runtests gained a `run_mpitests` driver that executes a whole
# directory's tests "inside a single MPI_Init/Finalize" -- 17 at a time in the
# run that found this -- and MPITEST_RUN_INDIVIDUAL is its off switch. Three
# reasons this project wants it off, and the first is about correctness rather
# than convenience:
#
#   1. **The capacity limits of NOTES.md #6.2 are per process.** Op-function
#      trampolines, keyval pairs and errhandler slots are never reclaimed, by
#      design and in the standard's case by requirement. Seventeen tests sharing
#      one process share one pool, so which test exhausts it depends on how many
#      ran before it -- an expected-failure list would then be a statement about
#      an ordering rather than about a test.
#   2. **A test that aborts takes its batch with it.** runtests restarts the
#      driver and continues, so the run survives; what it cannot recover is the
#      attribution, since the tests that never ran look the same as tests that
#      ran and passed.
#   3. It looks for the driver at `$srcdir/run_mpitests`, which an out-of-tree
#      build does not have -- run_mpitests is compiled from util/run_mpitests.c
#      into the *build* tree. The first run against 5.0.1 got 21 tests of 847
#      out of that, every one of them a launcher error, which is what sent
#      anyone looking.
#
# The cost is wall-clock: one launch per test is what the 4.3.x suite always did
# and what the timings in ci-scripts/suite/README.md were measured under.
export MPITEST_RUN_INDIVIDUAL=1

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
        # --dirs is a whitelist and --skip-dirs a blacklist, applied in that
        # order, so the two compose: a shard names either the directories it
        # takes or the ones the other shards already took.
        keep=1
        if [ -n "$dirs" ]; then
          case ",$dirs," in
            *",$name,"*) ;;
            *) keep=0; why="not in --dirs" ;;
          esac
        fi
        if [ "$keep" = 1 ] && [ -n "$skip_dirs" ]; then
          case ",$skip_dirs," in
            *",$name,"*) keep=0; why="in --skip-dirs" ;;
          esac
        fi
        if [ "$keep" = 1 ]; then echo "$entry"; else echo "# $name: $why"; fi ;;
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

# ------------------------------------------------------- excluded test lines
#
# **A test that takes the host down cannot be expected-failed.** check-tap.py
# gates on xfail-ci-*.txt, and every line in those files describes a test that
# ran and reported. One that exhausts the runner reports nothing: the job is
# killed, no artifact is uploaded, and there is no TAP line to match. So the only
# way to stop such a test costing a whole shard is to keep runtests from running
# it, which is what --exclude does.
#
# Filtering happens **in place, after the spawn pass above**, so the two compose
# instead of the second undoing the first -- the spawn filter derives `testlist`
# from `testlist.configured`, and re-deriving here would put the spawn lines
# back. Deleting lines is idempotent, so a reused work directory is safe.
#
# Both `testlist` and `testlist.dtp` are filtered: the dtp list is generated from
# maint/dtp-test-config.txt and is where the expensive tests live, so a pattern
# that only reached `testlist` would miss the case this exists for.
if [ ${#exclude_args[@]} -gt 0 ]; then
  step "excluding individual test lines"
  exclude_res=()
  for f in "${exclude_args[@]}"; do
    case $f in
      */*) ef=$f ;;
      *)   ef=$suitedir/$f ;;
    esac
    [ -f "$ef" ] || die "no exclusion list at $ef"
    echo "  from ${ef#"$repodir"/}"
    while IFS= read -r line; do
      case $line in ''|'#'*) continue ;; esac
      exclude_res+=("$line")
    done < "$ef"
  done
  if [ ${#exclude_res[@]} -eq 0 ]; then
    echo "  (no patterns; nothing excluded)"
  else
    dropped_total=0
    for sub in $covered; do
      for base in testlist testlist.dtp; do
        lf=$work/build/$sub/$base
        [ -f "$lf" ] || continue
        for re in "${exclude_res[@]}"; do
          # grep -c counts, grep -v filters; a pattern matching nothing is not an
          # error, because one list covers every directory.
          n=$(grep -cE "$re" "$lf" || true)
          [ "$n" = 0 ] && continue
          # `grep -v` exits 1 when it prints nothing, which happens when a pattern
          # matches *every* line. Testing the exit status would then skip the `mv`
          # and leave the file unfiltered, silently -- so the empty result is
          # written and moved regardless, and only `mv` failing is an error.
          grep -vE "$re" "$lf" > "$lf.tmp" || true
          mv "$lf.tmp" "$lf" || die "could not rewrite $lf"
          echo "  $sub/$base: dropped $n line(s) matching $re"
          dropped_total=$((dropped_total + n))
        done
      done
    done
    echo "  $dropped_total line(s) excluded in total"
  fi
fi

# ----------------------------------------------------- capped time limits
#
# **A test that hangs costs the timeout, and the timeout is 180 seconds.** In run
# 32586710591 the Open MPI `p2p` shard spent 39.1 of its 46.3 minutes on thirteen
# tests that ran to the limit and failed, and the `rest` shard 12.0 of 13.0 on four
# more -- 83% of the whole run's critical path, waiting for hangs that were already
# expected failures. The suite has the answer in its own vocabulary: `timeLimit=N`
# is a testlist key runtests parses (test/mpi/runtests, where it becomes the
# per-test `_timeout` and then MPIEXEC_TIMEOUT), and the suite's own lists already
# use it. This pass appends one to the lines a list names.
#
# **Why this and not the three neighbouring mechanisms.** A hang is not a runner
# kill, so --exclude is too blunt: the test would stop running and a release that
# fixed it would never say so. The suite's own `xfail=` key is the same loss by
# another spelling -- runtests SKIPs those without running them. And
# MPITEST_TIMEOUT_MULTIPLIER is global, so cutting it to reach these seventeen
# would also cut `coll/bcast_comm_world_only`, which *passes* over Open MPI at 84.4
# seconds. Capping named lines leaves every other test at 180 and keeps the capped
# ones in the run, in the TAP file and under the gate.
#
# **Recursive, unlike the exclusion pass above, and it creates the file it edits.**
# Five of the seventeen live in a subdirectory -- threads/comm, threads/part,
# threads/pt2pt, errors/coll, errors/comm -- which a top-level walk never reaches.
# And a directory whose source testlist is a plain file rather than a `.in` has no
# copy in the build tree at all; runtests falls back to the source tree for those
# (`! -s $listfile_path && -s $srcdir/$listfile`), so writing a build-tree copy is
# both how the cap takes effect and something this pass has to do for itself. The
# copy is made only when a pattern actually matches, so an unmatched directory is
# left exactly as configure left it.
#
# Idempotent for a given list: an existing `timeLimit=` on a matched line is
# stripped before the new one is appended, so a second run over the same work
# directory re-derives the same file rather than accumulating keys. It is *not*
# idempotent across a list that was **narrowed** -- a line that a previous run's
# pattern matched and this one's does not keeps the cap it was given, because
# nothing re-derives it from the original. `--reconfigure` is the reset.
#
# The lists themselves were resolved and parsed before the wrapper build, so a
# missing file or a malformed line fails in the first second rather than the
# fifth minute; what is left here is the rewriting.
if [ ${#timelimit_args[@]} -gt 0 ]; then
  step "capping the time limit on named test lines"
  for tf in "${timelimit_files[@]}"; do echo "  from ${tf#"$repodir"/}"; done
  if [ ${#timelimit_rules[@]} -eq 0 ]; then
    echo "  (no patterns; nothing capped)"
  else
    capped_total=0
    for sub in $covered; do
      # The union of both trees: the build tree has the generated lists
      # (testlist from testlist.in, and every testlist.dtp), the source tree has
      # the plain ones that were never copied.
      rels=$(
        { [ -d "$tree/$sub" ] && (cd "$tree" && find "$sub" \( -name testlist -o -name testlist.dtp \) )
          [ -d "$work/build/$sub" ] && (cd "$work/build" && find "$sub" \( -name testlist -o -name testlist.dtp \) )
        } 2>/dev/null | sort -u
      )
      for rel in $rels; do
        for rule in "${timelimit_rules[@]}"; do
          secs=${rule%% *}
          re=${rule#* }
          lf=$work/build/$rel
          src_lf=$tree/$rel
          # Read whichever file runtests would read, which is the build-tree copy
          # when it exists and is non-empty, and the source otherwise.
          if [ -s "$lf" ]; then read_lf=$lf; else read_lf=$src_lf; fi
          [ -f "$read_lf" ] || continue
          # Counted the way awk below rewrites: comment lines are not candidates,
          # so a pattern that could match one must not be reported as a cap.
          n=$(grep -vE '^[[:space:]]*#' "$read_lf" | grep -cE "$re" || true)
          [ "$n" = 0 ] && continue
          mkdir -p "$(dirname "$lf")"
          awk -v re="$re" -v secs="$secs" '
            /^[ \t]*#/ { print; next }
            $0 ~ re    { gsub(/[ \t]+timeLimit=[^ \t]+/, ""); print $0 " timeLimit=" secs; next }
                       { print }
          ' "$read_lf" > "$lf.tmp" || die "could not rewrite $lf"
          mv "$lf.tmp" "$lf" || die "could not rewrite $lf"
          echo "  $rel: capped $n line(s) at ${secs}s matching $re"
          capped_total=$((capped_total + n))
        done
      done
    done
    echo "  $capped_total line(s) capped in total"
  fi
fi

# ---------------------------------------------------------------- build them
#
# -k: a test that does not compile is a result (HISTORY.md expects some), so
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
# build failures HISTORY.md expects, and a line in the xfail list saying which
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
# **Teed rather than redirected**, and the reason is a failure mode this suite hit
# rather than tidiness: a job killed mid-run uploads no artifact, so the only
# record left is what reached stdout. Both Open MPI `rma` shards of run
# 32182485327 died at six minutes with a runner shutdown signal and nothing to
# show for it -- no TAP file, no logs, no way to name the test that did it. With
# the output teed, the last "Running tests in" line in the job log is that name.
# The cost is a thousand-odd lines in a log that is already grouped by step.
(cd "$work/build" && "$tree/runtests" "${runtests_opts[@]}") 2>&1 \
  | tee "$work/runtests.log"
echo "  runtests exited ${PIPESTATUS[0]}; output in $work/runtests.log"
[ -f "$tap" ] || die "runtests produced no TAP file"

# ------------------------------------------------------------------ the gate

if [ "$update_xfail" = 1 ]; then
  step "writing the observed failures into ${xfail_files[${#xfail_files[@]}-1]#"$repodir"/}"
  python3 "$suitedir/check-tap.py" "$tap" "${xfail_files[@]}" --update
  echo "  every new line needs a reason before it can gate anything"
fi

step "gate: observed results against$gate_names"
python3 "$suitedir/check-tap.py" "$tap" "${xfail_files[@]}" \
        "${flaky_opts[@]+"${flaky_opts[@]}"}" \
        ${dirs:+--dirs="$dirs"} || fail "the suite's result does not match its list"

printf '\n=== %s\n' \
  "$([ $status -eq 0 ] && echo 'suite matches its expected-failure list' \
                       || echo 'FAILURES above')"
echo "  covered: $covered"
exit $status
