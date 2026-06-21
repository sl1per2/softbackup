#!/bin/bash
set -e

VERSION=$(cat VERSION)
BUILD_DIR="$(pwd)/output/macos-x64"
mkdir -p "$BUILD_DIR"

if ! command -v x86_64-apple-darwin21-clang &>/dev/null; then
    echo "  ⚠️  macOS cross-compiler (osxcross) not found. Skipping."
    exit 0
fi

echo "  → Configuring CMake (macOS x64, Release)..."
cd ../..
cmake -B build/macos-x64 -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=x86_64-apple-darwin21-clang \
    -DCMAKE_CXX_COMPILER=x86_64-apple-darwin21-clang++ \
    -DCMAKE_CXX_STANDARD=17

cmake --build build/macos-x64 -j$(nproc) --target vovqa-core
cp build/macos-x64/vovqa-core "$BUILD_DIR/obs-agent"
echo "  → macOS x64 agent built: $BUILD_DIR/obs-agent"

cd - > /dev/null
