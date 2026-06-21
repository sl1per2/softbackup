import { useEffect, useState } from 'react';
import { Card, Row, Col, Button, Form, Input, Switch, Progress, Table, Space, message, Tag } from 'antd';
import { DownloadOutlined, ReloadOutlined } from '@ant-design/icons';
import { motion } from 'framer-motion';
import api from '../utils/api';
import { formatDate } from '../utils/format';

export default function Rescue() {
  const [images, setImages] = useState<any[]>([]);
  const [creating, setCreating] = useState(false);
  const [progress, setProgress] = useState(0);
  const [form] = Form.useForm();

  const fetchImages = async () => {
    try {
      const resp = await api.get('/api/rescue/list');
      setImages(resp.data.images || []);
    } catch {}
  };

  useEffect(() => { fetchImages(); }, []);

  const handleCreate = async () => {
    setCreating(true);
    setProgress(0);
    try {
      const values = await form.validateFields();
      await api.post('/api/rescue/create', values);
      message.success('Rescue image created');
      fetchImages();
    } catch (err: any) {
      message.error(err.response?.data?.detail || 'Error');
    }
    setCreating(false);
    setProgress(100);
  };

  const columns = [
    { title: 'Image', dataIndex: 'name', key: 'name' },
    { title: 'Created', dataIndex: 'created_at', key: 'created', render: (v: string) => formatDate(v) },
    { title: 'Size', dataIndex: 'size', key: 'size' },
    { title: 'Actions', key: 'actions', render: (_: any, r: any) => (
      <Button size="small" icon={<DownloadOutlined />}>Download</Button>
    )},
  ];

  return (
    <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.3 }}>
      <Row gutter={[16, 16]}>
        <Col xs={24} lg={12}>
          <Card title="Create Rescue Image">
            <p style={{ color: '#8B949E', marginBottom: 16 }}>
              Create a bootable rescue ISO for bare-metal recovery. Includes network tools and filesystem utilities.
            </p>
            <Form form={form} layout="vertical">
              <Form.Item name="hostname" label="Hostname" initialValue="obs-rescue"><Input /></Form.Item>
              <Form.Item name="ip_address" label="IP Address" initialValue="192.168.1.100"><Input /></Form.Item>
              <Form.Item name="netmask" label="Netmask" initialValue="255.255.255.0"><Input /></Form.Item>
              <Form.Item name="gateway" label="Gateway" initialValue="192.168.1.1"><Input /></Form.Item>
              <Form.Item name="include_network_tools" label="Network Tools" valuePropName="checked"><Switch defaultChecked /></Form.Item>
              <Form.Item name="include_fs_tools" label="Filesystem Tools" valuePropName="checked"><Switch defaultChecked /></Form.Item>
            </Form>
            {creating && <Progress percent={progress} status="active" style={{ marginBottom: 16 }} />}
            <Button type="primary" loading={creating} onClick={handleCreate}>Create Image</Button>
          </Card>
        </Col>
        <Col xs={24} lg={12}>
          <Card title="Available Images">
            <Table dataSource={images} columns={columns} rowKey="name" pagination={false} />
            {images.length === 0 && <p style={{ color: '#8B949E', textAlign: 'center', padding: 40 }}>No rescue images yet</p>}
          </Card>
        </Col>
      </Row>
    </motion.div>
  );
}
