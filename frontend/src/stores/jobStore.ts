import { create } from 'zustand';

interface JobData {
  id: number;
  policy_id: number | null;
  agent_id: number | null;
  storage_id: number | null;
  type: string;
  status: string;
  transport_mode_used: string | null;
  started_at: string | null;
  finished_at: string | null;
  size_bytes: number;
  size_transferred_bytes: number;
  dedup_saved_bytes: number;
  compression_ratio: number;
  cache_hit_ratio: number;
  zero_blocks_skipped: number;
  chunks_total: number;
  chunks_cached: number;
  error_log: string | null;
}

interface JobState {
  jobs: JobData[];
  filters: Record<string, any>;
  page: number;
  total: number;
  setJobs: (j: JobData[]) => void;
  setFilters: (f: Record<string, any>) => void;
  setPage: (p: number) => void;
  setTotal: (t: number) => void;
}

export const useJobStore = create<JobState>((set) => ({
  jobs: [],
  filters: {},
  page: 1,
  total: 0,
  setJobs: (jobs) => set({ jobs }),
  setFilters: (filters) => set({ filters }),
  setPage: (page) => set({ page }),
  setTotal: (total) => set({ total }),
}));
