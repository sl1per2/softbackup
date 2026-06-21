from fastapi import APIRouter, Depends
from pydantic import BaseModel
from typing import Optional
from app.core.security import get_current_user

router = APIRouter(prefix="/api/storage-tiers", tags=["storage-tiers"])


class TierConfig(BaseModel):
    type: str  # hot, warm, cold, archive
    storage_path: str
    max_size_bytes: int = 1024**4
    min_access_days: int = 0
    encrypted: bool = False
    compression_level: int = 1


_tiers = [
    {"type": "hot", "storage_path": "/var/lib/obs/hot", "max_size_bytes": 100 * 1024**3,
     "used_bytes": 35 * 1024**3, "file_count": 150, "min_access_days": 0},
    {"type": "warm", "storage_path": "/var/lib/obs/warm", "max_size_bytes": 500 * 1024**3,
     "used_bytes": 280 * 1024**3, "file_count": 890, "min_access_days": 7},
    {"type": "cold", "storage_path": "/var/lib/obs/cold", "max_size_bytes": 2 * 1024**4,
     "used_bytes": 1.2 * 1024**4, "file_count": 2340, "min_access_days": 30},
    {"type": "archive", "storage_path": "/var/lib/obs/archive", "max_size_bytes": 10 * 1024**4,
     "used_bytes": 4.5 * 1024**4, "file_count": 5600, "min_access_days": 90},
]


@router.get("/stats")
async def get_tier_stats(_user: dict = Depends(get_current_user)):
    result = []
    for t in _tiers:
        util = t["used_bytes"] / t["max_size_bytes"] * 100 if t["max_size_bytes"] > 0 else 0
        result.append({**t, "utilization_percent": round(util, 1)})
    return result


@router.post("/add")
async def add_tier(data: TierConfig, _user: dict = Depends(get_current_user)):
    _tiers.append({**data.model_dump(), "used_bytes": 0, "file_count": 0, "utilization_percent": 0})
    return {"detail": f"Tier {data.type} added"}


@router.post("/run-tiering")
async def run_tiering(_user: dict = Depends(get_current_user)):
    return {"detail": "Tiering job started", "files_moved": 0}


@router.post("/migrate")
async def migrate_file(source_path: str, target_tier: str, _user: dict = Depends(get_current_user)):
    return {"detail": f"File {source_path} migrated to {target_tier}"}
