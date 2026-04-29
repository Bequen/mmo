# ── build stage ──────────────────────────────────────────────────────────────
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    gcc-13 g++-13 \
    cmake \
    ninja-build \
    git \
    mold \
    libvulkan-dev \
    libsdl2-dev \
    libprotobuf-dev protobuf-compiler \
    libpq-dev \
    libpqxx-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_COMPILER=gcc-13 \
        -DCMAKE_CXX_COMPILER=g++-13 \
        -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu \
    && cmake --build build --target tw_server -j$(nproc)

# ── runtime stage ─────────────────────────────────────────────────────────────
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    libvulkan1 \
    libsdl2-2.0-0 \
    libprotobuf32t64 \
    libpq5 \
    libpqxx-dev \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /src/build/modules/server/tw_server /usr/local/bin/tw_server

EXPOSE 8101/udp 8102/udp

ENTRYPOINT ["tw_server"]
