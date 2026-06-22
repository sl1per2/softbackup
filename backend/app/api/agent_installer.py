from fastapi import APIRouter, Depends, HTTPException, WebSocket, WebSocketDisconnect
from pydantic import BaseModel
from typing import Optional
import asyncio
import json

from app.api.auth import get_current_user

router = APIRouter(prefix="/api/agent-installer", tags=["agent-installer"])


class InstallRequest(BaseModel):
    host: str
    port: int = 22
    username: str
    password: Optional[str] = None
    private_key: Optional[str] = None
    os_type: str = "linux"
    server_url: str = ""
    auth_token: str = ""
    agent_version: str = "latest"


@router.post("/install")
async def install_agent(
    body: InstallRequest,
    user=Depends(get_current_user),
):
    if body.os_type == "windows":
        return {"success": True, "message": "Use WinRM endpoint for Windows", "job_id": "winrm_needed"}
    return {"success": True, "message": f"Install queued for {body.host}", "job_id": f"install_{body.host}"}


@router.post("/check")
async def check_agent(
    host: str,
    port: int = 9900,
    user=Depends(get_current_user),
):
    import socket
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(3)
        result = sock.connect_ex((host, port))
        sock.close()
        return {"has_agent": result == 0, "host": host, "port": port}
    except Exception:
        return {"has_agent": False, "host": host, "port": port}
