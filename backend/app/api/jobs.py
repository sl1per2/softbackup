from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select
from typing import Optional, List
from datetime import datetime

from app.core.database import get_db
from app.models.models import BackupJob
from app.models.schemas import JobResponse
from app.api.auth import get_current_user

router = APIRouter(prefix="/api", tags=["api"])


@router.get("", response_model=List[JobResponse])
async def list_jobs(
    status: Optional[str] = None,
    agent_id: Optional[int] = None,
    policy_id: Optional[int] = None,
    job_type: Optional[str] = None,
    date_from: Optional[str] = None,
    date_to: Optional[str] = None,
    page: int = Query(1, ge=1),
    page_size: int = Query(20, ge=1, le=100),
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    query = select(BackupJob)

    if status:
        statuses = status.split(",")
        query = query.where(BackupJob.status.in_(statuses))
    if agent_id:
        query = query.where(BackupJob.agent_id == agent_id)
    if policy_id:
        query = query.where(BackupJob.policy_id == policy_id)
    if job_type:
        query = query.where(BackupJob.type == job_type)
    if date_from:
        try:
            dt = datetime.fromisoformat(date_from)
            query = query.where(BackupJob.started_at >= dt)
        except ValueError:
            pass
    if date_to:
        try:
            dt = datetime.fromisoformat(date_to)
            query = query.where(BackupJob.started_at <= dt)
        except ValueError:
            pass

    query = query.order_by(BackupJob.id.desc())
    query = query.offset((page - 1) * page_size).limit(page_size)

    result = await db.execute(query)
    return result.scalars().all()


@router.get("/{job_id}", response_model=JobResponse)
async def get_job(
    job_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(BackupJob).where(BackupJob.id == job_id))
    job = result.scalar_one_or_none()
    if not job:
        raise HTTPException(status_code=404, detail="Job not found")
    return job


@router.post("/{job_id}/cancel")
async def cancel_job(
    job_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(BackupJob).where(BackupJob.id == job_id))
    job = result.scalar_one_or_none()
    if not job:
        raise HTTPException(status_code=404, detail="Job not found")
    if job.status not in ("pending", "running"):
        raise HTTPException(status_code=400, detail="Job cannot be cancelled")

    job.status = "cancelled"
    job.finished_at = datetime.utcnow()
    await db.flush()
    await log_audit(db, user.id, "cancel", "job", job.id, {})
    return {"message": "Job cancelled"}


@router.get("/{job_id}/log")
async def get_job_log(
    job_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(BackupJob).where(BackupJob.id == job_id))
    job = result.scalar_one_or_none()
    if not job:
        raise HTTPException(status_code=404, detail="Job not found")

    return {
        "job_id": job_id,
        "log": job.error_log or "",
        "status": job.status,
    }
