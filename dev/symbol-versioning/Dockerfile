# Linux (glibc) is the only platform where the question exists: Mach-O has no
# symbol versioning, so the macOS build uses -exported_symbols_list and is
# unaffected either way.
FROM ubuntu:24.04
RUN apt-get update && apt-get install -y --no-install-recommends gcc binutils libc6-dev \
    && rm -rf /var/lib/apt/lists/*
COPY probe.sh /probe.sh
CMD ["sh", "/probe.sh"]
