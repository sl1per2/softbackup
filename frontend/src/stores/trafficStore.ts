import { create } from 'zustand';

interface TrafficStoreState {
  stats: any[];
  cacheStats: any;
  bandwidthUsage: any;
  setStats: (s: any[]) => void;
  setCacheStats: (s: any) => void;
  setBandwidthUsage: (u: any) => void;
}

export const useTrafficStore = create<TrafficStoreState>((set) => ({
  stats: [],
  cacheStats: null,
  bandwidthUsage: null,
  setStats: (stats) => set({ stats }),
  setCacheStats: (cacheStats) => set({ cacheStats }),
  setBandwidthUsage: (bandwidthUsage) => set({ bandwidthUsage }),
}));
