from pydantic import BaseModel, EmailStr
from typing import Optional, List
from datetime import datetime


class UserCreate(BaseModel):
    username: str
    email: str
    password: str
    role: str = "viewer"


class UserResponse(BaseModel):
    id: int
    username: str
    email: str
    role: str
    created_at: datetime

    class Config:
        from_attributes = True


class Token(BaseModel):
    access_token: str
    refresh_token: str
    token_type: str = "bearer"


class TokenRefresh(BaseModel):
    refresh_token: str


class AgentCreate(BaseModel):
    hostname: str
    ip: str
    os_type: str
    version: Optional[str] = None
    core_version: Optional[str] = None


class AgentUpdate(BaseModel):
    hostname: Optional[str] = None
    ip: Optional[str] = None
    os_type: Optional[str] = None
    version: Optional[str] = None
    status: Optional[str] = None
    transport_mode: Optional[str] = None


class AgentResponse(BaseModel):
    id: int
    hostname: str
    ip: str
    os_type: str
    version: Optional[str]
    status: str
    last_seen: Optional[datetime]
    core_version: Optional[str]
    transport_mode: str
    cache_size_bytes: int
    cache_hit_ratio: float

    class Config:
        from_attributes = True


class StorageCreate(BaseModel):
    name: str
    type: str
    config: dict = {}
    total_bytes: int = 0
    supports_fast_clone: bool = False


class StorageResponse(BaseModel):
    id: int
    name: str
    type: str
    config: dict
    total_bytes: int
    used_bytes: int
    supports_fast_clone: bool

    class Config:
        from_attributes = True


class PolicyCreate(BaseModel):
    name: str
    schedule: dict = {}
    retention_gfs: dict = {"daily": 7, "weekly": 4, "monthly": 12, "yearly": 1}
    source_paths: List[str] = []
    exclude_patterns: List[str] = []
    compression_level: int = 1
    encryption_enabled: bool = False
    transport_mode: str = "AUTO"
    bandwidth_limit_kbps: int = 0
    bandwidth_schedule: dict = {}
    cdp_enabled: bool = False
    cdp_interval_seconds: int = 1
    cdp_max_latency_ms: int = 5000
    cdp_retention_minutes: int = 60
    storage_id: Optional[int] = None


class PolicyUpdate(BaseModel):
    name: Optional[str] = None
    schedule: Optional[dict] = None
    retention_gfs: Optional[dict] = None
    source_paths: Optional[List[str]] = None
    exclude_patterns: Optional[List[str]] = None
    compression_level: Optional[int] = None
    encryption_enabled: Optional[bool] = None
    transport_mode: Optional[str] = None
    bandwidth_limit_kbps: Optional[int] = None
    bandwidth_schedule: Optional[dict] = None
    cdp_enabled: Optional[bool] = None
    cdp_interval_seconds: Optional[int] = None
    cdp_max_latency_ms: Optional[int] = None
    cdp_retention_minutes: Optional[int] = None
    storage_id: Optional[int] = None


class PolicyResponse(BaseModel):
    id: int
    name: str
    schedule: dict
    retention_gfs: dict
    source_paths: List[str]
    exclude_patterns: List[str]
    compression_level: int
    encryption_enabled: bool
    transport_mode: str
    bandwidth_limit_kbps: int
    bandwidth_schedule: dict
    cdp_enabled: bool
    cdp_interval_seconds: int
    cdp_max_latency_ms: int
    cdp_retention_minutes: int
    storage_id: Optional[int]

    class Config:
        from_attributes = True


class JobResponse(BaseModel):
    id: int
    policy_id: Optional[int]
    agent_id: Optional[int]
    storage_id: Optional[int]
    type: str
    status: str
    transport_mode_used: Optional[str]
    started_at: Optional[datetime]
    finished_at: Optional[datetime]
    size_bytes: int
    size_transferred_bytes: int
    dedup_saved_bytes: int
    compression_ratio: float
    cache_hit_ratio: float
    zero_blocks_skipped: int
    chunks_total: int
    chunks_cached: int
    error_log: Optional[str]

    class Config:
        from_attributes = True


class CDPSessionResponse(BaseModel):
    id: int
    policy_id: Optional[int]
    agent_id: Optional[int]
    status: str
    started_at: Optional[datetime]
    current_lag_ms: int
    iops: int
    throughput_mbps: float
    blocks_tracked: int

    class Config:
        from_attributes = True


class RecoveryPointResponse(BaseModel):
    id: int
    policy_id: Optional[int]
    cdp_session_id: Optional[int]
    timestamp: datetime
    block_count: int
    size_bytes: int
    is_consistent: bool

    class Config:
        from_attributes = True


class ZabbixConfigUpdate(BaseModel):
    server_url: Optional[str] = None
    trapper_port: Optional[int] = None
    api_token: Optional[str] = None
    enabled: Optional[bool] = None
    metrics_interval_seconds: Optional[int] = None


class ZabbixConfigResponse(BaseModel):
    id: int
    server_url: Optional[str]
    trapper_port: int
    enabled: bool
    last_sync_at: Optional[datetime]
    metrics_interval_seconds: int

    class Config:
        from_attributes = True


class TrafficStatsResponse(BaseModel):
    id: int
    agent_id: Optional[int]
    job_id: Optional[int]
    original_size: int
    transferred_size: int
    compression_ratio: float
    cache_hits: int
    zero_blocks: int
    duration_ms: int
    throughput_mbps: float
    timestamp: datetime

    class Config:
        from_attributes = True


class AuditLogResponse(BaseModel):
    id: int
    user_id: Optional[int]
    action: str
    resource_type: str
    resource_id: Optional[int]
    details: dict
    ip_address: Optional[str]
    created_at: datetime

    class Config:
        from_attributes = True


class DashboardSummary(BaseModel):
    total_agents: int
    agents_online: int
    jobs_successful_24h: int
    jobs_failed_24h: int
    cdp_sessions_active: int
    traffic_saved_bytes: int
    avg_cache_hit_ratio: float
    avg_compression_ratio: float
