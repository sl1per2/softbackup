#!/bin/bash
set -e

VERSION=${VERSION:-$(cat ../build/VERSION)}
SOURCE_DIR="${SOURCE_DIR:-../build/output}"
PACKAGE_DIR="${PACKAGE_DIR:-./output}"
SOURCE="$SOURCE_DIR/windows-x64/obs-agent.exe"
OUTPUT="$PACKAGE_DIR/obs-agent-${VERSION}-x64.msi"
WORKDIR="/tmp/obs-msi-x64-$$"

mkdir -p "$WORKDIR"
cp "$SOURCE" "$WORKDIR/obs-agent.exe" 2>/dev/null || { echo "  ⚠️  Binary not found. Skipping."; return 0; }

cat > "$WORKDIR/Product.wxs" <<WIXXML
<?xml version="1.0" encoding="UTF-8"?>
<Wix xmlns="http://schemas.microsoft.com/wix/2006/wi">
  <Product Id="*" Name="OBS Backup Agent x64" Language="1049" Version="${VERSION}"
           Manufacturer="OBS Backup" UpgradeCode="A1B2C3D4-E5F6-7890-ABCD-EF1234567890">
    <Package InstallerVersion="500" Compressed="yes" InstallScope="perMachine" Platform="x64"/>
    <MajorUpgrade DowngradeErrorMessage="A newer version of OBS Backup Agent x64 is already installed."/>
    <MediaTemplate EmbedCab="yes"/>
    <Feature Id="Complete" Title="OBS Backup Agent x64" Level="1">
      <ComponentRef Id="AgentExe"/>
    </Feature>
    <Directory Id="TARGETDIR" Name="SourceDir">
      <Directory Id="ProgramFiles64Folder">
        <Directory Id="INSTALLDIR" Name="OBS Agent">
          <Component Id="AgentExe" Guid="*">
            <File Id="obs-agent.exe" Source="obs-agent.exe" KeyPath="yes"/>
            <ServiceInstall Id="Svc" Name="OBS Agent" DisplayName="OBS Backup Agent (x64)"
                          Description="OBS Backup Agent for 64-bit Windows"
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
    echo "  → Windows x64 MSI: $OUTPUT"
else
    echo "  ⚠️  WiX (wixl) not installed. Install: sudo apt install wixl"
fi
rm -rf "$WORKDIR"
