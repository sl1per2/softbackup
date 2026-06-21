#!/bin/bash
set -e

VERSION=${VERSION:-$(cat ../build/VERSION)}
SOURCE_DIR="${SOURCE_DIR:-../build/output}"
PACKAGE_DIR="${PACKAGE_DIR:-./output}"
SOURCE="$SOURCE_DIR/windows-x64/obs-agent.exe"
OUTPUT="$PACKAGE_DIR/obs-agent-${VERSION}-x64.zip"
WORKDIR="/tmp/obs-zip-x64-$$"

mkdir -p "$WORKDIR"
cp "$SOURCE" "$WORKDIR/obs-agent.exe" 2>/dev/null || { echo "  ⚠️  Binary not found. Skipping."; return 0; }

cat > "$WORKDIR/install.bat" <<'BAT'
@echo off
echo ======================================
echo  OBS Backup Agent Installer (x64)
echo ======================================
echo.
echo Installing agent...
mkdir "C:\Program Files\OBS Agent" 2>nul
copy "%~dp0obs-agent.exe" "C:\Program Files\OBS Agent\"
echo.
echo Creating Windows service...
sc create "OBS Agent" binPath= "\"C:\Program Files\OBS Agent\obs-agent.exe\" --service" start= auto
sc description "OBS Agent" "OBS Backup Agent - Continuous data protection"
echo.
echo Starting service...
sc start "OBS Agent"
echo.
echo ======================================
echo  OBS Backup Agent installed!
echo  Configure: edit "C:\Program Files\OBS Agent\config.json"
echo  Status: sc query "OBS Agent"
echo ======================================
pause
BAT

cat > "$WORKDIR/uninstall.bat" <<'BAT'
@echo off
echo Stopping OBS Agent...
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
    "heartbeat_interval_seconds": 30,
    "cache_size_mb": 1024,
    "max_concurrent_jobs": 2,
    "bandwidth_limit_kbps": 0,
    "bandwidth_schedule": []
}
JSON

cat > "$WORKDIR/README.txt" <<'TXT'
OBS Backup Agent - Portable (x64)
==================================

Installation:
  1. Right-click install.bat -> Run as Administrator
  2. Edit config.json with server_url and token
  3. Service will start automatically

Uninstallation:
  1. Right-click uninstall.bat -> Run as Administrator

Manual start:
  obs-agent.exe --service
  obs-agent.exe --status
  obs-agent.exe --diag
TXT

cd "$WORKDIR" && zip -r "../$OUTPUT" . -x "*.DS_Store" > /dev/null
cd - > /dev/null
rm -rf "$WORKDIR"

echo "  → Windows x64 portable: $OUTPUT"
