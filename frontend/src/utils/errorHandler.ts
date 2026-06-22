import { message } from 'antd';

export function handleApiError(err: any, fallbackMsg = 'Operation failed') {
  const detail = err?.response?.data?.detail || err?.message || fallbackMsg;
  message.error(detail);
}

export async function apiCall<T>(
  fn: () => Promise<T>,
  errorMsg?: string,
): Promise<T | null> {
  try {
    return await fn();
  } catch (err: any) {
    handleApiError(err, errorMsg);
    return null;
  }
}
