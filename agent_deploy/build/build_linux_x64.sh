#!/bin/bash
set -e

VERSION=$(cat VERSION)
BUILD_DIR="$(pwd)/output/linux-x64"
mkdir -p "$BUILD_DIR"

echo "  → Configuring CMake (Linux x64, Release)..."
cd ../..
cmake -B build/linux-x64 -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=gcc \
    -DCMAKE_CXX_COMPILER=g++ \
    -DCMAKE_CXX_STANDARD=17

echo "  → Building..."
cmake --build build/linux-x64 -j$(nproc) --target vovqa-core

echo "  → Stripping binary..."
strip --strip-all build/linux-x64/vovqa-core

echo "  → Copying to output..."
cp build/linux-x64/vovqa-core "$BUILD_DIR/obs-agent"

echo "  → Linux x64 agent built: $BUILD_DIR/obs-agent"
file "$BUILD_DIR/obs-agent"

cd - > /dev/null
