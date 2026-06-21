from fastapi import APIRouter, Depends
from pydantic import BaseModel
from typing import Optional, List
from app.core.security import get_current_user

router = APIRouter(prefix="/api/backup-copy", tags=["backup-copy"])


class BackupCopyCreate(BaseModel):
    name: str
    source_storage_id: int
    dest_storage_id: int
    mode: str = "immediate"  # immediate, scheduled, mirror
    cron_schedule: Optional[str] = None
    health_check_after: bool = True
    verify_checksums: bool = True
    archive_retention_weekly: int = 4
    archive_retention_monthly: int = 12
    archive_retention_yearly: int = 7


@router.get("")
async def list_backup_copy_jobs(_user: dict = Depends(get_current_user)):
    return {"jobs": []}


@router.post("")
async def create_backup_copy(data: BackupCopyCreate, _user: dict = Depends(get_current_user)):
    import uuid
    job_id = f"bc-{uuid.uuid4().hex[:8]}"
    return {"detail": "Backup Copy job created", "job_id": job_id}


@router.get("/{job_id}")
async def get_backup_copy(job_id: str, _user: dict = Depends(get_current_user)):
    return {"job_id": job_id, "status": "idle"}


@router.delete("/{job_id}")
async def delete_backup_copy(job_id: str, _user: dict = Depends(get_current_user)):
    return {"detail": "Backup Copy job deleted"}


@router.post("/{job_id}/run")
async def run_backup_copy(job_id: str, _user: dict = Depends(get_current_user)):
    return {"detail": "Backup Copy started", "job_id": job_id, "status": "running"}


@router.get("/{job_id}/history")
async def get_copy_history(job_id: str, _user: dict = Depends(get_current_user)):
    return {"history": []}


@router.post("/{job_id}/archive")
async def create_archive_point(job_id: str, tier: str = "weekly", _user: dict = Depends(get_current_user)):
    return {"detail": f"GFS archive point ({tier}) created", "job_id": job_id}
