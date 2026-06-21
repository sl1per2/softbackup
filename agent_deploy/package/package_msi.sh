#!/bin/bash
set -e

PLATFORM=$1; ARCH=$2; VERSION=$3
SOURCE_DIR="${SOURCE_DIR:-../build/output}"
PACKAGE_DIR="${PACKAGE_DIR:-./output}"
SOURCE="$SOURCE_DIR/$PLATFORM/obs-agent"
OUTPUT="$PACKAGE_DIR/obs-agent-${VERSION}-${ARCH}.msi"
WORKDIR="/tmp/obs-agent-msi-$$"

mkdir -p "$WORKDIR"
cp "$SOURCE" "$WORKDIR/obs-agent.exe" 2>/dev/null || {
    echo "  ⚠️  Binary not found. Skipping MSI."
    return 0
}

cat > "$WORKDIR/Product.wxs" <<WIXXML
<?xml version="1.0" encoding="UTF-8"?>
<Wix xmlns="http://schemas.microsoft.com/wix/2006/wi">
  <Product Id="*" Name="OBS Backup Agent" Language="1049" Version="${VERSION}"
           Manufacturer="OBS Backup" UpgradeCode="12345678-1234-1234-1234-123456789abc">
    <Package InstallerVersion="200" Compressed="yes" InstallScope="perMachine"/>
    <MajorUpgrade DowngradeErrorMessage="A newer version is already installed."/>
    <MediaTemplate EmbedCab="yes"/>
    <Feature Id="Complete" Title="OBS Backup Agent" Level="1">
      <ComponentRef Id="AgentExe"/>
    </Feature>
    <Directory Id="TARGETDIR" Name="SourceDir">
      <Directory Id="ProgramFiles64Folder">
        <Directory Id="INSTALLDIR" Name="OBS Agent">
          <Component Id="AgentExe" Guid="*">
            <File Id="obs-agent.exe" Source="obs-agent.exe" KeyPath="yes"/>
            <ServiceInstall Id="Svc" Name="OBS Agent" DisplayName="OBS Backup Agent"
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
    echo "  → MSI package: $OUTPUT"
else
    echo "  ⚠️  WiX (wixl) not installed. Install: sudo apt install wixl"
fi

rm -rf "$WORKDIR"
