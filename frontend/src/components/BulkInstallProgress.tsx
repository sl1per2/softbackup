import React, { useEffect, useState } from 'react';
import { Modal, Table, Tag, Progress, Typography, Space, Button, message } from 'antd';
import { CheckCircleOutlined, CloseCircleOutlined, LoadingOutlined } from '@ant-design/icons';
import api from '../utils/api';

const { Text } = Typography;

interface Props {
  open: boolean;
  hosts: Array<{ host: string; os_type: string; username: string; password: string }>;
  onClose: () => void;
  onComplete?: () => void;
}

interface InstallJob {
  host: string;
  status: 'pending' | 'installing' | 'success' | 'failed';
  progress: number;
  message?: string;
}

export default function BulkInstallProgress({ open, hosts, onClose, onComplete }: Props) {
  const [jobs, setJobs] = useState<InstallJob[]>([]);
  const [running, setRunning] = useState(false);

  useEffect(() => {
    if (open && hosts.length > 0) {
      const initial = hosts.map(h => ({
        host: h.host,
        status: 'pending' as const,
        progress: 0,
      }));
      setJobs(initial);
      startBulkInstall(initial);
    }
  }, [open, hosts]);

  const startBulkInstall = async (initialJobs: InstallJob[]) => {
    setRunning(true);
    for (let i = 0; i < initialJobs.length; i++) {
      const host = hosts[i];
      setJobs(prev => prev.map((j, idx) => idx === i ? { ...j, status: 'installing', progress: 50 } : j));

      try {
        const endpoint = host.os_type === 'windows' ? '/agents/deploy/winrm' : '/agents/deploy/ssh';
        const res = await api.post(endpoint, {
          host: host.host, username: host.username, password: host.password,
          os_type: host.os_type, port: host.os_type === 'windows' ? 5985 : 22,
        });

        setJobs(prev => prev.map((j, idx) => idx === i ? {
          ...j,
          status: res.data.success ? 'success' : 'failed',
          progress: 100,
          message: res.data.success ? 'Installed' : res.data.error,
        } : j));
      } catch {
        setJobs(prev => prev.map((j, idx) => idx === i ? {
          ...j, status: 'failed', progress: 100, message: 'Connection failed',
        } : j));
      }
    }
    setRunning(false);
    onComplete?.();
  };

  const columns = [
    {
      title: 'Host',
      dataIndex: 'host',
      key: 'host',
      render: (v: string) => <Text code>{v}</Text>,
    },
    {
      title: 'Status',
      dataIndex: 'status',
      key: 'status',
      render: (s: string) => {
        const map: Record<string, { color: string; icon: React.ReactNode }> = {
          pending: { color: 'default', icon: null },
          installing: { color: 'processing', icon: <LoadingOutlined /> },
          success: { color: 'success', icon: <CheckCircleOutlined /> },
          failed: { color: 'error', icon: <CloseCircleOutlined /> },
        };
        const m = map[s] || map.pending;
        return <Tag color={m.color} icon={m.icon}>{s.toUpperCase()}</Tag>;
      },
    },
    {
      title: 'Progress',
      dataIndex: 'progress',
      key: 'progress',
      render: (v: number) => <Progress percent={v} size="small" />,
    },
    {
      title: 'Message',
      dataIndex: 'message',
      key: 'message',
      render: (v: string) => v ? <Text type="secondary" style={{ fontSize: 12 }}>{v}</Text> : '-',
    },
  ];

  const successCount = jobs.filter(j => j.status === 'success').length;
  const failCount = jobs.filter(j => j.status === 'failed').length;

  return (
    <Modal
      title={`Bulk Install — ${hosts.length} hosts`}
      open={open}
      onCancel={!running ? onClose : undefined}
      footer={!running ? [
        <Button key="close" type="primary" onClick={onClose}>Close</Button>,
      ] : undefined}
      width={700}
      closable={!running}
    >
      {jobs.length > 0 && (
        <div style={{ marginBottom: 16 }}>
          <Space>
            <Tag>Total: {jobs.length}</Tag>
            <Tag color="success">Success: {successCount}</Tag>
            {failCount > 0 && <Tag color="error">Failed: {failCount}</Tag>}
          </Space>
        </div>
      )}
      <Table dataSource={jobs} columns={columns} rowKey="host" pagination={false} size="small" />
    </Modal>
  );
}
