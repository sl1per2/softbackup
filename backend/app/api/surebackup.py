from fastapi import APIRouter, Depends
from pydantic import BaseModel
from typing import Optional, List
from app.core.security import get_current_user

router = APIRouter(prefix="/api/surebackup", tags=["surebackup"])


class SureBackupConfig(BaseModel):
    verify_after_full: bool = True
    verify_after_n_incrementals: int = 5
    verify_weekly: bool = True
    boot_timeout_seconds: int = 300
    heartbeat_timeout_seconds: int = 120
    checks: List[str] = ["heartbeat", "ping", "application"]
    application_scripts: List[str] = []


class SureBackupRunRequest(BaseModel):
    job_id: str
    checks: Optional[List[str]] = None


@router.get("/config")
async def get_config(_user: dict = Depends(get_current_user)):
    return {
        "verify_after_full": True,
        "verify_after_n_incrementals": 5,
        "verify_weekly": True,
        "boot_timeout_seconds": 300,
        "heartbeat_timeout_seconds": 120,
        "checks": ["heartbeat", "ping", "application"],
    }


@router.put("/config")
async def update_config(data: SureBackupConfig, _user: dict = Depends(get_current_user)):
    return {"detail": "SureBackup config updated", "config": data.model_dump()}


@router.post("/run")
async def run_surebackup(data: SureBackupRunRequest, _user: dict = Depends(get_current_user)):
    import uuid
    test_id = f"sb-{uuid.uuid4().hex[:8]}"
    return {"test_id": test_id, "job_id": data.job_id, "status": "running",
            "message": "SureBackup test started"}


@router.get("/results")
async def get_results(
    job_id: Optional[str] = None,
    date_from: Optional[str] = None,
    date_to: Optional[str] = None,
    _user: dict = Depends(get_current_user),
):
    return {"results": []}


@router.get("/summary")
async def get_summary(_user: dict = Depends(get_current_user)):
    return {"verified": 142, "total": 150, "pending": 8, "success_rate": 94.7}
