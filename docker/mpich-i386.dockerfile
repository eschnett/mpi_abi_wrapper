# The 32-bit row. Build it, and the build is the test:
#
#   docker build --file docker/mpich-i386.dockerfile --platform linux/386 .
#
# NOTES.md #10 calls 32-bit "load-bearing, not routine coverage", and #4.1 says
# why: an ABI handle is *pointer-sized*, so it is 32 bits here and 64 everywhere
# else this project runs. ILP32 is the only place the consequence is visible --
# no spare high bits for a tagging scheme, which is why #5.1's collision check
# is a run-time probe in mpiwrapper_selftest rather than a configure-time one.
# It is also where the status layouts of #4.2 shrink (Open MPI's `_ucount` is a
# `size_t`), and where `sizeof(impl status) <= 32` stops having room to spare.
#
# `linux/386` runs natively on an x86_64 kernel, so this costs about what a
# 64-bit build costs and needs no qemu. That is what makes it a cheap row on
# GitHub's x86_64 runners; on an arm64 development machine it is emulated and
# slow, which is a property of the machine and not of this file.
#
# Debian rather than Ubuntu because Ubuntu dropped i386 as a full architecture
# after 18.04. Debian still ships it as a port, with the same MPICH 4.2.1 the
# amd64 `debian:13` row uses -- so this row differs from that one in exactly one
# variable, which is the point of it.
FROM i386/debian:trixie-slim

SHELL ["/bin/bash", "-c"]
ENV DEBIAN_FRONTEND=noninteractive

# Installed here rather than left to ci-scripts/linux-test.sh's own apt block,
# which would also install them: as its own layer it is cached across runs and
# does not rebuild when the source changes. The script's block then finds
# everything present and is a no-op.
#
# This list is linux-test.sh's, and it is duplicated on purpose rather than
# sourced -- a dockerfile cannot run the script to learn what to install before
# it has installed anything. If the script's list grows, this one does not have
# to: the script installs the difference itself.
RUN <<EOF
    set -e
    apt-get update -qq
    apt-get install -y -qq --no-install-recommends \
        build-essential cmake python3 patch binutils poppler-utils \
        libmpich-dev mpich
    rm -rf /var/lib/apt/lists/*
EOF

WORKDIR /src
COPY . .

# Everything else is the same entry point the other Linux rows use, so the 32-bit
# row cannot drift from them: same configure, same build, same 13 ctest tests,
# same split between gating and informational steps. BUILD is set because /src is
# the image's own copy and the default /tmp/build-mpich is fine, but naming it
# keeps the log's paths honest about which row this is.
ENV BUILD=/tmp/build-mpich-i386
RUN ci-scripts/linux-test.sh mpich
