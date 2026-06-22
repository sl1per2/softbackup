from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy import select
from pydantic import BaseModel
from typing import Optional, List
import asyncio
import json
import subprocess

from app.core.database import get_db
from app.models.models import Agent
from app.api.auth import get_current_user

router = APIRouter(prefix="/api/agents", tags=["agent-control"])


class QuickBackupRequest(BaseModel):
    backup_type: str = "full"
    source_paths: List[str] = []
    storage_id: Optional[int] = None
    policy_id: Optional[int] = None


class AgentConfigUpdate(BaseModel):
    log_level: Optional[str] = None
    heartbeat_interval: Optional[int] = None
    cache_size_mb: Optional[int] = None
    max_concurrent_jobs: Optional[int] = None
    bandwidth_limit_kbps: Optional[int] = None


class SshDeployRequest(BaseModel):
    host: str
    port: int = 22
    username: str
    password: Optional[str] = None
    private_key: Optional[str] = None
    agent_version: str = "latest"
    server_url: str = ""
    auth_token: str = ""
    os_type: str = "linux"


class WinrmDeployRequest(BaseModel):
    host: str
    port: int = 5985
    username: str
    password: str
    use_ssl: bool = False
    agent_version: str = "latest"
    server_url: str = ""
    auth_token: str = ""


def _send_agent_command(agent_id: int, command: str, params: dict = None) -> dict:
    try:
        result = asyncio.get_event_loop().run_until_complete(_send_command_async(agent_id, command, params))
        return result
    except Exception as e:
        return {"success": False, "error": str(e)}


async def _send_command_async(agent_id: int, command: str, params: dict = None) -> dict:
    import httpx
    try:
        from app.core.database import async_session
        async with async_session() as db:
            result = await db.execute(select(Agent).where(Agent.id == agent_id))
            agent = result.scalar_one_or_none()
            if not agent:
                return {"success": False, "error": "Agent not found"}
            url = f"http://{agent.ip}:9900/api/command"
            payload = {"command": command, "params": params or {}}
            async with httpx.AsyncClient(timeout=10) as client:
                resp = await client.post(url, json=payload)
                return resp.json()
    except Exception as e:
        return {"success": False, "error": str(e)}


@router.get("/{agent_id}/metrics")
async def get_agent_metrics(
    agent_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(Agent).where(Agent.id == agent_id))
    agent = result.scalar_one_or_none()
    if not agent:
        raise HTTPException(status_code=404, detail="Agent not found")
    return {
        "agent_id": agent.id,
        "hostname": agent.hostname,
        "ip": agent.ip,
        "status": agent.status,
        "cpu_usage": 0.0,
        "memory_usage_mb": 0,
        "disk_usage_percent": 0,
        "uptime_seconds": 0,
        "active_jobs": 0,
    }


@router.post("/{agent_id}/command/start-backup")
async def start_backup(
    agent_id: int,
    body: QuickBackupRequest,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(Agent).where(Agent.id == agent_id))
    agent = result.scalar_one_or_none()
    if not agent:
        raise HTTPException(status_code=404, detail="Agent not found")

    job_id = f"adhoc_{agent_id}_{int(asyncio.get_event_loop().time())}"
    cmd_result = _send_agent_command(agent_id, "start_backup", {
        "job_id": job_id,
        "backup_type": body.backup_type,
        "source_paths": body.source_paths,
        "storage_id": body.storage_id,
        "policy_id": body.policy_id,
    })

    from app.models.models import BackupJob
    job = BackupJob(
        agent_id=agent.id,
        job_type=body.backup_type,
        status="running",
    )
    db.add(job)
    await db.commit()

    return {"success": True, "job_id": job_id, "message": f"Backup started: {body.backup_type}"}


@router.post("/{agent_id}/command/stop-jobs")
async def stop_all_jobs(
    agent_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(Agent).where(Agent.id == agent_id))
    agent = result.scalar_one_or_none()
    if not agent:
        raise HTTPException(status_code=404, detail="Agent not found")

    cmd_result = _send_agent_command(agent_id, "stop_all_jobs")
    return {"success": True, "message": "Stop command sent to all jobs"}


@router.post("/{agent_id}/command/restart")
async def restart_agent(
    agent_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(Agent).where(Agent.id == agent_id))
    agent = result.scalar_one_or_none()
    if not agent:
        raise HTTPException(status_code=404, detail="Agent not found")

    cmd_result = _send_agent_command(agent_id, "restart")
    return {"success": True, "message": "Restart command sent"}


@router.post("/{agent_id}/command/update")
async def update_agent(
    agent_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(Agent).where(Agent.id == agent_id))
    agent = result.scalar_one_or_none()
    if not agent:
        raise HTTPException(status_code=404, detail="Agent not found")

    cmd_result = _send_agent_command(agent_id, "self_update")
    return {"success": True, "message": "Update command sent"}


@router.post("/{agent_id}/command/flush-dirty-buffers")
async def flush_dirty_buffers(
    agent_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(Agent).where(Agent.id == agent_id))
    agent = result.scalar_one_or_none()
    if not agent:
        raise HTTPException(status_code=404, detail="Agent not found")

    cmd_result = _send_agent_command(agent_id, "flush_dirty_buffers")
    return {"success": True, "message": "Dirty buffer flush command sent"}


@router.post("/{agent_id}/command/clear-cache")
async def clear_chunk_cache(
    agent_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(Agent).where(Agent.id == agent_id))
    agent = result.scalar_one_or_none()
    if not agent:
        raise HTTPException(status_code=404, detail="Agent not found")

    cmd_result = _send_agent_command(agent_id, "clear_cache")
    return {"success": True, "message": "Cache clear command sent"}


@router.get("/{agent_id}/config")
async def get_agent_config(
    agent_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(Agent).where(Agent.id == agent_id))
    agent = result.scalar_one_or_none()
    if not agent:
        raise HTTPException(status_code=404, detail="Agent not found")

    cmd_result = _send_agent_command(agent_id, "get_config")
    if cmd_result.get("success"):
        return cmd_result.get("config", {})
    return {
        "server_url": "",
        "log_level": "info",
        "heartbeat_interval_sec": 30,
        "cache_size_mb": 1024,
        "max_concurrent_jobs": 2,
        "bandwidth_limit_kbps": 0,
    }


@router.put("/{agent_id}/config")
async def update_agent_config(
    agent_id: int,
    body: AgentConfigUpdate,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(Agent).where(Agent.id == agent_id))
    agent = result.scalar_one_or_none()
    if not agent:
        raise HTTPException(status_code=404, detail="Agent not found")

    config_update = body.model_dump(exclude_unset=True)
    cmd_result = _send_agent_command(agent_id, "update_config", config_update)
    return {"success": True, "message": "Configuration update sent. Agent will apply changes within 30 seconds."}


@router.post("/{agent_id}/command/apply-config")
async def apply_config_now(
    agent_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(Agent).where(Agent.id == agent_id))
    agent = result.scalar_one_or_none()
    if not agent:
        raise HTTPException(status_code=404, detail="Agent not found")

    cmd_result = _send_agent_command(agent_id, "restart")
    return {"success": True, "message": "Agent restarted to apply configuration"}


@router.get("/{agent_id}/logs")
async def get_agent_logs(
    agent_id: int,
    level: Optional[str] = None,
    search: Optional[str] = None,
    tail: int = Query(200, ge=1, le=5000),
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    result = await db.execute(select(Agent).where(Agent.id == agent_id))
    agent = result.scalar_one_or_none()
    if not agent:
        raise HTTPException(status_code=404, detail="Agent not found")

    cmd_result = _send_agent_command(agent_id, "get_logs", {
        "level": level, "search": search, "tail": tail,
    })
    if cmd_result.get("success"):
        return {"logs": cmd_result.get("logs", [])}
    return {"logs": [], "error": cmd_result.get("error", "Cannot reach agent")}


@router.get("/{agent_id}/logs/download")
async def download_agent_logs(
    agent_id: int,
    db: AsyncSession = Depends(get_db),
    user=Depends(get_current_user),
):
    from fastapi.responses import PlainTextResponse
    result = await db.execute(select(Agent).where(Agent.id == agent_id))
    agent = result.scalar_one_or_none()
    if not agent:
        raise HTTPException(status_code=404, detail="Agent not found")

    cmd_result = _send_agent_command(agent_id, "get_logs", {"tail": 5000})
    logs = cmd_result.get("logs", [])
    content = "\n".join(logs) if logs else "No logs available"
    return PlainTextResponse(content, filename=f"{agent.hostname}_logs.txt")


@router.post("/deploy/ssh")
async def deploy_agent_ssh(
    body: SshDeployRequest,
    user=Depends(get_current_user),
):
    try:
        import paramiko
        ssh = paramiko.SSHClient()
        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())

        connect_kwargs = {"hostname": body.host, "port": body.port, "username": body.username}
        if body.private_key:
            from io import StringIO
            pkey = paramiko.RSAKey.from_private_key(StringIO(body.private_key))
            connect_kwargs["pkey"] = pkey
        elif body.password:
            connect_kwargs["password"] = body.password

        ssh.connect(**connect_kwargs, timeout=15)

        is_linux = body.os_type in ("linux", "macos")
        if is_linux:
            install_url = "https://get.obs-backup.example.com/install.sh"
            cmd = f"curl -fsSL {install_url} | sudo bash -s -- --server {body.server_url} --token {body.auth_token}"
        else:
            return {"success": False, "error": "Use WinRM for Windows agents"}

        stdin, stdout, stderr = ssh.exec_command(cmd, timeout=120)
        exit_code = stdout.channel.recv_exit_status()
        out = stdout.read().decode()
        err = stderr.read().decode()
        ssh.close()

        if exit_code == 0:
            return {"success": True, "message": f"Agent deployed to {body.host}", "output": out}
        else:
            return {"success": False, "error": f"Install failed (exit {exit_code})", "output": err}
    except ImportError:
        return {"success": False, "error": "paramiko not installed. Run: pip install paramiko"}
    except Exception as e:
        return {"success": False, "error": str(e)}


@router.post("/deploy/winrm")
async def deploy_agent_winrm(
    body: WinrmDeployRequest,
    user=Depends(get_current_user),
):
    try:
        import winrm
        protocol = "https" if body.use_ssl else "http"
        session = winrm.Session(
            f"{protocol}://{body.host}:{body.port}",
            auth=(body.username, body.password),
        )
        install_url = "https://get.obs-backup.example.com/install.ps1"
        ps_cmd = f"irm {install_url} | iex -Args '--server','{body.server_url}','--token','{body.auth_token}'"
        result = session.run_ps(ps_cmd)

        if result.status_code == 0:
            return {"success": True, "message": f"Agent deployed to {body.host}"}
        else:
            return {"success": False, "error": result.std_err.decode(), "output": result.std_out.decode()}
    except ImportError:
        return {"success": False, "error": "pywinrm not installed. Run: pip install pywinrm"}
    except Exception as e:
        return {"success": False, "error": str(e)}
