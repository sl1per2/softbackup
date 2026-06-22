from pydantic import BaseModel
from typing import Optional, List
from datetime import datetime


class DirtyBufferLogCreate(BaseModel):
    backup_job_id: str
    agent_id: str
    plugin_name: str
    before_page_count: int = 0
    before_size_bytes: int = 0
    before_percent_ram: float = 0.0
    after_page_count: int = 0
    after_size_bytes: int = 0
    after_percent_ram: float = 0.0
    flush_status: str = "not_flushed"
    flush_duration_ms: int = 0
    error_message: str = ""
    component_details_json: str = "[]"
    consistency_level: str = "crash_consistent"
    is_consistent: bool = False
    total_ram_bytes: int = 0
    buffer_pool_size: int = 0
    flush_started_at: str = ""
    flush_finished_at: str = ""
    created_at: str = ""


class DirtyBufferLogResponse(BaseModel):
    id: int
    backup_job_id: str
    agent_id: str
    plugin_name: str
    before_page_count: int
    before_size_bytes: int
    before_percent_ram: float
    after_page_count: int
    after_size_bytes: int
    after_percent_ram: float
    flush_status: str
    flush_duration_ms: int
    error_message: str
    component_details_json: str
    consistency_level: str
    is_consistent: bool
    total_ram_bytes: int
    buffer_pool_size: int
    flush_started_at: str
    flush_finished_at: str
    created_at: Optional[str] = None

    class Config:
        from_attributes = True


class DirtyBufferStatsResponse(BaseModel):
    total_entries: int
    total_flushed: int
    total_failed: int
    total_inconsistent: int
    avg_flush_ms: float
    plugins: List[dict]
