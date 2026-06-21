"""Initial migration

Revision ID: 001
Revises:
Create Date: 2024-01-01 00:00:00

"""
from typing import Sequence, Union
from alembic import op
import sqlalchemy as sa

revision: str = "001"
down_revision: Union[str, None] = None
branch_labels: Union[str, Sequence[str], None] = None
depends_on: Union[str, Sequence[str], None] = None


def upgrade() -> None:
    user_role = sa.Enum("admin", "operator", "viewer", name="userrole")
    agent_status = sa.Enum("online", "offline", "maintenance", name="agentstatus")
    backup_type = sa.Enum("full", "incremental", "differential", "synthetic_full", name="backuptype")
    job_status = sa.Enum("pending", "running", "completed", "failed", "cancelled", name="jobstatus")
    cdp_status = sa.Enum("active", "paused", "stopped", "error", name="cdpsessionstatus")
    storage_type = sa.Enum("local", "nfs", "s3", name="storagetype")

    for e in [user_role, agent_status, backup_type, job_status, cdp_status, storage_type]:
        e.create(op.get_bind(), checkfirst=True)

    op.create_table(
        "users",
        sa.Column("id", sa.Integer(), primary_key=True, autoincrement=True),
        sa.Column("username", sa.String(64), unique=True, nullable=False),
        sa.Column("email", sa.String(255), unique=True, nullable=False),
        sa.Column("hashed_password", sa.String(255), nullable=False),
        sa.Column("role", user_role, nullable=False, server_default="viewer"),
        sa.Column("is_active", sa.Boolean(), server_default="true"),
        sa.Column("created_at", sa.DateTime(), server_default=sa.func.now()),
        sa.Column("last_login", sa.DateTime(), nullable=True),
    )

    op.create_table(
        "agents",
        sa.Column("id", sa.Integer(), primary_key=True, autoincrement=True),
        sa.Column("hostname", sa.String(255), nullable=False),
        sa.Column("ip", sa.String(45), nullable=False),
        sa.Column("os_type", sa.String(32), nullable=False),
        sa.Column("version", sa.String(32)),
        sa.Column("core_version", sa.String(32)),
        sa.Column("status", agent_status, server_default="offline"),
        sa.Column("last_seen", sa.DateTime(), nullable=True),
        sa.Column("transport_mode", sa.String(32), server_default="AUTO"),
        sa.Column("cache_size_bytes", sa.BigInteger(), server_default="0"),
        sa.Column("cache_hit_ratio", sa.Float(), server_default="0"),
    )

    op.create_table(
        "storages",
        sa.Column("id", sa.Integer(), primary_key=True, autoincrement=True),
        sa.Column("name", sa.String(128), nullable=False),
        sa.Column("type", storage_type, nullable=False),
        sa.Column("config", sa.JSON(), server_default="{}"),
        sa.Column("encrypted_credentials", sa.Text()),
        sa.Column("total_bytes", sa.BigInteger(), server_default="0"),
        sa.Column("used_bytes", sa.BigInteger(), server_default="0"),
        sa.Column("supports_fast_clone", sa.Boolean(), server_default="false"),
    )

    op.create_table(
        "backup_policies",
        sa.Column("id", sa.Integer(), primary_key=True, autoincrement=True),
        sa.Column("name", sa.String(128), nullable=False),
        sa.Column("schedule", sa.JSON(), server_default="{}"),
        sa.Column("retention_gfs", sa.JSON(), server_default="{}"),
        sa.Column("source_paths", sa.JSON(), server_default="[]"),
        sa.Column("exclude_patterns", sa.JSON(), server_default="[]"),
        sa.Column("compression_level", sa.Integer(), server_default="1"),
        sa.Column("encryption_enabled", sa.Boolean(), server_default="false"),
        sa.Column("transport_mode", sa.String(32), server_default="AUTO"),
        sa.Column("bandwidth_limit_kbps", sa.Integer(), server_default="0"),
        sa.Column("bandwidth_schedule", sa.JSON(), server_default="{}"),
        sa.Column("cdp_enabled", sa.Boolean(), server_default="false"),
        sa.Column("cdp_interval_seconds", sa.Integer(), server_default="5"),
        sa.Column("cdp_max_latency_ms", sa.Integer(), server_default="1000"),
        sa.Column("cdp_retention_minutes", sa.Integer(), server_default="60"),
        sa.Column("agent_id", sa.Integer(), sa.ForeignKey("agents.id")),
        sa.Column("storage_id", sa.Integer(), sa.ForeignKey("storages.id")),
        sa.Column("created_at", sa.DateTime(), server_default=sa.func.now()),
        sa.Column("updated_at", sa.DateTime(), server_default=sa.func.now()),
    )

    op.create_table(
        "backup_jobs",
        sa.Column("id", sa.Integer(), primary_key=True, autoincrement=True),
        sa.Column("policy_id", sa.Integer(), sa.ForeignKey("backup_policies.id")),
        sa.Column("agent_id", sa.Integer(), sa.ForeignKey("agents.id")),
        sa.Column("storage_id", sa.Integer(), sa.ForeignKey("storages.id")),
        sa.Column("type", backup_type, nullable=False),
        sa.Column("status", job_status, server_default="pending"),
        sa.Column("transport_mode_used", sa.String(32)),
        sa.Column("started_at", sa.DateTime()),
        sa.Column("finished_at", sa.DateTime()),
        sa.Column("size_bytes", sa.BigInteger(), server_default="0"),
        sa.Column("size_transferred_bytes", sa.BigInteger(), server_default="0"),
        sa.Column("dedup_saved_bytes", sa.BigInteger(), server_default="0"),
        sa.Column("compression_ratio", sa.Float(), server_default="0"),
        sa.Column("cache_hit_ratio", sa.Float(), server_default="0"),
        sa.Column("zero_blocks_skipped", sa.Integer(), server_default="0"),
        sa.Column("chunks_total", sa.Integer(), server_default="0"),
        sa.Column("chunks_cached", sa.Integer(), server_default="0"),
        sa.Column("error_log", sa.Text()),
    )

    op.create_table(
        "cdp_sessions",
        sa.Column("id", sa.Integer(), primary_key=True, autoincrement=True),
        sa.Column("policy_id", sa.Integer(), sa.ForeignKey("backup_policies.id")),
        sa.Column("agent_id", sa.Integer(), sa.ForeignKey("agents.id")),
        sa.Column("status", cdp_status, server_default="stopped"),
        sa.Column("started_at", sa.DateTime()),
        sa.Column("current_lag_ms", sa.Float(), server_default="0"),
        sa.Column("iops", sa.Float(), server_default="0"),
        sa.Column("throughput_mbps", sa.Float(), server_default="0"),
        sa.Column("blocks_tracked", sa.BigInteger(), server_default="0"),
    )

    op.create_table(
        "recovery_points",
        sa.Column("id", sa.Integer(), primary_key=True, autoincrement=True),
        sa.Column("policy_id", sa.Integer(), sa.ForeignKey("backup_policies.id")),
        sa.Column("cdp_session_id", sa.Integer(), sa.ForeignKey("cdp_sessions.id")),
        sa.Column("timestamp", sa.DateTime(), server_default=sa.func.now()),
        sa.Column("block_count", sa.BigInteger(), server_default="0"),
        sa.Column("size_bytes", sa.BigInteger(), server_default="0"),
        sa.Column("is_consistent", sa.Boolean(), server_default="false"),
    )

    op.create_table(
        "global_cache_stats",
        sa.Column("id", sa.Integer(), primary_key=True, autoincrement=True),
        sa.Column("agent_id", sa.Integer(), sa.ForeignKey("agents.id")),
        sa.Column("total_entries", sa.Integer(), server_default="0"),
        sa.Column("hit_count", sa.BigInteger(), server_default="0"),
        sa.Column("miss_count", sa.BigInteger(), server_default="0"),
        sa.Column("hit_ratio", sa.Float(), server_default="0"),
        sa.Column("size_bytes", sa.BigInteger(), server_default="0"),
        sa.Column("last_reset_at", sa.DateTime()),
    )

    op.create_table(
        "traffic_stats",
        sa.Column("id", sa.Integer(), primary_key=True, autoincrement=True),
        sa.Column("agent_id", sa.Integer(), sa.ForeignKey("agents.id")),
        sa.Column("job_id", sa.Integer(), sa.ForeignKey("backup_jobs.id")),
        sa.Column("original_size", sa.BigInteger(), server_default="0"),
        sa.Column("transferred_size", sa.BigInteger(), server_default="0"),
        sa.Column("compression_ratio", sa.Float(), server_default="0"),
        sa.Column("cache_hits", sa.Integer(), server_default="0"),
        sa.Column("zero_blocks", sa.Integer(), server_default="0"),
        sa.Column("duration_ms", sa.BigInteger(), server_default="0"),
        sa.Column("throughput_mbps", sa.Float(), server_default="0"),
        sa.Column("timestamp", sa.DateTime(), server_default=sa.func.now()),
    )

    op.create_table(
        "zabbix_configs",
        sa.Column("id", sa.Integer(), primary_key=True, autoincrement=True),
        sa.Column("server_url", sa.String(512)),
        sa.Column("trapper_port", sa.Integer(), server_default="10051"),
        sa.Column("api_token", sa.String(512)),
        sa.Column("enabled", sa.Boolean(), server_default="false"),
        sa.Column("last_sync_at", sa.DateTime()),
        sa.Column("metrics_interval_seconds", sa.Integer(), server_default="60"),
    )

    op.create_table(
        "audit_logs",
        sa.Column("id", sa.Integer(), primary_key=True, autoincrement=True),
        sa.Column("user_id", sa.Integer(), sa.ForeignKey("users.id")),
        sa.Column("action", sa.String(255), nullable=False),
        sa.Column("resource_type", sa.String(64)),
        sa.Column("resource_id", sa.String(64)),
        sa.Column("details", sa.JSON(), server_default="{}"),
        sa.Column("ip_address", sa.String(45)),
        sa.Column("created_at", sa.DateTime(), server_default=sa.func.now()),
    )

    # Seed admin user
    from app.core.security import get_password_hash
    op.execute(
        sa.text(
            "INSERT INTO users (username, email, hashed_password, role) VALUES "
            f"('admin', 'admin@obs.local', '{get_password_hash('admin123')}', 'admin')"
        )
    )


def downgrade() -> None:
    tables = [
        "audit_logs", "zabbix_configs", "traffic_stats", "global_cache_stats",
        "recovery_points", "cdp_sessions", "backup_jobs", "backup_policies",
        "storages", "agents", "users",
    ]
    for t in tables:
        op.drop_table(t)
    for e in ["userrole", "agentstatus", "backuptype", "jobstatus", "cdpsessionstatus", "storagetype"]:
        op.execute(f"DROP TYPE IF EXISTS {e}")
