import { useEffect, useState } from 'react';
import { Card, Row, Col, Button, Table, Tag, Space, Progress, Modal, Form, Input, InputNumber, Switch, Select, message } from 'antd';
import { SyncOutlined, PauseCircleOutlined, StopOutlined, PlusOutlined } from '@ant-design/icons';
import { motion } from 'framer-motion';
import GlassModal from '../components/GlassModal';
import api from '../utils/api';
import { formatBytes, formatDate } from '../utils/format';

export default function Replication() {
  const [jobs, setJobs] = useState<any[]>([]);
  const [modalOpen, setModalOpen] = useState(false);
  const [form] = Form.useForm();

  const fetchJobs = async () => {
    try {
      // In production: GET /api/replication/jobs
      setJobs([]);
    } catch {}
  };

  useEffect(() => { fetchJobs(); }, []);

  const handleStart = async () => {
    const values = await form.validateFields();
    try {
      await api.post('/api/replication/start', values);
      message.success('Replication started');
      setModalOpen(false);
      form.resetFields();
      fetchJobs();
    } catch (err: any) {
      message.error(err.response?.data?.detail || 'Error');
    }
  };

  const columns = [
    { title: 'Job ID', dataIndex: 'job_id', key: 'id' },
    { title: 'Status', dataIndex: 'status', key: 'status', render: (s: string) =>
      <Tag color={s === 'syncing' ? 'processing' : s === 'completed' ? 'success' : 'error'} icon={s === 'syncing' ? <SyncOutlined spin /> : undefined}>{s}</Tag>
    },
    { title: 'Progress', key: 'progress', render: (_: any, r: any) =>
      <Progress percent={r.total_bytes > 0 ? Math.round(r.transferred_bytes / r.total_bytes * 100) : 0} size="small" />
    },
    { title: 'Speed', dataIndex: 'speed_mbps', key: 'speed', render: (v: number) => `${v} MB/s` },
    { title: 'Started', dataIndex: 'started_at', key: 'started', render: (v: string) => formatDate(v) },
  ];

  return (
    <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.3 }}>
      <Card title="Remote Replication" extra={
        <Button type="primary" icon={<PlusOutlined />} onClick={() => setModalOpen(true)}>New Replication</Button>
      }>
        <p style={{ color: '#8B949E', marginBottom: 16 }}>
          Synchronize backups between sites following the 3-2-1 rule. Supports incremental replication with digest exchange.
        </p>
        <Table dataSource={jobs} columns={columns} rowKey="job_id" pagination={{ pageSize: 10 }} />
      </Card>

      <GlassModal open={modalOpen} onCancel={() => setModalOpen(false)} title="New Replication Job" width={600}
        footer={<Space><Button onClick={() => setModalOpen(false)}>Cancel</Button><Button type="primary" onClick={handleStart}>Start</Button></Space>}>
        <Form form={form} layout="vertical">
          <Form.Item name="source_storage_id" label="Source Storage" rules={[{ required: true }]}>
            <Select options={[{ value: 1, label: 'Primary Storage' }, { value: 2, label: 'Secondary Storage' }]} />
          </Form.Item>
          <Form.Item name="dest_host" label="Destination Host" rules={[{ required: true }]}><Input /></Form.Item>
          <Form.Item name="dest_port" label="Destination Port"><InputNumber defaultValue={9100} style={{ width: '100%' }} /></Form.Item>
          <Form.Item name="dest_storage_id" label="Destination Storage" rules={[{ required: true }]}>
            <Select options={[{ value: 3, label: 'Remote Storage' }]} />
          </Form.Item>
          <Form.Item name="compress" label="Compress" valuePropName="checked"><Switch defaultChecked /></Form.Item>
          <Form.Item name="encrypt" label="Encrypt" valuePropName="checked"><Switch defaultChecked /></Form.Item>
          <Form.Item name="bandwidth_limit_kbps" label="Bandwidth Limit (KB/s)"><InputNumber min={0} style={{ width: '100%' }} /></Form.Item>
          <Form.Item name="incremental_only" label="Incremental Only" valuePropName="checked"><Switch defaultChecked /></Form.Item>
        </Form>
      </GlassModal>
    </motion.div>
  );
}
