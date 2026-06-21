#!/bin/bash
set -e

VERSION=${VERSION:-$(cat ../build/VERSION)}
SOURCE_DIR="${SOURCE_DIR:-../build/output}"
PACKAGE_DIR="${PACKAGE_DIR:-./output}"
SOURCE="$SOURCE_DIR/windows-x86/obs-agent.exe"
OUTPUT="$PACKAGE_DIR/obs-agent-${VERSION}-x86.zip"
WORKDIR="/tmp/obs-zip-x86-$$"

mkdir -p "$WORKDIR"
cp "$SOURCE" "$WORKDIR/obs-agent.exe" 2>/dev/null || { echo "  ⚠️  Binary not found. Skipping."; return 0; }

cat > "$WORKDIR/install.bat" <<'BAT'
@echo off
echo ======================================
echo  OBS Backup Agent Installer (x86)
echo ======================================
echo.
echo Installing agent (32-bit)...
mkdir "C:\Program Files\OBS Agent" 2>nul
copy "%~dp0obs-agent.exe" "C:\Program Files\OBS Agent\"
echo.
echo Creating Windows service...
sc create "OBS Agent" binPath= "\"C:\Program Files\OBS Agent\obs-agent.exe\" --service" start= auto
sc description "OBS Agent" "OBS Backup Agent (32-bit)"
echo.
echo Starting service...
sc start "OBS Agent"
echo.
echo ======================================
echo  OBS Backup Agent (x86) installed!
echo  Configure: edit config.json
echo ======================================
pause
BAT

cat > "$WORKDIR/uninstall.bat" <<'BAT'
@echo off
sc stop "OBS Agent"
timeout /t 3 >nul
sc delete "OBS Agent"
rmdir /s /q "C:\Program Files\OBS Agent" 2>nul
echo Uninstalled.
pause
BAT

cat > "$WORKDIR/config.json" <<'JSON'
{
    "server_url": "",
    "token": "",
    "log_level": "info",
    "cache_size_mb": 512,
    "max_concurrent_jobs": 1
}
JSON

cd "$WORKDIR" && zip -r "../$OUTPUT" . > /dev/null
cd - > /dev/null
rm -rf "$WORKDIR"

echo "  → Windows x86 portable: $OUTPUT"
