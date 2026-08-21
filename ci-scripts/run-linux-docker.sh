#!/usr/bin/env bash
# Run linux-test.sh in a container, from any host that has Docker.
#
#   ci-scripts/run-linux-docker.sh              # both distro MPIs, each on its own image
#   ci-scripts/run-linux-docker.sh mpich        # one
#   ci-scripts/run-linux-docker.sh floor        # the MPI-3.0 floor, built from source
#   MPIABI_IMAGE=ubuntu:24.04 ci-scripts/run-linux-docker.sh   # override the image
#
# MPIABI_LINUX_SCRIPT names what runs inside; it defaults to linux-test.sh
# (build and ctest), the `floor` row uses linux-floor.sh, and S7's suite runner
# is the third:
#
#   MPIABI_LINUX_SCRIPT=/src/ci-scripts/suite/linux-suite.sh \
#     MPIABI_LINUX_OUT=$PWD/build/linux-out \
#     ci-scripts/run-linux-docker.sh openmpi
#
# MPIABI_LINUX_OUT, if set, is mounted read-write at /out and is where the
# inner script puts anything worth keeping -- the suite's TAP file and its
# logs, which are otherwise destroyed with the container and are exactly what
# triaging a failure needs.
#
# This exists because the developers' machines are macOS and the failure modes
# are not the same: the first Linux build of this project needed four fixes that
# macOS could not have shown, from _GNU_SOURCE to a GNU-vs-BSD difference in
# `patch`. Reaching Linux early, from the machine the work happens on, is worth
# a small script.
#
# The source tree is mounted **read-only** on purpose. That is what caught the
# `patch -o -` bug -- GNU patch writes a temporary file into the current
# directory -- and it keeps a Linux build from leaving artifacts in a macOS
# checkout. All build output goes to /tmp inside the container.
set -euo pipefail

# The default image is per-MPI, and neither default is arbitrary. MPIABI_IMAGE
# overrides both.
#
#   mpich -> debian:13. **Ubuntu 24.04's MPICH cannot run a multi-rank job at
#     all**, and does not say so: its libmpi is built --with-pmix and imports
#     PMIx_Init and no other PMI entry point, while the hydra it ships is a
#     PMI-1 server setting PMI_FD/PMI_RANK/PMI_SIZE that this libmpi never
#     reads. PMIx_Init finds no server, MPICH falls back to singleton
#     initialization, and `mpiexec -n 2` returns two processes that each report
#     "rank 0 of 1" and exit 0. There is no workaround inside that image --
#     Ubuntu 24.04 packages no PMIx launcher (no prrte, no prte, no prun) -- so
#     the fix is an image whose MPICH is coherent. Debian 13's MPICH 4.2.1 links
#     no libpmix and hydra's PMI-1 is what its libmpi speaks (HISTORY.md 2.14).
#
#   openmpi -> ubuntu:24.04, and this one must not drift: S7's expected-failure
#     list is calibrated against **the Open MPI 4.1.6 that Ubuntu 24.04 ships**
#     (ci-scripts/suite/xfail-openmpi.txt says so in its header, and its 168
#     lines are version-specific). Moving this row to another image invalidates
#     that file.
#   floor -> ubuntu:20.04, and this one is the image as much as it is the MPI:
#     MPICH 3.1.4's configure rejects gcc 11's gfortran and gcc 13's, so gcc 9
#     is what the row can be built with. See ci-scripts/linux-floor.sh.
image_for() {
  case ${MPIABI_IMAGE:-} in ?*) printf '%s' "$MPIABI_IMAGE"; return ;; esac
  case $1 in
    mpich) printf 'debian:13' ;;
    floor) printf 'ubuntu:20.04' ;;
    *)     printf 'ubuntu:24.04' ;;
  esac
}

# `floor` provisions its own MPI before running the same tests, since no current
# distribution packages anything as old as MPI-3.0's floor.
inner_for() {
  case ${MPIABI_LINUX_SCRIPT:-} in ?*) printf '%s' "$MPIABI_LINUX_SCRIPT"; return ;; esac
  case $1 in
    floor) printf '/src/ci-scripts/linux-floor.sh' ;;
    *)     printf '/src/ci-scripts/linux-test.sh' ;;
  esac
}

src=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
mpis=("$@")
# `floor` is not in the default set: it builds an MPI from source and takes
# some fifteen minutes, against under two for each of the rows below.
[ ${#mpis[@]} -gt 0 ] || mpis=(mpich openmpi)

command -v docker >/dev/null || { echo "docker not found" >&2; exit 2; }

# **--shm-size, because Docker's default is 64 MB and an MPI lives in
# /dev/shm.** A container gets 64 MB there whatever the host has; MPICH's
# ch4:shm maps its shared segments in that tmpfs, and touching a page the tmpfs
# cannot back raises SIGBUS -- which is not a signal x86 raises for anything the
# tests themselves do wrong.
#
# Measured, one flag apart in run 32431588989, on the i386 `rest` shard of the
# suite minus its io directory:
#
#   default (64 MB, and `size=65536k` is what the mount says) -- the job did not
#     finish. It hit its 40-minute limit inside `docker run`, having done work the
#     other leg did in 3m13s. In CI the same shard fails 172 tests, 139 of them
#     with "Bus error (signal 7)".
#   --shm-size=8g -- **283 tests, 248 passed, 33 failed, every failure already in
#     the shared expected-failure list, none unlisted, none a crash**, and the
#     gate green: 44 seconds of testing against x86_64's 55. Nothing about ILP32
#     left to explain.
#
# The mechanism is a leak, also measured: /dev/shm went 3 -> 22 -> 24 entries and
# 4.1 -> 36 -> 39 MB during those 44 seconds and **all 24 `mpich_shm_*` files
# were still there when the suite ended**. Nothing reclaims them, so against a
# 64 MB cap the tmpfs fills as the shard runs, and touching a page it cannot back
# is SIGBUS. The runner's own /dev/shm is 7.9 GB, which is why the four 64-bit
# rows -- which run on the runner rather than in a container -- never see this.
#
# It is a cap and not a reservation -- tmpfs pages are allocated on demand, and
# this leg used 39 MB of the 8 GB -- so a generous number costs nothing until
# something uses it, and something using all of it would itself be a finding. 8g
# is the value that was measured; MPIABI_DOCKER_SHM_SIZE overrides it.
# ci-scripts/suite/README.md has the full comparison and what it does not settle.
#
# The same trap exists for a `container:` job in GitHub Actions, which is
# .github/workflows/ci.yaml's `linux-distro` and `sanitize` rows; there the knob
# is `options: --shm-size=...`. Those rows run 13 ctest tests at two ranks and
# pass, so nothing is changed there on the strength of this measurement.
shm_size=${MPIABI_DOCKER_SHM_SIZE:-8g}

# **The suite's own knobs have to cross the container boundary, and they did not.**
# A `docker run` inherits nothing from the caller's environment, so a job setting
# MPITEST_TIMEOUT_MULTIPLIER outside this script was setting it for a shell that
# runs no tests -- which is how the i386 row came to be the one MPICH leg without
# the multiplier the others set, in a way no file said out loud.
#
# `${VAR:+-e VAR}` forwards a variable only when it is set *and non-empty*, which
# is deliberate rather than idiomatic: MPITEST_MEMORY_TOTAL is read by runtests
# with `defined()`, an empty string is defined, and `''` compares numerically as 0,
# so forwarding an empty one would skip every `mem=`-annotated test in the run.
# Absent is the only safe way to spell "default" for that family
# (ci-scripts/suite/README.md records what that trap cost the rma probe).
docker_env=(
  ${MPITEST_TIMEOUT_MULTIPLIER:+-e MPITEST_TIMEOUT_MULTIPLIER}
  ${MPITEST_MEMORY_TOTAL:+-e MPITEST_MEMORY_TOTAL}
  ${RUNTESTS_VERBOSE:+-e RUNTESTS_VERBOSE}
)

status=0
for mpi in "${mpis[@]}"; do
  image=$(image_for "$mpi")
  inner=$(inner_for "$mpi")
  printf '\n########## %s on %s ##########\n' "$mpi" "$image"
  docker run --rm \
    --shm-size="$shm_size" \
    -v "$src:/src:ro" \
    ${MPIABI_LINUX_OUT:+-v "$MPIABI_LINUX_OUT:/out"} \
    ${docker_env[@]+"${docker_env[@]}"} \
    -e SRC=/src \
    "$image" bash "$inner" "$mpi" ${MPIABI_LINUX_ARGS:-} || status=1
done

printf '\n########## %s\n' \
  "$([ $status -eq 0 ] && echo 'all MPIs passed' || echo 'FAILURES above')"
exit $status
