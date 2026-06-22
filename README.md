# OBS Backup — Enterprise Backup System

Corporate-grade backup and disaster recovery platform with web interface, agent-based architecture, and cross-platform support.

## Features

### Core Engine (C++17 OOP)
- **OOP Architecture** — Abstract Factory, Strategy, Observer, Command patterns; DI container; RAII
- **13 specialized agents** — Generic, ESXi, Hyper-V, MSSQL, PostgreSQL, Oracle, Proxmox, SAP HANA, Kubernetes, Linux FS, Windows FS, NDMP, Exchange
- **Incremental & CDP backups** — Changed Block Tracking (CBT), USN Journal, ReFS CBT, VSS Bitmap, QEMU dirty bitmaps
- **Deduplication & compression** — FastCDC + BLAKE2b-512, zstd/lz4/deflate
- **Encryption** — AES-256-GCM + RSA-4096 key wrap via OpenSSL
- **Multi-platform agents** — Linux (x86_64/ARM64), Windows (x86/x64), macOS (Intel/Apple Silicon)
- **Dirty Buffer Logger** — pre-backup dirty buffer capture, flush, consistency verification, audit trail

### Remote Agent Management
- **Full remote control** via web UI — start/stop/restart/update agents without SSH
- **Remote deployment** — install agents on new hosts via SSH (Linux) or WinRM (Windows)
- **Live log viewer** — terminal-style real-time log streaming with level filter and search
- **Remote configuration** — edit agent config.json directly from web interface
- **Quick Backup** — one-click backup launch with type/source selection

### Virtualization & Database Backup
- **VM backup** — VMware vSphere (CBT/HotAdd/NBD), Hyper-V (RCT/VSS), Proxmox (QEMU bitmap), KVM
- **Database backup** — PostgreSQL (WAL/PATRONI), MySQL (xtrabackup/binlog), MSSQL (VSS/TDE), Oracle (RMAN/BCT), MongoDB, SAP HANA
- **OS-level backup** — Windows VSS, Linux Btrfs/ZFS/LVM/dm-era send

### Enterprise Features
- **Replication** — async/sync to remote sites, DR failover/failback
- **Tape support** — LTO tape library, GFS retention policies
- **Immutability** — WORM storage, S3 Object Lock, air-gapped copies
- **Malware scanning** — ransomware detection, ClamAV integration
- **SureBackup** — automated backup verification in virtual labs
- **Traffic optimization** — bandwidth throttling, dedup-aware transfer, multi-stream
- **Scale-Out Repository** — pool multiple storage targets
- **Self-service portal** — end-user restore from web UI
- **Monitoring** — Zabbix, Prometheus, SIEM audit log
- **LDAP/AD** — centralized authentication

## Architecture

```
┌─────────────┐     ┌──────────────────┐     ┌─────────────┐
│  Frontend    │────▶│  Backend         │────▶│  PostgreSQL  │
│  React/Vite  │     │  FastAPI         │     │  + Redis     │
│  Ant Design  │     │  REST + WebSocket│     │              │
└─────────────┘     └──────┬───────────┘     └─────────────┘
                           │
                    ┌──────┴───────────┐
                    │  vovqa-core       │
                    │  C++17 OOP Agent  │
                    │  AgentFactory     │
                    │  BackupPipeline   │
                    └──────┬───────────┘
                           │
              ┌────────────┼────────────┐
              │            │            │
         ┌────┴────┐ ┌────┴────┐ ┌────┴────┐
         │  ESXi   │ │ Hyper-V │ │  K8s    │
         │  Agent  │ │  Agent  │ │  Agent  │
         └─────────┘ └─────────┘ └─────────┘
```

### C++ OOP Framework

```
IComponent
├── IAgent → GenericAgent → 12 specialized agents
├── IBackupEngine → DefaultBackupEngine
├── IDedupEngine → DefaultDedupEngine (FastCDC + SQLite)
├── ICryptoEngine → DefaultCryptoEngine (OpenSSL)
├── ICompressionEngine → ZstdCompressionEngine
├── ITransportEngine → DefaultTransportEngine
├── ICdpEngine → CdpEngine
└── IIpcServer → UnixSocketServer / NamedPipeServer

ServiceRegistry (DI Container)
EventBus (Observer Pattern)
CommandQueue (Command Pattern)
AgentFactory (Abstract Factory)
EngineFactory (Factory)
BackupPipeline (Composition)
```

## Tech Stack

| Layer | Technologies |
|-------|-------------|
| **Backend** | Python 3.11, FastAPI, SQLAlchemy, Celery, Redis, PostgreSQL |
| **Frontend** | React 18, TypeScript, Vite, Ant Design, Framer Motion, ECharts |
| **Core Agent** | C++17, CMake, Boost, OpenSSL, Protobuf, SQLite, libcurl |
| **Cross-compile** | MinGW-w64 (Windows x86/x64 from Linux), aarch64-linux-gnu (ARM64) |
| **Infra** | Docker, Docker Compose, Nginx |
| **IPC** | Boost.Asio Unix sockets (Linux) / Named Pipes (Windows) |

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

## Building Core Agent

```bash
# Dependencies
cd core/cmake && bash build-deps-local.sh

# Linux native build
cd core && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(nproc)

# Windows cross-compilation (requires MinGW-w64)
cmake .. -DCMAKE_TOOLCHAIN_FILE=../../cmake/mingw-w64-x86_64.cmake -DCMAKE_BUILD_TYPE=Release

# ARM64 cross-compilation
cmake .. -DCMAKE_TOOLCHAIN_FILE=../../cmake/aarch64-linux-gnu.cmake -DCMAKE_BUILD_TYPE=Release
```

## Running Tests

```bash
cd core/build
ctest --output-on-failure
# Tests: test_oop (OOP framework), test_dirty_buffer (DirtyBufferLogger)
```

## Agent Downloads

Download page: `http://<server>/agents-download`

| Platform | Arch | Binary |
|----------|------|--------|
| Linux | x86_64 | `obs-agent-linux-x64` |
| Linux | ARM64 | `obs-agent-linux-arm64` |
| Windows | x86_64 | `obs-agent-windows-x64.exe` |
| Windows | x86 | `obs-agent-windows-x86.exe` |
| macOS | Intel | `obs-agent-macos-x64` |
| macOS | Apple Silicon | `obs-agent-macos-arm64` |

### Remote Deploy (from Web UI)

1. Go to **Agents** → **Deploy New Agent**
2. Enter host IP, credentials, OS type
3. Click **Deploy** — agent is installed via SSH/WinRM automatically

### Manual Install

```bash
# Linux
curl -fsSL http://<server>/api/agents-download/download/obs-agent-linux-x64 -o /usr/local/bin/obs-agent
chmod +x /usr/local/bin/obs-agent
obs-agent start --server http://<server>:8000 --token <token>

# Windows (PowerShell)
Invoke-WebRequest -Uri "http://<server>/api/agents-download/download/obs-agent-windows-x64.exe" -OutFile "$env:TEMP\obs-agent.exe"
& "$env:TEMP\obs-agent.exe" --install-service --server http://<server>:8000 --token <token>
```

## API Reference (New Endpoints)

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/agents/{id}/metrics` | GET | Agent CPU/RAM/disk metrics |
| `/api/agents/{id}/config` | GET/PUT | Read/update agent configuration |
| `/api/agents/{id}/logs` | GET | Agent logs (filterable) |
| `/api/agents/{id}/logs/download` | GET | Download full agent log |
| `/api/agents/{id}/command/start-backup` | POST | Trigger backup |
| `/api/agents/{id}/command/stop-jobs` | POST | Stop all running jobs |
| `/api/agents/{id}/command/restart` | POST | Restart agent |
| `/api/agents/{id}/command/update` | POST | Self-update agent |
| `/api/agents/{id}/command/flush-dirty-buffers` | POST | Flush dirty buffers |
| `/api/agents/{id}/command/clear-cache` | POST | Clear dedup cache |
| `/api/agents/deploy/ssh` | POST | Deploy agent via SSH |
| `/api/agents/deploy/winrm` | POST | Deploy agent via WinRM |
| `/api/dirty-buffers` | GET | Dirty buffer log entries |
| `/api/dirty-buffers/stats` | GET | Aggregated dirty buffer statistics |
| `/api/dirty-buffers/failed` | GET | Failed flush entries |
| `/api/dirty-buffers/inconsistent` | GET | Inconsistent backup entries |
| `/api/dirty-buffers/log` | POST | Agent reports dirty buffer log |

## Web UI Pages

| Page | Description |
|------|-------------|
| Dashboard | Real-time overview of agents, jobs, storage |
| Agents | Agent list with remote management modal (6 tabs) |
| Policies | Backup policy management |
| Jobs | Backup job scheduling and monitoring |
| Storages | Storage target management |
| Dirty Buffers | Pre-backup flush history, consistency status |
| Virtualization | VMware/Hyper-V/Proxmox VM management |
| Databases | PostgreSQL/MySQL/MSSQL/Oracle backup |
| Replication | Async/sync replication management |
| DR Plans | Disaster recovery orchestration |
| SureBackup | Automated backup verification |
| Malware | Ransomware detection alerts |
| Audit | Complete audit trail |
| Agents Download | Platform-specific agent binaries |

## License

Proprietary — All rights reserved.
