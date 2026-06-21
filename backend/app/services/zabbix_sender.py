from app.services import celery_app
import asyncio


@celery_app.task(name="app.services.zabbix_sender.send_metrics_to_zabbix")
def send_metrics_to_zabbix():
    from sqlalchemy.ext.asyncio import create_async_engine, async_sessionmaker
    from sqlalchemy import select
    from app.models.models import ZabbixConfig, Agent, BackupJob, CDPSession
    from app.core.config import settings

    async def _run():
        engine = create_async_engine(settings.DATABASE_URL)
        async_session = async_sessionmaker(engine, expire_on_commit=False)

        async with async_session() as db:
            result = await db.execute(select(ZabbixConfig).where(ZabbixConfig.enabled == True))
            config = result.scalar_one_or_none()
            if not config:
                await engine.dispose()
                return

            agents = (await db.execute(select(Agent))).scalars().all()
            for agent in agents:
                metrics = {
                    "agent.status": 1 if agent.status == "online" else 0,
                    "agent.cache_hit_ratio": agent.cache_hit_ratio,
                    "agent.cache_size_bytes": agent.cache_size_bytes,
                }
                print(f"Sending metrics for {agent.hostname}: {metrics}")

            cdp_sessions = (await db.execute(
                select(CDPSession).where(CDPSession.status == "active")
            )).scalars().all()
            for session in cdp_sessions:
                metrics = {
                    "cdp.lag_ms": session.current_lag_ms,
                    "cdp.iops": session.iops,
                    "cdp.throughput_mbps": session.throughput_mbps,
                }
                print(f"Sending CDP metrics for session {session.id}: {metrics}")

            from datetime import datetime
            config.last_sync_at = datetime.utcnow()
            await db.commit()

        await engine.dispose()

    asyncio.run(_run())
