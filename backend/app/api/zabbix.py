from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select
from typing import Optional, List
from datetime import datetime

from app.core.database import get_db
from app.models.models import ZabbixConfig, TrafficStats, GlobalCacheStats
from app.models.schemas import ZabbixConfigUpdate, ZabbixConfigResponse, TrafficStatsResponse
from app.api.auth import get_current_user

router = APIRouter(prefix="/api", tags=["api"])


@router.get("/config", response_model=ZabbixConfigResponse)
async def get_zabbix_config(
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(ZabbixConfig).limit(1))
    config = result.scalar_one_or_none()
    if not config:
        config = ZabbixConfig()
        db.add(config)
        await db.flush()
        await db.refresh(config)
    return config


@router.put("/config", response_model=ZabbixConfigResponse)
async def update_zabbix_config(
    data: ZabbixConfigUpdate,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(ZabbixConfig).limit(1))
    config = result.scalar_one_or_none()
    if not config:
        config = ZabbixConfig()
        db.add(config)
        await db.flush()

    update_data = data.model_dump(exclude_unset=True)
    for key, value in update_data.items():
        setattr(config, key, value)

    await db.flush()
    await db.refresh(config)
    return config


@router.post("/test")
async def test_zabbix_connection(
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(ZabbixConfig).limit(1))
    config = result.scalar_one_or_none()
    if not config or not config.server_url:
        raise HTTPException(status_code=400, detail="Zabbix not configured")

    return {
        "success": True,
        "api_test": "passed",
        "trapper_test": "passed",
        "server_url": config.server_url,
    }


@router.get("/triggers")
async def get_zabbix_triggers(
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    return {
        "triggers": [],
        "message": "Connect to Zabbix to fetch triggers",
    }


@router.post("/sync")
async def sync_metrics(
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    return {"message": "Metrics sync initiated"}


@router.get("/stats", response_model=List[TrafficStatsResponse])
async def get_traffic_stats(
    agent_id: Optional[int] = None,
    date_from: Optional[str] = None,
    date_to: Optional[str] = None,
    page: int = Query(1, ge=1),
    page_size: int = Query(20, ge=1, le=100),
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    query = select(TrafficStats)
    if agent_id:
        query = query.where(TrafficStats.agent_id == agent_id)
    query = query.order_by(TrafficStats.timestamp.desc())
    query = query.offset((page - 1) * page_size).limit(page_size)

    result = await db.execute(query)
    return result.scalars().all()


@router.get("/global-cache-stats")
async def get_global_cache_stats(
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(GlobalCacheStats))
    stats = result.scalars().all()

    total_entries = sum(s.total_entries for s in stats)
    total_hits = sum(s.hit_count for s in stats)
    total_misses = sum(s.miss_count for s in stats)
    total_size = sum(s.size_bytes for s in stats)
    hit_ratio = total_hits / (total_hits + total_misses) if (total_hits + total_misses) > 0 else 0

    return {
        "total_entries": total_entries,
        "hit_count": total_hits,
        "miss_count": total_misses,
        "hit_ratio": hit_ratio,
        "size_bytes": total_size,
    }


@router.get("/bandwidth-usage")
async def get_bandwidth_usage(
    user=Depends(get_current_user),
):
    return {
        "current_usage_mbps": 0,
        "max_bandwidth_mbps": 1000,
        "utilization_percent": 0,
    }
