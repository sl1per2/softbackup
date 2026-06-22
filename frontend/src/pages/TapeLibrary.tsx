import { useEffect, useState } from 'react';
import { Card, Row, Col, Table, Button, Tag, Form, Input, InputNumber, Switch, Select, Space, Tabs, message, Progress } from 'antd';
import { DatabaseOutlined, UsbOutlined } from '@ant-design/icons';
import { motion } from 'framer-motion';
import api from '../utils/api';

export default function TapeLibrary() {
  const [library, setLibrary] = useState<any>(null);
  const [loading, setLoading] = useState(false);

  const fetchLibrary = async () => {
    setLoading(true);
    try {
      const resp = await api.get('/api/tape-library/inventory');
      setInventory(resp.data.inventory || []);
    } catch {
      message.error('Failed to load tape library inventory');
    }
    setLoading(false);
  };

  useEffect(() => { fetchLibrary(); }, []);

  const handleMount = async (barcode: string) => {
    try {
      await api.post(`/api/tape/mount?barcode=${barcode}&drive_index=0`);
      message.success(`Tape ${barcode} mounted`);
      fetchLibrary();
    } catch {
      message.error('Mount failed');
    }
  };

  const handleInventory = async () => {
    try {
      await api.post('/api/tape/inventory');
      message.success('Inventory complete');
      fetchLibrary();
    } catch {
      message.error('Inventory failed');
    }
  };

  const tapeColumns = [
    { title: 'Barcode', dataIndex: 'barcode', key: 'barcode', render: (t: string) => <strong>{t}</strong> },
    { title: 'Pool', dataIndex: 'pool', key: 'pool', render: (p: string) => <Tag color={p === 'daily' ? 'blue' : p === 'weekly' ? 'green' : 'purple'}>{p}</Tag> },
    { title: 'Capacity', key: 'cap', render: (_: any, r: any) => `${(r.capacity_bytes / 1024**4).toFixed(0)} TB` },
    { title: 'Used', key: 'used', render: (_: any, r: any) => <Progress percent={Math.round(r.used_bytes / r.capacity_bytes * 100)} size="small" /> },
    { title: 'Status', dataIndex: 'status', key: 'status', render: (s: string) => <Tag color={s === 'online' ? 'green' : 'default'}>{s}</Tag> },
    { title: 'Slot', dataIndex: 'slot', key: 'slot' },
    { title: 'Actions', key: 'actions', render: (_: any, r: any) => (
      <Space>
        <Button size="small" onClick={() => handleMount(r.barcode)}>Mount</Button>
        <Button size="small">Eject</Button>
      </Space>
    )},
  ];

  return (
    <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.3 }}>
      <Row gutter={[16, 16]} style={{ marginBottom: 16 }}>
        <Col xs={24} sm={8}>
          <Card size="small" title={<><DatabaseOutlined /> Library</>}>
            <div style={{ fontSize: 16, fontWeight: 600 }}>{library?.name || 'Not Connected'}</div>
            <div style={{ color: '#8B949E' }}>Slots: {library?.used_slots}/{library?.total_slots} | Drives: {library?.online_drives}/{library?.total_drives}</div>
          </Card>
        </Col>
        <Col xs={24} sm={8}>
          <Card size="small" title={<><UsbOutlined /> Drives</>}>
            {library?.drives?.map((d: any, i: number) => (
              <div key={i} style={{ marginBottom: 4 }}>
                <Tag color={d.online ? 'green' : 'red'}>{d.device_path}</Tag> {d.status}
              </div>
            ))}
          </Card>
        </Col>
        <Col xs={24} sm={8}>
          <Card size="small" title="Actions">
            <Space direction="vertical">
              <Button onClick={handleInventory}>Inventory Scan</Button>
              <Button>Format Tape</Button>
            </Space>
          </Card>
        </Col>
      </Row>

      <Card title="Tapes" size="small">
        <Table dataSource={library?.tapes || []} columns={tapeColumns} rowKey="barcode" loading={loading} pagination={{ pageSize: 10 }} size="small" />
      </Card>
    </motion.div>
  );
}
