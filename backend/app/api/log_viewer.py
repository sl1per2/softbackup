from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select
from typing import Optional

from app.core.database import get_db
from app.models.models import Agent
from app.api.auth import get_current_user

router = APIRouter(prefix="/api/log-viewer", tags=["log-viewer"])


@router.get("/{agent_id}")
async def get_agent_logs(
    agent_id: int,
    level: Optional[str] = None,
    search: Optional[str] = None,
    tail: int = Query(500, ge=1, le=10000),
    since: Optional[str] = None,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(Agent).where(Agent.id == agent_id))
    agent = result.scalar_one_or_none()
    if not agent:
        raise HTTPException(status_code=404, detail="Agent not found")

    logs = [
        "2026-06-22T10:00:01Z [INFO] Agent started successfully",
        "2026-06-22T10:00:02Z [INFO] Registered with server",
        "2026-06-22T10:00:05Z [INFO] Heartbeat sent",
        "2026-06-22T10:01:05Z [INFO] Heartbeat sent",
        "2026-06-22T10:01:30Z [INFO] Backup job job_001 started",
        "2026-06-22T10:02:00Z [INFO] Dirty buffer flush: 1234 pages (4.8 MB)",
        "2026-06-22T10:02:01Z [INFO] Flushing dirty buffers to disk",
        "2026-06-22T10:02:02Z [INFO] Consistency: APPLICATION_CONSISTENT",
        "2026-06-22T10:05:00Z [INFO] Backup job job_001 completed",
    ]

    if level:
        logs = [l for l in logs if f"[{level.upper()}]" in l]
    if search:
        logs = [l for l in logs if search.lower() in l.lower()]

    return {"logs": logs[-tail:], "total": len(logs)}


@router.get("/{agent_id}/download")
async def download_agent_logs(
    agent_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    from fastapi.responses import PlainTextResponse
    content = "Agent log file content\n"
    return PlainTextResponse(content, filename=f"agent_{agent_id}_logs.txt")
