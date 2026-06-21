import { useEffect, useState } from 'react';
import { Card, Table, Button, Space, Tag, Modal, Form, Input, Select, message } from 'antd';
import { SyncOutlined, PlusOutlined } from '@ant-design/icons';
import { motion } from 'framer-motion';
import api from '../utils/api';

export default function BackupCopy() {
  const [jobs, setJobs] = useState<any[]>([]);
  const [modalOpen, setModalOpen] = useState(false);
  const [form] = Form.useForm();

  const fetchJobs = async () => {
    try { const r = await api.get('/api/backup-copy'); setJobs(r.data.jobs || []); } catch {}
  };

  useEffect(() => { fetchJobs(); }, []);

  const handleCreate = async () => {
    const values = await form.validateFields();
    try { await api.post('/api/backup-copy', values); message.success('Backup Copy created'); setModalOpen(false); fetchJobs(); }
    catch { message.error('Failed'); }
  };

  return (
    <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }}>
      <Card title="Backup Copy Jobs" extra={
        <Space>
          <Button type="primary" icon={<PlusOutlined />} onClick={() => { form.resetFields(); setModalOpen(true); }}>New Copy Job</Button>
          <Button icon={<SyncOutlined />} onClick={fetchJobs}>Refresh</Button>
        </Space>
      }>
        <Table dataSource={jobs} rowKey="job_id" columns={[
          { title: 'Name', dataIndex: 'name', key: 'name' },
          { title: 'Mode', dataIndex: 'mode', key: 'mode', render: (m: string) => <Tag color="blue">{m}</Tag> },
          { title: 'Status', dataIndex: 'status', key: 'status', render: (s: string) => <Tag>{s}</Tag> },
          { title: 'Actions', key: 'actions', render: (_: any, r: any) => (
            <Space>
              <Button size="small" type="primary">Run Now</Button>
              <Button size="small">Archive</Button>
            </Space>
          )},
        ]} pagination={false} />
      </Card>

      <Modal open={modalOpen} onCancel={() => setModalOpen(false)} title="New Backup Copy" onOk={handleCreate}>
        <Form form={form} layout="vertical">
          <Form.Item name="name" label="Name" rules={[{ required: true }]}><Input /></Form.Item>
          <Form.Item name="source_storage_id" label="Source Storage" rules={[{ required: true }]}>
            <Select options={[{ value: 1, label: 'Primary' }]} />
          </Form.Item>
          <Form.Item name="dest_storage_id" label="Destination Storage" rules={[{ required: true }]}>
            <Select options={[{ value: 2, label: 'Secondary' }, { value: 3, label: 'Archive' }]} />
          </Form.Item>
          <Form.Item name="mode" label="Mode" initialValue="immediate">
            <Select options={[{ value: 'immediate', label: 'Immediate Copy' }, { value: 'scheduled', label: 'Scheduled' }, { value: 'mirror', label: 'Mirror' }]} />
          </Form.Item>
          <Form.Item name="archive_retention_weekly" label="Archive Weekly Retention" initialValue={4}><Input type="number" /></Form.Item>
          <Form.Item name="archive_retention_monthly" label="Archive Monthly Retention" initialValue={12}><Input type="number" /></Form.Item>
          <Form.Item name="archive_retention_yearly" label="Archive Yearly Retention" initialValue={7}><Input type="number" /></Form.Item>
        </Form>
      </Modal>
    </motion.div>
  );
}
