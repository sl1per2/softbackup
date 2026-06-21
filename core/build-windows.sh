#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -a, --arch ARCH     Target architecture: x86_64 (default) or i686"
    echo "  -b, --build-dir DIR Build directory (default: build/windows-\$ARCH)"
    echo "  -c, --clean         Clean build directory before building"
    echo "  -j, --jobs N        Number of parallel jobs (default: $(nproc))"
    echo "  -t, --target NAME   CMake target to build (default: vovqa-core)"
    echo "  -r, --release       Build in Release mode (default)"
    echo "  -d, --debug         Build in Debug mode"
    echo "  -h, --help          Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                          # Build vovqa-core for x86_64"
    echo "  $0 -a i686                  # Build vovqa-core for i686 (32-bit)"
    echo "  $0 -a x86_64 -c             # Clean build for x86_64"
    echo "  $0 --target test_dedup      # Build test_dedup target"
}

ARCH=x86_64
BUILD_DIR=""
CLEAN=0
JOBS=$(nproc)
TARGET=vovqa-core
BUILD_TYPE=Release

while [[ $# -gt 0 ]]; do
    case $1 in
        -a|--arch) ARCH="$2"; shift 2 ;;
        -b|--build-dir) BUILD_DIR="$2"; shift 2 ;;
        -c|--clean) CLEAN=1; shift ;;
        -j|--jobs) JOBS="$2"; shift 2 ;;
        -t|--target) TARGET="$2"; shift 2 ;;
        -r|--release) BUILD_TYPE=Release; shift ;;
        -d|--debug) BUILD_TYPE=Debug; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown option: $1"; usage; exit 1 ;;
    esac
done

if [ "$ARCH" != "x86_64" ] && [ "$ARCH" != "i686" ]; then
    echo "Error: Architecture must be x86_64 or i686"
    exit 1
fi

if [ -z "$BUILD_DIR" ]; then
    BUILD_DIR="$PROJECT_ROOT/core/build/windows-${ARCH}"
fi

echo "=== Building for Windows ${ARCH} ==="
echo "Build directory: $BUILD_DIR"
echo "Build type: $BUILD_TYPE"
echo "Target: $TARGET"
echo "Jobs: $JOBS"

# Build using Docker
docker build \
    --build-arg TARGET_ARCH=$ARCH \
    -t vovqa-windows-${ARCH}:latest \
    -f "$PROJECT_ROOT/core/Dockerfile.windows" \
    "$PROJECT_ROOT"

# Extract the built binary
CONTAINER_ID=$(docker create vovqa-windows-${ARCH}:latest)
mkdir -p "$BUILD_DIR"

docker cp "${CONTAINER_ID}:/build/build/vovqa-core.exe" "$BUILD_DIR/" 2>/dev/null || true
docker cp "${CONTAINER_ID}:/build/build/" "$BUILD_DIR/" 2>/dev/null || true
docker rm "$CONTAINER_ID"

echo "=== Build complete ==="
ls -la "$BUILD_DIR"/*.exe 2>/dev/null || echo "No .exe files found"
