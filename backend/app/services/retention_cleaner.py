from app.services.celery_app import celery_app
from datetime import datetime, timedelta, timezone
from typing import Optional
import asyncio
import logging

logger = logging.getLogger(__name__)


class GFSRetentionPolicy:
    """Grandfather-Father-Son retention policy implementation.

    GFS keeps backups on a rotating schedule:
    - Daily (Son): keep last N daily backups
    - Weekly (Father): keep last N weekly backups (typically Sunday)
    - Monthly (Grandfather): keep last N monthly backups (typically 1st)
    - Yearly (Great-Grandfather): keep last N yearly backups (typically Jan 1)
    """

    def __init__(self, daily: int = 7, weekly: int = 4, monthly: int = 12, yearly: int = 1):
        self.daily = daily
        self.weekly = weekly
        self.monthly = monthly
        self.yearly = yearly

    @classmethod
    def from_dict(cls, d: dict) -> 'GFSRetentionPolicy':
        return cls(
            daily=d.get('daily', 7),
            weekly=d.get('weekly', 4),
            monthly=d.get('monthly', 12),
            yearly=d.get('yearly', 1),
        )

    def to_dict(self) -> dict:
        return {
            'daily': self.daily,
            'weekly': self.weekly,
            'monthly': self.monthly,
            'yearly': self.yearly,
        }

    def classify_backup(self, backup_time: datetime, now: Optional[datetime] = None) -> str:
        """Classify which GFS tier a backup belongs to."""
        if now is None:
            now = datetime.now(timezone.utc)

        backup_date = backup_time.date()
        now_date = now.date()

        # Yearly: backup from a different year
        if backup_date.year < now_date.year:
            return 'yearly'

        # Monthly: backup from a different month, keep if within monthly retention
        if backup_date.month != now_date.month:
            return 'monthly'

        # Weekly: backup from a different week (Sunday = start of week)
        backup_week = backup_date.isocalendar()[1]
        now_week = now_date.isocalendar()[1]
        if backup_week != now_week:
            return 'weekly'

        # Daily: same week
        return 'daily'

    def should_keep(self, backup_time: datetime, backup_type: str = 'full',
                    now: Optional[datetime] = None) -> bool:
        """Determine if a backup should be kept based on GFS policy."""
        if now is None:
            now = datetime.now(timezone.utc)

        tier = self.classify_backup(backup_time, now)
        age_days = (now - backup_time).days

        if tier == 'yearly':
            return age_days <= self.yearly * 365
        elif tier == 'monthly':
            return age_days <= self.monthly * 30
        elif tier == 'weekly':
            return age_days <= self.weekly * 7
        else:  # daily
            return age_days <= self.daily

    def get_keep_dates(self, now: Optional[datetime] = None) -> dict:
        """Get the specific dates that should be kept under GFS policy."""
        if now is None:
            now = datetime.now(timezone.utc)

        keep = {'daily': [], 'weekly': [], 'monthly': [], 'yearly': []}

        # Daily: keep last N days
        for i in range(self.daily):
            d = (now - timedelta(days=i)).date()
            keep['daily'].append(d.isoformat())

        # Weekly: keep last N Sundays
        current_sunday = now.date() - timedelta(days=now.date().weekday() + 1)
        for i in range(self.weekly):
            d = current_sunday - timedelta(weeks=i)
            keep['weekly'].append(d.isoformat())

        # Monthly: keep 1st of last N months
        current_first = now.date().replace(day=1)
        for i in range(self.monthly):
            month = current_first.month - i
            year = current_first.year
            while month <= 0:
                month += 12
                year -= 1
            try:
                d = current_first.replace(year=year, month=month, day=1)
                keep['monthly'].append(d.isoformat())
            except ValueError:
                pass

        # Yearly: keep Jan 1 of last N years
        for i in range(self.yearly):
            year = now.year - i
            keep['yearly'].append(f"{year}-01-01")

        return keep


class GFSRetentionEngine:
    """Main GFS retention engine that processes backups and CDP recovery points."""

    def __init__(self):
        self.policy_cache = {}

    def get_policy_for_job(self, policy_retention: dict) -> GFSRetentionPolicy:
        """Get GFS policy from policy retention config."""
        return GFSRetentionPolicy.from_dict(policy_retention or {'daily': 7, 'weekly': 4, 'monthly': 12, 'yearly': 1})

    async def process_backup_retention(self, db, policy_id: int) -> dict:
        """Process GFS retention for a specific backup policy's jobs."""
        from sqlalchemy import select
        from app.models.models import BackupPolicy, BackupJob, JobStatus

        result = await db.execute(select(BackupPolicy).where(BackupPolicy.id == policy_id))
        policy = result.scalar_one_or_none()
        if not policy:
            return {'error': 'Policy not found'}

        gfs = self.get_policy_for_job(policy.retention_gfs or {})

        # Get all completed backups for this policy, ordered by time
        result = await db.execute(
            select(BackupJob).where(
                BackupJob.policy_id == policy_id,
                BackupJob.status == JobStatus.completed,
            ).order_by(BackupJob.finished_at.desc())
        )
        jobs = result.scalars().all()

        now = datetime.now(timezone.utc)
        keep_set = set()
        delete_list = []

        # First pass: identify which backups to keep
        for job in jobs:
            if job.finished_at is None:
                continue
            backup_time = job.finished_at.replace(tzinfo=timezone.utc) if job.finished_at.tzinfo is None else job.finished_at
            tier = gfs.classify_backup(backup_time, now)

            # Check if this is the latest backup for its tier
            if tier not in keep_set:
                keep_set.add(tier)
                continue

        # Second pass: mark older backups in same tier for deletion
        tier_seen = {}
        for job in jobs:
            if job.finished_at is None:
                continue
            backup_time = job.finished_at.replace(tzinfo=timezone.utc) if job.finished_at.tzinfo is None else job.finished_at
            tier = gfs.classify_backup(backup_time, now)

            if tier not in tier_seen:
                tier_seen[tier] = 0

            tier_seen[tier] += 1
            max_keep = {
                'daily': gfs.daily,
                'weekly': gfs.weekly,
                'monthly': gfs.monthly,
                'yearly': gfs.yearly,
            }[tier]

            if tier_seen[tier] > max_keep:
                delete_list.append(job.id)

        # Delete old backups
        deleted = 0
        for job_id in delete_list:
            result = await db.execute(select(BackupJob).where(BackupJob.id == job_id))
            job = result.scalar_one_or_none()
            if job:
                await db.delete(job)
                deleted += 1
                logger.info(f"GFS: deleting backup job {job_id} (policy {policy_id})")

        await db.commit()
        return {
            'policy_id': policy_id,
            'total_jobs': len(jobs),
            'kept': len(jobs) - deleted,
            'deleted': deleted,
            'gfs_config': gfs.to_dict(),
        }

    async def process_cdp_retention(self, db, session_id: int, retention_minutes: int = 60) -> dict:
        """Process retention for CDP recovery points."""
        from sqlalchemy import select
        from app.models.models import RecoveryPoint

        cutoff = datetime.now(timezone.utc) - timedelta(minutes=retention_minutes)
        result = await db.execute(
            select(RecoveryPoint).where(
                RecoveryPoint.cdp_session_id == session_id,
                RecoveryPoint.timestamp < cutoff,
            )
        )
        old_rps = result.scalars().all()

        deleted = 0
        for rp in old_rps:
            await db.delete(rp)
            deleted += 1

        await db.commit()
        return {'session_id': session_id, 'deleted': deleted}

    async def get_retention_summary(self, db, policy_id: int) -> dict:
        """Get retention summary for a policy showing what GFS tiers exist."""
        from sqlalchemy import select
        from app.models.models import BackupPolicy, BackupJob, JobStatus

        result = await db.execute(select(BackupPolicy).where(BackupPolicy.id == policy_id))
        policy = result.scalar_one_or_none()
        if not policy:
            return {'error': 'Policy not found'}

        gfs = self.get_policy_for_job(policy.retention_gfs or {})

        result = await db.execute(
            select(BackupJob).where(
                BackupJob.policy_id == policy_id,
                BackupJob.status == JobStatus.completed,
            ).order_by(BackupJob.finished_at.desc())
        )
        jobs = result.scalars().all()

        now = datetime.now(timezone.utc)
        tiers = {'daily': 0, 'weekly': 0, 'monthly': 0, 'yearly': 0}

        for job in jobs:
            if job.finished_at is None:
                continue
            backup_time = job.finished_at.replace(tzinfo=timezone.utc) if job.finished_at.tzinfo is None else job.finished_at
            tier = gfs.classify_backup(backup_time, now)
            tiers[tier] += 1

        return {
            'policy_id': policy_id,
            'gfs_config': gfs.to_dict(),
            'tiers': tiers,
            'total_backups': len(jobs),
            'keep_dates': gfs.get_keep_dates(now),
        }


# Singleton
gfs_engine = GFSRetentionEngine()


# ============================================================
# Celery Tasks
# ============================================================

@celery_app.task(name="app.services.retention_cleaner.cleanup_expired_backups")
def cleanup_expired_backups():
    """Main GFS cleanup task - runs every hour via Celery Beat."""
    from app.core.database import async_session
    from sqlalchemy import select
    from app.models.models import BackupPolicy

    async def _run():
        engine = None
        try:
            from sqlalchemy.ext.asyncio import create_async_engine
            from app.core.config import settings
            engine = create_async_engine(settings.DATABASE_URL)
            async_session_maker = __import__('sqlalchemy.ext.asyncio', fromlist=['async_sessionmaker']).async_sessionmaker
            session_maker = async_session_maker(engine, expire_on_commit=False)

            async with session_maker() as db:
                result = await db.execute(select(BackupPolicy))
                policies = result.scalars().all()
                total_deleted = 0

                for policy in policies:
                    if not policy.retention_gfs:
                        continue
                    try:
                        res = await gfs_engine.process_backup_retention(db, policy.id)
                        deleted = res.get('deleted', 0)
                        total_deleted += deleted
                        if deleted > 0:
                            logger.info(f"GFS cleanup: policy {policy.name} - deleted {deleted} backups")
                    except Exception as e:
                        logger.error(f"GFS cleanup error for policy {policy.id}: {e}")

                logger.info(f"GFS cleanup complete: deleted {total_deleted} backups across {len(policies)} policies")
                return {'deleted': total_deleted, 'policies_checked': len(policies)}
        finally:
            if engine:
                await engine.dispose()

    return asyncio.run(_run())


@celery_app.task(name="app.services.retention_cleaner.cleanup_recovery_points")
def cleanup_recovery_points():
    """CDP recovery point retention cleanup."""
    from app.core.database import async_session
    from sqlalchemy import select
    from app.models.models import CDPSession, BackupPolicy

    async def _run():
        engine = None
        try:
            from sqlalchemy.ext.asyncio import create_async_engine
            from app.core.config import settings
            engine = create_async_engine(settings.DATABASE_URL)
            session_maker = __import__('sqlalchemy.ext.asyncio', fromlist=['async_sessionmaker']).async_sessionmaker
            session_maker_obj = session_maker(engine, expire_on_commit=False)

            async with session_maker_obj() as db:
                sessions = (await db.execute(select(CDPSession))).scalars().all()
                total_deleted = 0

                for session in sessions:
                    # Get retention from policy
                    retention_minutes = 60  # default
                    if session.policy_id:
                        policy = (await db.execute(
                            select(BackupPolicy).where(BackupPolicy.id == session.policy_id)
                        )).scalar_one_or_none()
                        if policy:
                            retention_minutes = policy.cdp_retention_minutes or 60

                    res = await gfs_engine.process_cdp_retention(db, session.id, retention_minutes)
                    total_deleted += res.get('deleted', 0)

                logger.info(f"CDP retention cleanup: deleted {total_deleted} recovery points")
                return {'deleted': total_deleted}
        finally:
            if engine:
                await engine.dispose()

    return asyncio.run(_run())


@celery_app.task(name="app.services.retention_cleaner.gfs_report")
def gfs_report(policy_id: Optional[int] = None):
    """Generate GFS retention report."""
    from app.core.database import async_session
    from sqlalchemy import select
    from app.models.models import BackupPolicy

    async def _run():
        engine = None
        try:
            from sqlalchemy.ext.asyncio import create_async_engine
            from app.core.config import settings
            engine = create_async_engine(settings.DATABASE_URL)
            session_maker = __import__('sqlalchemy.ext.asyncio', fromlist=['async_sessionmaker']).async_sessionmaker
            session_maker_obj = session_maker(engine, expire_on_commit=False)

            async with session_maker_obj() as db:
                if policy_id:
                    result = await gfs_engine.get_retention_summary(db, policy_id)
                    return result
                else:
                    policies = (await db.execute(select(BackupPolicy))).scalars().all()
                    reports = []
                    for policy in policies:
                        if policy.retention_gfs:
                            report = await gfs_engine.get_retention_summary(db, policy.id)
                            reports.append(report)
                    return {'reports': reports}
        finally:
            if engine:
                await engine.dispose()

    return asyncio.run(_run())
