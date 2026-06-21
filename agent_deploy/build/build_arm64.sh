#!/bin/bash
set -e

OUTPUT="/output"
mkdir -p "$OUTPUT"

echo "========================================="
echo " Building Linux ARM64 Agent"
echo "========================================="

cd /build/core
rm -rf build

echo "▶ Configuring CMake (ARM64, simplified)..."
cmake -B build/arm64 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
  -DCMAKE_CXX_STANDARD=17 \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_FIND_ROOT_PATH=/usr/aarch64-linux-gnu \
  -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
  -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=BOTH \
  -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=BOTH \
  -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH

echo "▶ Building..."
cmake --build build/arm64 -j$(nproc) 2>&1 | tail -15

if [ -f build/arm64/vovqa-core ]; then
    aarch64-linux-gnu-strip --strip-all build/arm64/vovqa-core
    cp build/arm64/vovqa-core "$OUTPUT/obs-agent-linux-arm64"
    chmod +x "$OUTPUT/obs-agent-linux-arm64"
    echo ""
    echo "✅ Linux ARM64 built successfully!"
    file "$OUTPUT/obs-agent-linux-arm64"
    ls -la "$OUTPUT/obs-agent-linux-arm64"
else
    echo "❌ Build failed"
    exit 1
fi
