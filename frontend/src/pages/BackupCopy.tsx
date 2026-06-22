import { useEffect, useState, useCallback } from 'react';
import { Card, Table, Button, Space, Tag, Modal, Form, Input, InputNumber, Select, message, Popconfirm } from 'antd';
import { SyncOutlined, PlusOutlined, PlayCircleOutlined } from '@ant-design/icons';
import { motion } from 'framer-motion';
import api from '../utils/api';
import { useStorageOptions } from '../hooks/useData';

export default function BackupCopy() {
  const [jobs, setJobs] = useState<any[]>([]);
  const [loading, setLoading] = useState(false);
  const [modalOpen, setModalOpen] = useState(false);
  const [form] = Form.useForm();
  const { options: storageOptions } = useStorageOptions();

  const fetchJobs = useCallback(async () => {
    setLoading(true);
    try {
      const res = await api.get('/api/backup-copy');
      setJobs(res.data.jobs || res.data || []);
    } catch {
      message.error('Failed to load backup copy jobs');
    }
    setLoading(false);
  }, []);

  useEffect(() => { fetchJobs(); }, [fetchJobs]);

  const handleCreate = async () => {
    try {
      const values = await form.validateFields();
      await api.post('/api/backup-copy', values);
      message.success('Backup Copy job created');
      setModalOpen(false);
      form.resetFields();
      fetchJobs();
    } catch (err: any) {
      if (err.response?.data?.detail) {
        message.error(err.response.data.detail);
      }
    }
  };

  const handleRun = async (jobId: string) => {
    try {
      await api.post(`/api/backup-copy/${jobId}/run`);
      message.success('Backup Copy job started');
      fetchJobs();
    } catch {
      message.error('Failed to start backup copy');
    }
  };

  const columns = [
    {
      title: 'Name',
      dataIndex: 'name',
      key: 'name',
      render: (v: string) => <span style={{ color: '#00E5FF', fontWeight: 500 }}>{v}</span>,
    },
    {
      title: 'Mode',
      dataIndex: 'mode',
      key: 'mode',
      render: (m: string) => {
        const colors: Record<string, string> = { immediate: 'blue', scheduled: 'cyan', mirror: 'purple' };
        return <Tag color={colors[m] || 'default'}>{m}</Tag>;
      },
    },
    {
      title: 'Status',
      dataIndex: 'status',
      key: 'status',
      render: (s: string) => {
        const colors: Record<string, string> = { running: 'processing', completed: 'success', failed: 'error', pending: 'warning' };
        return <Tag color={colors[s] || 'default'}>{s?.toUpperCase()}</Tag>;
      },
    },
    {
      title: 'Source',
      dataIndex: 'source_storage',
      key: 'source',
      render: (v: string) => v || '-',
    },
    {
      title: 'Destination',
      dataIndex: 'dest_storage',
      key: 'dest',
      render: (v: string) => v || '-',
    },
    {
      title: 'Last Run',
      dataIndex: 'last_run_at',
      key: 'last_run',
      render: (v: string) => v ? new Date(v).toLocaleString() : '-',
    },
    {
      title: 'Actions',
      key: 'actions',
      render: (_: any, r: any) => (
        <Space>
          <Button size="small" type="primary" icon={<PlayCircleOutlined />} onClick={() => handleRun(r.job_id)}>Run Now</Button>
        </Space>
      ),
    },
  ];

  return (
    <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.3 }}>
      <Card
        title="Backup Copy Jobs"
        extra={
          <Space>
            <Button type="primary" icon={<PlusOutlined />} onClick={() => { form.resetFields(); setModalOpen(true); }}>New Copy Job</Button>
            <Button icon={<SyncOutlined />} onClick={fetchJobs}>Refresh</Button>
          </Space>
        }
      >
        <Table dataSource={jobs} columns={columns} rowKey="job_id" loading={loading} pagination={{ pageSize: 10 }} />
      </Card>

      <Modal open={modalOpen} onCancel={() => setModalOpen(false)} title="New Backup Copy" onOk={handleCreate} okText="Create">
        <Form form={form} layout="vertical" initialValues={{ mode: 'immediate', archive_retention_weekly: 4, archive_retention_monthly: 12, archive_retention_yearly: 7 }}>
          <Form.Item name="name" label="Name" rules={[{ required: true }]}>
            <Input placeholder="e.g. Secondary Site Copy" />
          </Form.Item>
          <Form.Item name="source_storage_id" label="Source Storage" rules={[{ required: true }]}>
            <Select placeholder="Select source storage" options={storageOptions} />
          </Form.Item>
          <Form.Item name="dest_storage_id" label="Destination Storage" rules={[{ required: true }]}>
            <Select placeholder="Select destination storage" options={storageOptions} />
          </Form.Item>
          <Form.Item name="mode" label="Copy Mode">
            <Select options={[
              { value: 'immediate', label: 'Immediate Copy' },
              { value: 'scheduled', label: 'Scheduled' },
              { value: 'mirror', label: 'Mirror (continuous)' },
            ]} />
          </Form.Item>
          <Form.Item name="archive_retention_weekly" label="Weekly Retention">
            <InputNumber min={0} max={520} style={{ width: '100%' }} />
          </Form.Item>
          <Form.Item name="archive_retention_monthly" label="Monthly Retention">
            <InputNumber min={0} max={120} style={{ width: '100%' }} />
          </Form.Item>
          <Form.Item name="archive_retention_yearly" label="Yearly Retention">
            <InputNumber min={0} max={30} style={{ width: '100%' }} />
          </Form.Item>
        </Form>
      </Modal>
    </motion.div>
  );
}
