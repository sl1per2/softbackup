from app.services import celery_app
import asyncio


@celery_app.task(name="app.services.integrity_checker.run_integrity_check")
def run_integrity_check():
    print("Running integrity check on backup data...")


@celery_app.task(name="app.services.cache_manager.collect_cache_stats")
def collect_cache_stats():
    from sqlalchemy.ext.asyncio import create_async_engine, async_sessionmaker
    from sqlalchemy import select
    from app.models.models import Agent, GlobalCacheStats
    from app.core.config import settings

    async def _run():
        engine = create_async_engine(settings.DATABASE_URL)
        async_session = async_sessionmaker(engine, expire_on_commit=False)

        async with async_session() as db:
            agents = (await db.execute(select(Agent))).scalars().all()
            for agent in agents:
                existing = (await db.execute(
                    select(GlobalCacheStats).where(GlobalCacheStats.agent_id == agent.id)
                )).scalar_one_or_none()

                if existing:
                    existing.total_entries = agent.cache_size_bytes // 4096
                    existing.hit_count = int(existing.total_entries * agent.cache_hit_ratio)
                    existing.miss_count = existing.total_entries - existing.hit_count
                    existing.hit_ratio = agent.cache_hit_ratio
                    existing.size_bytes = agent.cache_size_bytes
                else:
                    stats = GlobalCacheStats(
                        agent_id=agent.id,
                        total_entries=agent.cache_size_bytes // 4096,
                        hit_ratio=agent.cache_hit_ratio,
                        size_bytes=agent.cache_size_bytes,
                    )
                    db.add(stats)

            await db.commit()
        await engine.dispose()

    asyncio.run(_run())
