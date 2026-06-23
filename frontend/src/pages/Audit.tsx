import React, { useEffect, useState } from 'react';
import { Table, Tag, Select, DatePicker, Button, Typography, Space, message } from 'antd';
import { DownloadOutlined, ReloadOutlined } from '@ant-design/icons';
import { motion } from 'framer-motion';
import axios from 'axios';

const { Title, Text } = Typography;
const { RangePicker } = DatePicker;

export default function AuditPage() {
  const [logs, setLogs] = useState<any[]>([]);
  const [loading, setLoading] = useState(false);
  const [filters, setFilters] = useState<Record<string, any>>({});

  const fetchLogs = async () => {
    setLoading(true);
    try {
      const res = await axios.get('/api/audit', { params: filters });
      setLogs(res.data);
    } catch {
      message.error('Failed to load audit logs');
    }
    setLoading(false);
  };

  useEffect(() => { fetchLogs(); }, [filters]);

  const handleExport = async () => {
    const params = new URLSearchParams(filters as any);
    window.open(`/api/audit/export?${params.toString()}`, '_blank');
  };

  const columns = [
    {
      title: 'Timestamp',
      dataIndex: 'created_at',
      key: 'time',
      render: (t: string) => <Text>{new Date(t).toLocaleString()}</Text>,
    },
    {
      title: 'User',
      dataIndex: 'user_id',
      key: 'user',
      render: (v: number) => <Text code>#{v}</Text>,
    },
    {
      title: 'Action',
      dataIndex: 'action',
      key: 'action',
      render: (a: string) => {
        const colors: Record<string, string> = {
          create: 'success', update: 'processing', delete: 'error', run_policy: 'blue',
          cdp_start: 'cyan', cdp_stop: 'warning', cancel: 'default',
        };
        return <Tag color={colors[a] || 'default'}>{a}</Tag>;
      },
    },
    {
      title: 'Resource Type',
      dataIndex: 'resource_type',
      key: 'resource_type',
      render: (t: string) => <Tag>{t}</Tag>,
    },
    {
      title: 'Resource ID',
      dataIndex: 'resource_id',
      key: 'resource_id',
      render: (v: number) => v ? <Text code>#{v}</Text> : '-',
    },
    {
      title: 'Details',
      dataIndex: 'details',
      key: 'details',
      render: (d: any) => (
        <Text code style={{ fontSize: 11 }}>
          {JSON.stringify(d)}
        </Text>
      ),
    },
    {
      title: 'IP',
      dataIndex: 'ip_address',
      key: 'ip',
      render: (v: string) => v || '-',
    },
  ];

  return (
    <div>
      <Title level={4} style={{ color: '#E6E6E6', marginBottom: 24 }}>Audit Log</Title>
      <Space style={{ marginBottom: 16 }} wrap>
        <Select
          placeholder="Action"
          style={{ width: 150 }}
          allowClear
          onChange={(v) => setFilters((f) => ({ ...f, action: v }))}
          options={[
            { value: 'create', label: 'Create' },
            { value: 'update', label: 'Update' },
            { value: 'delete', label: 'Delete' },
            { value: 'run_policy', label: 'Run Policy' },
            { value: 'cdp_start', label: 'CDP Start' },
            { value: 'cancel', label: 'Cancel' },
          ]}
        />
        <Select
          placeholder="Resource Type"
          style={{ width: 150 }}
          allowClear
          onChange={(v) => setFilters((f) => ({ ...f, resource_type: v }))}
          options={[
            { value: 'agent', label: 'Agent' },
            { value: 'policy', label: 'Policy' },
            { value: 'storage', label: 'Storage' },
            { value: 'job', label: 'Job' },
          ]}
        />
        <RangePicker
          onChange={(d) => setFilters((f) => ({
            ...f,
            date_from: d?.[0]?.toISOString(),
            date_to: d?.[1]?.toISOString(),
          }))}
        />
        <Button icon={<ReloadOutlined />} onClick={fetchLogs} />
        <Button icon={<DownloadOutlined />} onClick={handleExport}>Export CSV</Button>
      </Space>

      <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.3 }}>
        <Table
          dataSource={logs}
          columns={columns}
          rowKey="id"
          loading={loading}
          pagination={{ pageSize: 20 }}
        />
      </motion.div>
    </div>
  );
}
