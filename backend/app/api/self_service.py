from fastapi import APIRouter, Depends
from pydantic import BaseModel
from typing import Optional, List
from app.core.security import get_current_user

router = APIRouter(prefix="/api/self-service", tags=["self-service"])


class SelfServiceBackupRequest(BaseModel):
    vm_id: str
    source_path: str = "/"
    retention_days: int = 7


@router.get("/vms")
async def list_my_vms(_user: dict = Depends(get_current_user)):
    return {"vms": [
        {"vm_id": "vm-001", "name": "WebServer", "status": "online", "owner": _user.username},
        {"vm_id": "vm-002", "name": "AppServer", "status": "online", "owner": _user.username},
    ]}


@router.post("/backup")
async def quick_backup(data: SelfServiceBackupRequest, _user: dict = Depends(get_current_user)):
    import uuid
    return {"detail": "Quick backup started", "job_id": f"adhoc-{uuid.uuid4().hex[:8]}",
            "vm_id": data.vm_id, "retention_days": data.retention_days}


@router.get("/backups")
async def list_my_backups(vm_id: Optional[str] = None, _user: dict = Depends(get_current_user)):
    return {"backups": []}


@router.post("/restore-file")
async def restore_file(data: dict, _user: dict = Depends(get_current_user)):
    return {"detail": "File restore started"}


@router.get("/quota")
async def get_quota(_user: dict = Depends(get_current_user)):
    return {"max_backups_per_day": 5, "backups_used_today": 2, "max_storage_gb": 100, "storage_used_gb": 35}
