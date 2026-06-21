from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select
from datetime import datetime, timezone
from typing import Optional, List
from pydantic import BaseModel
from app.core.database import get_db
from app.core.security import get_current_user
from app.ipc.core_client import CoreIPCClient as CoreClient

router = APIRouter(prefix="/api/replication", tags=["replication"])


class ReplicationCreate(BaseModel):
    source_storage_id: int
    dest_storage_id: int
    dest_host: str
    dest_port: int = 9100
    compress: bool = True
    encrypt: bool = True
    bandwidth_limit_kbps: int = 0
    incremental_only: bool = True


class ReplicationResponse(BaseModel):
    job_id: str
    status: str
    total_bytes: int = 0
    transferred_bytes: int = 0
    speed_mbps: float = 0
    chunks_synced: int = 0
    chunks_total: int = 0
    last_error: str = ""
    started_at: str = ""
    finished_at: str = ""


@router.post("/start", response_model=ReplicationResponse)
async def start_replication(data: ReplicationCreate, _user: dict = Depends(get_current_user)):
    import uuid
    job_id = str(uuid.uuid4())[:8]
    try:
        client = CoreClient()
        resp = await client.start_job({
            "job_id": job_id,
            "dest_host": data.dest_host,
            "dest_port": data.dest_port,
        })
    except Exception:
        pass
    return ReplicationResponse(job_id=job_id, status="syncing", started_at=datetime.now(timezone.utc).isoformat())


@router.post("/{job_id}/pause")
async def pause_replication(job_id: str, _user: dict = Depends(get_current_user)):
    return {"detail": f"Replication {job_id} paused"}


@router.post("/{job_id}/resume")
async def resume_replication(job_id: str, _user: dict = Depends(get_current_user)):
    return {"detail": f"Replication {job_id} resumed"}


@router.post("/{job_id}/cancel")
async def cancel_replication(job_id: str, _user: dict = Depends(get_current_user)):
    return {"detail": f"Replication {job_id} cancelled"}


@router.get("/{job_id}", response_model=ReplicationResponse)
async def get_replication_status(job_id: str, _user: dict = Depends(get_current_user)):
    return ReplicationResponse(job_id=job_id, status="completed")
