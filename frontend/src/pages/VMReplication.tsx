import { useEffect, useState } from 'react';
import { Card, Table, Button, Space, Tag, Progress, Row, Col, Statistic, message } from 'antd';
import { SyncOutlined, PauseCircleOutlined, RocketOutlined } from '@ant-design/icons';
import { motion } from 'framer-motion';
import api from '../utils/api';

export default function VMReplication() {
  const [reps, setReps] = useState<any[]>([]);

  const fetchData = async () => {
    try {
      const r = await api.get('/replication-v2');
      setReps(r.data.replications || []);
    } catch {
      message.error('Failed to load VM replication jobs');
    }
  };

  useEffect(() => { fetchData(); }, []);

  return (
    <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }}>
      <Card title="VM Replication" extra={
        <Space>
          <Button type="primary" icon={<SyncOutlined />}>New Replication</Button>
          <Button onClick={fetchData}>Refresh</Button>
        </Space>
      }>
        <p style={{ color: '#8B949E', marginBottom: 16 }}>
          Continuous VM replication with configurable RPO (5+ minutes). Failover/failback for DR.
        </p>
        <Table dataSource={reps} rowKey="job_id" columns={[
          { title: 'VM', dataIndex: 'vm_name', key: 'vm' },
          { title: 'Source → Target', key: 'route', render: (_: any, r: any) => `${r.source_host} → ${r.target_host}` },
          { title: 'Status', dataIndex: 'status', key: 'status', render: (s: string) => <Tag color={s === 'syncing' ? 'processing' : 'default'}>{s}</Tag> },
          { title: 'Lag', dataIndex: 'lag_seconds', key: 'lag', render: (v: number) => `${v}s` },
          { title: 'Actions', key: 'actions', render: (_: any, r: any) => (
            <Space>
              <Button size="small" danger icon={<RocketOutlined />}>Failover</Button>
              <Button size="small" icon={<PauseCircleOutlined />}>Pause</Button>
            </Space>
          )},
        ]} pagination={false} />
      </Card>
    </motion.div>
  );
}
