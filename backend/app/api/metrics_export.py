from fastapi import APIRouter, Depends
from app.core.security import get_current_user

router = APIRouter(prefix="/api/metrics-export", tags=["metrics-export"])


@router.get("/prometheus")
async def prometheus_metrics():
    """Prometheus-compatible metrics endpoint (unauthenticated)."""
    from app.core.database import async_session
    from sqlalchemy import select, func
    from app.models.models import Agent, BackupJob, CDPSession, AgentStatus, JobStatus, CdpSessionStatus

    lines = []
    try:
        async with async_session() as db:
            # Agent metrics
            total = (await db.execute(select(func.count(Agent.id)))).scalar() or 0
            online = (await db.execute(select(func.count(Agent.id)).where(Agent.status == AgentStatus.online))).scalar() or 0
            lines.append(f'obs_agents_total {total}')
            lines.append(f'obs_agents_online {online}')

            # Job metrics
            running = (await db.execute(select(func.count(BackupJob.id)).where(BackupJob.status == JobStatus.running))).scalar() or 0
            completed = (await db.execute(select(func.count(BackupJob.id)).where(BackupJob.status == JobStatus.completed))).scalar() or 0
            failed = (await db.execute(select(func.count(BackupJob.id)).where(BackupJob.status == JobStatus.failed))).scalar() or 0
            lines.append(f'obs_jobs_running {running}')
            lines.append(f'obs_jobs_completed_total {completed}')
            lines.append(f'obs_jobs_failed_total {failed}')

            # CDP metrics
            cdp_active = (await db.execute(select(func.count(CDPSession.id)).where(CDPSession.status == CdpSessionStatus.active))).scalar() or 0
            max_lag = (await db.execute(select(func.max(CDPSession.current_lag_ms)).where(CDPSession.status == CdpSessionStatus.active))).scalar() or 0
            lines.append(f'obs_cdp_sessions_active {cdp_active}')
            lines.append(f'obs_cdp_max_lag_ms {max_lag}')
    except Exception:
        lines.append('obs_error 1')

    return "\n".join(lines), {"Content-Type": "text/plain; version=0.0.4; charset=utf-8"}


@router.get("/grafana-dashboard")
async def grafana_dashboard_config(_user: dict = Depends(get_current_user)):
    """Return Grafana dashboard JSON template."""
    return {
        "dashboard": {
            "title": "OBS Backup",
            "panels": [
                {"title": "Agents Online", "type": "stat", "targets": [{"expr": "obs_agents_online"}]},
                {"title": "Jobs Running", "type": "stat", "targets": [{"expr": "obs_jobs_running"}]},
                {"title": "CDP Lag", "type": "graph", "targets": [{"expr": "obs_cdp_max_lag_ms"}]},
                {"title": "Failed Jobs", "type": "stat", "targets": [{"expr": "obs_jobs_failed_total"}]},
            ]
        }
    }


@router.get("/config")
async def get_export_config(_user: dict = Depends(get_current_user)):
    return {
        "prometheus_enabled": True,
        "prometheus_endpoint": "/api/metrics-export/prometheus",
        "loki_enabled": False,
        "grafana_url": "",
    }


@router.put("/config")
async def update_export_config(data: dict, _user: dict = Depends(get_current_user)):
    return {"detail": "Metrics export config updated"}
