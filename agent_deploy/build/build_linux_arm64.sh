#!/bin/bash
set -e

VERSION=$(cat VERSION)
BUILD_DIR="$(pwd)/output/linux-arm64"
mkdir -p "$BUILD_DIR"

if ! command -v aarch64-linux-gnu-gcc &>/dev/null; then
    echo "  → Installing ARM64 cross-compiler..."
    sudo apt install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
fi

echo "  → Configuring CMake (Linux ARM64, Release)..."
cd ../..
cmake -B build/linux-arm64 -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
    -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
    -DCMAKE_CXX_STANDARD=17

echo "  → Building..."
cmake --build build/linux-arm64 -j$(nproc) --target vovqa-core

echo "  → Stripping binary..."
aarch64-linux-gnu-strip --strip-all build/linux-arm64/vovqa-core

echo "  → Copying to output..."
cp build/linux-arm64/vovqa-core "$BUILD_DIR/obs-agent"

echo "  → Linux ARM64 agent built: $BUILD_DIR/obs-agent"
file "$BUILD_DIR/obs-agent"

cd - > /dev/null
