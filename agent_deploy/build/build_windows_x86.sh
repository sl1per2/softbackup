#!/bin/bash
set -e

VERSION=$(cat VERSION)
BUILD_DIR="$(pwd)/output/windows-x86"
mkdir -p "$BUILD_DIR"

echo "  → Configuring CMake (Windows x86, Release)..."
cd ../..
cmake -B build/windows-x86 -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=i686-w64-mingw32-gcc \
    -DCMAKE_CXX_COMPILER=i686-w64-mingw32-g++ \
    -DCMAKE_CXX_STANDARD=17

cmake --build build/windows-x86 -j$(nproc) --target vovqa-core 2>/dev/null || {
    echo "  ⚠️  Windows x86 cross-compilation skipped"
    cd - > /dev/null
    return 0
}

i686-w64-mingw32-strip --strip-all build/windows-x86/vovqa-core.exe 2>/dev/null || true
cp build/windows-x86/vovqa-core.exe "$BUILD_DIR/obs-agent.exe" 2>/dev/null
echo "  → Windows x86 agent built: $BUILD_DIR/obs-agent.exe"

cd - > /dev/null
