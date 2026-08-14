ARG IMAGE=debian:bookworm-slim
FROM ${IMAGE}
RUN apt-get update && apt-get install -y --no-install-recommends gcc libc6-dev binutils \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /bench
COPY bench.h libcallee.c libdispatch.c bench.c run.sh ./
RUN chmod +x run.sh
CMD ["./run.sh", "20000000", "9"]
