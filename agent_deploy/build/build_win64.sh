#!/bin/bash
set -e

OUTPUT="/output"
mkdir -p "$OUTPUT"

echo "========================================="
echo " Building Windows x64 Agent (MinGW)"
echo "========================================="

cd /build/core
rm -rf build

# Try to cross-compile protobuf for Windows
echo "▶ Cross-compiling protobuf for Windows..."
cd /tmp
if [ ! -d "protobuf-3.21.12" ]; then
    wget -q https://github.com/protocolbuffers/protobuf/releases/download/v21.12/protobuf-cpp-3.21.12.tar.gz 2>/dev/null || true
    tar xzf protobuf-cpp-3.21.12.tar.gz 2>/dev/null || true
fi

if [ -d "/tmp/protobuf-3.21.12" ]; then
    cd /tmp/protobuf-3.21.12
    rm -rf build-win64
    mkdir build-win64 && cd build-win64
    cmake .. -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_SYSTEM_NAME=Windows \
      -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
      -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
      -DCMAKE_INSTALL_PREFIX=/cross/win64 2>/dev/null || true
    make -j$(nproc) 2>/dev/null || true
    make install 2>/dev/null || true
    echo "  Protobuf cross-compile attempted"
fi

# Build the agent
echo ""
echo "▶ Building OBS Agent for Windows x64..."
cd /build/core

cmake -B build/win64 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
  -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
  -DCMAKE_CXX_STANDARD=17 \
  -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_FIND_ROOT_PATH=/usr/x86_64-w64-mingw32 \
  -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
  -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=BOTH \
  -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=BOTH \
  -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH 2>&1 | tail -10

cmake --build build/win64 -j$(nproc) 2>&1 | tail -10 || true

if [ -f build/win64/vovqa-core.exe ]; then
    x86_64-w64-mingw32-strip --strip-all build/win64/vovqa-core.exe
    cp build/win64/vovqa-core.exe "$OUTPUT/obs-agent-windows-x64.exe"
    echo "✅ Windows x64: $OUTPUT/obs-agent-windows-x64.exe"
    file "$OUTPUT/obs-agent-windows-x64.exe"
    ls -la "$OUTPUT/obs-agent-windows-x64.exe"
else
    echo "❌ Windows x64 build failed"
    echo "  MinGW cannot cross-compile with OpenSSL/protobuf dependencies"
    echo "  Use Windows build machine or Docker with Windows base image"
fi

echo ""
echo "========================================="
echo " Results:"
echo "========================================="
ls -la "$OUTPUT/" 2>/dev/null || echo "No output"
