from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select
from typing import Optional, List
from datetime import datetime

from app.core.database import get_db
from app.models.models import BackupPolicy, CDPSession, RecoveryPoint, BackupJob, JobState
from app.models.schemas import PolicyCreate, PolicyUpdate, PolicyResponse, RecoveryPointResponse
from app.api.auth import get_current_user

router = APIRouter(prefix="/api", tags=["api"])


@router.get("", response_model=List[PolicyResponse])
async def list_policies(
    search: Optional[str] = None,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    query = select(BackupPolicy)
    if search:
        query = query.where(BackupPolicy.name.ilike(f"%{search}%"))
    result = await db.execute(query)
    return result.scalars().all()


@router.post("", response_model=PolicyResponse, status_code=201)
async def create_policy(
    data: PolicyCreate,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    policy = BackupPolicy(**data.model_dump())
    db.add(policy)
    await db.flush()
    await db.refresh(policy)
    await log_audit(db, user.id, "create", "policy", policy.id, {})
    return policy


@router.get("/{policy_id}", response_model=PolicyResponse)
async def get_policy(
    policy_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(BackupPolicy).where(BackupPolicy.id == policy_id))
    policy = result.scalar_one_or_none()
    if not policy:
        raise HTTPException(status_code=404, detail="Policy not found")
    return policy


@router.put("/{policy_id}", response_model=PolicyResponse)
async def update_policy(
    policy_id: int,
    data: PolicyUpdate,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(BackupPolicy).where(BackupPolicy.id == policy_id))
    policy = result.scalar_one_or_none()
    if not policy:
        raise HTTPException(status_code=404, detail="Policy not found")

    update_data = data.model_dump(exclude_unset=True)
    for key, value in update_data.items():
        setattr(policy, key, value)

    await db.flush()
    await db.refresh(policy)
    await log_audit(db, user.id, "update", "policy", policy.id, update_data)
    return policy


@router.delete("/{policy_id}")
async def delete_policy(
    policy_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(BackupPolicy).where(BackupPolicy.id == policy_id))
    policy = result.scalar_one_or_none()
    if not policy:
        raise HTTPException(status_code=404, detail="Policy not found")

    await db.delete(policy)
    await log_audit(db, user.id, "delete", "policy", policy.id, {})
    return {"message": "Policy deleted"}


@router.post("/{policy_id}/run")
async def run_policy(
    policy_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(BackupPolicy).where(BackupPolicy.id == policy_id))
    policy = result.scalar_one_or_none()
    if not policy:
        raise HTTPException(status_code=404, detail="Policy not found")

    job = BackupJob(
        policy_id=policy.id,
        storage_id=policy.storage_id,
        type="full",
        status=JobState.PENDING.value,
    )
    db.add(job)
    await db.flush()
    await db.refresh(job)
    await log_audit(db, user.id, "run_policy", "policy", policy.id, {"job_id": job.id})
    return {"message": "Backup job created", "job_id": job.id}


@router.post("/{policy_id}/cdp/start")
async def start_cdp(
    policy_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(BackupPolicy).where(BackupPolicy.id == policy_id))
    policy = result.scalar_one_or_none()
    if not policy:
        raise HTTPException(status_code=404, detail="Policy not found")
    if not policy.cdp_enabled:
        raise HTTPException(status_code=400, detail="CDP not enabled in policy")

    session = CDPSession(
        policy_id=policy.id,
        status="active",
        started_at=datetime.utcnow(),
    )
    db.add(session)
    await db.flush()
    await db.refresh(session)
    await log_audit(db, user.id, "cdp_start", "policy", policy.id, {"session_id": session.id})
    return {"message": "CDP session started", "session_id": session.id}


@router.post("/{policy_id}/cdp/stop")
async def stop_cdp(
    policy_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(
        select(CDPSession).where(CDPSession.policy_id == policy_id, CDPSession.status == "active")
    )
    session = result.scalar_one_or_none()
    if not session:
        raise HTTPException(status_code=404, detail="No active CDP session found")

    session.status = "stopped"
    await db.flush()
    await log_audit(db, user.id, "cdp_stop", "policy", policy.id, {"session_id": session.id})
    return {"message": "CDP session stopped"}


@router.get("/{policy_id}/cdp/status")
async def cdp_status(
    policy_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(
        select(CDPSession).where(CDPSession.policy_id == policy_id, CDPSession.status == "active")
    )
    session = result.scalar_one_or_none()
    if not session:
        return {"active": False}
    return {
        "active": True,
        "session_id": session.id,
        "lag_ms": session.current_lag_ms,
        "iops": session.iops,
        "throughput_mbps": session.throughput_mbps,
        "blocks_tracked": session.blocks_tracked,
    }


@router.get("/{policy_id}/cdp/recovery-points", response_model=List[RecoveryPointResponse])
async def list_recovery_points(
    policy_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(
        select(RecoveryPoint).where(RecoveryPoint.policy_id == policy_id)
        .order_by(RecoveryPoint.timestamp.desc())
    )
    return result.scalars().all()


@router.post("/{policy_id}/cdp/restore")
async def cdp_restore(
    policy_id: int,
    body: dict,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    rp_id = body.get("recovery_point_id")
    if not rp_id:
        raise HTTPException(status_code=400, detail="recovery_point_id required")

    result = await db.execute(select(RecoveryPoint).where(RecoveryPoint.id == rp_id))
    rp = result.scalar_one_or_none()
    if not rp:
        raise HTTPException(status_code=404, detail="Recovery point not found")

    await log_audit(db, user.id, "cdp_restore", "recovery_point", rp.id, {"policy_id": policy_id})
    return {"message": "Restore initiated", "recovery_point_id": rp_id}


@router.put("/{policy_id}/bandwidth")
async def update_bandwidth(
    policy_id: int,
    body: dict,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(BackupPolicy).where(BackupPolicy.id == policy_id))
    policy = result.scalar_one_or_none()
    if not policy:
        raise HTTPException(status_code=404, detail="Policy not found")

    policy.bandwidth_limit_kbps = body.get("limit_kbps", 0)
    policy.bandwidth_schedule = body.get("schedule", {})
    await db.flush()
    await log_audit(db, user.id, "update_bandwidth", "policy", policy.id, body)
    return {"message": "Bandwidth settings updated"}
