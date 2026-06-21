from fastapi import APIRouter, Depends
from pydantic import BaseModel
from typing import Optional
from app.core.security import get_current_user

router = APIRouter(prefix="/api/immutability", tags=["immutability"])


class ImmutabilityConfig(BaseModel):
    enabled: bool = False
    mode: str = "governance"  # governance, compliance
    retention_days: int = 30
    require_mfa_for_delete: bool = False


class MfaDeleteRequest(BaseModel):
    job_id: str
    mfa_code: Optional[str] = None


@router.get("/config")
async def get_config(_user: dict = Depends(get_current_user)):
    return {"enabled": False, "mode": "governance", "retention_days": 30, "require_mfa": False}


@router.put("/config")
async def update_config(data: ImmutabilityConfig, _user: dict = Depends(get_current_user)):
    return {"detail": "Immutability config updated", "config": data.model_dump()}


@router.put("/job/{job_id}")
async def apply_immutability(job_id: str, data: ImmutabilityConfig, _user: dict = Depends(get_current_user)):
    return {"detail": f"Immutability applied to job {job_id}", "mode": data.mode, "days": data.retention_days}


@router.get("/job/{job_id}/status")
async def get_immutability_status(job_id: str, _user: dict = Depends(get_current_user)):
    return {"job_id": job_id, "locked": True, "mode": "governance", "expires_at": "2026-07-21"}


@router.post("/delete")
async def request_mfa_delete(data: MfaDeleteRequest, _user: dict = Depends(get_current_user)):
    return {"detail": "MFA delete requested", "approval_id": "mfa-001", "status": "pending_approval"}


@router.post("/approve")
async def approve_delete(approval_id: str, _user: dict = Depends(get_current_user)):
    return {"detail": f"Delete approved: {approval_id}", "job_deleted": True}
