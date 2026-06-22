from contextlib import asynccontextmanager
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware
from app.core.config import settings
from app.core.database import engine, Base, async_session
from app.core.security import get_password_hash
from app.core.ws_manager import ws_manager as manager
from app.core.audit_middleware import AuditMiddleware
from app.core.rate_limit import RateLimitMiddleware
from app.api import auth, agents, storages, policies, jobs, agent_api, dashboard, traffic, zabbix, audit
from app.api import replication, rescue, metrics_export, ldap, siem, tape, storage_tiers, virtualization, dbms, mail, os_backup, directory, gfs, agents_download
from app.api import surebackup, backup_copy, tape_library as tape_lib, immutability, malware, dr, replication_v2, self_service, k8s, adhoc_backup, dirty_buffers, agent_control, discovery, agent_installer, config_editor, log_viewer
import asyncio


@asynccontextmanager
async def lifespan(app: FastAPI):
    # Startup
    async with engine.begin() as conn:
        await conn.run_sync(Base.metadata.create_all)

    # Seed admin user
    from sqlalchemy import select
    from app.models.models import User
    async with async_session() as db:
        result = await db.execute(select(User).where(User.username == "admin"))
        if not result.scalar_one_or_none():
            admin = User(
                username="admin",
                email="admin@obs.local",
                hashed_password=get_password_hash("admin123"),
                role="admin",
            )
            db.add(admin)
            await db.commit()

    yield

    # Shutdown
    await engine.dispose()


app = FastAPI(
    title="OBS Backup API",
    description="Corporate Backup System API",
    version="1.0.0",
    lifespan=lifespan,
)

# Middleware
app.add_middleware(CORSMiddleware, allow_origins=settings.CORS_ORIGINS, allow_credentials=True,
                   allow_methods=["*"], allow_headers=["*"])
app.add_middleware(AuditMiddleware)
app.add_middleware(RateLimitMiddleware)

# Routers
app.include_router(auth.router)
app.include_router(agents.router)
app.include_router(storages.router)
app.include_router(policies.router)
app.include_router(jobs.router)
app.include_router(agent_api.router)
app.include_router(dashboard.router)
app.include_router(traffic.router)
app.include_router(zabbix.router)
app.include_router(audit.router)
app.include_router(replication.router)
app.include_router(rescue.router)
app.include_router(metrics_export.router)
app.include_router(ldap.router)
app.include_router(siem.router)
app.include_router(tape.router)
app.include_router(storage_tiers.router)
app.include_router(virtualization.router)
app.include_router(dbms.router)
app.include_router(mail.router)
app.include_router(os_backup.router)
app.include_router(directory.router)
app.include_router(gfs.router)
app.include_router(agents_download.router)
app.include_router(agents_download.download_page_router)
app.include_router(surebackup.router)
app.include_router(backup_copy.router)
app.include_router(tape_lib.router)
app.include_router(immutability.router)
app.include_router(malware.router)
app.include_router(dr.router)
app.include_router(replication_v2.router)
app.include_router(self_service.router)
app.include_router(k8s.router)
app.include_router(adhoc_backup.router)
app.include_router(dirty_buffers.router)
app.include_router(agent_control.router)
app.include_router(discovery.router)
app.include_router(agent_installer.router)
app.include_router(config_editor.router)
app.include_router(log_viewer.router)


@app.get("/health")
async def health():
    return {"status": "ok", "service": "obs-backup"}


@app.websocket("/ws/dashboard")
async def dashboard_ws(websocket: WebSocket):
    await manager.connect(websocket, "dashboard")
    try:
        while True:
            await websocket.receive_text()
    except WebSocketDisconnect:
        manager.disconnect(websocket, "dashboard")


@app.websocket("/ws/jobs")
async def jobs_ws(websocket: WebSocket):
    await manager.connect(websocket, "jobs")
    try:
        while True:
            await websocket.receive_text()
    except WebSocketDisconnect:
        manager.disconnect(websocket, "jobs")


@app.websocket("/ws/cdp")
async def cdp_ws(websocket: WebSocket):
    await manager.connect(websocket, "cdp")
    try:
        while True:
            await websocket.receive_text()
    except WebSocketDisconnect:
        manager.disconnect(websocket, "cdp")


@app.websocket("/ws/traffic")
async def traffic_ws(websocket: WebSocket):
    await manager.connect(websocket, "traffic")
    try:
        while True:
            await websocket.receive_text()
    except WebSocketDisconnect:
        manager.disconnect(websocket, "traffic")


@app.websocket("/ws/agent-logs/{agent_id}")
async def agent_logs_ws(websocket: WebSocket, agent_id: int):
    await manager.connect(websocket, f"agent-logs-{agent_id}")
    try:
        while True:
            data = await websocket.receive_text()
            if data == "subscribe":
                pass
    except WebSocketDisconnect:
        manager.disconnect(websocket, f"agent-logs-{agent_id}")


@app.websocket("/ws/agent-commands")
async def agent_commands_ws(websocket: WebSocket):
    await manager.connect(websocket, "agent-commands")
    try:
        while True:
            data = await websocket.receive_text()
            import json
            cmd = json.loads(data)
            agent_id = cmd.get("agent_id")
            command = cmd.get("command")
            await websocket.send_json({"status": "sent", "command": command, "agent_id": agent_id})
    except WebSocketDisconnect:
        manager.disconnect(websocket, "agent-commands")


@app.websocket("/ws/install-progress/{job_id}")
async def install_progress_ws(websocket: WebSocket, job_id: str):
    await manager.connect(websocket, f"install-{job_id}")
    try:
        while True:
            await websocket.receive_text()
    except WebSocketDisconnect:
        manager.disconnect(websocket, f"install-{job_id}")
