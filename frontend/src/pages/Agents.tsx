import React, { useEffect, useState, useCallback } from 'react';
import { Table, Tag, Input, Select, Button, Badge, Space, Typography, message } from 'antd';
import { SearchOutlined, ReloadOutlined, CloudDownloadOutlined } from '@ant-design/icons';
import { motion } from 'framer-motion';
import api from '../utils/api';
import { useAgentStore } from '../stores/agentStore';
import AgentDetailModal from '../components/AgentDetailModal';

const { Title, Text } = Typography;

export default function Agents() {
  const { agents, setAgents, selectedAgent, setSelectedAgent } = useAgentStore();
  const [loading, setLoading] = useState(false);
  const [search, setSearch] = useState('');
  const [statusFilter, setStatusFilter] = useState<string | null>(null);
  const [modalOpen, setModalOpen] = useState(false);

  const fetchAgents = useCallback(async () => {
    setLoading(true);
    try {
      const params: any = {};
      if (search) params.search = search;
      if (statusFilter) params.status = statusFilter;
      const res = await api.get('/api/agents', { params });
      setAgents(res.data);
    } catch (err: any) {
      message.error('Failed to load agents');
    }
    setLoading(false);
  }, [search, statusFilter]);

  useEffect(() => { fetchAgents(); }, [fetchAgents]);

  const columns = [
    {
      title: 'Hostname',
      dataIndex: 'hostname',
      key: 'hostname',
      render: (text: string) => <Text strong style={{ color: '#00E5FF', cursor: 'pointer' }}>{text}</Text>,
    },
    { title: 'IP', dataIndex: 'ip', key: 'ip' },
    { title: 'OS', dataIndex: 'os_type', key: 'os_type', render: (t: string) => <Tag>{t}</Tag> },
    {
      title: 'Transport',
      dataIndex: 'transport_mode',
      key: 'transport',
      render: (t: string) => <Tag color="blue">{t}</Tag>,
    },
    {
      title: 'Cache Hit%',
      dataIndex: 'cache_hit_ratio',
      key: 'cache',
      render: (v: number) => `${(v * 100).toFixed(1)}%`,
    },
    {
      title: 'Status',
      dataIndex: 'status',
      key: 'status',
      render: (status: string) => {
        const colorMap: Record<string, string> = {
          online: 'success', offline: 'default', backing_up: 'processing', cdp_active: 'processing', error: 'error',
        };
        return <Badge status={colorMap[status] as any || 'default'} text={status?.toUpperCase()} />;
      },
    },
    {
      title: 'Last Seen',
      dataIndex: 'last_seen',
      key: 'last_seen',
      render: (t: string) => t ? new Date(t).toLocaleString() : '-',
    },
    {
      title: 'Actions',
      key: 'actions',
      render: (_: any, record: any) => (
        <Space>
          <Button size="small" onClick={() => { setSelectedAgent(record); setModalOpen(true); }}>
            Manage
          </Button>
        </Space>
      ),
    },
  ];

  return (
    <div>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: 24 }}>
        <Title level={4} style={{ color: '#E6E6E6', margin: 0 }}>Agents</Title>
      </div>

      <Space style={{ marginBottom: 16 }}>
        <Input.Search
          placeholder="Search agents..."
          style={{ width: 300 }}
          onSearch={(v) => setSearch(v)}
        />
        <Select
          placeholder="Status"
          style={{ width: 150 }}
          allowClear
          onChange={(v) => setStatusFilter(v)}
          options={[
            { value: 'online', label: 'Online' },
            { value: 'offline', label: 'Offline' },
            { value: 'backing_up', label: 'Backing Up' },
            { value: 'cdp_active', label: 'CDP Active' },
            { value: 'error', label: 'Error' },
          ]}
        />
        <Button icon={<ReloadOutlined />} onClick={fetchAgents} />
      </Space>

      <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.3 }}>
        <Table
          dataSource={agents}
          columns={columns}
          rowKey="id"
          loading={loading}
          pagination={{ pageSize: 20 }}
        />
      </motion.div>

      <AgentDetailModal
        agent={selectedAgent}
        open={modalOpen}
        onClose={() => { setModalOpen(false); setSelectedAgent(null); fetchAgents(); }}
      />
    </div>
  );
}
