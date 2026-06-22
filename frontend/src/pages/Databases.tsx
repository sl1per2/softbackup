import { useEffect, useState } from 'react';
import { Table, Card, Button, Space, Tag, Tabs, Form, Input, InputNumber, Select, Switch, message, Row, Col, Progress } from 'antd';
import { DatabaseOutlined, ReloadOutlined, ThunderboltOutlined, CheckCircleOutlined } from '@ant-design/icons';
import { motion } from 'framer-motion';
import GlassModal from '../components/GlassModal';
import api from '../utils/api';

const dbColors: Record<string, string> = {
  postgresql: '#336791', mysql: '#4479A1', mssql: '#CC2927', oracle: '#F80000',
  sap_hana: '#0070F2', postgres_pro: '#336791', tantor: '#336791',
  arenadata: '#FF6600', greenplum: '#1A8C41', yandexdb: '#FC3F1D',
  red_database: '#E41E31', brest: '#0066CC', mariadb: '#003545', mongodb: '#47A248',
};

export default function Databases() {
  const [databases, setDatabases] = useState<any[]>([]);
  const [loading, setLoading] = useState(false);
  const [connectOpen, setConnectOpen] = useState(false);
  const [form] = Form.useForm();

  const fetchDatabases = useCallback(async () => {
    setLoading(true);
    try {
      const resp = await api.get('/api/dbms/supported');
      setDatabases(resp.data.databases || []);
    } catch {
      message.error('Failed to load database list');
    }
    setLoading(false);
  }, []);

  useEffect(() => { fetchDatabases(); }, []);

  const handleConnect = async () => {
    const values = await form.validateFields();
    try {
      await api.post('/api/dbms/test', values);
      message.success('Connected!');
      setConnectOpen(false);
    } catch {
      message.error('Connection failed');
    }
  };

  const columns = [
    {
      title: 'Database', key: 'db',
      render: (_: any, r: any) => (
        <Space>
          <DatabaseOutlined style={{ color: dbColors[r.id] || '#00E5FF', fontSize: 20 }} />
          <div>
            <div style={{ fontWeight: 600 }}>{r.name}</div>
            <div style={{ fontSize: 11, color: '#8B949E' }}>Versions: {r.versions}</div>
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
        <Button size="small" type="primary" onClick={() => {
          form.setFieldsValue({ db_type: r.id });
          setConnectOpen(true);
        }}>Connect</Button>
      ),
    },
  ];

  return (
    <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.3 }}>
      <Card title={<><DatabaseOutlined /> Supported Database Systems</>}
        extra={<Space>
          <Button icon={<ReloadOutlined />} onClick={fetchDatabases}>Refresh</Button>
          <Button type="primary" icon={<ThunderboltOutlined />} onClick={() => setConnectOpen(true)}>New Connection</Button>
        </Space>}>
        <p style={{ color: '#8B949E', marginBottom: 16 }}>
          Backup and restore for 14+ database systems including PostgreSQL, MySQL, SQL Server, Oracle, SAP HANA,
          and Russian databases (Arenadata, Greenplum, YandexDB, РЕД БД, Тантор).
        </p>
        <Table dataSource={databases} columns={columns} rowKey="id" loading={loading} pagination={false} />
      </Card>

      <GlassModal open={connectOpen} onCancel={() => setConnectOpen(false)} title="Connect to Database" width={600}
        footer={<Space><Button onClick={() => setConnectOpen(false)}>Cancel</Button><Button type="primary" onClick={handleConnect}>Connect</Button></Space>}>
        <Form form={form} layout="vertical">
          <Form.Item name="db_type" label="Database Type" rules={[{ required: true }]}>
            <Select options={databases.map(d => ({ value: d.id, label: d.name }))} />
          </Form.Item>
          <Form.Item name="host" label="Host" rules={[{ required: true }]}><Input /></Form.Item>
          <Form.Item name="port" label="Port" rules={[{ required: true }]}><InputNumber style={{ width: '100%' }} /></Form.Item>
          <Form.Item name="username" label="Username" rules={[{ required: true }]}><Input /></Form.Item>
          <Form.Item name="password" label="Password" rules={[{ required: true }]}><Input.Password /></Form.Item>
          <Form.Item name="database" label="Database" rules={[{ required: true }]}><Input /></Form.Item>
          <Form.Item name="ssl" label="SSL" valuePropName="checked"><Switch /></Form.Item>
        </Form>
      </GlassModal>
    </motion.div>
  );
}
