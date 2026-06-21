from app.services import celery_app


@celery_app.task(bind=True, name="app.services.scheduler.run_scheduled_jobs")
def run_scheduled_jobs(self):
    from croniter import croniter
    from datetime import datetime
    import asyncio
    from sqlalchemy.ext.asyncio import create_async_engine, async_sessionmaker
    from app.models.models import BackupPolicy, BackupJob
    from app.core.config import settings

    async def _check():
        engine = create_async_engine(settings.DATABASE_URL)
        async_session = async_sessionmaker(engine, expire_on_commit=False)

        async with async_session() as db:
            from sqlalchemy import select
            result = await db.execute(select(BackupPolicy))
            policies = result.scalars().all()

            now = datetime.utcnow()
            for policy in policies:
                if not policy.schedule or "cron" not in policy.schedule:
                    continue

                cron = policy.schedule.get("cron", "")
                if not cron:
                    continue

                try:
                    cron_it = croniter(cron, now - __import__("datetime").timedelta(minutes=1))
                    next_run = cron_it.get_next(datetime)
                    if next_run <= now:
                        job = BackupJob(
                            policy_id=policy.id,
                            storage_id=policy.storage_id,
                            type="full",
                            status="pending",
                        )
                        db.add(job)
                except Exception:
                    continue

            await db.commit()
        await engine.dispose()

    asyncio.run(_check())


@celery_app.task(bind=True, name="app.services.scheduler.check_bandwidth_windows")
def check_bandwidth_windows(self):
    pass
