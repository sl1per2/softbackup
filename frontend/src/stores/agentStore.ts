import { create } from 'zustand';

interface AgentData {
  id: number;
  hostname: string;
  ip: string;
  os_type: string;
  version: string | null;
  status: string;
  last_seen: string | null;
  core_version: string | null;
  transport_mode: string;
  cache_size_bytes: number;
  cache_hit_ratio: number;
}

interface AgentState {
  agents: AgentData[];
  selectedAgent: AgentData | null;
  setAgents: (a: AgentData[]) => void;
  setSelectedAgent: (a: AgentData | null) => void;
  updateAgent: (id: number, data: Partial<AgentData>) => void;
}

export const useAgentStore = create<AgentState>((set) => ({
  agents: [],
  selectedAgent: null,
  setAgents: (agents) => set({ agents }),
  setSelectedAgent: (agent) => set({ selectedAgent: agent }),
  updateAgent: (id, data) =>
    set((state) => ({
      agents: state.agents.map((a) => (a.id === id ? { ...a, ...data } : a)),
    })),
}));
