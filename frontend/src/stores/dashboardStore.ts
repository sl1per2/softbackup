import { create } from 'zustand';

interface DashboardSummary {
  total_agents: number;
  agents_online: number;
  jobs_successful_24h: number;
  jobs_failed_24h: number;
  cdp_sessions_active: number;
  traffic_saved_bytes: number;
  avg_cache_hit_ratio: number;
  avg_compression_ratio: number;
}

interface DashboardState {
  summary: DashboardSummary | null;
  chartData: any;
  recentJobs: any[];
  wsConnected: boolean;
  setSummary: (s: DashboardSummary) => void;
  setChartData: (d: any) => void;
  setRecentJobs: (j: any[]) => void;
  setWsConnected: (c: boolean) => void;
}

export const useDashboardStore = create<DashboardState>((set) => ({
  summary: null,
  chartData: null,
  recentJobs: [],
  wsConnected: false,
  setSummary: (s) => set({ summary: s }),
  setChartData: (d) => set({ chartData: d }),
  setRecentJobs: (j) => set({ recentJobs: j }),
  setWsConnected: (c) => set({ wsConnected: c }),
}));
