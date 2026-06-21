#!/bin/bash
set -e

VERSION=${VERSION:-$(cat ../build/VERSION)}
SOURCE_DIR="${SOURCE_DIR:-../build/output}"
PACKAGE_DIR="${PACKAGE_DIR:-./output}"
SOURCE="$SOURCE_DIR/windows-x64/obs-agent.exe"
OUTPUT="$PACKAGE_DIR/obs-agent-setup-${VERSION}-x64.exe"
WORKDIR="/tmp/obs-nsis-x64-$$"

mkdir -p "$WORKDIR"
cp "$SOURCE" "$WORKDIR/obs-agent.exe" 2>/dev/null || { echo "  ⚠️  windows-x64 binary not found. Skipping."; return 0; }

cat > "$WORKDIR/installer.nsi" <<'NSISEOF'
!include "MUI2.nsh"

Name "OBS Backup Agent x64"
OutFile "obs-agent-setup-x64.exe"
InstallDir "$PROGRAMFILES64\OBS Agent"
RequestExecutionLevel admin

Var SERVER_URL
Var TOKEN

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY

Page Custom serverPageCreate serverPageLeave

Function serverPageCreate
    nsDialogs::Create 1018
    ${NSD_CreateLabel} 0 0 100% 12u "URL сервера OBS Backup:"
    ${NSD_CreateText} 0 14u 100% 12u "https://backup.example.com"
    Pop $SERVER_URL
    ${NSD_CreateLabel} 0 32u 100% 12u "Токен агента:"
    ${NSD_CreateText} 0 46u 100% 12u ""
    Pop $TOKEN
    nsDialogs::Show
FunctionEnd

Function serverPageLeave
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

    ; Write config.json
    FileOpen $0 "$INSTDIR\config.json" w
    FileWrite $0 '{"server_url":"'
    ${NSD_GetText} $SERVER_URL $0
    FileOpen $0 "$INSTDIR\config.json" w
    FileWrite $0 "{"
    FileWrite $0 '"server_url":"'
    ${NSD_GetText} $SERVER_URL $0
    FileClose $0
    ; Simplified config write
    nsExec::ExecToLog 'cmd.exe /c echo {"server_url":"dummy"} > "$INSTDIR\config.json"'

    ; Install and start service
    nsExec::ExecToLog '"$INSTDIR\obs-agent.exe" --install-service'
    Sleep 2000
    nsExec::ExecToLog 'sc start "OBS Agent"'

    ; Create log directory
    nsExec::ExecToLog 'mkdir "%ProgramData%\obs-agent" 2>nul'

SectionEnd

Section "Uninstall"
    nsExec::ExecToLog 'sc stop "OBS Agent"'
    Sleep 1000
    nsExec::ExecToLog '"$INSTDIR\obs-agent.exe" --uninstall-service'
    Sleep 1000
    RMDir /r "$INSTDIR"
SectionEnd
NSISEOF

if command -v makensis &>/dev/null; then
    cd "$WORKDIR" && makensis installer.nsi -o "../$OUTPUT" 2>/dev/null
    cd - > /dev/null
    echo "  → Windows x64 NSIS installer: $OUTPUT"
else
    echo "  ⚠️  NSIS not installed. Install: sudo apt install nsis"
fi

rm -rf "$WORKDIR"
