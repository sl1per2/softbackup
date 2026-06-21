import { Tag, Badge, Spin } from 'antd';
import {
  CheckCircleOutlined, CloseCircleOutlined, SyncOutlined,
  ClockCircleOutlined, MinusCircleOutlined,
} from '@ant-design/icons';

const statusConfig: Record<string, { color: string; icon: React.ReactNode }> = {
  online: { color: 'green', icon: <Badge status="processing" color="green" /> },
  offline: { color: 'red', icon: <Badge status="error" /> },
  maintenance: { color: 'orange', icon: <Badge status="warning" /> },
  pending: { color: 'default', icon: <ClockCircleOutlined /> },
  running: { color: 'processing', icon: <Spin size="small" /> },
  completed: { color: 'success', icon: <CheckCircleOutlined /> },
  failed: { color: 'error', icon: <CloseCircleOutlined /> },
  cancelled: { color: 'default', icon: <MinusCircleOutlined /> },
  active: { color: 'processing', icon: <SyncOutlined spin /> },
  paused: { color: 'warning', icon: <ClockCircleOutlined /> },
  stopped: { color: 'default', icon: <MinusCircleOutlined /> },
};

export default function StatusBadge({ status }: { status: string }) {
  const config = statusConfig[status] || { color: 'default', icon: null };
  return (
    <Tag color={config.color} icon={config.icon}>
      {status.toUpperCase()}
    </Tag>
  );
}
