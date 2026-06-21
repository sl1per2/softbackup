from fastapi import APIRouter, Depends
from pydantic import BaseModel
from typing import Optional
from app.core.security import get_current_user

router = APIRouter(prefix="/api/tape", tags=["tape"])


class TapeInfo(BaseModel):
    barcode: str
    pool: str
    capacity_bytes: int
    used_bytes: int
    status: str
    slot: int


class TapeDriveInfo(BaseModel):
    device_path: str
    online: bool
    current_barcode: str = ""
    status: str


@router.get("/library")
async def get_tape_library(_user: dict = Depends(get_current_user)):
    return {
        "name": "Tape Library",
        "type": "tape_library",
        "total_slots": 24,
        "used_slots": 8,
        "total_drives": 2,
        "online_drives": 2,
        "tapes": [
            {"barcode": f"TAPE{i+1}", "pool": "daily" if i < 4 else "weekly" if i < 8 else "monthly",
             "capacity_bytes": 12 * 1024**3, "used_bytes": int(12 * 1024**3 * 0.7),
             "status": "online" if i < 8 else "offline", "slot": i}
            for i in range(24)
        ],
        "drives": [
            {"device_path": f"/dev/nst{i}", "online": True, "current_barcode": "", "status": "idle"}
            for i in range(2)
        ],
    }


@router.post("/mount")
async def mount_tape(barcode: str, drive_index: int = 0, _user: dict = Depends(get_current_user)):
    return {"detail": f"Tape {barcode} mounted to drive {drive_index}"}


@router.post("/unmount")
async def unmount_tape(drive_index: int = 0, _user: dict = Depends(get_current_user)):
    return {"detail": f"Tape unmounted from drive {drive_index}"}


@router.post("/inventory")
async def inventory(_user: dict = Depends(get_current_user)):
    return {"detail": "Inventory scan complete", "tapes_found": 8}


@router.post("/eject")
async def eject_tape(barcode: str, _user: dict = Depends(get_current_user)):
    return {"detail": f"Tape {barcode} ejected"}


@router.post("/move")
async def move_tape(source_slot: int, dest_slot: int, _user: dict = Depends(get_current_user)):
    return {"detail": f"Tape moved from slot {source_slot} to {dest_slot}"}
