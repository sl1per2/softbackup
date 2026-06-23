import { useEffect, useState, useCallback } from 'react';
import { Card, Table, Button, Space, Tag, message } from 'antd';
import { ThunderboltOutlined, PlayCircleOutlined, ReloadOutlined } from '@ant-design/icons';
import { motion } from 'framer-motion';
import api from '../utils/api';

export default function DisasterRecovery() {
  const [plans, setPlans] = useState<any[]>([]);
  const [runs, setRuns] = useState<any[]>([]);
  const [loading, setLoading] = useState(false);

  const fetchData = useCallback(async () => {
    setLoading(true);
    try {
      const [p, r] = await Promise.all([api.get('/api/dr/plans'), api.get('/api/dr/runs')]);
      setPlans(p.data.plans || []);
      setRuns(r.data.runs || []);
    } catch {
      message.error('Failed to load DR data');
    }
    setLoading(false);
  }, []);

  useEffect(() => { fetchData(); }, []);

  const handleRun = async (planId: string) => {
    try {
      const resp = await api.post(`/api/dr/plans/${planId}/run`);
      message.success(`DR Plan started: ${resp.data.run_id}`);
      fetchData();
    } catch { message.error('Failed'); }
  };

  return (
    <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }}>
      <Card title="Disaster Recovery Plans" extra={
        <Space>
          <Button type="primary" icon={<ThunderboltOutlined />}>New DR Plan</Button>
          <Button icon={<ReloadOutlined />} onClick={fetchData}>Refresh</Button>
        </Space>
      }>
        <Table dataSource={plans} rowKey="plan_id" columns={[
          { title: 'Name', dataIndex: 'name', key: 'name', render: (n: string) => <strong>{n}</strong> },
          { title: 'Steps', key: 'steps', render: (_: any, r: any) => `${r.steps?.length || 0} VMs` },
          { title: 'Actions', key: 'actions', render: (_: any, r: any) => (
            <Space>
              <Button type="primary" danger icon={<PlayCircleOutlined />} onClick={() => handleRun(r.plan_id)}>Launch DR</Button>
              <Button size="small">Edit</Button>
            </Space>
          )},
        ]} pagination={false} />
      </Card>

      {runs.length > 0 && (
        <Card title="DR Run History" style={{ marginTop: 16 }}>
          <Table dataSource={runs} rowKey="run_id" columns={[
            { title: 'Run ID', dataIndex: 'run_id', key: 'id' },
            { title: 'Plan', dataIndex: 'plan_id', key: 'plan' },
            { title: 'Status', dataIndex: 'status', key: 'status', render: (s: string) => <Tag color={s === 'completed' ? 'green' : s === 'running' ? 'blue' : 'red'}>{s}</Tag> },
            { title: 'RTO', dataIndex: 'total_rto_seconds', key: 'rto', render: (v: number) => `${v}s` },
          ]} pagination={{ pageSize: 5 }} />
        </Card>
      )}
    </motion.div>
  );
}
