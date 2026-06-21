#!/bin/bash
set -e

VERSION=$(cat VERSION)
BUILD_DIR="$(pwd)/output/windows-x64"
mkdir -p "$BUILD_DIR"

if ! command -v x86_64-w64-mingw32-gcc &>/dev/null; then
    echo "  → Installing MinGW-w64 cross-compiler..."
    sudo apt install -y mingw-w64
fi

echo "  → Configuring CMake (Windows x64, Release)..."
cd ../..
cmake -B build/windows-x64 -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
    -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
    -DCMAKE_CXX_STANDARD=17

echo "  → Building..."
cmake --build build/windows-x64 -j$(nproc) --target vovqa-core 2>/dev/null || {
    echo "  ⚠️  Windows cross-compilation requires MinGW-compatible deps"
    echo "  → Skipping Windows x64 build on this host"
    cd - > /dev/null
    return 0
}

x86_64-w64-mingw32-strip --strip-all build/windows-x64/vovqa-core.exe 2>/dev/null || true
cp build/windows-x64/vovqa-core.exe "$BUILD_DIR/obs-agent.exe" 2>/dev/null
echo "  → Windows x64 agent built: $BUILD_DIR/obs-agent.exe"

cd - > /dev/null
