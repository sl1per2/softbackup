from fastapi import APIRouter, Depends, Query
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select
from typing import Optional, List

from app.core.database import get_db
from app.models.models import TrafficStats
from app.models.schemas import TrafficStatsResponse
from app.api.auth import get_current_user

router = APIRouter(prefix="/api", tags=["api"])


@router.get("/stats", response_model=List[TrafficStatsResponse])
async def list_traffic_stats(
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
    from app.models.models import GlobalCacheStats
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
