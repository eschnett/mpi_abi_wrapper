#!/usr/bin/env bash
# What ci-scripts/install-mvapich.sh's header claims about MVAPICH 4.1, checked
# rather than asserted. Runs *inside* a Linux environment; run.sh puts one
# around it.
#
#   MPI_SRC_DIR must be on the container's own filesystem, not a bind mount.
#   That is not a preference -- see README.md, "The failure that was the
#   harness's".
#
# Every line it prints is a claim in install-mvapich.sh or in the ci.yaml
# comment above the `mvapich` matrix rows.

set -uo pipefail

prefix=${PREFIX:-/opt/mvapich}
version=${VERSION:-4.1}
repo=${REPO:-/repo}
status=0

step() { printf '\n=== %s\n' "$*"; }
fail() { echo "FAILED: $*" >&2; status=1; }

export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
# libibverbs-dev and librdmacm-dev are not optional and not about having
# InfiniBand. MVAPICH's bundled libfabric carries two providers that MPICH's
# does not -- prov/mverbs and prov/ucr -- and they include <infiniband/ib.h>
# unconditionally, so a stock configure on a machine with no rdma-core headers
# fails at prov/ucr/src/ucr_domain.lo. The headers are enough; no hardware and
# no kernel module are involved.
apt-get install -y -qq --no-install-recommends \
    build-essential gfortran cmake python3 patch binutils curl ca-certificates \
    pkg-config libibverbs-dev librdmacm-dev >/dev/null \
  || { echo "package install failed" >&2; exit 2; }

step "stock configure, build, install (no --with-device, no --enable-mpi-abi)"
"$repo/ci-scripts/install-mvapich.sh" "$prefix" "$version" \
  || { fail "install-mvapich.sh"; exit $status; }

step "what the source tree declares"
# The two numbers install-mvapich.sh's header quotes. mpi.h.in holds no
# prototypes at all -- mpi.h includes src/include/mpi_proto.h, which is where
# the `_c` surface actually is, and grepping the wrong file reports zero.
tree=$(dirname "$(dirname "$prefix")")
for t in "${MPI_SRC_DIR:-}/mvapich-$version" /build/"mvapich-$version"; do
  [ -d "$t" ] && tree=$t && break
done
if [ -d "$tree/src/include" ]; then
  grep -h 'define MPI_VERSION\|define MPI_SUBVERSION' "$tree/src/include/mpi.h.in"
  echo "  _c prototypes in src/include/mpi_proto.h: \
$(grep -c 'MPI_[A-Za-z_]*_c(' "$tree/src/include/mpi_proto.h")"
  echo "  --enable-mpi-abi present in configure.ac: \
$(grep -c 'enable-mpi-abi' "$tree/configure.ac") occurrence(s), default \
$(grep -o 'enable_mpi_abi=[a-z]*' "$tree/configure.ac" | head -1)"
  echo "  process manager: $(ls -d "$tree"/src/pm/*/ | xargs -n1 basename | tr '\n' ' ')"
  echo "  bundled modules: $(ls "$tree/modules" | tr '\n' ' ')"
else
  echo "  (source tree not retained; skipping the tarball-level claims)"
fi

step "the installed wrapper"
"$prefix/bin/mpicc" --version | head -1
"$prefix/bin/mpichversion" 2>/dev/null | head -4 || true
ls "$prefix"/lib/libmpi* 2>/dev/null

step "declared MPI level, through the installed mpi.h"
cat > /tmp/v.c <<'EOF'
#include <mpi.h>
#include <stdio.h>
int main(void) { printf("MPI %d.%d\n", MPI_VERSION, MPI_SUBVERSION); return 0; }
EOF
"$prefix/bin/mpicc" -o /tmp/v /tmp/v.c && /tmp/v || fail "mpi.h does not compile"

step "the large-count surface, at link time rather than by grep"
# dev/probe_impl.py's compile-only probe would see a declaration that has no
# implementation; install-mpich.sh's header records that exact trap for
# MPI_Type_create_f90_real. So this links and runs.
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
if "$prefix/bin/mpicc" -o /tmp/lc /tmp/lc.c 2>/tmp/lc.err && /tmp/lc; then
  echo "  MPI_Type_size_c links and returns 4 -- the _c surface is real here"
else
  head -5 /tmp/lc.err 2>/dev/null
  fail "the _c surface is declared but does not link/run"
fi

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
"$prefix/bin/mpicc" -o /tmp/hello /tmp/hello.c || fail "hello does not build"
if "$prefix/bin/mpiexec" -n 2 /tmp/hello; then
  echo "  Hydra launched two ranks with no fabric configured"
else
  fail "mpiexec -n 2 did not produce a two-rank job"
fi

step "symbol binding (the same report linux-test.sh prints)"
for so in "$prefix"/lib/libmpi.so*; do
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

step "the wrapper, end to end"
ci_status=0
# CTEST_TIMEOUT for the reason ci.yaml sets it on these legs: MPI_Dist_graph_create
# does not return here, so abi_arrays_test would otherwise cost ctest's 1500 s
# default to re-establish the hang this script is what documents.
CTEST_TIMEOUT=${CTEST_TIMEOUT:-45} \
  "$repo/ci-scripts/linux-test.sh" "$prefix/bin/mpicc" || ci_status=1
"$repo/ci-scripts/check-install.sh" "$prefix/bin/mpicc" || ci_status=1
[ $ci_status -eq 0 ] || fail "linux-test.sh / check-install.sh"

printf '\n=== %s\n' \
  "$([ $status -eq 0 ] && echo 'PROBE-MVAPICH: all claims hold' \
                       || echo 'PROBE-MVAPICH: FAILURES above')"
exit $status
