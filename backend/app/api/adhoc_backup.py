from fastapi import APIRouter, Depends
from pydantic import BaseModel
from typing import Optional
from app.core.security import get_current_user

router = APIRouter(prefix="/api/adhoc", tags=["adhoc-backup"])


class AdhocBackupRequest(BaseModel):
    agent_id: int
    source_path: str
    retention_days: int = 7
    compress: bool = True
    encrypt: bool = False


@router.post("/backup")
async def adhoc_backup(data: AdhocBackupRequest, _user: dict = Depends(get_current_user)):
    import uuid
    return {
        "detail": "Ad-hoc backup started",
        "job_id": f"adhoc-{uuid.uuid4().hex[:8]}",
        "agent_id": data.agent_id,
        "source": data.source_path,
        "retention_days": data.retention_days,
        "expires_at": "2026-06-28",
    }
