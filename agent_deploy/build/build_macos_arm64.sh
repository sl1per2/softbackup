#!/bin/bash
set -e

VERSION=$(cat VERSION)
BUILD_DIR="$(pwd)/output/macos-arm64"
mkdir -p "$BUILD_DIR"

if ! command -v arm64-apple-darwin21-clang &>/dev/null; then
    echo "  ⚠️  macOS ARM64 cross-compiler not found. Skipping."
    exit 0
fi

echo "  → Configuring CMake (macOS ARM64, Release)..."
cd ../..
cmake -B build/macos-arm64 -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=arm64-apple-darwin21-clang \
    -DCMAKE_CXX_COMPILER=arm64-apple-darwin21-clang++ \
    -DCMAKE_CXX_STANDARD=17

cmake --build build/macos-arm64 -j$(nproc) --target vovqa-core
cp build/macos-arm64/vovqa-core "$BUILD_DIR/obs-agent"
echo "  → macOS ARM64 agent built: $BUILD_DIR/obs-agent"

cd - > /dev/null
