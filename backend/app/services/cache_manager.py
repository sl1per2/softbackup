from app.services.celery_app import celery_app
import logging

logger = logging.getLogger(__name__)


@celery_app.task(bind=True)
def collect_cache_stats(self):
    """Collect GlobalCacheStats from agents and provide optimization recommendations."""
    import asyncio
    from app.core.database import async_session
    from sqlalchemy import select
    from app.models.models import Agent, GlobalCacheStats, AgentStatus

    async def _run():
        async with async_session() as db:
            result = await db.execute(
                select(Agent).where(Agent.status == AgentStatus.online)
            )
            agents = result.scalars().all()
            recommendations = []

            for agent in agents:
                cache_result = await db.execute(
                    select(GlobalCacheStats).where(GlobalCacheStats.agent_id == agent.id)
                )
                stats = cache_result.scalar_one_or_none()

                if stats:
                    if stats.hit_ratio < 0.3:
                        recommendations.append({
                            "agent_id": agent.id,
                            "hostname": agent.hostname,
                            "recommendation": "Cache hit ratio is low. Consider increasing cache size.",
                            "current_size_gb": stats.size_bytes / (1024**3),
                        })
                else:
                    agent.cache_hit_ratio = 0.0

            await db.commit()
            return {"agents_checked": len(agents), "recommendations": recommendations}

    try:
        loop = asyncio.new_event_loop()
        result = loop.run_until_complete(_run())
        loop.close()
        return result
    except Exception as e:
        logger.error(f"Cache manager error: {e}")
        return {"error": str(e)}
