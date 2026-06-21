from app.services import celery_app
import asyncio


@celery_app.task(bind=True, name="app.services.backup_executor.execute_backup")
def execute_backup(self, job_id: int):
    from sqlalchemy.ext.asyncio import create_async_engine, async_sessionmaker
    from sqlalchemy import select
    from app.models.models import BackupJob, BackupPolicy
    from app.core.config import settings

    async def _run():
        engine = create_async_engine(settings.DATABASE_URL)
        async_session = async_sessionmaker(engine, expire_on_commit=False)

        async with async_session() as db:
            result = await db.execute(select(BackupJob).where(BackupJob.id == job_id))
            job = result.scalar_one_or_none()
            if not job:
                return

            job.status = "running"
            await db.commit()

            policy_result = await db.execute(
                select(BackupPolicy).where(BackupPolicy.id == job.policy_id)
            )
            policy = policy_result.scalar_one_or_none()

            try:
                from app.ipc.core_client import CoreIPCClient
                client = CoreIPCClient(settings.CORE_SOCKET_PATH)

                config = {
                    "job_id": str(job.id),
                    "policy_id": str(job.policy_id or ""),
                    "storage_id": str(job.storage_id or ""),
                    "backup_type": job.type.upper(),
                    "source_paths": policy.source_paths if policy else [],
                    "exclude_patterns": policy.exclude_patterns if policy else [],
                    "compression_level": policy.compression_level if policy else 1,
                    "transport_mode": policy.transport_mode if policy else "AUTO",
                    "bandwidth_limit_kbps": policy.bandwidth_limit_kbps if policy else 0,
                }

                resp = await client.start_job(config)
                await client.close()

                if resp.get("success"):
                    job.status = "completed"
                else:
                    job.status = "failed"
                    job.error_log = resp.get("error", "Unknown error")
            except Exception as e:
                job.status = "failed"
                job.error_log = str(e)

            from datetime import datetime
            job.finished_at = datetime.utcnow()
            await db.commit()

        await engine.dispose()

    asyncio.run(_run())
