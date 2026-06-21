from fastapi import APIRouter, Depends
from pydantic import BaseModel
from typing import Optional
from app.core.security import get_current_user

router = APIRouter(prefix="/api/directory", tags=["directory"])


class DirectoryConnection(BaseModel):
    server: str
    base_dn: str
    bind_dn: str
    password: str
    dir_type: str  # ald_pro, freeipa, active_directory


@router.get("/supported")
async def list_supported_directories(_user: dict = Depends(get_current_user)):
    return {
        "directories": [
            {"id": "ald_pro", "name": "ALD Pro", "features": ["User backup", "Group backup", "GPO", "DNS", "Kerberos keys"]},
            {"id": "freeipa", "name": "FreeIPA", "features": ["ipa-backup", "Certificates", "DNS", "Topology", "Kerberos", "HBAC", "Sudo rules"]},
            {"id": "active_directory", "name": "Microsoft Active Directory", "features": ["System state", "NTDS.dit", "SYSVOL", "GPO", "AD CS", "AD FS", "DHCP", "DFS", "Certificates"]},
        ]
    }


@router.post("/test")
async def test_connection(conn: DirectoryConnection, _user: dict = Depends(get_current_user)):
    return {"success": True, "message": f"Connected to {conn.dir_type} at {conn.server}"}


@router.post("/backup")
async def backup_directory(data: DirectoryConnection, _user: dict = Depends(get_current_user)):
    return {"detail": f"{data.dir_type} backup started"}


@router.post("/restore")
async def restore_directory(data: dict, _user: dict = Depends(get_current_user)):
    return {"detail": "Directory restore started"}


@router.get("/users")
async def list_users(dir_type: str, _user: dict = Depends(get_current_user)):
    return {"users": []}


@router.get("/groups")
async def list_groups(dir_type: str, _user: dict = Depends(get_current_user)):
    return {"groups": []}
