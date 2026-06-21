# Build C++ core
FROM ubuntu:22.04 AS cpp-build

RUN apt-get update && apt-get install -y \
    build-essential cmake \
    libprotobuf-dev protobuf-compiler \
    libboost-all-dev \
    libssl-dev \
    libsqlite3-dev \
    libcurl4-openssl-dev \
    libzstd-dev \
    liblz4-dev \
    libsodium-dev \
    libblake3-dev \
    nlohmann-json3-dev \
    libspdlog-dev \
    pkg-config

WORKDIR /build
COPY core/ .
RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libprotobuf23 libboost-system1.74.0 libboost-filesystem1.74.0 \
    libssl1.1 libsqlite3-0 libcurl4 libzstd1 liblz4-1 libsodium23 libspdlog1 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=cpp-build /build/build/obs-core /usr/local/bin/

RUN useradd -r -s /bin/false obs && \
    mkdir -p /var/lib/obs /var/log/obs /var/cache/obs /etc/obs && \
    chown -R obs:obs /var/lib/obs /var/log/obs /var/cache/obs

COPY core/config.example.json /etc/obs/agent.json

USER obs
CMD ["obs-core", "/etc/obs/agent.json"]
