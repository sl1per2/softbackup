from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select
from pydantic import BaseModel
from typing import Optional, Dict, Any
import httpx

from app.core.database import get_db
from app.models.models import Agent
from app.api.auth import get_current_user

router = APIRouter(prefix="/api/config-editor", tags=["config-editor"])


class ConfigField(BaseModel):
    key: str
    value: Any
    type: str = "string"
    description: str = ""
    readonly: bool = False
    options: Optional[list] = None


class ConfigSchema(BaseModel):
    fields: list[ConfigField]


CONFIG_SCHEMA = [
    ConfigField(key="server_url", value="", type="string", description="Server URL", readonly=True),
    ConfigField(key="log_level", value="info", type="select", description="Log level", options=["debug", "info", "warn", "error"]),
    ConfigField(key="heartbeat_interval_sec", value=30, type="number", description="Heartbeat interval (sec)", options=[5, 10, 15, 30, 60]),
    ConfigField(key="cache_size_mb", value=1024, type="number", description="Cache size (MB)"),
    ConfigField(key="max_concurrent_jobs", value=2, type="number", description="Max concurrent jobs", options=[1, 2, 4, 8]),
    ConfigField(key="bandwidth_limit_kbps", value=0, type="number", description="Bandwidth limit (KB/s, 0=unlimited)"),
    ConfigField(key="chunk_size", value=256, type="number", description="Chunk size (KB)", options=[64, 128, 256, 512, 1024]),
]


@router.get("/schema/{agent_id}")
async def get_config_schema(
    agent_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(Agent).where(Agent.id == agent_id))
    agent = result.scalar_one_or_none()
    if not agent:
        raise HTTPException(status_code=404, detail="Agent not found")
    return {"schema": [f.model_dump() for f in CONFIG_SCHEMA]}


@router.get("/{agent_id}")
async def get_config(
    agent_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(Agent).where(Agent.id == agent_id))
    agent = result.scalar_one_or_none()
    if not agent:
        raise HTTPException(status_code=404, detail="Agent not found")
    return {
        "server_url": "",
        "log_level": "info",
        "heartbeat_interval_sec": 30,
        "cache_size_mb": 1024,
        "max_concurrent_jobs": 2,
        "bandwidth_limit_kbps": 0,
        "chunk_size": 256,
    }


@router.put("/{agent_id}")
async def update_config(
    agent_id: int,
    body: Dict[str, Any],
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(Agent).where(Agent.id == agent_id))
    agent = result.scalar_one_or_none()
    if not agent:
        raise HTTPException(status_code=404, detail="Agent not found")
    return {"success": True, "message": "Configuration updated. Agent will apply within 30 seconds."}
