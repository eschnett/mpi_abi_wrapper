#!/usr/bin/env bash

# Is the `i386 / rest` shard's SIGBUS storm about 32 bits, or about a 64 MB
# /dev/shm? Run from inside a linux/386 container by
# .github/workflows/i386-shm-probe.yaml, which carries the reasoning, the
# evidence it is built on and the two-leg design; this file is only the half that
# has to run inside the container.
#
#   docker run --rm --shm-size=64m|8g -v $PWD:/src:ro -v $OUT:/out -e SRC=/src \
#     i386/debian:trixie-slim bash /src/ci-scripts/suite/i386-shm-probe.sh
#
# Disposable, like the workflow: delete both once the answer is in
# ci-scripts/suite/README.md.

set -uo pipefail

SRC=${SRC:-/src}
OUT=/out
[ -d "$OUT" ] || { echo "$0: /out is not mounted" >&2; exit 2; }

prefix=$OUT/opt/mpich-5.0.1

say() { printf '\n=== %s\n' "$*"; }

say "what this container's /dev/shm actually is"
df -h /dev/shm "$OUT" / || true
grep -E ' /dev/shm ' /proc/mounts || true
echo "  entries in /dev/shm at the start: $(ls -A /dev/shm 2>/dev/null | wc -l)"
ls -la /dev/shm 2>/dev/null | head -20
command -v ipcs >/dev/null && ipcs -m | head -10
free -m
echo "  nproc: $(nproc)"

# Every ten seconds, on stdout: the two numbers that would explain either
# failure mode, and the count of segments nobody cleaned up. On stdout because
# the failure this branch is chasing loses the job's artifacts -- and, in run
# 32425661806, its log as well, which is why this probe is built to end on its
# own terms rather than to survive being killed.
monitor() {
  while :; do
    printf '[mon %s] /dev/shm %s  entries=%s  memavail=%sMB\n' \
      "$(date -u +%H:%M:%S)" \
      "$(df -h /dev/shm | awk 'NR==2 {print $3 " of " $2}')" \
      "$(ls -A /dev/shm 2>/dev/null | wc -l)" \
      "$(awk '/MemAvailable/ {print int($2/1024)}' /proc/meminfo)"
    sleep 10
  done
}
monitor &
mon=$!
trap 'kill "$mon" 2>/dev/null' EXIT

# The `rest` shard as CI runs it, minus io: --skip-dirs already carries the four
# the shard skips, and io is 18 of the shard's 20 minutes while the storm
# reproduces in the 2.5 minutes that are not it. The gate will fail -- that is
# what this run is for -- so its status is reported rather than obeyed.
say "the rest shard without io, otherwise exactly as CI runs it"
bash "$SRC/ci-scripts/suite/i386-suite.sh" mpich \
    --xfail=xfail-ci-mpich.txt \
    --xfail=xfail-ci-mpich-i386.txt \
    --flaky=flaky-ci-mpich.txt \
    --skip-dirs=coll,rma,threads,pt2pt,part,io
echo "  i386-suite.sh exited $?"

say "how the run counted"
tap=$OUT/suite/summary.tap
if [ -f "$tap" ]; then
  echo "  ok:     $(grep -c '^ok ' "$tap")"
  echo "  not ok: $(grep -c '^not ok ' "$tap")"
else
  echo "  no TAP file at $tap"
fi
if [ -f "$OUT/suite/runtests.log" ]; then
  echo "  SIGBUS reported by the launcher: $(grep -c 'Bus error' "$OUT/suite/runtests.log")"
  echo "  other bad terminations:          $(grep -c 'BAD TERMINATION' "$OUT/suite/runtests.log")"
fi

say "/dev/shm afterwards"
df -h /dev/shm
echo "  entries left behind: $(ls -A /dev/shm 2>/dev/null | wc -l)"
ls -A /dev/shm 2>/dev/null | head -20

# ---------------------------------------------------------------------- blame
#
# Two tests that SIGBUS in CI: the first one the cancelled run recorded, and one
# from a directory that failed 12 of 12. Standalone under gdb, because MPICH
# supports singleton init and a backtrace with no launcher between gdb and the
# rank names the library the signal lands in. If they pass standalone, the
# mpiexec leg at the end is the one that reproduces it.
say "installing gdb"
DEBIAN_FRONTEND=noninteractive apt-get install -y -qq gdb >/dev/null \
  || echo "  gdb did not install; the backtraces below will be missing"

tree=$(echo "$OUT"/suite-src/mpich-*/test/mpi)
echo "  suite tree: $tree"

probes="info/infotest datatype/pairtype_size_extent"

# The wrapper's prefix bakes its own RPATHs and src/mpi_abi/bootstrap.c has a
# compiled-in default for the implementation, so a standalone run should need
# neither of these. Both are set anyway, and `ldd` printed, because a
# "libmpi_abi.so not found" here would otherwise read as a result.
export LD_LIBRARY_PATH=$OUT/suite/prefix/lib:$OUT/suite/prefix/lib64:$prefix/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}

for t in $probes; do
  d=${t%/*}; p=${t#*/}
  say "$t through the wrapper, standalone, under gdb"
  [ -x "$OUT/suite/build/$d/$p" ] && ldd "$OUT/suite/build/$d/$p" | head -12
  if [ -x "$OUT/suite/build/$d/$p" ]; then
    ( cd "$OUT/suite/build/$d" \
      && timeout 120 gdb -batch -ex run -ex 'bt 25' -ex 'info sharedlibrary' \
                         --args "./$p" 2>&1 | tail -60 )
  else
    echo "  not built at $OUT/suite/build/$d/$p"
  fi
done

# The same programs against MPICH itself. If they SIGBUS here too, no line of
# this project is involved -- the S7 method the rest of this branch used for
# "is it ours", and the only one that has ever settled such a question here.
say "configuring the same tree against MPICH, with no wrapper in the path"
mkdir -p "$OUT/native"
( cd "$OUT/native" && "$tree/configure" \
      --with-mpi="$prefix" --enable-strictmpi --enable-fortran=no \
      >configure.log 2>&1 ) \
  || { echo "  configure failed:"; tail -20 "$OUT/native/configure.log"; }

for t in $probes; do
  d=${t%/*}; p=${t#*/}
  say "$t with no wrapper, standalone, under gdb"
  if ( cd "$OUT/native/$d" && make "$p" >/dev/null 2>&1 ); then
    ( cd "$OUT/native/$d" \
      && timeout 120 gdb -batch -ex run -ex 'bt 25' --args "./$p" 2>&1 | tail -40 )
  else
    echo "  BUILD FAILED for the native $t"
  fi
done

# Singleton init is not what runtests does, so the same two programs are run the
# way the suite runs them -- through mpiexec-filter, which is what supplies the
# launcher's flags -- in case the difference is the launcher and not the call.
say "and once more through mpiexec, both ways"
export MPIEXEC_FILTER_LAUNCHER=$prefix/bin/mpiexec
export MPIEXEC_FILTER_KIND=hydra
for t in $probes; do
  d=${t%/*}; p=${t#*/}
  for leg in wrapper native; do
    case $leg in
      wrapper) dir=$OUT/suite/build/$d ;;
      native)  dir=$OUT/native/$d ;;
    esac
    echo "--- $leg: mpiexec -n 2 ./$p"
    if [ -x "$dir/$p" ]; then
      # No pipe around the launcher: `... | tail` reports tail's status, which
      # is the trap the errcount probe paid for -- every row of its first run
      # said exit=0.
      out=$OUT/mpiexec-$leg-$p.out
      ( cd "$dir" && timeout 120 \
        "$SRC/ci-scripts/suite/mpiexec-filter" -n 2 "./$p" ) >"$out" 2>&1
      rc=$?
      tail -20 "$out"
      grep -q "No Errors" "$out" && verdict=PASSED || verdict=FAILED
      echo "  exit $rc -> $verdict"
    else
      echo "  not built"
    fi
  done
done

kill "$mon" 2>/dev/null
say "probe done"
