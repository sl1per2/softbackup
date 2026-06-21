export interface User {
  id: number;
  username: string;
  email: string;
  role: string;
  is_active: boolean;
  created_at?: string;
  last_login?: string;
}

export interface Agent {
  id: number;
  hostname: string;
  ip: string;
  os_type: string;
  version?: string;
  core_version?: string;
  status: 'online' | 'offline' | 'maintenance';
  last_seen?: string;
  transport_mode: string;
  cache_size_bytes: number;
  cache_hit_ratio: number;
}

export interface Storage {
  id: number;
  name: string;
  type: 'local' | 'nfs' | 's3';
  config: Record<string, any>;
  total_bytes: number;
  used_bytes: number;
  supports_fast_clone: boolean;
}

export interface Policy {
  id: number;
  name: string;
  schedule: Record<string, any>;
  retention_gfs: Record<string, any>;
  source_paths: string[];
  exclude_patterns: string[];
  compression_level: number;
  encryption_enabled: boolean;
  transport_mode: string;
  bandwidth_limit_kbps: number;
  bandwidth_schedule: Record<string, any>;
  cdp_enabled: boolean;
  cdp_interval_seconds: number;
  cdp_max_latency_ms: number;
  cdp_retention_minutes: number;
  agent_id?: number;
  storage_id?: number;
  created_at?: string;
  updated_at?: string;
}

export interface Job {
  id: number;
  policy_id?: number;
  agent_id?: number;
  storage_id?: number;
  type: string;
  status: 'pending' | 'running' | 'completed' | 'failed' | 'cancelled';
  transport_mode_used?: string;
  started_at?: string;
  finished_at?: string;
  size_bytes: number;
  size_transferred_bytes: number;
  dedup_saved_bytes: number;
  compression_ratio: number;
  cache_hit_ratio: number;
  zero_blocks_skipped: number;
  chunks_total: number;
  chunks_cached: number;
  error_log?: string;
}

export interface CDPSession {
  id: number;
  policy_id?: number;
  agent_id?: number;
  status: 'active' | 'paused' | 'stopped' | 'error';
  started_at?: string;
  current_lag_ms: number;
  iops: number;
  throughput_mbps: number;
  blocks_tracked: number;
}

export interface RecoveryPoint {
  id: number;
  policy_id?: number;
  cdp_session_id?: number;
  timestamp?: string;
  block_count: number;
  size_bytes: number;
  is_consistent: boolean;
}

export interface TrafficStatsItem {
  id: number;
  agent_id?: number;
  job_id?: number;
  original_size: number;
  transferred_size: number;
  compression_ratio: number;
  cache_hits: number;
  zero_blocks: number;
  duration_ms: number;
  throughput_mbps: number;
  timestamp?: string;
}

export interface GlobalCacheStatsItem {
  id: number;
  agent_id?: number;
  total_entries: number;
  hit_count: number;
  miss_count: number;
  hit_ratio: number;
  size_bytes: number;
  last_reset_at?: string;
}

export interface ZabbixConfig {
  id: number;
  server_url?: string;
  trapper_port: number;
  api_token?: string;
  enabled: boolean;
  last_sync_at?: string;
  metrics_interval_seconds: number;
}

export interface AuditLogEntry {
  id: number;
  user_id?: number;
  action: string;
  resource_type?: string;
  resource_id?: string;
  details: Record<string, any>;
  ip_address?: string;
  created_at?: string;
}

export interface DashboardSummary {
  total_agents: number;
  online_agents: number;
  successful_jobs_24h: number;
  active_cdp_sessions: number;
  traffic_saved_tb: number;
  cache_hit_ratio: number;
  avg_compression_ratio: number;
}

export interface LoginResponse {
  access_token: string;
  refresh_token: string;
  token_type: string;
}

export interface WebSocketMessage {
  type: string;
  data?: any;
}
