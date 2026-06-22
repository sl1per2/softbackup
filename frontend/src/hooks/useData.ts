import { useState, useEffect, useCallback } from 'react';
import { message } from 'antd';
import api from '../utils/api';

export function useStorages() {
  const [storages, setStorages] = useState<any[]>([]);
  const [loading, setLoading] = useState(false);

  const fetch = useCallback(async () => {
    setLoading(true);
    try {
      const res = await api.get('/api/storages');
      setStorages(res.data);
    } catch (err: any) {
      message.error('Failed to load storages');
    }
    setLoading(false);
  }, []);

  useEffect(() => { fetch(); }, [fetch]);

  return { storages, loading, refetch: fetch };
}

export function usePolicies() {
  const [policies, setPolicies] = useState<any[]>([]);
  const [loading, setLoading] = useState(false);

  const fetch = useCallback(async () => {
    setLoading(true);
    try {
      const res = await api.get('/api/policies');
      setPolicies(res.data);
    } catch (err: any) {
      message.error('Failed to load policies');
    }
    setLoading(false);
  }, []);

  useEffect(() => { fetch(); }, [fetch]);

  return { policies, loading, refetch: fetch };
}

export function useStorageOptions() {
  const { storages, loading } = useStorages();
  const options = storages.map((s: any) => ({
    value: s.id,
    label: `${s.name} (${s.type})`,
  }));
  return { options, loading };
}

export function usePolicyOptions() {
  const { policies, loading } = usePolicies();
  const options = policies.map((p: any) => ({
    value: p.id,
    label: p.name,
  }));
  return { options, loading };
}
