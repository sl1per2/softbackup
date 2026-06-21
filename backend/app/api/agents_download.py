import os
from fastapi import APIRouter, Depends, HTTPException, Query
from fastapi.responses import FileResponse, HTMLResponse
from app.core.security import get_current_user
from typing import Optional

router = APIRouter(prefix="/api/agents-download", tags=["agents-download"])
download_page_router = APIRouter(tags=["agents-download"])

AGENT_DIR = "/app/agents"

KNOWN_AGENTS = [
    {"filename": "obs-agent-linux-x64", "os": "Linux", "arch": "x86_64", "version": "1.0.0", "description": "Linux x64 — OBS Backup Core Agent"},
    {"filename": "obs-agent-linux-arm64", "os": "Linux", "arch": "ARM64", "version": "1.0.0", "description": "Linux ARM64 — OBS Backup Core Agent"},
    {"filename": "obs-agent-windows-x64.exe", "os": "Windows", "arch": "x86_64", "version": "1.0.0", "description": "Windows x64 — OBS Backup Core Agent"},
    {"filename": "obs-agent-windows-x86.exe", "os": "Windows", "arch": "x86", "version": "1.0.0", "description": "Windows x86 — OBS Backup Core Agent"},
    {"filename": "obs-agent-macos-x64", "os": "macOS", "arch": "Intel", "version": "1.0.0", "description": "macOS Intel — OBS Backup Core Agent"},
    {"filename": "obs-agent-macos-arm64", "os": "macOS", "arch": "Apple Silicon", "version": "1.0.0", "description": "macOS Apple Silicon — OBS Backup Core Agent"},
]


def get_agents_list():
    agents = []
    for agent in KNOWN_AGENTS:
        filepath = os.path.join(AGENT_DIR, agent["filename"])
        size = os.path.getsize(filepath) if os.path.exists(filepath) else 0
        agents.append({
            **agent,
            "size": size,
            "available": size > 0,
        })
    return agents


def _human_size(n: int) -> str:
    for u in ("B", "KB", "MB", "GB"):
        if n < 1024:
            return f"{n:.1f} {u}"
        n /= 1024
    return f"{n:.1f} TB"


@download_page_router.get("/agents-download")
async def download_page():
    agents = get_agents_list()
    os_icons = {"Linux": "&#127463;&#127475;", "Windows": "&#127468;&#127463;", "macOS": "&#127463;&#127475;"}
    os_colors = {"Linux": "#00E5FF", "Windows": "#A855F7", "macOS": "#8B949E"}

    rows = ""
    for a in agents:
        size_str = _human_size(a["size"]) if a["available"] else '<span style="color:#666">Not built</span>'
        btn = (
            f'<a href="/api/agents-download/download/{a["filename"]}" '
            f'style="display:inline-block;padding:6px 16px;background:#00E5FF;color:#000;'
            f'text-decoration:none;border-radius:6px;font-weight:600;font-size:13px;'
            f'{"opacity:0.4;pointer-events:none" if not a["available"] else ""}">'
            f"Download</a>"
            if a["available"]
            else '<span style="color:#666;font-size:12px">Not built</span>'
        )
        badge_color = os_colors.get(a["os"], "#888")
        rows += f"""
        <tr>
            <td style="padding:12px 16px;border-bottom:1px solid #222">
                <div style="font-weight:600;color:#e6e6e6">{a["description"]}</div>
                <div style="font-size:11px;color:#888">{a["filename"]}</div>
            </td>
            <td style="padding:12px 16px;border-bottom:1px solid #222">
                <span style="background:{badge_color}22;color:{badge_color};padding:2px 10px;border-radius:4px;font-size:12px">{a["os"]}</span>
            </td>
            <td style="padding:12px 16px;border-bottom:1px solid #222;color:#aaa;font-size:13px">{a["arch"]}</td>
            <td style="padding:12px 16px;border-bottom:1px solid #222;color:#aaa;font-size:13px">v{a["version"]}</td>
            <td style="padding:12px 16px;border-bottom:1px solid #222;color:#aaa;font-size:13px">{size_str}</td>
            <td style="padding:12px 16px;border-bottom:1px solid #222;text-align:right">{btn}</td>
        </tr>"""

    install_linux = 'curl -fsSL http://SERVER/api/agents-download/download/obs-agent-linux-x64 -o /usr/local/bin/obs-agent &amp;&amp; chmod +x /usr/local/bin/obs-agent'
    install_windows = 'Invoke-WebRequest -Uri "http://SERVER/api/agents-download/download/obs-agent-windows-x64.exe" -OutFile "$env:TEMP\\obs-agent.exe"'

    html = f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>OBS Backup — Agent Downloads</title>
<style>
* {{ margin:0; padding:0; box-sizing:border-box; }}
body {{ background:#0a0a0a; color:#e6e6e6; font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif; padding:32px; }}
.container {{ max-width:960px; margin:0 auto; }}
h1 {{ font-size:24px; margin-bottom:8px; }}
h1 span {{ color:#00E5FF; }}
.subtitle {{ color:#888; font-size:14px; margin-bottom:24px; }}
table {{ width:100%; border-collapse:collapse; background:#111; border-radius:12px; overflow:hidden; }}
th {{ padding:12px 16px; text-align:left; font-size:12px; color:#666; text-transform:uppercase; letter-spacing:0.5px; border-bottom:1px solid #222; }}
.section {{ margin-top:32px; background:#111; border-radius:12px; padding:20px; }}
.section h2 {{ font-size:16px; margin-bottom:12px; color:#00E5FF; }}
.code {{ background:#000; padding:12px 16px; border-radius:8px; font-family:monospace; font-size:12px; color:#aaa; overflow-x:auto; white-space:nowrap; }}
.badge {{ display:inline-block;padding:2px 8px;border-radius:4px;font-size:11px;margin-right:4px; }}
</style>
</head>
<body>
<div class="container">
<h1><span>OBS Backup</span> — Agent Downloads</h1>
<p class="subtitle">Download the agent for your platform. Install on target servers to enable backup protection.</p>

<table>
<thead>
<tr><th>Agent</th><th>OS</th><th>Arch</th><th>Version</th><th>Size</th><th style="text-align:right">Download</th></tr>
</thead>
<tbody>
{rows}
</tbody>
</table>

<div class="section">
<h2>&#128187; Quick Install — Linux</h2>
<div class="code">{install_linux}</div>
</div>

<div class="section">
<h2>&#128187; Quick Install — Windows (PowerShell)</h2>
<div class="code">{install_windows}</div>
</div>
</div>
</body>
</html>"""
    return HTMLResponse(content=html)


@router.get("/list")
async def list_agents():
    return {"agents": get_agents_list()}


@router.get("/by-os")
async def list_agents_by_os(os_type: str = Query(..., description="Linux, Windows, or macOS")):
    all_agents = get_agents_list()
    filtered = [a for a in all_agents if a["os"].lower() == os_type.lower()]
    return {"os": os_type, "agents": filtered}


@router.get("/download/{filename}")
async def download_agent(filename: str):
    if "/" in filename or "\\" in filename or ".." in filename:
        raise HTTPException(status_code=400, detail="Invalid filename")

    filepath = os.path.join(AGENT_DIR, filename)
    if not os.path.exists(filepath):
        raise HTTPException(status_code=404, detail=f"Agent binary not found: {filename}")

    return FileResponse(
        path=filepath,
        filename=filename,
        media_type="application/octet-stream",
    )


@router.get("/install-script/{os_type}")
async def get_install_script(os_type: str):
    scripts = {
        "linux": """#!/bin/bash
set -e
# OBS Backup Agent Installer for Linux

RED='\\033[0;31m'; GREEN='\\033[0;32m'; CYAN='\\033[0;36m'; NC='\\033[0m'
echo -e "${CYAN}=== OBS Backup Agent Installer ===${NC}"

if [ "$EUID" -ne 0 ]; then echo -e "${RED}Run as root${NC}"; exit 1; fi

SERVER="${OBS_SERVER_URL:-http://localhost:8000}"
AGENT_URL="$SERVER/api/agents-download/download/obs-agent-linux-x64"
INSTALL_DIR="/opt/obs-agent"

echo -e "${CYAN}Downloading agent from $SERVER...${NC}"
mkdir -p "$INSTALL_DIR"
curl -sSL "$AGENT_URL" -o "$INSTALL_DIR/obs-agent"
chmod +x "$INSTALL_DIR/obs-agent"

cat > /etc/systemd/system/obs-agent.service << EOF
[Unit]
Description=OBS Backup Agent
After=network.target
[Service]
Type=simple
ExecStart=$INSTALL_DIR/obs-agent --socket-path /tmp/obs-core.sock --log-level info
Restart=always
RestartSec=5
[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable --now obs-agent
echo -e "${GREEN}=== Agent installed and started ===${NC}"
echo "Status: systemctl status obs-agent"
""",
        "windows": """@echo off
REM OBS Backup Agent Installer for Windows
echo === OBS Backup Agent Installer ===
echo.
echo Downloading obs-agent-windows-x64.exe...
echo.
echo 1. Download obs-agent-windows-x64.exe from the web interface
echo 2. Place it in C:\\Program Files\\OBS Agent\\
echo 3. Run as Administrator:
echo    obs-agent-windows-x64.exe --install-service
echo    sc start "OBS Agent"
echo.
echo Configure: edit C:\\Program Files\\OBS Agent\\config.json
""",
        "macos": """#!/bin/bash
set -e
echo "=== OBS Backup Agent Installer (macOS) ==="
SERVER="${OBS_SERVER_URL:-http://localhost:8000}"
curl -sSL "$SERVER/api/agents-download/download/obs-agent-macos-x64" -o /usr/local/bin/obs-agent
chmod +x /usr/local/bin/obs-agent
mkdir -p /Library/LaunchDaemons /var/log/obs-agent
echo "=== Agent installed. Start with: obs-agent --service ==="
""",
    }
    if os_type not in scripts:
        raise HTTPException(status_code=400, detail=f"Unsupported OS: {os_type}. Use linux, windows, or macos")

    from fastapi.responses import PlainTextResponse
    return PlainTextResponse(
        content=scripts[os_type],
        media_type="text/plain",
        headers={"Content-Disposition": f"attachment; filename=install-obs-agent-{os_type}.sh"},
    )


@router.get("/install-script/{os_type}")
async def get_install_script(os_type: str):
    scripts = {
        "linux": """#!/bin/bash
set -e
# OBS Backup Agent Installer for Linux

RED='\\033[0;31m'; GREEN='\\033[0;32m'; CYAN='\\033[0;36m'; NC='\\033[0m'
echo -e "${CYAN}=== OBS Backup Agent Installer ===${NC}"

if [ "$EUID" -ne 0 ]; then echo -e "${RED}Run as root${NC}"; exit 1; fi

SERVER="${OBS_SERVER_URL:-http://localhost:8000}"
AGENT_URL="$SERVER/api/agents-download/download/obs-agent-linux-x64"
INSTALL_DIR="/opt/obs-agent"

echo -e "${CYAN}Downloading agent from $SERVER...${NC}"
mkdir -p "$INSTALL_DIR"
curl -sSL "$AGENT_URL" -o "$INSTALL_DIR/obs-agent"
chmod +x "$INSTALL_DIR/obs-agent"

cat > /etc/systemd/system/obs-agent.service << EOF
[Unit]
Description=OBS Backup Agent
After=network.target
[Service]
Type=simple
ExecStart=$INSTALL_DIR/obs-agent --socket-path /tmp/obs-core.sock --log-level info
Restart=always
RestartSec=5
[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable --now obs-agent
echo -e "${GREEN}=== Agent installed and started ===${NC}"
echo "Status: systemctl status obs-agent"
""",
        "windows": """@echo off
REM OBS Backup Agent Installer for Windows
echo === OBS Backup Agent Installer ===
echo.
echo Downloading obs-agent-windows-x64.exe...
echo.
echo 1. Download obs-agent-windows-x64.exe from the web interface
echo 2. Place it in C:\\Program Files\\OBS Agent\\
echo 3. Run as Administrator:
echo    obs-agent-windows-x64.exe --install-service
echo    sc start "OBS Agent"
echo.
echo Configure: edit C:\\Program Files\\OBS Agent\\config.json
""",
        "macos": """#!/bin/bash
set -e
echo "=== OBS Backup Agent Installer (macOS) ==="
SERVER="${OBS_SERVER_URL:-http://localhost:8000}"
curl -sSL "$SERVER/api/agents-download/download/obs-agent-macos-x64" -o /usr/local/bin/obs-agent
chmod +x /usr/local/bin/obs-agent
mkdir -p /Library/LaunchDaemons /var/log/obs-agent
echo "=== Agent installed. Start with: obs-agent --service ==="
""",
    }
    if os_type not in scripts:
        raise HTTPException(status_code=400, detail=f"Unsupported OS: {os_type}. Use linux, windows, or macos")

    from fastapi.responses import PlainTextResponse
    return PlainTextResponse(
        content=scripts[os_type],
        media_type="text/plain",
        headers={"Content-Disposition": f"attachment; filename=install-obs-agent-{os_type}.sh"},
    )
