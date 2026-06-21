#!/bin/bash
set -e

VERSION=${VERSION:-$(cat ../build/VERSION)}
SOURCE_DIR="${SOURCE_DIR:-../build/output}"
PACKAGE_DIR="${PACKAGE_DIR:-./output}"
SOURCE="$SOURCE_DIR/windows-x86/obs-agent.exe"
OUTPUT="$PACKAGE_DIR/obs-agent-${VERSION}-x86.msi"
WORKDIR="/tmp/obs-msi-x86-$$"

mkdir -p "$WORKDIR"
cp "$SOURCE" "$WORKDIR/obs-agent.exe" 2>/dev/null || { echo "  ⚠️  Binary not found. Skipping."; return 0; }

cat > "$WORKDIR/Product.wxs" <<WIXXML
<?xml version="1.0" encoding="UTF-8"?>
<Wix xmlns="http://schemas.microsoft.com/wix/2006/wi">
  <Product Id="*" Name="OBS Backup Agent x86" Language="1049" Version="${VERSION}"
           Manufacturer="OBS Backup" UpgradeCode="B2C3D4E5-F6A7-8901-BCDE-F12345678901">
    <Package InstallerVersion="400" Compressed="yes" InstallScope="perMachine" Platform="x86"/>
    <MajorUpgrade DowngradeErrorMessage="A newer version of OBS Backup Agent x86 is already installed."/>
    <MediaTemplate EmbedCab="yes"/>
    <Feature Id="Complete" Title="OBS Backup Agent x86" Level="1">
      <ComponentRef Id="AgentExe"/>
    </Feature>
    <Directory Id="TARGETDIR" Name="SourceDir">
      <Directory Id="ProgramFilesFolder">
        <Directory Id="INSTALLDIR" Name="OBS Agent">
          <Component Id="AgentExe" Guid="*">
            <File Id="obs-agent.exe" Source="obs-agent.exe" KeyPath="yes"/>
            <ServiceInstall Id="Svc" Name="OBS Agent" DisplayName="OBS Backup Agent (x86)"
                          Description="OBS Backup Agent for 32-bit Windows"
                          Start="auto" Type="ownProcess" ErrorControl="normal"/>
            <ServiceControl Id="SvcCtrl" Name="OBS Agent" Start="install" Stop="both" Remove="uninstall"/>
          </Component>
        </Directory>
      </Directory>
    </Directory>
  </Product>
</WixXML

if command -v wixl &>/dev/null; then
    wixl "$WORKDIR/Product.wxs" -o "$OUTPUT"
    echo "  → Windows x86 MSI: $OUTPUT"
else
    echo "  ⚠️  WiX (wixl) not installed. Install: sudo apt install wixl"
fi
rm -rf "$WORKDIR"
