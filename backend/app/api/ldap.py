from fastapi import APIRouter, Depends, Query
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select, and_
from datetime import datetime
from typing import Optional, List
from pydantic import BaseModel
from app.core.database import get_db
from app.core.security import get_current_user
import json

router = APIRouter(prefix="/api/ldap", tags=["ldap"])


class LdapConfigResponse(BaseModel):
    id: int = 1
    server_url: str = ""
    base_dn: str = ""
    bind_dn: str = ""
    use_ssl: bool = True
    enabled: bool = False
    group_mapping: dict = {}
    sync_interval_seconds: int = 300

    class Config:
        from_attributes = True


class LdapConfigUpdate(BaseModel):
    server_url: Optional[str] = None
    base_dn: Optional[str] = None
    bind_dn: Optional[str] = None
    bind_password: Optional[str] = None
    use_ssl: Optional[bool] = None
    enabled: Optional[bool] = None
    group_mapping: Optional[dict] = None
    sync_interval_seconds: Optional[int] = None


# In-memory config store (in production: DB table)
_ldap_config = {
    "server_url": "",
    "base_dn": "",
    "bind_dn": "",
    "use_ssl": True,
    "enabled": False,
    "group_mapping": {
        "CN=BackupAdmins,OU=Groups": "admin",
        "CN=BackupOperators,OU=Groups": "operator",
        "CN=BackupViewers,OU=Groups": "viewer",
    },
    "sync_interval_seconds": 300,
}


@router.get("/config")
async def get_ldap_config(_user: dict = Depends(get_current_user)):
    return _ldap_config


@router.put("/config")
async def update_ldap_config(data: LdapConfigUpdate, _user: dict = Depends(get_current_user)):
    for key, value in data.model_dump(exclude_unset=True).items():
        if key != "bind_password":
            _ldap_config[key] = value
    return {"detail": "LDAP config updated"}


@router.post("/test")
async def test_ldap_connection(_user: dict = Depends(get_current_user)):
    if not _ldap_config.get("server_url"):
        return {"success": False, "message": "LDAP server not configured"}
    # In production: actual LDAP bind test
    return {"success": True, "message": "Connection test (simulated)", "users_found": 0}


@router.post("/sync")
async def sync_ldap_users(_user: dict = Depends(get_current_user)):
    # In production: LDAP search + create/update local users
    return {"detail": "LDAP sync triggered", "users_synced": 0}


@router.get("/users")
async def list_ldap_users(_user: dict = Depends(get_current_user)):
    return {"users": []}
