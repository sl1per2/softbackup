from sqlalchemy import Column, Integer, String, DateTime, Boolean, Float, Text, JSON, ForeignKey, Enum as SQLEnum
from sqlalchemy.orm import relationship
from sqlalchemy.sql import func
from app.core.database import Base
import enum


class UserRole(str, enum.Enum):
    ADMIN = "admin"
    OPERATOR = "operator"
    VIEWER = "viewer"


class AgentStatus(str, enum.Enum):
    ONLINE = "online"
    OFFLINE = "offline"
    BACKING_UP = "backing_up"
    CDP_ACTIVE = "cdp_active"
    ERROR = "error"


class JobState(str, enum.Enum):
    PENDING = "pending"
    RUNNING = "running"
    COMPLETED = "completed"
    FAILED = "failed"
    CANCELLED = "cancelled"


class BackupType(str, enum.Enum):
    FULL = "full"
    INCREMENTAL = "incremental"
    DIFFERENTIAL = "differential"


class User(Base):
    __tablename__ = "users"

    id = Column(Integer, primary_key=True, index=True)
    username = Column(String(50), unique=True, index=True, nullable=False)
    email = Column(String(255), unique=True, index=True, nullable=False)
    hashed_password = Column(String(255), nullable=False)
    role = Column(String(20), default=UserRole.VIEWER.value)
    created_at = Column(DateTime(timezone=True), server_default=func.now())
    last_login = Column(DateTime(timezone=True), nullable=True)

    audit_logs = relationship("AuditLog", back_populates="user")


class Agent(Base):
    __tablename__ = "agents"

    id = Column(Integer, primary_key=True, index=True)
    hostname = Column(String(255), nullable=False)
    ip = Column(String(45), nullable=False)
    os_type = Column(String(50), nullable=False)
    version = Column(String(50), nullable=True)
    status = Column(String(20), default=AgentStatus.OFFLINE.value)
    last_seen = Column(DateTime(timezone=True), nullable=True)
    core_version = Column(String(50), nullable=True)
    transport_mode = Column(String(50), default="AUTO")
    cache_size_bytes = Column(Integer, default=0)
    cache_hit_ratio = Column(Float, default=0.0)

    jobs = relationship("BackupJob", back_populates="agent")
    cdp_sessions = relationship("CDPSession", back_populates="agent")
    traffic_stats = relationship("TrafficStats", back_populates="agent")
    cache_stats = relationship("GlobalCacheStats", back_populates="agent")


class Storage(Base):
    __tablename__ = "storages"

    id = Column(Integer, primary_key=True, index=True)
    name = Column(String(255), nullable=False)
    type = Column(String(50), nullable=False)
    config = Column(JSON, default={})
    encrypted_credentials = Column(Text, nullable=True)
    total_bytes = Column(Integer, default=0)
    used_bytes = Column(Integer, default=0)
    supports_fast_clone = Column(Boolean, default=False)

    jobs = relationship("BackupJob", back_populates="storage")


class BackupPolicy(Base):
    __tablename__ = "backup_policies"

    id = Column(Integer, primary_key=True, index=True)
    name = Column(String(255), nullable=False)
    schedule = Column(JSON, default={})
    retention_gfs = Column(JSON, default={"daily": 7, "weekly": 4, "monthly": 12, "yearly": 1})
    source_paths = Column(JSON, default=[])
    exclude_patterns = Column(JSON, default=[])
    compression_level = Column(Integer, default=1)
    encryption_enabled = Column(Boolean, default=False)
    transport_mode = Column(String(50), default="AUTO")
    bandwidth_limit_kbps = Column(Integer, default=0)
    bandwidth_schedule = Column(JSON, default={})
    cdp_enabled = Column(Boolean, default=False)
    cdp_interval_seconds = Column(Integer, default=1)
    cdp_max_latency_ms = Column(Integer, default=5000)
    cdp_retention_minutes = Column(Integer, default=60)
    storage_id = Column(Integer, ForeignKey("storages.id"), nullable=True)

    jobs = relationship("BackupJob", back_populates="policy")
    cdp_sessions = relationship("CDPSession", back_populates="policy")
    recovery_points = relationship("RecoveryPoint", back_populates="policy")
    storage = relationship("Storage")


class BackupJob(Base):
    __tablename__ = "backup_jobs"

    id = Column(Integer, primary_key=True, index=True)
    policy_id = Column(Integer, ForeignKey("backup_policies.id"), nullable=True)
    agent_id = Column(Integer, ForeignKey("agents.id"), nullable=True)
    storage_id = Column(Integer, ForeignKey("storages.id"), nullable=True)
    type = Column(String(20), nullable=False)
    status = Column(String(20), default=JobState.PENDING.value)
    transport_mode_used = Column(String(50), nullable=True)
    started_at = Column(DateTime(timezone=True), nullable=True)
    finished_at = Column(DateTime(timezone=True), nullable=True)
    size_bytes = Column(Integer, default=0)
    size_transferred_bytes = Column(Integer, default=0)
    dedup_saved_bytes = Column(Integer, default=0)
    compression_ratio = Column(Float, default=1.0)
    cache_hit_ratio = Column(Float, default=0.0)
    zero_blocks_skipped = Column(Integer, default=0)
    chunks_total = Column(Integer, default=0)
    chunks_cached = Column(Integer, default=0)
    error_log = Column(Text, nullable=True)

    policy = relationship("BackupPolicy", back_populates="jobs")
    agent = relationship("Agent", back_populates="jobs")
    storage = relationship("Storage", back_populates="jobs")
    traffic_stats = relationship("TrafficStats", back_populates="job")


class CDPSession(Base):
    __tablename__ = "cdp_sessions"

    id = Column(Integer, primary_key=True, index=True)
    policy_id = Column(Integer, ForeignKey("backup_policies.id"), nullable=True)
    agent_id = Column(Integer, ForeignKey("agents.id"), nullable=True)
    status = Column(String(20), default="inactive")
    started_at = Column(DateTime(timezone=True), nullable=True)
    current_lag_ms = Column(Integer, default=0)
    iops = Column(Integer, default=0)
    throughput_mbps = Column(Float, default=0.0)
    blocks_tracked = Column(Integer, default=0)

    policy = relationship("BackupPolicy", back_populates="cdp_sessions")
    agent = relationship("Agent", back_populates="cdp_sessions")
    recovery_points = relationship("RecoveryPoint", back_populates="cdp_session")


class RecoveryPoint(Base):
    __tablename__ = "recovery_points"

    id = Column(Integer, primary_key=True, index=True)
    policy_id = Column(Integer, ForeignKey("backup_policies.id"), nullable=True)
    cdp_session_id = Column(Integer, ForeignKey("cdp_sessions.id"), nullable=True)
    timestamp = Column(DateTime(timezone=True), server_default=func.now())
    block_count = Column(Integer, default=0)
    size_bytes = Column(Integer, default=0)
    is_consistent = Column(Boolean, default=True)

    policy = relationship("BackupPolicy", back_populates="recovery_points")
    cdp_session = relationship("CDPSession", back_populates="recovery_points")


class GlobalCacheStats(Base):
    __tablename__ = "global_cache_stats"

    id = Column(Integer, primary_key=True, index=True)
    agent_id = Column(Integer, ForeignKey("agents.id"), nullable=True)
    total_entries = Column(Integer, default=0)
    hit_count = Column(Integer, default=0)
    miss_count = Column(Integer, default=0)
    hit_ratio = Column(Float, default=0.0)
    size_bytes = Column(Integer, default=0)
    last_reset_at = Column(DateTime(timezone=True), nullable=True)

    agent = relationship("Agent", back_populates="cache_stats")


class TrafficStats(Base):
    __tablename__ = "traffic_stats"

    id = Column(Integer, primary_key=True, index=True)
    agent_id = Column(Integer, ForeignKey("agents.id"), nullable=True)
    job_id = Column(Integer, ForeignKey("backup_jobs.id"), nullable=True)
    original_size = Column(Integer, default=0)
    transferred_size = Column(Integer, default=0)
    compression_ratio = Column(Float, default=1.0)
    cache_hits = Column(Integer, default=0)
    zero_blocks = Column(Integer, default=0)
    duration_ms = Column(Integer, default=0)
    throughput_mbps = Column(Float, default=0.0)
    timestamp = Column(DateTime(timezone=True), server_default=func.now())

    agent = relationship("Agent", back_populates="traffic_stats")
    job = relationship("BackupJob", back_populates="traffic_stats")


class ZabbixConfig(Base):
    __tablename__ = "zabbix_config"

    id = Column(Integer, primary_key=True, index=True)
    server_url = Column(String(500), nullable=True)
    trapper_port = Column(Integer, default=10051)
    api_token = Column(String(500), nullable=True)
    enabled = Column(Boolean, default=False)
    last_sync_at = Column(DateTime(timezone=True), nullable=True)
    metrics_interval_seconds = Column(Integer, default=60)


class AuditLog(Base):
    __tablename__ = "audit_logs"

    id = Column(Integer, primary_key=True, index=True)
    user_id = Column(Integer, ForeignKey("users.id"), nullable=True)
    action = Column(String(100), nullable=False)
    resource_type = Column(String(50), nullable=False)
    resource_id = Column(Integer, nullable=True)
    details = Column(JSON, default={})
    ip_address = Column(String(45), nullable=True)
    created_at = Column(DateTime(timezone=True), server_default=func.now())

    user = relationship("User", back_populates="audit_logs")
