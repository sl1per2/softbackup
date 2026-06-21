from fastapi import APIRouter, Depends, HTTPException, WebSocket, WebSocketDisconnect
from pydantic import BaseModel
from typing import Optional, List
from datetime import datetime, timezone
from app.core.security import get_current_user
from app.core.ws_manager import ws_manager as manager

router = APIRouter(prefix="/api/virtualization", tags=["virtualization"])


class HypervisorConnection(BaseModel):
    name: str
    type: str  # vmware, hyperv, proxmox
    host: str
    port: int = 443
    username: str
    password: str
    verify_ssl: bool = True


class VmBackupRequest(BaseModel):
    vm_id: str
    storage_id: str
    transport: str = "NETWORK"
    use_cbt: bool = True
    quiesce: bool = True
    encryption_enabled: bool = False
    compression_level: int = 1
    bandwidth_limit_kbps: int = 0
    create_snapshot: bool = True


class VmRestoreRequest(BaseModel):
    backup_id: str
    target_host: str
    target_datastore: str
    power_on_after: bool = False
    new_vm_name: Optional[str] = None


# In-memory store (in production: DB)
_connections = {}
_vms_cache = []
_backup_jobs = []


@router.get("/hypervisors")
async def list_hypervisors(_user: dict = Depends(get_current_user)):
    return list(_connections.values())


@router.post("/hypervisors")
async def add_hypervisor(data: HypervisorConnection, _user: dict = Depends(get_current_user)):
    import uuid
    conn_id = str(uuid.uuid4())[:8]
    _connections[conn_id] = {
        "id": conn_id,
        "name": data.name,
        "type": data.type,
        "host": data.host,
        "port": data.port,
        "connected": True,
        "vm_count": 0,
    }
    return {"detail": f"Hypervisor {data.name} connected", "id": conn_id}


@router.delete("/hypervisors/{conn_id}")
async def remove_hypervisor(conn_id: str, _user: dict = Depends(get_current_user)):
    if conn_id not in _connections:
        raise HTTPException(status_code=404, detail="Connection not found")
    del _connections[conn_id]
    return {"detail": "Hypervisor disconnected"}


@router.get("/hypervisors/{conn_id}/test")
async def test_hypervisor(conn_id: str, _user: dict = Depends(get_current_user)):
    if conn_id not in _connections:
        raise HTTPException(status_code=404, detail="Connection not found")
    conn = _connections[conn_id]
    return {"success": True, "message": f"Connected to {conn['type']} at {conn['host']}"}


@router.get("/vms")
async def list_vms(conn_id: Optional[str] = None, _user: dict = Depends(get_current_user)):
    vms = [
        {"vm_id": "vm-001", "name": "WebServer-01", "host": "vcenter.local", "type": "vmware",
         "power_state": "on", "cpu": 4, "memory_mb": 8192, "disk_gb": 200, "os": "Linux",
         "ip": "10.0.1.10", "tools": True},
        {"vm_id": "vm-002", "name": "DBServer-01", "host": "vcenter.local", "type": "vmware",
         "power_state": "on", "cpu": 8, "memory_mb": 32768, "disk_gb": 1000, "os": "Windows",
         "ip": "10.0.1.20", "tools": True},
        {"vm_id": "qemu/100", "name": "ProxmoxVM-01", "host": "proxmox.local", "type": "proxmox",
         "power_state": "on", "cpu": 2, "memory_mb": 4096, "disk_gb": 50, "os": "Linux",
         "ip": "10.0.2.10", "tools": False},
        {"vm_id": "hyperv-001", "name": "HyperV-Win01", "host": "hyperv.local", "type": "hyperv",
         "power_state": "on", "cpu": 4, "memory_mb": 16384, "disk_gb": 500, "os": "Windows",
         "ip": "10.0.3.10", "tools": True},
    ]
    if conn_id:
        vms = [v for v in vms if v.get("host", "").startswith(conn_id)]
    return vms


@router.post("/vms/{vm_id}/backup")
async def backup_vm(vm_id: str, data: VmBackupRequest, _user: dict = Depends(get_current_user)):
    import uuid
    job_id = "vmb-" + str(uuid.uuid4())[:8]
    job = {
        "job_id": job_id, "vm_id": vm_id, "status": "running",
        "transport": data.transport, "started_at": datetime.now(timezone.utc).isoformat(),
        "total_bytes": 100 * 1024**3, "transferred_bytes": 0, "speed_mbps": 0,
    }
    _backup_jobs.append(job)

    await manager.broadcast("dashboard", {"type": "vm_backup_started", "data": job})
    return {"detail": "Backup started", "job_id": job_id}


@router.post("/vms/{vm_id}/restore")
async def restore_vm(vm_id: str, data: VmRestoreRequest, _user: dict = Depends(get_current_user)):
    return {"detail": f"Restore initiated for {vm_id}", "target": data.target_host}


@router.post("/vms/{vm_id}/snapshot")
async def create_snapshot(vm_id: str, name: str = "obs-snapshot", quiesce: bool = True,
                         _user: dict = Depends(get_current_user)):
    return {"detail": f"Snapshot '{name}' created for {vm_id}", "quiesce": quiesce}


@router.delete("/vms/{vm_id}/snapshot/{snapshot_id}")
async def remove_snapshot(vm_id: str, snapshot_id: str, _user: dict = Depends(get_current_user)):
    return {"detail": f"Snapshot {snapshot_id} removed from {vm_id}"}


@router.post("/vms/{vm_id}/power/on")
async def power_on(vm_id: str, _user: dict = Depends(get_current_user)):
    return {"detail": f"VM {vm_id} powered on"}


@router.post("/vms/{vm_id}/power/off")
async def power_off(vm_id: str, _user: dict = Depends(get_current_user)):
    return {"detail": f"VM {vm_id} powered off"}


@router.post("/vms/{vm_id}/power/suspend")
async def suspend(vm_id: str, _user: dict = Depends(get_current_user)):
    return {"detail": f"VM {vm_id} suspended"}


@router.get("/vms/{vm_id}/snapshots")
async def list_snapshots(vm_id: str, _user: dict = Depends(get_current_user)):
    return [
        {"id": "snap-1", "name": "Before Update", "created": "2024-01-15T10:00:00Z", "quiesced": True},
        {"id": "snap-2", "name": "OBS Backup", "created": "2024-01-16T02:00:00Z", "quiesced": True},
    ]


@router.get("/backup-jobs")
async def list_backup_jobs(_user: dict = Depends(get_current_user)):
    return _backup_jobs


@router.get("/features")
async def get_features(_user: dict = Depends(get_current_user)):
    return {
        "vmware": {
            "versions": ["vSphere 5.5", "vSphere 6.0", "vSphere 6.5", "vSphere 6.7",
                        "vSphere 7.0", "vSphere 8.0", "ESXi 5.5-8.0"],
            "transports": ["Hot Add", "NBD", "Network", "Direct SAN", "HotAdd+NBD"],
            "features": ["CBT (Changed Block Tracking)", "VDDK integration",
                        "Storage vMotion", "vSphere Replication", "Instant VM Recovery",
                        "Application-aware processing", "Memory snapshot"],
        },
        "hyperv": {
            "versions": ["Hyper-V 2012", "Hyper-V 2012 R2", "Hyper-V 2016",
                        "Hyper-V 2019", "Hyper-V 2022", "Windows Server 2025"],
            "transports": ["Network (SMB)", "Hot Add", "Direct CSV"],
            "features": ["RCT (Resilient Change Tracking)", "VSS checkpoints",
                        "Production Checkpoints", "Cluster Shared Volumes",
                        "Replica", "Hyper-V Backup (WBEM)", "ReFS block clone"],
        },
        "proxmox": {
            "versions": ["Proxmox VE 5.x", "Proxmox VE 6.x", "Proxmox VE 7.x", "Proxmox VE 8.x"],
            "transports": ["Network (API)", "Direct stream"],
            "features": ["vzdump backup/restore", "QEMU snapshots",
                        "LXC container backup", "ZFS integration",
                        "Ceph storage", "Backup without temp storage",
                        "OVA/OVF import"],
        },
    }
