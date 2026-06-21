from celery import Celery
from app.core.config import settings

celery_app = Celery(
    "obs_backup",
    broker=settings.REDIS_URL,
    backend=settings.REDIS_URL,
)

celery_app.conf.update(
    task_serializer="json",
    accept_content=["json"],
    result_serializer="json",
    timezone="UTC",
    enable_utc=True,
    task_track_started=True,
    task_acks_late=True,
    worker_prefetch_multiplier=1,
    beat_schedule={
        "check-scheduled-jobs": {
            "task": "app.services.scheduler.create_scheduled_jobs",
            "schedule": 60.0,
        },
        "cleanup-expired": {
            "task": "app.services.retention_cleaner.cleanup_expired_backups",
            "schedule": 3600.0,
        },
        "send-zabbix-metrics": {
            "task": "app.services.zabbix_sender.send_zabbix_metrics",
            "schedule": 60.0,
        },
        "monitor-cdp": {
            "task": "app.services.cdp_monitor.monitor_cdp_sessions",
            "schedule": 5.0,
        },
        "collect-cache-stats": {
            "task": "app.services.cache_manager.collect_cache_stats",
            "schedule": 300.0,
        },
    },
)
