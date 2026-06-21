#!/bin/bash
set -e

PLATFORM=$1; ARCH=$2; FORMAT=$3; VERSION=$4
SOURCE_DIR="${SOURCE_DIR:-../build/output}"
PACKAGE_DIR="${PACKAGE_DIR:-./output}"
WORKDIR="/tmp/obs-portable-$$"

mkdir -p "$WORKDIR"

if [ "$FORMAT" = "tar.gz" ]; then
    cp "$SOURCE_DIR/$PLATFORM/obs-agent" "$WORKDIR/"
    cp ../package/systemd/obs-agent.service "$WORKDIR/" 2>/dev/null || true
    tar -czf "$PACKAGE_DIR/obs-agent-${VERSION}-${PLATFORM}.tar.gz" -C "$WORKDIR" .
    echo "  → Portable tar.gz: obs-agent-${VERSION}-${PLATFORM}.tar.gz"

elif [ "$FORMAT" = "zip" ]; then
    cp "$SOURCE_DIR/$PLATFORM/obs-agent.exe" "$WORKDIR/" 2>/dev/null || true
    cat > "$WORKDIR/install.bat" <<'BAT'
@echo off
echo Installing OBS Backup Agent...
mkdir "C:\Program Files\OBS Agent" 2>nul
copy "%~dp0obs-agent.exe" "C:\Program Files\OBS Agent\"
sc create "OBS Agent" binPath= "\"C:\Program Files\OBS Agent\obs-agent.exe\" --service" start= auto
sc start "OBS Agent"
echo Done.
BAT
    cd "$WORKDIR" && zip -r "../$PACKAGE_DIR/obs-agent-${VERSION}-${PLATFORM}.zip" .
    cd - > /dev/null
    echo "  → Portable zip: obs-agent-${VERSION}-${PLATFORM}.zip"
fi

rm -rf "$WORKDIR"
