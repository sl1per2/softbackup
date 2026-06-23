import { useEffect, useState, useCallback } from 'react';
import { Card, Table, Tag, Button, Space, Progress, Modal, Form, Input, InputNumber, Switch, Select, message } from 'antd';
import { SyncOutlined, PlusOutlined, PauseCircleOutlined, StopOutlined } from '@ant-design/icons';
import { motion } from 'framer-motion';
import api from '../utils/api';
import { useStorageOptions } from '../hooks/useData';

export default function Replication() {
  const [jobs, setJobs] = useState<any[]>([]);
  const [loading, setLoading] = useState(false);
  const [modalOpen, setModalOpen] = useState(false);
  const [form] = Form.useForm();
  const { options: storageOptions } = useStorageOptions();

  const fetchJobs = useCallback(async () => {
    setLoading(true);
    try {
      const res = await api.get('/replication');
      setJobs(res.data.jobs || res.data || []);
    } catch (err: any) {
      message.error('Failed to load replication jobs');
    }
    setLoading(false);
  }, []);

  useEffect(() => { fetchJobs(); }, [fetchJobs]);

  const handleStart = async () => {
    try {
      const values = await form.validateFields();
      await api.post('/replication/start', values);
      message.success('Replication started');
      setModalOpen(false);
      form.resetFields();
      fetchJobs();
    } catch (err: any) {
      if (err.response?.data?.detail) {
        message.error(err.response.data.detail);
      }
    }
  };

  const handlePause = async (jobId: string) => {
    try {
      await api.post(`/replication/${jobId}/pause`);
      message.success('Replication paused');
      fetchJobs();
    } catch {
      message.error('Failed to pause replication');
    }
  };

  const handleStop = async (jobId: string) => {
    try {
      await api.post(`/replication/${jobId}/stop`);
      message.success('Replication stopped');
      fetchJobs();
    } catch {
      message.error('Failed to stop replication');
    }
  };

  const columns = [
    {
      title: 'Job ID',
      dataIndex: 'job_id',
      key: 'id',
      render: (v: string) => <Tag>{v}</Tag>,
    },
    {
      title: 'Status',
      dataIndex: 'status',
      key: 'status',
      render: (s: string) => {
        const colors: Record<string, string> = {
          syncing: 'processing', completed: 'success', failed: 'error', paused: 'warning',
        };
        return <Tag color={colors[s] || 'default'} icon={s === 'syncing' ? <SyncOutlined spin /> : undefined}>{s}</Tag>;
      },
    },
    {
      title: 'Progress',
      key: 'progress',
      render: (_: any, r: any) => (
        <Progress
          percent={r.total_bytes > 0 ? Math.round(r.transferred_bytes / r.total_bytes * 100) : 0}
          size="small"
          status={r.status === 'failed' ? 'exception' : r.status === 'completed' ? 'success' : 'active'}
        />
      ),
    },
    {
      title: 'Speed',
      dataIndex: 'speed_mbps',
      key: 'speed',
      render: (v: number) => v ? `${v.toFixed(1)} MB/s` : '-',
    },
    {
      title: 'Source',
      dataIndex: 'source_storage',
      key: 'source',
      render: (v: string) => v || '-',
    },
    {
      title: 'Destination',
      dataIndex: 'dest_host',
      key: 'dest',
      render: (v: string) => v || '-',
    },
    {
      title: 'Started',
      dataIndex: 'started_at',
      key: 'started',
      render: (v: string) => v ? new Date(v).toLocaleString() : '-',
    },
    {
      title: 'Actions',
      key: 'actions',
      render: (_: any, r: any) => (
        <Space>
          {r.status === 'syncing' && (
            <>
              <Button size="small" icon={<PauseCircleOutlined />} onClick={() => handlePause(r.job_id)}>Pause</Button>
              <Button size="small" danger icon={<StopOutlined />} onClick={() => handleStop(r.job_id)}>Stop</Button>
            </>
          )}
        </Space>
      ),
    },
  ];

  return (
    <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.3 }}>
      <Card title="Remote Replication" extra={
        <Button type="primary" icon={<PlusOutlined />} onClick={() => setModalOpen(true)}>New Replication</Button>
      }>
        <p style={{ color: '#8B949E', marginBottom: 16 }}>
          Synchronize backups between sites following the 3-2-1 rule. Supports incremental replication with digest exchange.
        </p>
        <Table dataSource={jobs} columns={columns} rowKey="job_id" loading={loading} pagination={{ pageSize: 10 }} />
      </Card>

      <Modal
        title="New Replication Job"
        open={modalOpen}
        onCancel={() => setModalOpen(false)}
        footer={
          <Space>
            <Button onClick={() => setModalOpen(false)}>Cancel</Button>
            <Button type="primary" onClick={handleStart}>Start</Button>
          </Space>
        }
        width={600}
      >
        <Form form={form} layout="vertical">
          <Form.Item name="source_storage_id" label="Source Storage" rules={[{ required: true }]}>
            <Select placeholder="Select source storage" options={storageOptions} />
          </Form.Item>
          <Form.Item name="dest_host" label="Destination Host" rules={[{ required: true }]}>
            <Input placeholder="e.g. 192.168.2.100 or replica.example.com" />
          </Form.Item>
          <Form.Item name="dest_port" label="Destination Port">
            <InputNumber defaultValue={9100} style={{ width: '100%' }} />
          </Form.Item>
          <Form.Item name="dest_storage_id" label="Destination Storage" rules={[{ required: true }]}>
            <Select placeholder="Select destination storage" options={storageOptions} />
          </Form.Item>
          <Form.Item name="compress" label="Compress" valuePropName="checked">
            <Switch defaultChecked />
          </Form.Item>
          <Form.Item name="encrypt" label="Encrypt" valuePropName="checked">
            <Switch defaultChecked />
          </Form.Item>
          <Form.Item name="bandwidth_limit_kbps" label="Bandwidth Limit (KB/s)">
            <InputNumber min={0} style={{ width: '100%' }} placeholder="0 = unlimited" />
          </Form.Item>
          <Form.Item name="incremental_only" label="Incremental Only" valuePropName="checked">
            <Switch defaultChecked />
          </Form.Item>
        </Form>
      </Modal>
    </motion.div>
  );
}
