import { create } from 'zustand';
import axios from 'axios';

interface AuthState {
  accessToken: string | null;
  refreshTokenValue: string | null;
  user: any | null;
  isAuthenticated: boolean;
  login: (username: string, password: string) => Promise<boolean>;
  logout: () => void;
  doRefreshToken: () => Promise<void>;
  loadFromStorage: () => void;
}

export const useAuthStore = create<AuthState>((set, get) => ({
  accessToken: null,
  refreshTokenValue: null,
  user: null,
  isAuthenticated: false,

  loadFromStorage: () => {
    const token = localStorage.getItem('access_token');
    const refresh = localStorage.getItem('refresh_token');
    if (token) {
      set({ accessToken: token, refreshTokenValue: refresh, isAuthenticated: true });
      axios.defaults.headers.common['Authorization'] = `Bearer ${token}`;
    }
  },

  login: async (username, password) => {
    try {
      const formData = new URLSearchParams();
      formData.append('username', username);
      formData.append('password', password);
      const res = await axios.post('/api/login', formData);
      const { access_token, refresh_token } = res.data;
      localStorage.setItem('access_token', access_token);
      localStorage.setItem('refresh_token', refresh_token);
      axios.defaults.headers.common['Authorization'] = `Bearer ${access_token}`;
      set({ accessToken: access_token, refreshTokenValue: refresh_token, isAuthenticated: true });
      return true;
    } catch {
      return false;
    }
  },

  logout: () => {
    localStorage.removeItem('access_token');
    localStorage.removeItem('refresh_token');
    delete axios.defaults.headers.common['Authorization'];
    set({ accessToken: null, refreshTokenValue: null, user: null, isAuthenticated: false });
  },

  doRefreshToken: async () => {
    const refresh = get().refreshTokenValue;
    if (!refresh) return;
    try {
      const res = await axios.post('/api/refresh', { refresh_token: refresh });
      const { access_token, refresh_token } = res.data;
      localStorage.setItem('access_token', access_token);
      localStorage.setItem('refresh_token', refresh_token);
      axios.defaults.headers.common['Authorization'] = `Bearer ${access_token}`;
      set({ accessToken: access_token, refreshTokenValue: refresh_token });
    } catch {
      get().logout();
    }
  },
}));
