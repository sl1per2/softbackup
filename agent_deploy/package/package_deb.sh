#!/bin/bash
set -e

PLATFORM=$1
ARCH=$2
VERSION=$3
SOURCE_DIR="${SOURCE_DIR:-../build/output}"
PACKAGE_DIR="${PACKAGE_DIR:-./output}"

SOURCE="$SOURCE_DIR/$PLATFORM/obs-agent"
OUTPUT="$PACKAGE_DIR/obs-agent_${VERSION}_${ARCH}.deb"
WORKDIR="/tmp/obs-agent-deb-$$"

mkdir -p "$WORKDIR/DEBIAN"
mkdir -p "$WORKDIR/usr/bin"
mkdir -p "$WORKDIR/etc/obs-agent"
mkdir -p "$WORKDIR/usr/lib/systemd/system"
mkdir -p "$WORKDIR/usr/share/man/man1"
mkdir -p "$WORKDIR/usr/share/man/man5"
mkdir -p "$WORKDIR/usr/share/bash-completion/completions"

cp "$SOURCE" "$WORKDIR/usr/bin/obs-agent"
chmod 755 "$WORKDIR/usr/bin/obs-agent"

cat > "$WORKDIR/etc/obs-agent/config.json" <<'EOF'
{
    "server_url": "",
    "token": "",
    "log_level": "info",
    "heartbeat_interval_seconds": 30,
    "cache_size_mb": 1024,
    "cache_path": "/var/cache/obs-agent",
    "max_concurrent_jobs": 2,
    "bandwidth_limit_kbps": 0
}
EOF

cp ../package/systemd/obs-agent.service "$WORKDIR/usr/lib/systemd/system/" 2>/dev/null || true
cp ../docs/obs-agent.1 "$WORKDIR/usr/share/man/man1/" 2>/dev/null || true
cp ../docs/obs-agent-config.5 "$WORKDIR/usr/share/man/man5/" 2>/dev/null || true
cp ../completions/obs-agent.bash "$WORKDIR/usr/share/bash-completion/completions/" 2>/dev/null || true

cat > "$WORKDIR/DEBIAN/control" <<EOF
Package: obs-agent
Version: ${VERSION}
Section: utils
Priority: optional
Architecture: ${ARCH}
Depends: systemd, libcurl4, libssl3
Maintainer: OBS Backup <support@obs-backup.example.com>
Description: OBS Backup Agent
 Кроссплатформенный агент для системы резервного копирования OBS Backup.
EOF

cat > "$WORKDIR/DEBIAN/postinst" <<'SCRIPT'
#!/bin/bash
set -e
mkdir -p /var/log/obs-agent /var/cache/obs-agent
chmod 750 /var/log/obs-agent /var/cache/obs-agent
systemctl daemon-reload
systemctl enable obs-agent 2>/dev/null || true
systemctl start obs-agent 2>/dev/null || true
SCRIPT
chmod 755 "$WORKDIR/DEBIAN/postinst"

cat > "$WORKDIR/DEBIAN/prerm" <<'SCRIPT'
#!/bin/bash
systemctl stop obs-agent 2>/dev/null || true
systemctl disable obs-agent 2>/dev/null || true
SCRIPT
chmod 755 "$WORKDIR/DEBIAN/prerm"

dpkg-deb --build "$WORKDIR" "$OUTPUT"
rm -rf "$WORKDIR"

echo "  → DEB package: $OUTPUT"
