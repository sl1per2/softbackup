from fastapi import APIRouter, Depends
from pydantic import BaseModel
from typing import Optional, List
from app.core.security import get_current_user

router = APIRouter(prefix="/api/replication-v2", tags=["vm-replication"])


class VmReplicationCreate(BaseModel):
    name: str
    source_vm_id: str
    source_host: str
    target_host: str
    target_datastore: str
    rpo_seconds: int = 300
    memory_mb: int = 1024
    cpu_count: int = 2


@router.get("")
async def list_replications(_user: dict = Depends(get_current_user)):
    return {"replications": []}


@router.post("")
async def create_replication(data: VmReplicationCreate, _user: dict = Depends(get_current_user)):
    import uuid
    job_id = f"repl-{uuid.uuid4().hex[:8]}"
    return {"detail": "Replication created", "job_id": job_id, "name": data.name}


@router.get("/{job_id}")
async def get_replication(job_id: str, _user: dict = Depends(get_current_user)):
    return {"job_id": job_id, "status": "syncing", "bytes_replicated": 0, "lag_seconds": 0}


@router.post("/{job_id}/start")
async def start_replication(job_id: str, _user: dict = Depends(get_current_user)):
    return {"detail": "Replication started", "job_id": job_id}


@router.post("/{job_id}/stop")
async def stop_replication(job_id: str, _user: dict = Depends(get_current_user)):
    return {"detail": "Replication stopped"}


@router.post("/{job_id}/failover")
async def failover(job_id: str, _user: dict = Depends(get_current_user)):
    return {"detail": "Failover initiated", "job_id": job_id}


@router.post("/{job_id}/failback")
async def failback(job_id: str, _user: dict = Depends(get_current_user)):
    return {"detail": "Failback initiated", "job_id": job_id}
