from app.services import celery_app
import asyncio


@celery_app.task(name="app.services.cdp_monitor.monitor_cdp_sessions")
def monitor_cdp_sessions():
    from sqlalchemy.ext.asyncio import create_async_engine, async_sessionmaker
    from sqlalchemy import select
    from app.models.models import CDPSession, BackupPolicy
    from app.core.config import settings

    async def _run():
        engine = create_async_engine(settings.DATABASE_URL)
        async_session = async_sessionmaker(engine, expire_on_commit=False)

        async with async_session() as db:
            sessions = (await db.execute(
                select(CDPSession).where(CDPSession.status == "active")
            )).scalars().all()

            for session in sessions:
                if session.policy_id:
                    policy_result = await db.execute(
                        select(BackupPolicy).where(BackupPolicy.id == session.policy_id)
                    )
                    policy = policy_result.scalar_one_or_none()
                    if policy and session.current_lag_ms > policy.cdp_max_latency_ms:
                        print(f"CDP LAG WARNING: session {session.id} "
                              f"lag={session.current_lag_ms}ms > max={policy.cdp_max_latency_ms}ms")

        await engine.dispose()

    asyncio.run(_run())
