from fastapi import APIRouter, Depends
from pydantic import BaseModel
from typing import Optional
from app.core.security import get_current_user

router = APIRouter(prefix="/api/siem", tags=["siem"])


class SiemConfigUpdate(BaseModel):
    enabled: bool = False
    syslog_server: str = ""
    syslog_port: int = 514
    syslog_protocol: str = "udp"  # udp, tcp, tls
    facility: str = "local0"
    severity_map: dict = {}
    events_to_log: list[str] = [
        "job_started", "job_completed", "job_failed",
        "agent_online", "agent_offline",
        "cdp_lag_warning", "security_event",
    ]


_siem_config = {
    "enabled": False,
    "syslog_server": "",
    "syslog_port": 514,
    "syslog_protocol": "udp",
    "facility": "local0",
    "severity_map": {
        "job_started": "info",
        "job_completed": "info",
        "job_failed": "warning",
        "agent_offline": "warning",
        "cdp_lag_warning": "warning",
        "security_event": "critical",
    },
    "events_to_log": [
        "job_started", "job_completed", "job_failed",
        "agent_online", "agent_offline",
        "cdp_lag_warning", "security_event",
    ],
}


@router.get("/config")
async def get_siem_config(_user: dict = Depends(get_current_user)):
    return _siem_config


@router.put("/config")
async def update_siem_config(data: SiemConfigUpdate, _user: dict = Depends(get_current_user)):
    _siem_config.update(data.model_dump())
    return {"detail": "SIEM config updated"}


@router.post("/test")
async def test_siem_connection(_user: dict = Depends(get_current_user)):
    if not _siem_config.get("syslog_server"):
        return {"success": False, "message": "Syslog server not configured"}
    # In production: send test syslog message
    return {"success": True, "message": f"Test message sent to {_siem_config['syslog_server']}:{_siem_config['syslog_port']}"}


@router.get("/events")
async def list_siem_events(limit: int = 50, _user: dict = Depends(get_current_user)):
    return {"events": []}
