from fastapi import APIRouter, Depends
from pydantic import BaseModel
from typing import Optional
from app.core.security import get_current_user

router = APIRouter(prefix="/api/os-backup", tags=["os-backup"])


class FileBackupRequest(BaseModel):
    source_paths: list[str]
    exclude_patterns: list[str] = []
    output_path: str
    compress: bool = True
    encrypt: bool = False
    incremental: bool = False
    preserve_permissions: bool = True


class LvmSnapshotRequest(BaseModel):
    vg_name: str
    lv_name: str
    snap_name: str
    size: str = "5G"


@router.get("/supported-filesystems")
async def supported_filesystems(_user: dict = Depends(get_current_user)):
    return {
        "linux": [
            {"fs": "ext4", "snapshots": True, "reflink": False, "cow": False},
            {"fs": "ext3", "snapshots": True, "reflink": False, "cow": False},
            {"fs": "ext2", "snapshots": False, "reflink": False, "cow": False},
            {"fs": "xfs", "snapshots": True, "reflink": True, "cow": False},
            {"fs": "zfs", "snapshots": True, "reflink": True, "cow": True},
            {"fs": "btrfs", "snapshots": True, "reflink": True, "cow": True},
            {"fs": "fat32", "snapshots": False, "reflink": False, "cow": False},
            {"fs": "lvm", "snapshots": True, "reflink": False, "cow": False},
        ],
        "windows": [
            {"fs": "ntfs", "snapshots": True, "vss": True, "reFs": True},
            {"fs": "fat32", "snapshots": False, "vss": False},
            {"fs": "refs", "snapshots": True, "block_clone": True, "vss": True},
        ],
    }


@router.post("/backup")
async def start_file_backup(data: FileBackupRequest, _user: dict = Depends(get_current_user)):
    return {"detail": "File backup started", "sources": len(data.source_paths)}


@router.post("/restore")
async def restore_files(data: dict, _user: dict = Depends(get_current_user)):
    return {"detail": "File restore started"}


@router.get("/lvm/snapshots")
async def list_lvm_snapshots(vg_name: str, lv_name: str, _user: dict = Depends(get_current_user)):
    return {"snapshots": []}


@router.post("/lvm/snapshot")
async def create_lvm_snapshot(data: LvmSnapshotRequest, _user: dict = Depends(get_current_user)):
    return {"detail": f"LVM snapshot {data.snap_name} created"}


@router.delete("/lvm/snapshot")
async def remove_lvm_snapshot(vg_name: str, snap_name: str, _user: dict = Depends(get_current_user)):
    return {"detail": f"LVM snapshot {snap_name} removed"}


@router.get("/zfs/snapshots")
async def list_zfs_snapshots(dataset: str, _user: dict = Depends(get_current_user)):
    return {"snapshots": []}


@router.post("/zfs/snapshot")
async def create_zfs_snapshot(dataset: str, snap_name: str, _user: dict = Depends(get_current_user)):
    return {"detail": f"ZFS snapshot {snap_name} created"}


@router.get("/btrfs/snapshots")
async def list_btrfs_snapshots(subvolume: str, _user: dict = Depends(get_current_user)):
    return {"snapshots": []}
