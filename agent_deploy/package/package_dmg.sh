#!/bin/bash
set -e

PLATFORM=$1; VERSION=$2
SOURCE_DIR="${SOURCE_DIR:-../build/output}"
PACKAGE_DIR="${PACKAGE_DIR:-./output}"
SOURCE="$SOURCE_DIR/$PLATFORM/obs-agent"
OUTPUT="$PACKAGE_DIR/obs-agent-${VERSION}-${PLATFORM}.dmg"
WORKDIR="/tmp/obs-agent-dmg-$$"

mkdir -p "$WORKDIR/OBS Agent"
cp "$SOURCE" "$WORKDIR/OBS Agent/obs-agent" 2>/dev/null || { echo "  ⚠️  Binary not found."; return 0; }
chmod 755 "$WORKDIR/OBS Agent/obs-agent"

cp ../package/macos/com.obs.agent.plist "$WORKDIR/OBS Agent/" 2>/dev/null || true

cat > "$WORKDIR/OBS Agent/install.sh" <<'SCRIPT'
#!/bin/bash
sudo mkdir -p /usr/local/bin /Library/LaunchDaemons /var/log/obs-agent
sudo cp obs-agent /usr/local/bin/
sudo chmod 755 /usr/local/bin/obs-agent
sudo cp com.obs.agent.plist /Library/LaunchDaemons/
sudo launchctl load /Library/LaunchDaemons/com.obs.agent.plist
echo "✅ OBS Backup Agent installed."
SCRIPT
chmod 755 "$WORKDIR/OBS Agent/install.sh"

if command -v create-dmg &>/dev/null; then
    create-dmg \
        --volname "OBS Agent ${VERSION}" \
        --window-pos 200 120 --window-size 600 400 \
        --icon-size 100 --icon "obs-agent" 200 200 \
        "$OUTPUT" "$WORKDIR/OBS Agent" 2>/dev/null
    echo "  → DMG: $OUTPUT"
else
    echo "  ⚠️  create-dmg not found. Creating zip instead..."
    cd "$WORKDIR" && zip -r "../$OUTPUT.zip" "OBS Agent"
    cd - > /dev/null
    echo "  → ZIP: $OUTPUT.zip"
fi

rm -rf "$WORKDIR"
