# OBS Backup — Enterprise Backup System

Corporate-grade backup and disaster recovery platform with web interface, agent-based architecture, and cross-platform support.

## Features

- **Incremental & CDP backups** — Changed Block Tracking (CBT), USN Journal, ReFS CBT, VSS Bitmap
- **Deduplication & compression** — block-level dedup engine with zstd/lz4/blake3
- **Encryption** — AES-256 backup encryption via OpenSSL
- **Multi-platform agents** — Linux (x86_64/ARM64), Windows (x86/x64), macOS (Intel/Apple Silicon)
- **VM backup** — VMware vSphere, Hyper-V (WMI/RCT/VHDX), Proxmox, KVM, Astra VE, ALT-Virt, Rus-BIT
- **Database backup** — Oracle (RMAN), PostgreSQL, MySQL, MS SQL, MongoDB
- **OS-level backup** — Windows VSS, Linux LVM snapshots, filesystem-level
- **Replication** — async/sync replication to remote sites, DR failover
- **Tape support** — LTO tape library management, GFS retention policies
- **Immutability** — WORM storage, object lock, air-gapped backup copies
- **Malware scanning** — backup integrity verification, ransomware detection
- **Traffic optimization** — bandwidth throttling, dedup-aware transfer
- **Self-service portal** — end-user restore from web UI
- **Monitoring** — Zabbix integration, Prometheus metrics export, SIEM audit log
- **LDAP/AD** — centralized authentication

## Architecture

```
┌─────────────┐     ┌──────────────┐     ┌─────────────┐
│   Frontend   │────▶│    Backend    │────▶│  PostgreSQL  │
│  React/Vite  │     │   FastAPI     │     │   + Redis    │
└─────────────┘     └──────┬───────┘     └─────────────┘
                           │
                    ┌──────┴───────┐
                    │   vovqa-core  │
                    │  C++ Agent    │
                    └──────┬───────┘
                           │
              ┌────────────┼────────────┐
              │            │            │
         ┌────┴────┐ ┌────┴────┐ ┌────┴────┐
         │  Linux  │ │ Windows │ │  macOS  │
         │  Agent  │ │  Agent  │ │  Agent  │
         └─────────┘ └─────────┘ └─────────┘
```

## Tech Stack

| Layer | Technologies |
|-------|-------------|
| **Backend** | Python 3.11, FastAPI, SQLAlchemy, Celery, Redis, PostgreSQL |
| **Frontend** | React 18, TypeScript, Vite, Ant Design, Framer Motion |
| **Core Agent** | C++17, CMake, Boost, OpenSSL, Protobuf, SQLite, libcurl |
| **Cross-compile** | MinGW-w64 (Windows x86/x64 from Linux) |
| **Infra** | Docker, Docker Compose, Nginx |
| **Backup formats** | VSS, VHDX, LTO, ReFS CBT, USN Journal, LVM snapshots |

## Quick Start

```bash
# Clone
git clone https://github.com/sl1per2/softbackup.git
cd softbackup

# Start all services
docker-compose up -d --build

# Access
# Web UI:  http://localhost
# API:     http://localhost:8000/docs
# Default credentials: admin / admin123
```

## Agent Downloads

Download page: `http://<server>/agents-download`

| Platform | Arch | Binary |
|----------|------|--------|
| Linux | x86_64 | `obs-agent-linux-x64` (3 MB) |
| Windows | x86_64 | `obs-agent-windows-x64.exe` (14 MB) |

### Install Agent (Linux)
```bash
curl -fsSL http://<server>/api/agents-download/download/obs-agent-linux-x64 -o /usr/local/bin/obs-agent
chmod +x /usr/local/bin/obs-agent
```

### Install Agent (Windows)
```powershell
Invoke-WebRequest -Uri "http://<server>/api/agents-download/download/obs-agent-windows-x64.exe" -OutFile "$env:TEMP\obs-agent.exe"
```

## Building Core Agent

```bash
# Linux native build
cd core && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(nproc)

# Windows cross-compilation (requires MinGW-w64)
cmake .. -DCMAKE_TOOLCHAIN_FILE=../../cmake/mingw-w64-x86_64.cmake -DCMAKE_BUILD_TYPE=Release
```

## License

Proprietary — All rights reserved.
