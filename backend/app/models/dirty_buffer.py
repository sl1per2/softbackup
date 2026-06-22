from sqlalchemy import Column, Integer, String, Float, Text, Boolean
from sqlalchemy.sql import func
from app.core.database import Base


class DirtyBufferLog(Base):
    __tablename__ = "dirty_buffer_logs"

    id = Column(Integer, primary_key=True, index=True)
    backup_job_id = Column(String(100), nullable=False, index=True)
    agent_id = Column(String(100), nullable=False)
    plugin_name = Column(String(50), nullable=False, index=True)

    before_page_count = Column(Integer, default=0)
    before_size_bytes = Column(Integer, default=0)
    before_percent_ram = Column(Float, default=0.0)

    after_page_count = Column(Integer, default=0)
    after_size_bytes = Column(Integer, default=0)
    after_percent_ram = Column(Float, default=0.0)

    flush_status = Column(String(20), default="not_flushed", index=True)
    flush_duration_ms = Column(Integer, default=0)
    error_message = Column(Text, default="")
    component_details_json = Column(Text, default="[]")
    consistency_level = Column(String(30), default="crash_consistent")
    is_consistent = Column(Boolean, default=False)

    total_ram_bytes = Column(Integer, default=0)
    buffer_pool_size = Column(Integer, default=0)

    flush_started_at = Column(String(30), default="")
    flush_finished_at = Column(String(30), default="")
    created_at = Column(String(30), server_default=func.now())
