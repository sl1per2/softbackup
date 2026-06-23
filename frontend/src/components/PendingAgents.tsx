import React, { useEffect, useState, useCallback } from 'react';
import { Card, Table, Tag, Button, Space, Typography, message } from 'antd';
import { CloudServerOutlined, ReloadOutlined, CheckCircleOutlined, ClockCircleOutlined } from '@ant-design/icons';
import api from '../utils/api';

const { Text } = Typography;

export default function PendingAgents() {
  const [agents, setAgents] = useState<any[]>([]);
  const [loading, setLoading] = useState(false);

  const fetchPending = useCallback(async () => {
    setLoading(true);
    try {
      const res = await api.get('/agents', { params: { status: 'pending' } });
      setAgents(res.data || []);
    } catch {
      message.error('Failed to load pending agents');
    }
    setLoading(false);
  }, []);

  useEffect(() => { fetchPending(); }, [fetchPending]);

  const handleApprove = async (agentId: number) => {
    try {
      await api.post(`/agents/${agentId}/approve`);
      message.success('Agent approved');
      fetchPending();
    } catch {
      message.error('Failed to approve agent');
    }
  };

  const handleReject = async (agentId: number) => {
    try {
      await api.post(`/agents/${agentId}/reject`);
      message.success('Agent rejected');
      fetchPending();
    } catch {
      message.error('Failed to reject agent');
    }
  };

  const columns = [
    {
      title: 'Hostname',
      dataIndex: 'hostname',
      key: 'hostname',
      render: (v: string) => <Text strong style={{ color: '#00E5FF' }}>{v}</Text>,
    },
    { title: 'IP', dataIndex: 'ip', key: 'ip' },
    { title: 'OS', dataIndex: 'os_type', key: 'os_type', render: (v: string) => <Tag>{v}</Tag> },
    { title: 'Version', dataIndex: 'version', key: 'version' },
    {
      title: 'Registered',
      dataIndex: 'created_at',
      key: 'created',
      render: (v: string) => v ? new Date(v).toLocaleString() : '-',
    },
    {
      title: 'Actions',
      key: 'actions',
      render: (_: any, r: any) => (
        <Space>
          <Button size="small" type="primary" icon={<CheckCircleOutlined />}
            onClick={() => handleApprove(r.id)}>Approve</Button>
          <Button size="small" danger onClick={() => handleReject(r.id)}>Reject</Button>
        </Space>
      ),
    },
  ];

  return (
    <Card title={<><ClockCircleOutlined /> Pending Agent Registrations</>}
      extra={<Button icon={<ReloadOutlined />} onClick={fetchPending}>Refresh</Button>}
    >
      <Table dataSource={agents} columns={columns} rowKey="id" loading={loading}
        pagination={{ pageSize: 10 }} locale={{ emptyText: 'No pending agents' }} />
    </Card>
  );
}
