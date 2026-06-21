import { useState, useCallback } from 'react';
import api from '../utils/api';

export function useApi() {
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const request = useCallback(async <T>(fn: () => Promise<T>): Promise<T | null> => {
    setLoading(true);
    setError(null);
    try {
      const result = await fn();
      return result;
    } catch (err: any) {
      setError(err.response?.data?.detail || err.message || 'Request failed');
      return null;
    } finally {
      setLoading(false);
    }
  }, []);

  return { loading, error, request, api };
}
