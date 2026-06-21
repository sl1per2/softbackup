import React, { useEffect, useState } from 'react';
import { Table, Button, Input, Modal, Form, Switch, Select, InputNumber, Tabs, Tag, Space, Typography, message } from 'antd';
import { PlusOutlined, PlayCircleOutlined, SyncOutlined, SearchOutlined } from '@ant-design/icons';
import { motion } from 'framer-motion';
import axios from 'axios';

const { Title, Text } = Typography;

export default function Policies() {
  const [policies, setPolicies] = useState<any[]>([]);
  const [loading, setLoading] = useState(false);
  const [modalOpen, setModalOpen] = useState(false);
  const [editingPolicy, setEditingPolicy] = useState<any>(null);
  const [search, setSearch] = useState('');
  const [form] = Form.useForm();

  const fetchPolicies = async () => {
    setLoading(true);
    try {
      const params: any = {};
      if (search) params.search = search;
      const res = await axios.get('/api/policies', { params });
      setPolicies(res.data);
    } catch {}
    setLoading(false);
  };

  useEffect(() => { fetchPolicies(); }, [search]);

  const handleSave = async () => {
    try {
      const values = await form.validateFields();
      if (editingPolicy) {
        await axios.put(`/api/policies/${editingPolicy.id}`, values);
      } else {
        await axios.post('/api/policies', values);
      }
      message.success('Policy saved');
      setModalOpen(false);
      fetchPolicies();
    } catch {}
  };

  const handleRun = async (id: number) => {
    try {
      await axios.post(`/api/policies/${id}/run`);
      message.success('Backup job created');
    } catch {
      message.error('Failed to start');
    }
  };

  const columns = [
    {
      title: 'Name',
      dataIndex: 'name',
      key: 'name',
      render: (t: string) => <Text strong style={{ color: '#00E5FF' }}>{t}</Text>,
    },
    {
      title: 'Type',
      key: 'type',
      render: (_: any, r: any) => <Tag color="blue">{r.schedule?.type || 'FULL'}</Tag>,
    },
    {
      title: 'Compression',
      dataIndex: 'compression_level',
      key: 'compression',
      render: (v: number) => ['None', 'ZSTD-3', 'ZSTD-12'][v] || 'ZSTD-3',
    },
    {
      title: 'CDP',
      dataIndex: 'cdp_enabled',
      key: 'cdp',
      render: (v: boolean) => v ? <Tag color="processing">ACTIVE</Tag> : <Tag>OFF</Tag>,
    },
    {
      title: 'Retention',
      key: 'retention',
      render: (_: any, r: any) => {
        const gfs = r.retention_gfs || {};
        return `D:${gfs.daily || 0} W:${gfs.weekly || 0} M:${gfs.monthly || 0}`;
      },
    },
    {
      title: 'Bandwidth',
      dataIndex: 'bandwidth_limit_kbps',
      key: 'bw',
      render: (v: number) => v > 0 ? `${v} KB/s` : 'Unlimited',
    },
    {
      title: 'Actions',
      key: 'actions',
      render: (_: any, record: any) => (
        <Space>
          <Button size="small" icon={<PlayCircleOutlined />} onClick={() => handleRun(record.id)}>Run</Button>
          <Button size="small" onClick={() => { setEditingPolicy(record); form.setFieldsValue(record); setModalOpen(true); }}>Edit</Button>
          {record.cdp_enabled && (
            <>
              <Button size="small" type="primary" icon={<SyncOutlined />} onClick={() => axios.post(`/api/policies/${record.id}/cdp/start`)}>CDP Start</Button>
              <Button size="small" danger onClick={() => axios.post(`/api/policies/${record.id}/cdp/stop`)}>CDP Stop</Button>
            </>
          )}
        </Space>
      ),
    },
  ];

  return (
    <div>
      <Title level={4} style={{ color: '#E6E6E6', marginBottom: 24 }}>Policies</Title>
      <Space style={{ marginBottom: 16 }}>
        <Input.Search placeholder="Search policies..." style={{ width: 300 }} onSearch={setSearch} />
        <Button type="primary" icon={<PlusOutlined />} onClick={() => { setEditingPolicy(null); form.resetFields(); setModalOpen(true); }}>
          Create Policy
        </Button>
      </Space>

      <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.3 }}>
        <Table dataSource={policies} columns={columns} rowKey="id" loading={loading} pagination={{ pageSize: 20 }} />
      </motion.div>

      <Modal
        title={editingPolicy ? 'Edit Policy' : 'Create Policy'}
        open={modalOpen}
        onCancel={() => setModalOpen(false)}
        onOk={handleSave}
        width={900}
        okText="Save"
      >
        <Form form={form} layout="vertical" initialValues={{ compression_level: 1, transport_mode: 'AUTO', cdp_enabled: false }}>
          <Tabs items={[
            {
              key: 'general',
              label: 'General',
              children: (
                <>
                  <Form.Item name="name" label="Name" rules={[{ required: true }]}>
                    <Input placeholder="Policy name" />
                  </Form.Item>
                  <Form.Item name="transport_mode" label="Transport Mode">
                    <Select options={[
                      { value: 'AUTO', label: 'Auto' },
                      { value: 'DIRECT_SAN', label: 'Direct SAN' },
                      { value: 'HOT_ADD', label: 'Hot Add' },
                      { value: 'NETWORK', label: 'Network' },
                    ]} />
                  </Form.Item>
                </>
              ),
            },
            {
              key: 'sources',
              label: 'Sources',
              children: (
                <>
                  <Form.Item name="source_paths" label="Source Paths">
                    <Select mode="tags" placeholder="/path/to/backup" />
                  </Form.Item>
                  <Form.Item name="exclude_patterns" label="Exclude Patterns">
                    <Select mode="tags" placeholder="*.tmp, *.log" />
                  </Form.Item>
                </>
              ),
            },
            {
              key: 'retention',
              label: 'Retention',
              children: (
                <Form.Item name="retention_gfs" label="GFS Retention">
                  <Space>
                    <InputNumber addonAfter="days" min={0} placeholder="Daily" />
                    <InputNumber addonAfter="weeks" min={0} placeholder="Weekly" />
                    <InputNumber addonAfter="months" min={0} placeholder="Monthly" />
                  </Space>
                </Form.Item>
              ),
            },
            {
              key: 'cdp',
              label: 'CDP',
              children: (
                <>
                  <Form.Item name="cdp_enabled" label="Enable CDP" valuePropName="checked">
                    <Switch />
                  </Form.Item>
                  <Form.Item name="cdp_interval_seconds" label="Interval (seconds)">
                    <InputNumber min={1} max={300} />
                  </Form.Item>
                  <Form.Item name="cdp_max_latency_ms" label="Max Latency (ms)">
                    <InputNumber min={100} max={60000} />
                  </Form.Item>
                  <Form.Item name="cdp_retention_minutes" label="Retention (minutes)">
                    <InputNumber min={1} max={43200} />
                  </Form.Item>
                </>
              ),
            },
            {
              key: 'optimization',
              label: 'Optimization',
              children: (
                <>
                  <Form.Item name="compression_level" label="Compression">
                    <Select options={[
                      { value: 0, label: 'None' },
                      { value: 1, label: 'ZSTD-3 (Balanced)' },
                      { value: 2, label: 'ZSTD-12 (Maximum)' },
                    ]} />
                  </Form.Item>
                  <Form.Item name="bandwidth_limit_kbps" label="Bandwidth Limit (KB/s)">
                    <InputNumber min={0} placeholder="0 = unlimited" />
                  </Form.Item>
                </>
              ),
            },
            {
              key: 'encryption',
              label: 'Encryption',
              children: (
                <Form.Item name="encryption_enabled" label="Enable Encryption" valuePropName="checked">
                  <Switch />
                </Form.Item>
              ),
            },
          ]} />
        </Form>
      </Modal>
    </div>
  );
}
