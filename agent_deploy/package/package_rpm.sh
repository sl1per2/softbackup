#!/bin/bash
set -e

PLATFORM=$1; ARCH=$2; VERSION=$3
SOURCE_DIR="${SOURCE_DIR:-../build/output}"
PACKAGE_DIR="${PACKAGE_DIR:-./output}"
SOURCE="$SOURCE_DIR/$PLATFORM/obs-agent"
OUTPUT="$PACKAGE_DIR/obs-agent-${VERSION}-1.${ARCH}.rpm"
SPEC="/tmp/obs-agent-${VERSION}.spec"

cat > "$SPEC" <<EOF
Name: obs-agent
Version: ${VERSION}
Release: 1
Summary: OBS Backup Agent
License: Proprietary
URL: https://obs-backup.example.com
BuildArch: ${ARCH}
Requires: systemd
%description
Кроссплатформенный агент для системы резервного копирования OBS Backup.
%install
mkdir -p %{buildroot}/usr/bin %{buildroot}/etc/obs-agent %{buildroot}/usr/lib/systemd/system
cp ${SOURCE} %{buildroot}/usr/bin/obs-agent
chmod 755 %{buildroot}/usr/bin/obs-agent
cp ../package/systemd/obs-agent.service %{buildroot}/usr/lib/systemd/system/ 2>/dev/null || true
%post
mkdir -p /var/log/obs-agent /var/cache/obs-agent
systemctl daemon-reload 2>/dev/null || true
%preun
systemctl stop obs-agent 2>/dev/null || true
%files
/usr/bin/obs-agent
/etc/obs-agent
/usr/lib/systemd/system/obs-agent.service
EOF

rpmbuild -bb "$SPEC" --define "_rpmdir /tmp/obs-rpms" --define "_sourcedir /tmp" 2>/dev/null || {
    echo "  ⚠️  rpmbuild not available. Skipping RPM."
    rm -f "$SPEC"
    return 0
}
rm -f "$SPEC"
echo "  → RPM package built"
