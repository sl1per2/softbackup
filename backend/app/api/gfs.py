from fastapi import APIRouter, Depends, HTTPException
from pydantic import BaseModel
from typing import Optional
from app.core.security import get_current_user

router = APIRouter(prefix="/api/gfs", tags=["gfs"])


class GFSConfig(BaseModel):
    daily: int = 7
    weekly: int = 4
    monthly: int = 12
    yearly: int = 1


class GFSConfigUpdate(BaseModel):
    daily: Optional[int] = None
    weekly: Optional[int] = None
    monthly: Optional[int] = None
    yearly: Optional[int] = None


@router.get("/config")
async def get_gfs_config(_user: dict = Depends(get_current_user)):
    """Get default GFS retention configuration."""
    return {
        "daily": 7,
        "weekly": 4,
        "monthly": 12,
        "yearly": 1,
        "description": {
            "daily": "Keep last N daily backups (Son)",
            "weekly": "Keep last N weekly backups on Sunday (Father)",
            "monthly": "Keep last N monthly backups on 1st (Grandfather)",
            "yearly": "Keep last N yearly backups on Jan 1 (Great-Grandfather)",
        },
    }


@router.get("/policy/{policy_id}")
async def get_policy_gfs(policy_id: int, _user: dict = Depends(get_current_user)):
    """Get GFS configuration for a specific backup policy."""
    from app.core.database import async_session
    from sqlalchemy import select
    from app.models.models import BackupPolicy

    async with async_session() as db:
        result = await db.execute(select(BackupPolicy).where(BackupPolicy.id == policy_id))
        policy = result.scalar_one_or_none()
        if not policy:
            raise HTTPException(status_code=404, detail="Policy not found")

        return {
            "policy_id": policy_id,
            "policy_name": policy.name,
            "retention_gfs": policy.retention_gfs or {"daily": 7, "weekly": 4, "monthly": 12, "yearly": 1},
        }


@router.put("/policy/{policy_id}")
async def update_policy_gfs(policy_id: int, data: GFSConfig, _user: dict = Depends(get_current_user)):
    """Update GFS retention configuration for a backup policy."""
    from app.core.database import async_session
    from sqlalchemy import select, update
    from app.models.models import BackupPolicy

    async with async_session() as db:
        result = await db.execute(select(BackupPolicy).where(BackupPolicy.id == policy_id))
        policy = result.scalar_one_or_none()
        if not policy:
            raise HTTPException(status_code=404, detail="Policy not found")

        await db.execute(
            update(BackupPolicy).where(BackupPolicy.id == policy_id).values(
                retention_gfs=data.model_dump()
            )
        )
        await db.commit()

    return {"detail": "GFS retention updated", "config": data.model_dump()}


@router.get("/policy/{policy_id}/summary")
async def get_gfs_summary(policy_id: int, _user: dict = Depends(get_current_user)):
    """Get GFS retention summary showing backup counts per tier."""
    from app.services.retention_cleaner import gfs_engine
    from app.core.database import async_session

    async with async_session() as db:
        result = await gfs_engine.get_retention_summary(db, policy_id)
        return result


@router.get("/policy/{policy_id}/keep-dates")
async def get_keep_dates(policy_id: int, _user: dict = Depends(get_current_user)):
    """Get the dates that should be kept under GFS policy."""
    from app.services.retention_cleaner import GFSRetentionPolicy
    from app.core.database import async_session
    from sqlalchemy import select
    from app.models.models import BackupPolicy

    async with async_session() as db:
        result = await db.execute(select(BackupPolicy).where(BackupPolicy.id == policy_id))
        policy = result.scalar_one_or_none()
        if not policy:
            raise HTTPException(status_code=404, detail="Policy not found")

        gfs = GFSRetentionPolicy.from_dict(policy.retention_gfs or {})
        return {
            "policy_id": policy_id,
            "config": gfs.to_dict(),
            "keep_dates": gfs.get_keep_dates(),
        }


@router.post("/policy/{policy_id}/run-cleanup")
async def run_cleanup(policy_id: int, _user: dict = Depends(get_current_user)):
    """Manually trigger GFS cleanup for a specific policy."""
    from app.services.retention_cleaner import cleanup_expired_backups

    # Trigger async cleanup
    cleanup_expired_backups.delay()
    return {"detail": "GFS cleanup triggered"}


@router.get("/report")
async def gfs_report(_user: dict = Depends(get_current_user)):
    """Get GFS retention report for all policies."""
    from app.services.retention_cleaner import gfs_report as gfs_report_task
    from app.core.database import async_session
    from sqlalchemy import select
    from app.models.models import BackupPolicy

    async with async_session() as db:
        policies = (await db.execute(select(BackupPolicy))).scalars().all()
        reports = []
        for policy in policies:
            if policy.retention_gfs:
                from app.services.retention_cleaner import gfs_engine
                report = await gfs_engine.get_retention_summary(db, policy.id)
                reports.append(report)
        return {"reports": reports}
