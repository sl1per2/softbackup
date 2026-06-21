#!/bin/bash
set -e

OUTPUT="/output"
mkdir -p "$OUTPUT"

echo "========================================="
echo " Cross-Platform Build"
echo "========================================="

cd /build/core

# === LINUX ARM64 ===
echo ""
echo "▶ Building Linux ARM64..."
rm -rf build/arm64
cmake -B build/arm64 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
  -DCMAKE_CXX_STANDARD=17 \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_FIND_ROOT_PATH=/usr/aarch64-linux-gnu \
  -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
  -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
  -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY

cmake --build build/arm64 -j$(nproc) 2>&1 | tail -5

if [ -f build/arm64/vovqa-core ]; then
    aarch64-linux-gnu-strip --strip-all build/arm64/vovqa-core
    cp build/arm64/vovqa-core "$OUTPUT/obs-agent-linux-arm64"
    chmod +x "$OUTPUT/obs-agent-linux-arm64"
    echo "✅ Linux ARM64: $OUTPUT/obs-agent-linux-arm64"
else
    echo "❌ Linux ARM64 build failed"
fi

# === WINDOWS x64 ===
echo ""
echo "▶ Building Windows x64..."
rm -rf build/win64
cmake -B build/win64 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
  -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
  -DCMAKE_CXX_STANDARD=17

cmake --build build/win64 -j$(nproc) 2>&1 | tail -5 || true

if [ -f build/win64/vovqa-core.exe ]; then
    x86_64-w64-mingw32-strip --strip-all build/win64/vovqa-core.exe
    cp build/win64/vovqa-core.exe "$OUTPUT/obs-agent-windows-x64.exe"
    echo "✅ Windows x64: $OUTPUT/obs-agent-windows-x64.exe"
else
    echo "❌ Windows x64 build failed (MinGW deps issue)"
fi

# === WINDOWS x86 ===
echo ""
echo "▶ Building Windows x86..."
rm -rf build/win32
cmake -B build/win32 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=i686-w64-mingw32-gcc \
  -DCMAKE_CXX_COMPILER=i686-w64-mingw32-g++ \
  -DCMAKE_CXX_STANDARD=17

cmake --build build/win32 -j$(nproc) 2>&1 | tail -5 || true

if [ -f build/win32/vovqa-core.exe ]; then
    i686-w64-mingw32-strip --strip-all build/win32/vovqa-core.exe
    cp build/win32/vovqa-core.exe "$OUTPUT/obs-agent-windows-x86.exe"
    echo "✅ Windows x86: $OUTPUT/obs-agent-windows-x86.exe"
else
    echo "❌ Windows x86 build failed (MinGW deps issue)"
fi

echo ""
echo "========================================="
echo " Build Results:"
echo "========================================="
ls -la "$OUTPUT/"
file "$OUTPUT"/* 2>/dev/null || true
