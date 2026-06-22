"""Add dirty buffer logs and agent control tables

Revision ID: 002
Revises: 001
Create Date: 2026-06-22 00:00:00

"""
from typing import Sequence, Union
from alembic import op
import sqlalchemy as sa

revision: str = "002"
down_revision: Union[str, None] = "001"
branch_labels: Union[str, Sequence[str], None] = None
depends_on: Union[str, Sequence[str], None] = None


def upgrade() -> None:
    # Dirty buffer logs
    op.create_table(
        "dirty_buffer_logs",
        sa.Column("id", sa.Integer(), primary_key=True, autoincrement=True),
        sa.Column("backup_job_id", sa.String(100), nullable=False, index=True),
        sa.Column("agent_id", sa.String(100), nullable=False),
        sa.Column("plugin_name", sa.String(50), nullable=False, index=True),
        sa.Column("before_page_count", sa.Integer(), default=0),
        sa.Column("before_size_bytes", sa.Integer(), default=0),
        sa.Column("before_percent_ram", sa.Float(), default=0.0),
        sa.Column("after_page_count", sa.Integer(), default=0),
        sa.Column("after_size_bytes", sa.Integer(), default=0),
        sa.Column("after_percent_ram", sa.Float(), default=0.0),
        sa.Column("flush_status", sa.String(20), default="not_flushed", index=True),
        sa.Column("flush_duration_ms", sa.Integer(), default=0),
        sa.Column("error_message", sa.Text(), default=""),
        sa.Column("component_details_json", sa.Text(), default="[]"),
        sa.Column("consistency_level", sa.String(30), default="crash_consistent"),
        sa.Column("is_consistent", sa.Boolean(), default=False),
        sa.Column("total_ram_bytes", sa.Integer(), default=0),
        sa.Column("buffer_pool_size", sa.Integer(), default=0),
        sa.Column("flush_started_at", sa.String(30), default=""),
        sa.Column("flush_finished_at", sa.String(30), default=""),
        sa.Column("created_at", sa.String(30), server_default=sa.func.now()),
    )

    # Agent install jobs
    op.create_table(
        "agent_install_jobs",
        sa.Column("id", sa.Integer(), primary_key=True, autoincrement=True),
        sa.Column("host", sa.String(255), nullable=False),
        sa.Column("port", sa.Integer(), default=22),
        sa.Column("username", sa.String(100), nullable=False),
        sa.Column("os_type", sa.String(20), default="linux"),
        sa.Column("status", sa.String(20), default="pending", index=True),
        sa.Column("error_message", sa.Text(), nullable=True),
        sa.Column("agent_id", sa.Integer(), nullable=True),
        sa.Column("started_at", sa.String(30), nullable=True),
        sa.Column("finished_at", sa.String(30), nullable=True),
        sa.Column("created_at", sa.String(30), server_default=sa.func.now()),
    )

    # Agent command logs
    op.create_table(
        "agent_command_logs",
        sa.Column("id", sa.Integer(), primary_key=True, autoincrement=True),
        sa.Column("agent_id", sa.Integer(), nullable=False, index=True),
        sa.Column("command", sa.String(100), nullable=False),
        sa.Column("params_json", sa.Text(), default="{}"),
        sa.Column("status", sa.String(20), default="pending"),
        sa.Column("response_json", sa.Text(), default="{}"),
        sa.Column("user_id", sa.Integer(), nullable=True),
        sa.Column("created_at", sa.String(30), server_default=sa.func.now()),
    )

    # Indexes
    op.create_index("idx_dbl_created", "dirty_buffer_logs", ["created_at"])
    op.create_index("idx_aij_host", "agent_install_jobs", ["host"])
    op.create_index("idx_acl_agent", "agent_command_logs", ["agent_id"])


def downgrade() -> None:
    op.drop_table("agent_command_logs")
    op.drop_table("agent_install_jobs")
    op.drop_table("dirty_buffer_logs")
