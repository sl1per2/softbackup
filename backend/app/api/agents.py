from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select, func
from typing import Optional, List
from datetime import datetime

from app.core.database import get_db
from app.models.models import Agent
from app.models.schemas import AgentCreate, AgentUpdate, AgentResponse
from app.api.auth import get_current_user

router = APIRouter(prefix="/api/agents", tags=["agents"])


@router.get("", response_model=List[AgentResponse])
async def list_agents(
    status: Optional[str] = None,
    os_type: Optional[str] = None,
    page: int = Query(1, ge=1),
    page_size: int = Query(20, ge=1, le=100),
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    query = select(Agent)
    if status:
        query = query.where(Agent.status == status)
    if os_type:
        query = query.where(Agent.os_type == os_type)
    query = query.offset((page - 1) * page_size).limit(page_size)

    result = await db.execute(query)
    agents = result.scalars().all()
    return agents


@router.post("", response_model=AgentResponse, status_code=201)
async def create_agent(
    agent_data: AgentCreate,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    agent = Agent(
        hostname=agent_data.hostname,
        ip=agent_data.ip,
        os_type=agent_data.os_type,
        version=agent_data.version,
        core_version=agent_data.core_version,
    )
    db.add(agent)
    await db.flush()
    await db.refresh(agent)
    await log_audit(db, user.id, "create", "agent", agent.id, {})
    return agent


@router.get("/{agent_id}", response_model=AgentResponse)
async def get_agent(
    agent_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(Agent).where(Agent.id == agent_id))
    agent = result.scalar_one_or_none()
    if not agent:
        raise HTTPException(status_code=404, detail="Agent not found")
    return agent


@router.put("/{agent_id}", response_model=AgentResponse)
async def update_agent(
    agent_id: int,
    agent_data: AgentUpdate,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(Agent).where(Agent.id == agent_id))
    agent = result.scalar_one_or_none()
    if not agent:
        raise HTTPException(status_code=404, detail="Agent not found")

    update_data = agent_data.model_dump(exclude_unset=True)
    for key, value in update_data.items():
        setattr(agent, key, value)

    await db.flush()
    await db.refresh(agent)
    await log_audit(db, user.id, "update", "agent", agent.id, update_data)
    return agent


@router.delete("/{agent_id}")
async def delete_agent(
    agent_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(Agent).where(Agent.id == agent_id))
    agent = result.scalar_one_or_none()
    if not agent:
        raise HTTPException(status_code=404, detail="Agent not found")

    await db.delete(agent)
    await log_audit(db, user.id, "delete", "agent", agent.id, {})
    return {"message": "Agent deleted"}


@router.post("/{agent_id}/upgrade")
async def upgrade_agent(
    agent_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(Agent).where(Agent.id == agent_id))
    agent = result.scalar_one_or_none()
    if not agent:
        raise HTTPException(status_code=404, detail="Agent not found")

    await log_audit(db, user.id, "upgrade", "agent", agent.id, {})
    return {"message": "Upgrade command sent", "agent_id": agent_id}
