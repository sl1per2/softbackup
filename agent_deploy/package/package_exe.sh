#!/bin/bash
set -e

PLATFORM=$1; ARCH=$2; VERSION=$3
SOURCE_DIR="${SOURCE_DIR:-../build/output}"
PACKAGE_DIR="${PACKAGE_DIR:-./output}"
SOURCE="$SOURCE_DIR/$PLATFORM/obs-agent.exe"
OUTPUT="$PACKAGE_DIR/obs-agent-setup-${VERSION}-${ARCH}.exe"
WORKDIR="/tmp/obs-agent-nsis-$$"

mkdir -p "$WORKDIR"
cp "$SOURCE" "$WORKDIR/obs-agent.exe" 2>/dev/null || { echo "  ⚠️  Binary not found. Skipping."; return 0; }

cat > "$WORKDIR/installer.nsi" <<'NSISEOF'
!include "MUI2.nsh"
Name "OBS Backup Agent"
OutFile "obs-agent-setup.exe"
InstallDir "$PROGRAMFILES64\OBS Agent"
RequestExecutionLevel admin

Var SERVER_URL
Var TOKEN

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY

Page Custom serverPage serverLeave

Function serverPage
    nsDialogs::Create 1018
    ${NSD_CreateLabel} 0 0 100% 12u "Server URL:"
    ${NSD_CreateText} 0 14u 100% 12u "https://backup.example.com"
    Pop $SERVER_URL
    ${NSD_CreateLabel} 0 30u 100% 12u "Agent Token:"
    ${NSD_CreateText} 0 44u 100% 12u ""
    Pop $TOKEN
    nsDialogs::Show
FunctionEnd

Function serverLeave
    ${NSD_GetText} $SERVER_URL $0
    ${NSD_GetText} $TOKEN $1
FunctionEnd

!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "English"

Section "Install"
    SetOutPath "$INSTDIR"
    File "obs-agent.exe"
    ExecShell "" "$\"$INSTDIR\obs-agent.exe$\"" "--install-service"
    Sleep 2000
    ExecShell "" "sc" 'start "OBS Agent"'
SectionEnd

Section "Uninstall"
    ExecShell "" "sc" 'stop "OBS Agent"'
    ExecShell "" "sc" 'delete "OBS Agent"'
    RMDir /r "$INSTDIR"
SectionEnd
NSISEOF

if command -v makensis &>/dev/null; then
    cd "$WORKDIR" && makensis installer.nsi -o "../$OUTPUT" 2>/dev/null
    echo "  → NSIS installer: $OUTPUT"
else
    echo "  ⚠️  NSIS not installed. Install: sudo apt install nsis"
fi

rm -rf "$WORKDIR"
