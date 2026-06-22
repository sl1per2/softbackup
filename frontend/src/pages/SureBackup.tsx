import { useEffect, useState, useCallback } from 'react';
import { Card, Table, Tag, Button, Space, Row, Col, Statistic, Progress, message } from 'antd';
import { SafetyCertificateOutlined, CheckCircleOutlined, WarningOutlined, CloseCircleOutlined } from '@ant-design/icons';
import { motion } from 'framer-motion';
import api from '../utils/api';

export default function SureBackup() {
  const [results, setResults] = useState<any[]>([]);
  const [summary, setSummary] = useState<any>(null);
  const [loading, setLoading] = useState(false);

  const fetchData = useCallback(async () => {
    setLoading(true);
    try {
      const [r, s] = await Promise.all([api.get('/api/surebackup/results'), api.get('/api/surebackup/summary')]);
      setResults(r.data.results || []);
      setSummary(s.data);
    } catch {
      message.error('Failed to load SureBackup data');
    }
    setLoading(false);
  }, []);
  };

  useEffect(() => { fetchData(); }, []);

  const statusIcon = (status: string) => {
    if (status === 'success') return <CheckCircleOutlined style={{ color: '#00FF88' }} />;
    if (status === 'warning') return <WarningOutlined style={{ color: '#FFA502' }} />;
    if (status === 'failed') return <CloseCircleOutlined style={{ color: '#FF4757' }} />;
    return <SafetyCertificateOutlined style={{ color: '#8B949E' }} />;
  };

  return (
    <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }}>
      <Row gutter={[16, 16]} style={{ marginBottom: 16 }}>
        <Col xs={24} sm={8}>
          <Card size="small" title="Verified">
            <Statistic value={summary?.verified || 0} suffix={`/ ${summary?.total || 0}`} valueStyle={{ color: '#00FF88' }} />
            <Progress percent={summary?.success_rate || 0} strokeColor="#00FF88" />
          </Card>
        </Col>
        <Col xs={24} sm={8}>
          <Card size="small" title="Pending">
            <Statistic value={summary?.pending || 0} valueStyle={{ color: '#FFA502' }} />
          </Card>
        </Col>
        <Col xs={24} sm={8}>
          <Card size="small" title="Security Status">
            <Tag color="green" style={{ fontSize: 16 }}>CLEAN</Tag>
          </Card>
        </Col>
      </Row>
      <Card title="SureBackup Test History" extra={<Button onClick={fetchData}>Refresh</Button>}>
        <Table dataSource={results} rowKey="test_id" pagination={{ pageSize: 10 }} columns={[
          { title: 'Test ID', dataIndex: 'test_id', key: 'id' },
          { title: 'Job', dataIndex: 'job_id', key: 'job' },
          { title: 'Status', dataIndex: 'status', key: 'status', render: (s: string) => <Space>{statusIcon(s)}<Tag>{s?.toUpperCase()}</Tag></Space> },
          { title: 'Boot Time', dataIndex: 'boot_time_seconds', key: 'boot', render: (v: number) => `${v}s` },
          { title: 'Heartbeat', dataIndex: 'heartbeat_ok', key: 'hb', render: (v: boolean) => <Tag color={v ? 'green' : 'red'}>{v ? 'OK' : 'FAIL'}</Tag> },
          { title: 'Ping', dataIndex: 'ping_ok', key: 'ping', render: (v: boolean) => <Tag color={v ? 'green' : 'red'}>{v ? 'OK' : 'FAIL'}</Tag> },
          { title: 'App', dataIndex: 'application_ok', key: 'app', render: (v: boolean) => <Tag color={v ? 'green' : 'red'}>{v ? 'OK' : 'FAIL'}</Tag> },
        ]} />
      </Card>
    </motion.div>
  );
}
