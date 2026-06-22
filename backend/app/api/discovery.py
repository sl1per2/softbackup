from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy.ext.asyncio import AsyncSession
from pydantic import BaseModel
from typing import Optional, List
import subprocess
import socket

from app.core.database import get_db
from app.api.auth import get_current_user

router = APIRouter(prefix="/api/discovery", tags=["discovery"])


class DiscoveryRequest(BaseModel):
    subnet: str = "192.168.1.0/24"
    port: int = 9900
    timeout_ms: int = 500


class DiscoveredHost(BaseModel):
    ip: str
    hostname: str
    os_type: str
    has_agent: bool
    agent_version: Optional[str] = None
    port_open: bool


@router.post("/scan", response_model=List[DiscoveredHost])
async def scan_network(
    body: DiscoveryRequest,
    user=Depends(get_current_user),
):
    import ipaddress
    discovered = []
    try:
        network = ipaddress.ip_network(body.subnet, strict=False)
        for ip in list(network.hosts())[:256]:
            ip_str = str(ip)
            port_open = False
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(body.timeout_ms / 1000.0)
                result = sock.connect_ex((ip_str, body.port))
                port_open = (result == 0)
                sock.close()
            except Exception:
                pass

            hostname = ""
            try:
                hostname = socket.gethostbyaddr(ip_str)[0]
            except Exception:
                hostname = ip_str

            if port_open:
                discovered.append(DiscoveredHost(
                    ip=ip_str,
                    hostname=hostname,
                    os_type="linux",
                    has_agent=True,
                    agent_version="1.0.0",
                    port_open=True,
                ))
            else:
                discovered.append(DiscoveredHost(
                    ip=ip_str,
                    hostname=hostname,
                    os_type="unknown",
                    has_agent=False,
                    port_open=False,
                ))
    except ValueError:
        raise HTTPException(status_code=400, detail="Invalid subnet format")

    return discovered


@router.get("/subnets")
async def get_subnets(user=Depends(get_current_user)):
    return {"subnets": ["192.168.1.0/24", "10.0.0.0/24", "172.16.0.0/24"]}
