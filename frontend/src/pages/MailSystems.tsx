import { useEffect, useState, useCallback } from 'react';
import { Table, Card, Button, Space, Tag, Form, Input, InputNumber, Select, Switch, message } from 'antd';
import { MailOutlined, ReloadOutlined } from '@ant-design/icons';
import { motion } from 'framer-motion';
import GlassModal from '../components/GlassModal';
import api from '../utils/api';

const mailColors: Record<string, string> = {
  exchange: '#0078D4', communigate: '#4CAF50', vk_workspace: '#5181B1',
  rupost: '#FF5722', mailion: '#9C27B0',
};

export default function MailSystems() {
  const [systems, setSystems] = useState<any[]>([]);
  const [loading, setLoading] = useState(false);
  const [connectOpen, setConnectOpen] = useState(false);
  const [form] = Form.useForm();

  const fetchSystems = useCallback(async () => {
    setLoading(true);
    try {
      const resp = await api.get('/mail/supported');
      setSystems(resp.data.systems || []);
    } catch {
      message.error('Failed to load mail systems');
    }
    setLoading(false);
  }, []);

  useEffect(() => { fetchSystems(); }, []);

  const handleConnect = async () => {
    const values = await form.validateFields();
    try {
      await api.post('/mail/test', values);
      message.success('Connected!');
      setConnectOpen(false);
    } catch {
      message.error('Connection failed');
    }
  };

  const columns = [
    {
      title: 'Mail System', key: 'mail',
      render: (_: any, r: any) => (
        <Space>
          <MailOutlined style={{ color: mailColors[r.id] || '#00E5FF', fontSize: 20 }} />
          <div>
            <div style={{ fontWeight: 600 }}>{r.name}</div>
            <div style={{ fontSize: 11, color: '#8B949E' }}>{r.versions}</div>
          </div>
        </Space>
      ),
    },
    {
      title: 'Features', key: 'features',
      render: (_: any, r: any) => (
        <Space wrap size={[0, 4]}>
          {r.features?.map((f: string) => <Tag key={f} color="blue">{f}</Tag>)}
        </Space>
      ),
    },
    {
      title: 'Actions', key: 'actions',
      render: (_: any, r: any) => (
        <Space>
          <Button size="small" type="primary" onClick={() => { form.setFieldsValue({ mail_type: r.id }); setConnectOpen(true); }}>Connect</Button>
          <Button size="small">Backup</Button>
        </Space>
      ),
    },
  ];

  return (
    <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.3 }}>
      <Card title={<><MailOutlined /> Supported Mail Systems</>}
        extra={<Space>
          <Button icon={<ReloadOutlined />} onClick={fetchSystems}>Refresh</Button>
          <Button type="primary" icon={<MailOutlined />} onClick={() => setConnectOpen(true)}>New Connection</Button>
        </Space>}>
        <p style={{ color: '#8B949E', marginBottom: 16 }}>
          Backup and restore for 5 mail systems: Microsoft Exchange, CommuniGate Pro, VK WorkSpace, RuPost, Mailion.
          Supports mailbox-level, granular item restore, public folders, and distribution groups.
        </p>
        <Table dataSource={systems} columns={columns} rowKey="id" loading={loading} pagination={false} />
      </Card>

      <GlassModal open={connectOpen} onCancel={() => setConnectOpen(false)} title="Connect to Mail System" width={600}
        footer={<Space><Button onClick={() => setConnectOpen(false)}>Cancel</Button><Button type="primary" onClick={handleConnect}>Connect</Button></Space>}>
        <Form form={form} layout="vertical">
          <Form.Item name="mail_type" label="Mail System" rules={[{ required: true }]}>
            <Select options={systems.map(s => ({ value: s.id, label: s.name }))} />
          </Form.Item>
          <Form.Item name="server" label="Server" rules={[{ required: true }]}><Input /></Form.Item>
          <Form.Item name="port" label="Port"><InputNumber defaultValue={443} style={{ width: '100%' }} /></Form.Item>
          <Form.Item name="username" label="Username" rules={[{ required: true }]}><Input /></Form.Item>
          <Form.Item name="password" label="Password" rules={[{ required: true }]}><Input.Password /></Form.Item>
          <Form.Item name="use_ssl" label="SSL" valuePropName="checked"><Switch defaultChecked /></Form.Item>
        </Form>
      </GlassModal>
    </motion.div>
  );
}
