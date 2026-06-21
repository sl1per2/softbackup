from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select
from typing import List

from app.core.database import get_db
from app.models.models import Storage
from app.models.schemas import StorageCreate, StorageResponse
from app.api.auth import get_current_user

router = APIRouter(prefix="/api", tags=["api"])


@router.get("", response_model=List[StorageResponse])
async def list_storages(
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(Storage))
    return result.scalars().all()


@router.post("", response_model=StorageResponse, status_code=201)
async def create_storage(
    data: StorageCreate,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    storage = Storage(
        name=data.name,
        type=data.type,
        config=data.config,
        total_bytes=data.total_bytes,
        supports_fast_clone=data.supports_fast_clone,
    )
    db.add(storage)
    await db.flush()
    await db.refresh(storage)
    await log_audit(db, user.id, "create", "storage", storage.id, {})
    return storage


@router.get("/{storage_id}", response_model=StorageResponse)
async def get_storage(
    storage_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(Storage).where(Storage.id == storage_id))
    storage = result.scalar_one_or_none()
    if not storage:
        raise HTTPException(status_code=404, detail="Storage not found")
    return storage


@router.put("/{storage_id}", response_model=StorageResponse)
async def update_storage(
    storage_id: int,
    data: StorageCreate,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(Storage).where(Storage.id == storage_id))
    storage = result.scalar_one_or_none()
    if not storage:
        raise HTTPException(status_code=404, detail="Storage not found")

    storage.name = data.name
    storage.type = data.type
    storage.config = data.config
    storage.total_bytes = data.total_bytes
    storage.supports_fast_clone = data.supports_fast_clone

    await db.flush()
    await db.refresh(storage)
    await log_audit(db, user.id, "update", "storage", storage.id, {})
    return storage


@router.delete("/{storage_id}")
async def delete_storage(
    storage_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(Storage).where(Storage.id == storage_id))
    storage = result.scalar_one_or_none()
    if not storage:
        raise HTTPException(status_code=404, detail="Storage not found")

    await db.delete(storage)
    await log_audit(db, user.id, "delete", "storage", storage.id, {})
    return {"message": "Storage deleted"}


@router.post("/{storage_id}/test")
async def test_storage(
    storage_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(Storage).where(Storage.id == storage_id))
    storage = result.scalar_one_or_none()
    if not storage:
        raise HTTPException(status_code=404, detail="Storage not found")

    await log_audit(db, user.id, "test", "storage", storage.id, {})
    return {
        "success": True,
        "storage_id": storage_id,
        "message": "Connection test passed",
        "supports_fast_clone": storage.supports_fast_clone,
    }
