#!/bin/bash
set -e

VERSION=$(cat ../build/VERSION)
SOURCE_DIR="../build/output"
PACKAGE_DIR="./output"
export SOURCE_DIR PACKAGE_DIR VERSION

mkdir -p "$PACKAGE_DIR"

echo "========================================="
echo " OBS Backup Agent Packaging v${VERSION}"
echo "========================================="

# Linux x64
if [ -f "$SOURCE_DIR/linux-x64/obs-agent" ]; then
    echo ""
    echo "▶ Packaging Linux x64..."
    bash ./package_deb.sh linux-x64 amd64 "$VERSION" || true
    bash ./package_rpm.sh linux-x64 x86_64 "$VERSION" || true
    bash ./package_portable.sh linux-x64 amd64 tar.gz "$VERSION" || true
    echo "✅ Linux x64 done"
fi

# Linux ARM64
if [ -f "$SOURCE_DIR/linux-arm64/obs-agent" ]; then
    echo ""
    echo "▶ Packaging Linux ARM64..."
    bash ./package_deb.sh linux-arm64 arm64 "$VERSION" || true
    bash ./package_rpm.sh linux-arm64 aarch64 "$VERSION" || true
    bash ./package_portable.sh linux-arm64 arm64 tar.gz "$VERSION" || true
    echo "✅ Linux ARM64 done"
fi

# Windows x64 — MSI
if [ -f "$SOURCE_DIR/windows-x64/obs-agent.exe" ]; then
    echo ""
    echo "▶ Packaging Windows x64 MSI..."
    bash ./package_msi_win64.sh || true
    echo "✅ Windows x64 MSI done"
fi

# Windows x64 — NSIS Installer
if [ -f "$SOURCE_DIR/windows-x64/obs-agent.exe" ]; then
    echo ""
    echo "▶ Packaging Windows x64 NSIS..."
    bash ./package_exe_win64.sh || true
    echo "✅ Windows x64 NSIS done"
fi

# Windows x64 — Portable ZIP
if [ -f "$SOURCE_DIR/windows-x64/obs-agent.exe" ]; then
    echo ""
    echo "▶ Packaging Windows x64 Portable..."
    bash ./package_portable_win64.sh || true
    echo "✅ Windows x64 Portable done"
fi

# Windows x86 — MSI
if [ -f "$SOURCE_DIR/windows-x86/obs-agent.exe" ]; then
    echo ""
    echo "▶ Packaging Windows x86 MSI..."
    bash ./package_msi_win32.sh || true
    echo "✅ Windows x86 MSI done"
fi

# Windows x86 — NSIS Installer
if [ -f "$SOURCE_DIR/windows-x86/obs-agent.exe" ]; then
    echo ""
    echo "▶ Packaging Windows x86 NSIS..."
    bash ./package_exe_win32.sh || true
    echo "✅ Windows x86 NSIS done"
fi

# Windows x86 — Portable ZIP
if [ -f "$SOURCE_DIR/windows-x86/obs-agent.exe" ]; then
    echo ""
    echo "▶ Packaging Windows x86 Portable..."
    bash ./package_portable_win32.sh || true
    echo "✅ Windows x86 Portable done"
fi

# macOS x64
if [ -f "$SOURCE_DIR/macos-x64/obs-agent" ]; then
    echo ""
    echo "▶ Packaging macOS x64..."
    bash ./package_dmg.sh macos-x64 "$VERSION" || true
    bash ./package_portable.sh macos-x64 x64 tar.gz "$VERSION" || true
    echo "✅ macOS x64 done"
fi

# macOS ARM64
if [ -f "$SOURCE_DIR/macos-arm64/obs-agent" ]; then
    echo ""
    echo "▶ Packaging macOS ARM64..."
    bash ./package_dmg.sh macos-arm64 "$VERSION" || true
    bash ./package_portable.sh macos-arm64 arm64 tar.gz "$VERSION" || true
    echo "✅ macOS ARM64 done"
fi

echo ""
echo "========================================="
echo " All packages created!"
echo " Output: $PACKAGE_DIR"
echo "========================================="
echo ""
echo "Available packages:"
ls -lh "$PACKAGE_DIR"/*.deb "$PACKAGE_DIR"/*.rpm "$PACKAGE_DIR"/*.msi "$PACKAGE_DIR"/*.exe "$PACKAGE_DIR"/*.zip "$PACKAGE_DIR"/*.dmg "$PACKAGE_DIR"/*.tar.gz 2>/dev/null || echo "  (none found — check build output)"
