#!/bin/bash
set -e

VERSION=$(cat VERSION)
BUILD_DIR="$(pwd)/output"
mkdir -p "$BUILD_DIR"

echo "========================================="
echo " OBS Backup Agent Build System v${VERSION}"
echo "========================================="

PLATFORMS=(
    "linux-x64"
    "linux-arm64"
    "windows-x64"
    "windows-x86"
    "macos-x64"
    "macos-arm64"
)

for platform in "${PLATFORMS[@]}"; do
    echo ""
    echo "▶ Building for ${platform}..."
    SCRIPT="./build_${platform}.sh"
    if [ -f "$SCRIPT" ]; then
        bash "$SCRIPT" || echo "⚠️  Skipped ${platform}"
    else
        echo "  ⚠️  Script not found: $SCRIPT"
    fi
    echo "✅ ${platform} done"
done

echo ""
echo "========================================="
echo " All agents built successfully!"
echo " Output: $BUILD_DIR"
echo "========================================="
ls -la "$BUILD_DIR"
