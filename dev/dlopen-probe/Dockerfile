# Linux results for the dlopen probe, from a macOS host.
#
#   docker build -t dlopen-probe . && docker run --rm dlopen-probe
#
# Add --build-arg IMAGE=i386/debian:bookworm for a 32-bit run, which is worth
# doing: ABI handles are pointer-sized, so the 32-bit case differs elsewhere in the
# design and it is cheap to confirm the loader behaves the same here.
ARG IMAGE=debian:bookworm-slim
FROM ${IMAGE}

RUN apt-get update && apt-get install -y --no-install-recommends \
        gcc libc6-dev binutils \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /probe
COPY probe.h libimpl.c libwrap.c libabi.c app.c plugin.c hostapp.c run.sh ./
RUN chmod +x run.sh

CMD ["./run.sh"]
