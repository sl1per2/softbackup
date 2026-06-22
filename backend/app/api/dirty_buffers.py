from fastapi import APIRouter, Depends, Query
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select, func as sql_func
from typing import Optional, List

from app.core.database import get_db
from app.models.dirty_buffer import DirtyBufferLog
from app.models.dirty_buffer_schemas import DirtyBufferLogCreate, DirtyBufferLogResponse, DirtyBufferStatsResponse
from app.api.auth import get_current_user

router = APIRouter(prefix="/api/dirty-buffers", tags=["dirty-buffers"])


@router.post("/log", response_model=DirtyBufferLogResponse)
async def create_dirty_buffer_log(
    entry: DirtyBufferLogCreate,
    db: AsyncSession = Depends(get_db),
):
    log = DirtyBufferLog(**entry.model_dump())
    db.add(log)
    await db.commit()
    await db.refresh(log)
    return log


@router.get("", response_model=List[DirtyBufferLogResponse])
async def list_dirty_buffer_logs(
    plugin_name: Optional[str] = None,
    flush_status: Optional[str] = None,
    backup_job_id: Optional[str] = None,
    is_consistent: Optional[bool] = None,
    page: int = Query(1, ge=1),
    page_size: int = Query(50, ge=1, le=200),
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    query = select(DirtyBufferLog)

    if plugin_name:
        query = query.where(DirtyBufferLog.plugin_name == plugin_name)
    if flush_status:
        query = query.where(DirtyBufferLog.flush_status == flush_status)
    if backup_job_id:
        query = query.where(DirtyBufferLog.backup_job_id == backup_job_id)
    if is_consistent is not None:
        query = query.where(DirtyBufferLog.is_consistent == is_consistent)

    query = query.order_by(DirtyBufferLog.id.desc())
    query = query.offset((page - 1) * page_size).limit(page_size)

    result = await db.execute(query)
    return result.scalars().all()


@router.get("/stats", response_model=DirtyBufferStatsResponse)
async def get_dirty_buffer_stats(
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    total = (await db.execute(select(sql_func.count(DirtyBufferLog.id)))).scalar() or 0
    flushed = (await db.execute(
        select(sql_func.count(DirtyBufferLog.id)).where(DirtyBufferLog.flush_status == "flushed")
    )).scalar() or 0
    failed = (await db.execute(
        select(sql_func.count(DirtyBufferLog.id)).where(DirtyBufferLog.flush_status.in_(["failed", "timeout"]))
    )).scalar() or 0
    inconsistent = (await db.execute(
        select(sql_func.count(DirtyBufferLog.id)).where(DirtyBufferLog.is_consistent == False)
    )).scalar() or 0
    avg_ms = (await db.execute(
        select(sql_func.avg(DirtyBufferLog.flush_duration_ms)).where(DirtyBufferLog.flush_status == "flushed")
    )).scalar() or 0.0

    plugin_rows = (await db.execute(
        select(
            DirtyBufferLog.plugin_name,
            sql_func.count(DirtyBufferLog.id),
            sql_func.avg(DirtyBufferLog.flush_duration_ms),
            sql_func.sum(DirtyBufferLog.before_page_count),
        ).group_by(DirtyBufferLog.plugin_name)
    )).all()

    plugins = [
        {"name": r[0], "count": r[1], "avg_ms": round(r[2] or 0, 1), "total_pages": r[3] or 0}
        for r in plugin_rows
    ]

    return DirtyBufferStatsResponse(
        total_entries=total,
        total_flushed=flushed,
        total_failed=failed,
        total_inconsistent=inconsistent,
        avg_flush_ms=round(avg_ms, 1),
        plugins=plugins,
    )


@router.get("/failed", response_model=List[DirtyBufferLogResponse])
async def get_failed_flushes(
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    query = (
        select(DirtyBufferLog)
        .where(DirtyBufferLog.flush_status.in_(["failed", "timeout"]))
        .order_by(DirtyBufferLog.id.desc())
        .limit(100)
    )
    result = await db.execute(query)
    return result.scalars().all()


@router.get("/inconsistent", response_model=List[DirtyBufferLogResponse])
async def get_inconsistent_backups(
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    query = (
        select(DirtyBufferLog)
        .where(DirtyBufferLog.is_consistent == False)
        .order_by(DirtyBufferLog.id.desc())
        .limit(100)
    )
    result = await db.execute(query)
    return result.scalars().all()


@router.get("/{log_id}", response_model=DirtyBufferLogResponse)
async def get_dirty_buffer_log(
    log_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(DirtyBufferLog).where(DirtyBufferLog.id == log_id))
    log = result.scalar_one_or_none()
    if not log:
        from fastapi import HTTPException
        raise HTTPException(status_code=404, detail="Log not found")
    return log
