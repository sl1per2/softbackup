import { useEffect, useState } from 'react';
import { Card, Table, Button, Space, Form, InputNumber, Tag, Row, Col, Statistic, message } from 'antd';
import { HistoryOutlined, ReloadOutlined, SaveOutlined } from '@ant-design/icons';
import { motion } from 'framer-motion';
import api from '../utils/api';

interface GFSConfig {
  daily: number;
  weekly: number;
  monthly: number;
  yearly: number;
}

interface GFSReport {
  policy_id: number;
  gfs_config: GFSConfig;
  tiers: { daily: number; weekly: number; monthly: number; yearly: number };
  total_backups: number;
  keep_dates: Record<string, string[]>;
}

export default function GfsRetention() {
  const [reports, setReports] = useState<GFSReport[]>([]);
  const [loading, setLoading] = useState(false);
  const [form] = Form.useForm();

  const fetchReports = async () => {
    setLoading(true);
    try {
      const resp = await api.get('/gfs');
      setReports(resp.data.policies || []);
    } catch {
      message.error('Failed to load GFS retention policies');
    }
    setLoading(false);
  };

  useEffect(() => { fetchReports(); }, []);

  const tierColors: Record<string, string> = {
    daily: '#00E5FF',
    weekly: '#00FF88',
    monthly: '#FFA502',
    yearly: '#A855F7',
  };

  const columns = [
    {
      title: 'Policy', key: 'policy',
      render: (_: any, r: GFSReport) => <strong>Policy #{r.policy_id}</strong>,
    },
    {
      title: 'GFS Config', key: 'config',
      render: (_: any, r: GFSReport) => (
        <Space>
          <Tag color={tierColors.daily}>Daily: {r.gfs_config?.daily}</Tag>
          <Tag color={tierColors.weekly}>Weekly: {r.gfs_config?.weekly}</Tag>
          <Tag color={tierColors.monthly}>Monthly: {r.gfs_config?.monthly}</Tag>
          <Tag color={tierColors.yearly}>Yearly: {r.gfs_config?.yearly}</Tag>
        </Space>
      ),
    },
    {
      title: 'Current Backups', key: 'tiers',
      render: (_: any, r: GFSReport) => (
        <Space>
          <Statistic title="Daily" value={r.tiers?.daily || 0} valueStyle={{ color: tierColors.daily, fontSize: 16 }} />
          <Statistic title="Weekly" value={r.tiers?.weekly || 0} valueStyle={{ color: tierColors.weekly, fontSize: 16 }} />
          <Statistic title="Monthly" value={r.tiers?.monthly || 0} valueStyle={{ color: tierColors.monthly, fontSize: 16 }} />
          <Statistic title="Yearly" value={r.tiers?.yearly || 0} valueStyle={{ color: tierColors.yearly, fontSize: 16 }} />
        </Space>
      ),
    },
    {
      title: 'Total', key: 'total',
      render: (_: any, r: GFSReport) => <Tag>{r.total_backups}</Tag>,
    },
  ];

  return (
    <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.3 }}>
      <Row gutter={[16, 16]} style={{ marginBottom: 16 }}>
        <Col xs={24} lg={12}>
          <Card size="small" title="GFS Policy Overview">
            <p style={{ color: '#8B949E', marginBottom: 16 }}>
              Grandfather-Father-Son retention keeps backups on a rotating schedule.
              Daily (Son), Weekly on Sunday (Father), Monthly on 1st (Grandfather), Yearly on Jan 1 (Great-Grandfather).
            </p>
            <Form form={form} layout="inline" initialValues={{ daily: 7, weekly: 4, monthly: 12, yearly: 1 }}>
              <Form.Item name="daily" label={<Tag color={tierColors.daily}>Daily</Tag>}>
                <InputNumber min={1} max={365} />
              </Form.Item>
              <Form.Item name="weekly" label={<Tag color={tierColors.weekly}>Weekly</Tag>}>
                <InputNumber min={1} max={52} />
              </Form.Item>
              <Form.Item name="monthly" label={<Tag color={tierColors.monthly}>Monthly</Tag>}>
                <InputNumber min={1} max={120} />
              </Form.Item>
              <Form.Item name="yearly" label={<Tag color={tierColors.yearly}>Yearly</Tag>}>
                <InputNumber min={1} max={99} />
              </Form.Item>
            </Form>
          </Card>
        </Col>
        <Col xs={24} lg={12}>
          <Card size="small" title="Actions">
            <Space direction="vertical" style={{ width: '100%' }}>
              <Button type="primary" icon={<HistoryOutlined />} block onClick={() => {
                api.post('/gfs/run-cleanup');
                message.success('GFS cleanup triggered');
              }}>Run GFS Cleanup Now</Button>
              <Button icon={<ReloadOutlined />} block onClick={fetchReports}>Refresh Report</Button>
            </Space>
          </Card>
        </Col>
      </Row>

      <Card title={<><HistoryOutlined /> GFS Retention Report</>} extra={
        <Button icon={<ReloadOutlined />} onClick={fetchReports}>Refresh</Button>
      }>
        <Table dataSource={reports} columns={columns} rowKey="policy_id" loading={loading} pagination={false} />
        {reports.length === 0 && (
          <div style={{ textAlign: 'center', padding: 40, color: '#8B949E' }}>
            No policies with GFS retention configured yet.
            Configure GFS in Policy settings to see the retention report.
          </div>
        )}
      </Card>
    </motion.div>
  );
}
