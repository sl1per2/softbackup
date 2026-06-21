import React, { useEffect, useState } from 'react';
import { Card, Row, Col, Button, Modal, Form, Input, Select, InputNumber, Progress, Typography, Tag, Space, message } from 'antd';
import { PlusOutlined, CloudServerOutlined, DatabaseOutlined, HddOutlined, CheckCircleOutlined } from '@ant-design/icons';
import { motion } from 'framer-motion';
import axios from 'axios';

const { Title, Text } = Typography;

export default function Storages() {
  const [storages, setStorages] = useState<any[]>([]);
  const [loading, setLoading] = useState(false);
  const [modalOpen, setModalOpen] = useState(false);
  const [editingStorage, setEditingStorage] = useState<any>(null);
  const [form] = Form.useForm();

  const fetchStorages = async () => {
    setLoading(true);
    try {
      const res = await axios.get('/api/storages');
      setStorages(res.data);
    } catch {}
    setLoading(false);
  };

  useEffect(() => { fetchStorages(); }, []);

  const handleSave = async () => {
    try {
      const values = await form.validateFields();
      if (editingStorage) {
        await axios.put(`/api/storages/${editingStorage.id}`, values);
      } else {
        await axios.post('/api/storages', values);
      }
      message.success('Storage saved');
      setModalOpen(false);
      fetchStorages();
    } catch {}
  };

  const handleTest = async (id: number) => {
    try {
      const res = await axios.post(`/api/storages/${id}/test`);
      message.success(res.data.message);
    } catch {
      message.error('Connection test failed');
    }
  };

  const getStorageIcon = (type: string) => {
    switch (type) {
      case 's3': return <CloudServerOutlined style={{ fontSize: 32, color: '#FFA502' }} />;
      case 'nfs': return <HddOutlined style={{ fontSize: 32, color: '#00E5FF' }} />;
      default: return <DatabaseOutlined style={{ fontSize: 32, color: '#00FF88' }} />;
    }
  };

  return (
    <div>
      <Title level={4} style={{ color: '#E6E6E6', marginBottom: 24 }}>Storages</Title>
      <Button type="primary" icon={<PlusOutlined />} onClick={() => { setEditingStorage(null); form.resetFields(); setModalOpen(true); }} style={{ marginBottom: 16 }}>
        Add Storage
      </Button>

      <Row gutter={[16, 16]}>
        {storages.map((storage, idx) => (
          <Col xs={24} sm={12} lg={8} key={storage.id}>
            <motion.div
              initial={{ opacity: 0, y: 20 }}
              animate={{ opacity: 1, y: 0 }}
              transition={{ delay: idx * 0.05, duration: 0.3 }}
            >
              <Card
                className="widget-card"
                hoverable
                style={{ cursor: 'pointer' }}
                actions={[
                  <Button size="small" onClick={() => handleTest(storage.id)}>Test</Button>,
                  <Button size="small" onClick={() => { setEditingStorage(storage); form.setFieldsValue(storage); setModalOpen(true); }}>Edit</Button>,
                ]}
              >
                <div style={{ display: 'flex', alignItems: 'center', gap: 16, marginBottom: 16 }}>
                  {getStorageIcon(storage.type)}
                  <div>
                    <Text strong style={{ fontSize: 16 }}>{storage.name}</Text>
                    <br />
                    <Tag color="blue">{storage.type.toUpperCase()}</Tag>
                    {storage.supports_fast_clone && <Tag color="success">Fast Clone</Tag>}
                  </div>
                </div>
                <Progress
                  percent={storage.total_bytes > 0 ? Math.round((storage.used_bytes / storage.total_bytes) * 100) : 0}
                  strokeColor="#00E5FF"
                  format={(p) => `${(storage.used_bytes / 1073741824).toFixed(1)} / ${(storage.total_bytes / 1073741824).toFixed(1)} TB`}
                />
              </Card>
            </motion.div>
          </Col>
        ))}
      </Row>

      <Modal
        title={editingStorage ? 'Edit Storage' : 'Add Storage'}
        open={modalOpen}
        onCancel={() => setModalOpen(false)}
        onOk={handleSave}
        width={600}
        okText="Save"
      >
        <Form form={form} layout="vertical">
          <Form.Item name="name" label="Name" rules={[{ required: true }]}>
            <Input placeholder="Storage name" />
          </Form.Item>
          <Form.Item name="type" label="Type" rules={[{ required: true }]}>
            <Select options={[
              { value: 'local', label: 'Local Storage' },
              { value: 'nfs', label: 'NFS' },
              { value: 's3', label: 'S3 / Object Storage' },
              { value: 'smb', label: 'SMB/CIFS' },
            ]} />
          </Form.Item>
          <Form.Item name="total_bytes" label="Total Capacity (bytes)">
            <InputNumber style={{ width: '100%' }} min={0} />
          </Form.Item>
          <Form.Item name="supports_fast_clone" label="Supports Fast Clone" valuePropName="checked">
            <Select options={[{ value: false, label: 'No' }, { value: true, label: 'Yes' }]} />
          </Form.Item>
        </Form>
      </Modal>
    </div>
  );
}
