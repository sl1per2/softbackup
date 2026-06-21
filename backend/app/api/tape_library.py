from fastapi import APIRouter, Depends
from pydantic import BaseModel
from typing import Optional, List
from app.core.security import get_current_user

router = APIRouter(prefix="/api/tape-lib", tags=["tape-library"])


class TapeMoveRequest(BaseModel):
    barcode: str
    target_pool: str


class TapeWriteRequest(BaseModel):
    job_id: str
    tape_barcodes: List[str]


@router.get("/library")
async def get_library(_user: dict = Depends(get_current_user)):
    return {
        "vendor": "LTO Library",
        "model": "LTO-9",
        "serial": "VOL001",
        "slots_count": 24,
        "drives_count": 2,
        "device_path": "/dev/sg2",
        "online": True,
    }


@router.post("/inventory")
async def inventory(_user: dict = Depends(get_current_user)):
    return {"detail": "Inventory scan started", "tapes_found": 12}


@router.get("/tapes")
async def list_tapes(pool: Optional[str] = None, _user: dict = Depends(get_current_user)):
    tapes = [
        {"barcode": f"LTO9-{i+1}", "pool": "active" if i < 4 else "archive" if i < 8 else "free",
         "capacity_bytes": 12 * 1024**3, "used_bytes": int(12 * 1024**3 * 0.6) if i < 8 else 0,
         "write_count": 15 + i * 2 if i < 8 else 0, "location": f"slot_{i}"}
        for i in range(12)
    ]
    if pool:
        tapes = [t for t in tapes if t["pool"] == pool]
    return {"tapes": tapes}


@router.post("/write")
async def write_to_tape(data: TapeWriteRequest, _user: dict = Depends(get_current_user)):
    return {"detail": "Tape write started", "job_id": data.job_id, "tapes": data.tape_barcodes}


@router.post("/read")
async def read_from_tape(barcode: str, output_dir: str = "/tmp/tape-restore", _user: dict = Depends(get_current_user)):
    return {"detail": f"Reading from tape {barcode}", "output": output_dir}


@router.post("/format")
async def format_tape(barcode: str, _user: dict = Depends(get_current_user)):
    return {"detail": f"Tape {barcode} formatted"}


@router.post("/mount")
async def mount_tape(barcode: str, drive_index: int = 0, _user: dict = Depends(get_current_user)):
    return {"detail": f"Tape {barcode} mounted to drive {drive_index}"}


@router.post("/unmount")
async def unmount_tape(drive_index: int = 0, _user: dict = Depends(get_current_user)):
    return {"detail": f"Drive {drive_index} unmounted"}


@router.put("/tapes/pool")
async def move_tape_pool(data: TapeMoveRequest, _user: dict = Depends(get_current_user)):
    return {"detail": f"Tape {data.barcode} moved to {data.target_pool}"}
