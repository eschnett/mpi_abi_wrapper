#!/usr/bin/env bash
# What ci.yaml's `linux-oneapi` job assumes about Intel MPI, checked rather than
# asserted: the package name, where the wrapper lands, what mpi.h declares,
# whether the `_c` surface is there at link time, the library SONAMEs
# check-install.sh's dependency check will match, and whether mpiexec starts
# two ranks with no fabric.
#
# Runs *inside* a Linux environment, as root; run.sh puts one around it.
# x86_64 only, and that is the implementation's limit rather than this
# script's: Intel MPI ships no aarch64 build, which is why the job has one leg
# where every other from-source row has two.

set -uo pipefail

repo=${REPO:-/repo}
# The apt version to pin, matching ci.yaml's `linux-oneapi` job. Empty means
# whatever the repository offers newest, which is what this probe was first
# written to do and is now the wrong thing -- see README.md, "Which Intel MPI
# merits wrapping".
version=${VERSION:-2021.15.0-493}
status=0

step() { printf '\n=== %s\n' "$*"; }
fail() { echo "FAILED: $*" >&2; status=1; }

case $(uname -m) in
  x86_64) ;;
  *) echo "Intel MPI is x86_64-only; this is $(uname -m)" >&2; exit 2 ;;
esac

export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
# gnupg, not gpg-agent. ci.yaml's `compile` job gets away with naming neither
# because a GitHub runner image has gpg preinstalled; a bare container does
# not, and `gpg --dearmor` then fails inside a pipe whose exit status curl
# reports as "Failure writing output to destination" (exit 23) -- a missing
# package reported as a network fault, which is install-mpich.sh's exit-23
# lesson in a second place.
apt-get install -y -qq --no-install-recommends \
    build-essential cmake python3 patch binutils poppler-utils curl \
    ca-certificates gnupg pkg-config >/dev/null \
  || { echo "package install failed" >&2; exit 2; }

step "the oneAPI apt repository (the one ci.yaml already adds for icx)"
curl -fsSL https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB \
    | gpg --dearmor -o /usr/share/keyrings/oneapi-archive-keyring.gpg \
  || { fail "could not fetch or dearmor the Intel key"; exit $status; }
echo "deb [signed-by=/usr/share/keyrings/oneapi-archive-keyring.gpg] https://apt.repos.intel.com/oneapi all main" \
    > /etc/apt/sources.list.d/oneAPI.list
apt-get update -qq
apt-cache policy intel-oneapi-mpi-devel | head -3

step "install intel-oneapi-mpi-devel${version:+=$version}"
apt-get install -y -qq "intel-oneapi-mpi-devel${version:+=$version}" >/dev/null \
  || { fail "apt could not install intel-oneapi-mpi-devel${version:+=$version}"; exit $status; }
ls -d /opt/intel/oneapi/mpi/*/ 2>/dev/null

step "does this release ship a standard-ABI library of its own?"
# The question that decides whether wrapping this release means anything.
# 2021.17 is the first that ships one, so a pinned row must land at or below
# 2021.16 -- README.md, "Which Intel MPI merits wrapping", has the bisection.
if ls /opt/intel/oneapi/mpi/latest/lib/libmpi_abi.so* >/dev/null 2>&1; then
  fail "this release ships libmpi_abi.so, so wrapping it is redundant:"
  ls -l /opt/intel/oneapi/mpi/latest/lib/libmpi_abi.so* | sed 's/^/    /'
else
  echo "  no libmpi_abi.so -- this release has no standard ABI, which is what"
  echo "  makes it worth wrapping, and removes the NOTES.md #13.2 collision"
fi

step "the environment script, and what it puts on PATH"
vars=/opt/intel/oneapi/mpi/latest/env/vars.sh
[ -f "$vars" ] || { fail "$vars is not where the job expects it"; exit $status; }
# `set +u` around it deliberately: the vars.sh scripts read unset variables,
# which is why ci.yaml's icx step brackets setvars.sh the same way.
set +u; source "$vars"; set -u
echo "  I_MPI_ROOT=${I_MPI_ROOT:-<unset>}"
MPICC=$(command -v mpicc) || { fail "no mpicc on PATH after sourcing vars.sh"; exit $status; }
echo "  mpicc: $MPICC"
"$MPICC" -show

step "declared MPI level"
cat > /tmp/v.c <<'EOF'
#include <mpi.h>
#include <stdio.h>
int main(void) { printf("MPI %d.%d\n", MPI_VERSION, MPI_SUBVERSION); return 0; }
EOF
"$MPICC" -o /tmp/v /tmp/v.c && /tmp/v || fail "mpi.h does not compile"

step "the large-count surface, at link time rather than by grep"
cat > /tmp/lc.c <<'EOF'
#include <mpi.h>
int main(int argc, char **argv) {
    MPI_Count sz = 0;
    MPI_Init(&argc, &argv);
    MPI_Type_size_c(MPI_INT, &sz);
    MPI_Finalize();
    return sz == 4 ? 0 : 1;
}
EOF
if "$MPICC" -o /tmp/lc /tmp/lc.c 2>/tmp/lc.err && /tmp/lc; then
  echo "  MPI_Type_size_c links and returns 4"
else
  echo "  absent or non-functional:"; head -3 /tmp/lc.err 2>/dev/null
  echo "  (not a failure -- decision 6's stubs cover it, as they do for Open MPI)"
fi

step "library SONAMEs check-install.sh's dependency check must match"
# check_only_mpi_abi_dependency() greps for libmpi\.|libmpich|libpmpi|libopen_mpi.
# Intel MPI names its library libmpi.so.12, so `libmpi\.` matches and no new
# alternative is needed -- worth checking rather than assuming, because a miss
# turns that leg into a silent pass.
ls "${I_MPI_ROOT}"/lib/libmpi* 2>/dev/null | head

step "two ranks, with no wrapper involved (ci.yaml's gate, run early)"
cat > /tmp/hello.c <<'EOF'
#include <mpi.h>
#include <stdio.h>
int main(int argc, char **argv) {
    int size = -1, rank = -1;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    printf("rank %d of %d\n", rank, size);
    MPI_Finalize();
    return size == 2 ? 0 : 1;
}
EOF
"$MPICC" -o /tmp/hello /tmp/hello.c || fail "hello does not build"
# shm is the shared-memory-only fabric. Named here for the same reason
# scripts/host-env.sh names FI_PROVIDER=tcp: a runner has no interconnect, and
# an implementation guessing at one is test/README.md's whole subject.
export I_MPI_FABRICS=shm
if mpiexec -n 2 /tmp/hello; then
  echo "  two ranks under I_MPI_FABRICS=shm"
else
  fail "mpiexec -n 2 did not produce a two-rank job"
fi

step "symbol binding (the same report linux-test.sh prints)"
for so in "${I_MPI_ROOT}"/lib/libmpi.so*; do
  case "$so" in *.so|*.so.[0-9]*) ;; *) continue ;; esac
  [ -e "$so" ] || continue
  nm -D --defined-only "$so" 2>/dev/null |
    awk -v lib="$(basename "$so")" '
      $2 == "T" && $3 ~ /^MPI_/  { t++ } $2 == "W" && $3 ~ /^MPI_/  { w++ }
      $2 == "T" && $3 ~ /^PMPI_/ { pt++ } $2 == "W" && $3 ~ /^PMPI_/ { pw++ }
      $3 == "MPI_Send" || $3 == "PMPI_Send" { addr[$3] = $1 }
      END { if (t + w + pt + pw == 0) exit
            printf "%-24s MPI_: %4d T %4d W | PMPI_: %4d T %4d W", lib, t, w, pt, pw
            if (addr["MPI_Send"] != "" && addr["MPI_Send"] == addr["PMPI_Send"])
              printf "  (MPI_Send and PMPI_Send share an address)"
            print "" }'
  break
done

step "the collision that a newer release would have brought back"
# Intel MPI ships its own libmpi_abi.so -- it implements the standard ABI
# natively -- and vars.sh puts that directory on LD_LIBRARY_PATH. Our binaries
# record `NEEDED libmpi_abi.so` with a RUNPATH to the build tree, and DT_RUNPATH
# is searched *after* LD_LIBRARY_PATH, so the loader prefers Intel's. Print both
# resolutions rather than asserting one: this is the reason ci.yaml's
# linux-oneapi job does not export LD_LIBRARY_PATH.
echo "  Intel MPI's own ABI library:"
ls -l "${I_MPI_ROOT}"/lib/libmpi_abi.so* 2>/dev/null | sed 's/^/    /' \
  || echo "    none -- this release does not ship one, and the hazard below is moot"
if [ -e "${I_MPI_ROOT}/lib/libmpi_abi.so" ]; then
  echo "  MPI_* symbols it exports: \
$(nm -D --defined-only "${I_MPI_ROOT}/lib/libmpi_abi.so" 2>/dev/null |
  grep -cE ' (T|W) MPI_')"
fi

step "the wrapper, end to end"
ci_status=0
# MPI_LABEL because the label linux-test.sh would otherwise derive from
# $MPICC is `2021.18` -- `command -v mpicc` resolves through the `latest`
# symlink to the versioned directory -- which names nothing a reader recognises.
#
# `env -u LD_LIBRARY_PATH` is belt and braces at the pinned version, which
# ships no libmpi_abi.so for vars.sh to put in the way -- but it is exactly
# what stops a version bump past 2021.16 from turning into five mysterious
# failures instead of a clear one. NOTES.md #13.2 has the measurement from
# 2021.18: with the variable exported, five of thirteen tests bind to Intel's
# ABI library rather than ours.
env -u LD_LIBRARY_PATH MPI_LABEL=intelmpi \
  "$repo/ci-scripts/linux-test.sh" "$MPICC" || ci_status=1
# check-install.sh needs no such help: it already runs every consumption route
# under `env -u LD_LIBRARY_PATH`, which is why its six legs passed even on the
# run where ctest did not.
"$repo/ci-scripts/check-install.sh" "$MPICC" || ci_status=1
[ $ci_status -eq 0 ] || fail "linux-test.sh / check-install.sh"

printf '\n=== %s\n' \
  "$([ $status -eq 0 ] && echo 'PROBE-INTELMPI: all claims hold' \
                       || echo 'PROBE-INTELMPI: FAILURES above')"
exit $status
