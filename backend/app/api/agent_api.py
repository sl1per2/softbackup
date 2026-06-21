from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select
from datetime import datetime

from app.core.database import get_db
from app.models.models import Agent, BackupJob
from app.models.schemas import AgentCreate

router = APIRouter(prefix="/api", tags=["api"])


@router.get("/tasks")
async def get_tasks(
    db: AsyncSession = Depends(get_db),
):
    return {"tasks": []}


@router.post("/status")
async def update_status(
    body: dict,
    db: AsyncSession = Depends(get_db),
):
    agent_id = body.get("agent_id")
    task_id = body.get("task_id")
    status = body.get("status")

    if agent_id:
        result = await db.execute(select(Agent).where(Agent.hostname == agent_id))
        agent = result.scalar_one_or_none()
        if agent:
            agent.last_seen = datetime.utcnow()
            agent.status = "online" if status != "FAILED" else "error"
            await db.commit()

    return {"success": True}


@router.post("/register")
async def register_agent(
    body: AgentCreate,
    db: AsyncSession = Depends(get_db),
):
    result = await db.execute(select(Agent).where(Agent.hostname == body.hostname))
    existing = result.scalar_one_or_none()

    if existing:
        existing.ip = body.ip
        existing.os_type = body.os_type
        existing.version = body.version
        existing.core_version = body.core_version
        existing.last_seen = datetime.utcnow()
        existing.status = "online"
        await db.commit()
        await db.refresh(existing)
        return {"agent_id": existing.hostname, "id": existing.id}

    agent = Agent(
        hostname=body.hostname,
        ip=body.ip,
        os_type=body.os_type,
        version=body.version,
        core_version=body.core_version,
        status="online",
        last_seen=datetime.utcnow(),
    )
    db.add(agent)
    await db.flush()
    await db.refresh(agent)
    return {"agent_id": agent.hostname, "id": agent.id}


@router.post("/heartbeat")
async def heartbeat(
    body: dict,
    db: AsyncSession = Depends(get_db),
):
    agent_id = body.get("agent_id")
    if agent_id:
        result = await db.execute(select(Agent).where(Agent.hostname == agent_id))
        agent = result.scalar_one_or_none()
        if agent:
            agent.last_seen = datetime.utcnow()
            agent.status = body.get("status", "online")
            await db.commit()

    return {"success": True}


@router.post("/metrics")
async def receive_metrics(
    body: dict,
    db: AsyncSession = Depends(get_db),
):
    return {"success": True}
