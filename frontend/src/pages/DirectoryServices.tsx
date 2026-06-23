import { useEffect, useState } from 'react';
import { Table, Card, Button, Space, Tag, Form, Input, Select, message } from 'antd';
import { TeamOutlined, ReloadOutlined } from '@ant-design/icons';
import { motion } from 'framer-motion';
import GlassModal from '../components/GlassModal';
import api from '../utils/api';

export default function DirectoryServices() {
  const [dirs, setDirs] = useState<any[]>([]);
  const [loading, setLoading] = useState(false);
  const [connectOpen, setConnectOpen] = useState(false);
  const [form] = Form.useForm();

  const fetchDirs = async () => {
    setLoading(true);
    try {
      const resp = await api.get('/directory/supported');
      setDirs(resp.data.services || []);
    } catch {
      message.error('Failed to load directory services');
    }
    setLoading(false);
  };

  useEffect(() => { fetchDirs(); }, []);

  const handleConnect = async () => {
    const values = await form.validateFields();
    try {
      await api.post('/directory/test', values);
      message.success('Connected!');
      setConnectOpen(false);
    } catch {
      message.error('Connection failed');
    }
  };

  const columns = [
    {
      title: 'Directory Service', key: 'dir',
      render: (_: any, r: any) => (
        <Space>
          <TeamOutlined style={{ color: '#00E5FF', fontSize: 20 }} />
          <div>
            <div style={{ fontWeight: 600 }}>{r.name}</div>
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
          <Button size="small" type="primary" onClick={() => { form.setFieldsValue({ dir_type: r.id }); setConnectOpen(true); }}>Connect</Button>
          <Button size="small">Backup</Button>
        </Space>
      ),
    },
  ];

  return (
    <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.3 }}>
      <Card title={<><TeamOutlined /> Directory Services</>}
        extra={<Button type="primary" onClick={() => setConnectOpen(true)}>New Connection</Button>}>
        <p style={{ color: '#8B949E', marginBottom: 16 }}>
          Backup and restore for ALD Pro, FreeIPA, and Microsoft Active Directory.
          Supports users, groups, GPO, DNS, certificates, system state, NTDS.dit, SYSVOL.
        </p>
        <Table dataSource={dirs} columns={columns} rowKey="id" loading={loading} pagination={false} />
      </Card>

      <GlassModal open={connectOpen} onCancel={() => setConnectOpen(false)} title="Connect to Directory" width={600}
        footer={<Space><Button onClick={() => setConnectOpen(false)}>Cancel</Button><Button type="primary" onClick={handleConnect}>Connect</Button></Space>}>
        <Form form={form} layout="vertical">
          <Form.Item name="dir_type" label="Directory Type" rules={[{ required: true }]}>
            <Select options={dirs.map(d => ({ value: d.id, label: d.name }))} />
          </Form.Item>
          <Form.Item name="server" label="Server" rules={[{ required: true }]}><Input placeholder="dc.example.com" /></Form.Item>
          <Form.Item name="base_dn" label="Base DN" rules={[{ required: true }]}><Input placeholder="dc=example,dc=com" /></Form.Item>
          <Form.Item name="bind_dn" label="Bind DN" rules={[{ required: true }]}><Input placeholder="cn=admin,dc=example,dc=com" /></Form.Item>
          <Form.Item name="password" label="Password" rules={[{ required: true }]}><Input.Password /></Form.Item>
        </Form>
      </GlassModal>
    </motion.div>
  );
}
