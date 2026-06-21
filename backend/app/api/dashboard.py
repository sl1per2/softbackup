from fastapi import APIRouter, Depends
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select, func
from datetime import datetime, timedelta

from app.core.database import get_db
from app.models.models import Agent, BackupJob, CDPSession, TrafficStats
from app.models.schemas import DashboardSummary
from app.api.auth import get_current_user

router = APIRouter(prefix="/api", tags=["api"])


@router.get("/summary", response_model=DashboardSummary)
async def get_summary(
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    total_agents = (await db.execute(select(func.count(Agent.id)))).scalar() or 0

    agents_online = (await db.execute(
        select(func.count(Agent.id)).where(Agent.status == "online")
    )).scalar() or 0

    now = datetime.utcnow()
    day_ago = now - timedelta(hours=24)

    jobs_successful = (await db.execute(
        select(func.count(BackupJob.id)).where(
            BackupJob.status == "completed",
            BackupJob.started_at >= day_ago
        )
    )).scalar() or 0

    jobs_failed = (await db.execute(
        select(func.count(BackupJob.id)).where(
            BackupJob.status == "failed",
            BackupJob.started_at >= day_ago
        )
    )).scalar() or 0

    cdp_active = (await db.execute(
        select(func.count(CDPSession.id)).where(CDPSession.status == "active")
    )).scalar() or 0

    traffic_result = await db.execute(
        select(
            func.sum(TrafficStats.original_size - TrafficStats.transferred_size)
        ).where(TrafficStats.timestamp >= day_ago)
    )
    traffic_saved = traffic_result.scalar() or 0

    cache_result = await db.execute(
        select(func.avg(Agent.cache_hit_ratio))
    )
    avg_cache = cache_result.scalar() or 0.0

    compression_result = await db.execute(
        select(func.avg(BackupJob.compression_ratio)).where(
            BackupJob.status == "completed",
            BackupJob.started_at >= day_ago
        )
    )
    avg_compression = compression_result.scalar() or 1.0

    return DashboardSummary(
        total_agents=total_agents,
        agents_online=agents_online,
        jobs_successful_24h=jobs_successful,
        jobs_failed_24h=jobs_failed,
        cdp_sessions_active=cdp_active,
        traffic_saved_bytes=traffic_saved or 0,
        avg_cache_hit_ratio=avg_cache or 0.0,
        avg_compression_ratio=avg_compression or 1.0,
    )


@router.get("/charts")
async def get_charts(
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    now = datetime.utcnow()
    days_ago = now - timedelta(days=30)

    jobs = (await db.execute(
        select(BackupJob).where(BackupJob.started_at >= days_ago)
        .order_by(BackupJob.started_at)
    )).scalars().all()

    backup_volumes = []
    transfer_volumes = []
    for job in jobs:
        if job.started_at:
            label = job.started_at.strftime("%Y-%m-%d")
            backup_volumes.append({"label": label, "value": job.size_bytes})
            transfer_volumes.append({"label": label, "value": job.size_transferred_bytes})

    cdp_sessions = (await db.execute(
        select(CDPSession).where(CDPSession.started_at >= days_ago)
    )).scalars().all()

    cdp_latency = []
    for s in cdp_sessions:
        if s.started_at:
            cdp_latency.append({
                "label": s.started_at.strftime("%Y-%m-%d %H:%M"),
                "value": s.current_lag_ms,
            })

    return {
        "backup_volumes": backup_volumes[-50:],
        "transfer_volumes": transfer_volumes[-50:],
        "cdp_latency": cdp_latency[-50:],
        "cache_hit_trend": [],
    }


@router.get("/recent-jobs")
async def get_recent_jobs(
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(
        select(BackupJob).order_by(BackupJob.id.desc()).limit(10)
    )
    jobs = result.scalars().all()

    return [
        {
            "id": job.id,
            "type": job.type,
            "status": job.status,
            "started_at": job.started_at.isoformat() if job.started_at else None,
            "size_bytes": job.size_bytes,
            "agent_id": job.agent_id,
        }
        for job in jobs
    ]
