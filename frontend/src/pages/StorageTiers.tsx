import { useEffect, useState } from 'react';
import { Card, Row, Col, Table, Tag, Button, Progress, Space, Form, Input, InputNumber, Select, Switch, message } from 'antd';
import { ReloadOutlined, SwapOutlined } from '@ant-design/icons';
import { motion } from 'framer-motion';
import api from '../utils/api';
import { formatBytes } from '../utils/format';

const tierColors: Record<string, string> = {
  hot: '#FF4757', warm: '#FFA502', cold: '#00E5FF', archive: '#A855F7',
};

export default function StorageTiers() {
  const [tiers, setTiers] = useState<any[]>([]);
  const [loading, setLoading] = useState(false);

  const fetchTiers = async () => {
    setLoading(true);
    try {
      const resp = await api.get('/storage-tiers');
      setTiers(resp.data.tiers || []);
    } catch {
      message.error('Failed to load storage tiers');
    }
    setLoading(false);
  };

  useEffect(() => { fetchTiers(); }, []);

  const handleRunTiering = async () => {
    try {
      await api.post('/storage-tiers/run-tiering');
      message.success('Tiering job started');
      fetchTiers();
    } catch {
      message.error('Failed to start tiering');
    }
  };

  return (
    <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.3 }}>
      <div style={{ marginBottom: 16 }}>
        <Space>
          <Button type="primary" icon={<SwapOutlined />} onClick={handleRunTiering}>Run Tiering</Button>
          <Button icon={<ReloadOutlined />} onClick={fetchTiers}>Refresh</Button>
        </Space>
      </div>

      <Row gutter={[16, 16]}>
        {tiers.map((tier) => (
          <Col xs={24} sm={12} lg={6} key={tier.type}>
            <motion.div whileHover={{ scale: 1.02 }} transition={{ duration: 0.2 }}>
              <Card
                title={<span><Tag color={tierColors[tier.type]}>{tier.type.toUpperCase()}</Tag> Storage</span>}
                size="small"
                style={{ borderTop: `3px solid ${tierColors[tier.type]}` }}
              >
                <Progress
                  percent={Math.round(tier.utilization_percent)}
                  strokeColor={tier.utilization_percent > 80 ? '#FF4757' : tierColors[tier.type]}
                  format={() => `${formatBytes(tier.used_bytes)} / ${formatBytes(tier.max_size_bytes)}`}
                />
                <div style={{ marginTop: 8, color: '#8B949E', fontSize: 12 }}>
                  {tier.file_count} files | Min age: {tier.min_access_days} days
                </div>
              </Card>
            </motion.div>
          </Col>
        ))}
      </Row>

      <Card title="Tier Policy" style={{ marginTop: 16 }} size="small">
        <Table
          dataSource={tiers}
          rowKey="type"
          pagination={false}
          size="small"
          columns={[
            { title: 'Tier', dataIndex: 'type', key: 'type', render: (t: string) => <Tag color={tierColors[t]}>{t.toUpperCase()}</Tag> },
            { title: 'Path', dataIndex: 'storage_path', key: 'path' },
            { title: 'Used', key: 'used', render: (_: any, r: any) => formatBytes(r.used_bytes) },
            { title: 'Capacity', key: 'cap', render: (_: any, r: any) => formatBytes(r.max_size_bytes) },
            { title: 'Utilization', key: 'util', render: (_: any, r: any) => <Progress percent={Math.round(r.utilization_percent)} size="small" /> },
            { title: 'Min Age (days)', dataIndex: 'min_access_days', key: 'age' },
            { title: 'Files', dataIndex: 'file_count', key: 'files' },
          ]}
        />
      </Card>
    </motion.div>
  );
}
