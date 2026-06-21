import React, { useEffect, useState } from 'react';
import { Table, Tag, Input, Select, Button, Modal, Tabs, Space, Typography, Descriptions, Badge } from 'antd';
import { SearchOutlined, ReloadOutlined, CloudServerOutlined, UpOutlined } from '@ant-design/icons';
import { motion } from 'framer-motion';
import axios from 'axios';
import { useAgentStore } from '../stores/agentStore';
import ReactECharts from 'echarts-for-react';

const { Title, Text } = Typography;

export default function Agents() {
  const { agents, setAgents, selectedAgent, setSelectedAgent } = useAgentStore();
  const [loading, setLoading] = useState(false);
  const [search, setSearch] = useState('');
  const [statusFilter, setStatusFilter] = useState<string | null>(null);
  const [modalOpen, setModalOpen] = useState(false);

  const fetchAgents = async () => {
    setLoading(true);
    try {
      const params: any = {};
      if (search) params.status = search;
      if (statusFilter) params.status = statusFilter;
      const res = await axios.get('/api/agents', { params });
      setAgents(res.data);
    } catch {}
    setLoading(false);
  };

  useEffect(() => {
    fetchAgents();
  }, [statusFilter]);

  const columns = [
    {
      title: 'Hostname',
      dataIndex: 'hostname',
      key: 'hostname',
      render: (text: string) => <Text strong style={{ color: '#00E5FF' }}>{text}</Text>,
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
            Details
          </Button>
          <Button size="small" icon={<UpOutlined />}>Upgrade</Button>
        </Space>
      ),
    },
  ];

  return (
    <div>
      <Title level={4} style={{ color: '#E6E6E6', marginBottom: 24 }}>Agents</Title>
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

      <Modal
        title={selectedAgent?.hostname}
        open={modalOpen}
        onCancel={() => setModalOpen(false)}
        footer={null}
        width={700}
        styles={{ body: { background: 'transparent' } }}
      >
        {selectedAgent && (
          <Tabs
            items={[
              {
                key: 'info',
                label: 'Info',
                children: (
                  <Descriptions bordered column={2} size="small">
                    <Descriptions.Item label="Hostname">{selectedAgent.hostname}</Descriptions.Item>
                    <Descriptions.Item label="IP">{selectedAgent.ip}</Descriptions.Item>
                    <Descriptions.Item label="OS">{selectedAgent.os_type}</Descriptions.Item>
                    <Descriptions.Item label="Version">{selectedAgent.version}</Descriptions.Item>
                    <Descriptions.Item label="Core">{selectedAgent.core_version}</Descriptions.Item>
                    <Descriptions.Item label="Transport">{selectedAgent.transport_mode}</Descriptions.Item>
                  </Descriptions>
                ),
              },
              {
                key: 'traffic',
                label: 'Traffic',
                children: (
                  <ReactECharts
                    style={{ height: 250 }}
                    option={{
                      backgroundColor: 'transparent',
                      series: [{
                        type: 'gauge',
                        progress: { show: true, width: 14 },
                        min: 0, max: 100,
                        axisLine: { lineStyle: { width: 14, color: [[1, 'rgba(255,255,255,0.1)']] } },
                        pointer: { show: false },
                        detail: { valueAnimation: true, fontSize: 24, formatter: '{value}%', color: '#00FF88', offsetCenter: [0, '10%'] },
                        data: [{ value: (selectedAgent.cache_hit_ratio * 100).toFixed(0) as any }],
                      }],
                      title: { text: 'Cache Hit Ratio', left: 'center', textStyle: { color: '#8B949E' } },
                    }}
                  />
                ),
              },
            ]}
          />
        )}
      </Modal>
    </div>
  );
}
