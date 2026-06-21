#!/bin/bash
# OBS Backup Agent — universal installer
# Usage: curl -fsSL https://<server>/install.sh | sudo bash
set -e

SERVER_URL=""
TOKEN=""
VERSION="1.0.0"

RED='\033[0;31m'; GREEN='\033[0;32m'; CYAN='\033[0;36m'; NC='\033[0m'

banner() {
    echo -e "${CYAN}"
    echo " ╔══════════════════════════════════════╗"
    echo " ║   OBS Backup Agent Installer          ║"
    echo " ║   Version ${VERSION}                     ║"
    echo " ╚══════════════════════════════════════╝"
    echo -e "${NC}"
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --server) SERVER_URL="$2"; shift 2 ;;
        --token) TOKEN="$2"; shift 2 ;;
        --version) VERSION="$2"; shift 2 ;;
        --help) echo "Usage: $0 --server URL --token TOKEN"; exit 0 ;;
        *) echo -e "${RED}Unknown option: $1${NC}"; exit 1 ;;
    esac
done

if [ -z "$SERVER_URL" ] || [ -z "$TOKEN" ]; then
    echo -e "${RED}Error: --server and --token are required${NC}"
    exit 1
fi

DL="${SERVER_URL}/api/agents-download/download"

# Detect OS and arch
OS="unknown"; ARCH="unknown"
case "$(uname -s)" in
    Linux) OS="linux" ;;
    Darwin) OS="macos" ;;
    *) echo -e "${RED}Unsupported OS${NC}"; exit 1 ;;
esac
case "$(uname -m)" in
    x86_64|amd64) ARCH="amd64" ;;
    aarch64|arm64) ARCH="arm64" ;;
    *) echo -e "${RED}Unsupported arch${NC}"; exit 1 ;;
esac
echo -e "${GREEN}Detected: ${OS} / ${ARCH}${NC}"

install_linux() {
    echo -e "${CYAN}Installing on Linux (${ARCH})...${NC}"
    if command -v apt-get &>/dev/null; then
        PKG="obs-agent_${VERSION}_${ARCH}.deb"
        curl -fsSL "${DL}/${PKG}" -o "/tmp/${PKG}"
        dpkg -i "/tmp/${PKG}" && rm "/tmp/${PKG}"
    elif command -v dnf &>/dev/null; then
        RPM_ARCH=$([ "$ARCH" = "arm64" ] && echo "aarch64" || echo "x86_64")
        PKG="obs-agent-${VERSION}-1.${RPM_ARCH}.rpm"
        curl -fsSL "${DL}/${PKG}" -o "/tmp/${PKG}"
        dnf install -y "/tmp/${PKG}" && rm "/tmp/${PKG}"
    else
        PKG="obs-agent-${VERSION}-linux-${ARCH}.tar.gz"
        curl -fsSL "${DL}/${PKG}" -o "/tmp/${PKG}"
        tar -xzf "/tmp/${PKG}" -C /opt
        cp /opt/obs-agent/systemd/obs-agent.service /etc/systemd/system/ 2>/dev/null || true
        systemctl daemon-reload && rm "/tmp/${PKG}"
    fi
}

install_macos() {
    echo -e "${CYAN}Installing on macOS...${NC}"
    PKG="obs-agent-${VERSION}.dmg"
    curl -fsSL "${DL}/${PKG}" -o "/tmp/${PKG}"
    hdiutil attach "/tmp/${PKG}" -nobrowse -quiet
    cp /Volumes/OBS\ Agent/obs-agent /usr/local/bin/
    hdiutil detach /Volumes/OBS\ Agent -quiet
    rm "/tmp/${PKG}"
}

configure() {
    DIR="/etc/obs-agent"; [ "$OS" = "macos" ] && DIR="/usr/local/etc/obs-agent"
    mkdir -p "$DIR"
    cat > "${DIR}/config.json" <<EOF
{"server_url":"${SERVER_URL}","token":"${TOKEN}","log_level":"info","cache_size_mb":1024}
EOF
    echo -e "${GREEN}Config: ${DIR}/config.json${NC}"
}

start() {
    if [ "$OS" = "linux" ]; then
        systemctl enable obs-agent 2>/dev/null || true
        systemctl start obs-agent 2>/dev/null || true
    elif [ "$OS" = "macos" ]; then
        launchctl load /Library/LaunchDaemons/com.obs.agent.plist 2>/dev/null || true
    fi
    echo -e "${GREEN}✅ Agent started.${NC}"
}

banner
if [ "$OS" = "linux" ]; then install_linux; else install_macos; fi
configure
start

echo ""
echo -e "${GREEN}╔══════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  ✅ OBS Backup Agent ${VERSION} installed!    ║${NC}"
echo -e "${GREEN}║  Server: ${SERVER_URL}${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════╝${NC}"
