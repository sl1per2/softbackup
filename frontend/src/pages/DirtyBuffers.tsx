import React, { useEffect, useState } from 'react';
import { Table, Tag, Select, Button, Typography, Space, Card, Row, Col, Statistic, Tooltip, Drawer, Descriptions } from 'antd';
import {
  ReloadOutlined, CheckCircleOutlined, CloseCircleOutlined,
  WarningOutlined, ClockCircleOutlined, DatabaseOutlined,
  ClearOutlined, ThunderboltOutlined,
} from '@ant-design/icons';
import { motion } from 'framer-motion';
import axios from 'axios';

const { Title, Text } = Typography;

interface DirtyBufferLog {
  id: number;
  backup_job_id: string;
  agent_id: string;
  plugin_name: string;
  before_page_count: number;
  before_size_bytes: number;
  before_percent_ram: number;
  after_page_count: number;
  after_size_bytes: number;
  after_percent_ram: number;
  flush_status: string;
  flush_duration_ms: number;
  error_message: string;
  component_details_json: string;
  consistency_level: string;
  is_consistent: boolean;
  total_ram_bytes: number;
  buffer_pool_size: number;
  flush_started_at: string;
  flush_finished_at: string;
  created_at: string;
}

interface DirtyBufferStats {
  total_entries: number;
  total_flushed: number;
  total_failed: number;
  total_inconsistent: number;
  avg_flush_ms: number;
  plugins: Array<{ name: string; count: number; avg_ms: number; total_pages: number }>;
}

function formatBytes(bytes: number): string {
  if (bytes === 0) return '0 B';
  const units = ['B', 'KB', 'MB', 'GB', 'TB'];
  const i = Math.floor(Math.log(bytes) / Math.log(1024));
  return (bytes / Math.pow(1024, i)).toFixed(1) + ' ' + units[i];
}

function formatPages(pages: number): string {
  return pages.toLocaleString() + ' pages';
}

const statusColors: Record<string, string> = {
  flushed: 'success',
  not_flushed: 'default',
  timeout: 'warning',
  failed: 'error',
};

const consistencyColors: Record<string, string> = {
  application_consistent: 'success',
  crash_consistent: 'error',
  file_consistent: 'warning',
};

export default function DirtyBuffersPage() {
  const [logs, setLogs] = useState<DirtyBufferLog[]>([]);
  const [stats, setStats] = useState<DirtyBufferStats | null>(null);
  const [loading, setLoading] = useState(false);
  const [filters, setFilters] = useState<Record<string, any>>({});
  const [detailDrawer, setDetailDrawer] = useState<DirtyBufferLog | null>(null);

  const fetchData = async () => {
    setLoading(true);
    try {
      const [logsRes, statsRes] = await Promise.all([
        axios.get('/api/dirty-buffers', { params: filters }),
        axios.get('/api/dirty-buffers/stats'),
      ]);
      setLogs(logsRes.data);
      setStats(statsRes.data);
    } catch {
      message.error('Failed to load dirty buffer data');
    }
    setLoading(false);
  };

  useEffect(() => { fetchData(); }, [filters]);

  const columns = [
    {
      title: 'ID',
      dataIndex: 'id',
      key: 'id',
      width: 60,
      render: (v: number) => <Text code>#{v}</Text>,
    },
    {
      title: 'Job',
      dataIndex: 'backup_job_id',
      key: 'job',
      render: (v: string) => <Text code style={{ fontSize: 11 }}>{v}</Text>,
    },
    {
      title: 'Plugin',
      dataIndex: 'plugin_name',
      key: 'plugin',
      render: (v: string) => <Tag color="blue">{v}</Tag>,
    },
    {
      title: 'Agent',
      dataIndex: 'agent_id',
      key: 'agent',
      render: (v: string) => <Text>{v}</Text>,
    },
    {
      title: 'Before',
      key: 'before',
      render: (_: any, r: DirtyBufferLog) => (
        <Tooltip title={`${r.before_percent_ram.toFixed(1)}% of RAM`}>
          <Text>{formatPages(r.before_page_count)}</Text>
          <br />
          <Text type="secondary" style={{ fontSize: 11 }}>{formatBytes(r.before_size_bytes)}</Text>
        </Tooltip>
      ),
    },
    {
      title: 'After',
      key: 'after',
      render: (_: any, r: DirtyBufferLog) => (
        <Tooltip title={`${r.after_percent_ram.toFixed(1)}% of RAM`}>
          <Text>{formatPages(r.after_page_count)}</Text>
          <br />
          <Text type="secondary" style={{ fontSize: 11 }}>{formatBytes(r.after_size_bytes)}</Text>
        </Tooltip>
      ),
    },
    {
      title: 'Flushed',
      key: 'flushed',
      render: (_: any, r: DirtyBufferLog) => {
        const diff = r.before_page_count - r.after_page_count;
        const color = diff > 0 ? '#52c41a' : '#ff4d4f';
        return <Text style={{ color }}>{diff > 0 ? '+' : ''}{diff} pages</Text>;
      },
    },
    {
      title: 'Status',
      dataIndex: 'flush_status',
      key: 'status',
      render: (v: string) => {
        const icons: Record<string, React.ReactNode> = {
          flushed: <CheckCircleOutlined />,
          failed: <CloseCircleOutlined />,
          timeout: <ClockCircleOutlined />,
          not_flushed: <ClearOutlined />,
        };
        return <Tag color={statusColors[v]} icon={icons[v]}>{v}</Tag>;
      },
    },
    {
      title: 'Duration',
      dataIndex: 'flush_duration_ms',
      key: 'duration',
      render: (v: number) => <Text>{v} ms</Text>,
    },
    {
      title: 'Consistency',
      key: 'consistency',
      render: (_: any, r: DirtyBufferLog) => {
        const icons: Record<string, React.ReactNode> = {
          application_consistent: <CheckCircleOutlined />,
          crash_consistent: <CloseCircleOutlined />,
          file_consistent: <WarningOutlined />,
        };
        return (
          <Tag color={consistencyColors[r.consistency_level]} icon={icons[r.consistency_level]}>
            {r.consistency_level.replace('_', ' ')}
          </Tag>
        );
      },
    },
    {
      title: 'Time',
      dataIndex: 'created_at',
      key: 'time',
      render: (v: string) => <Text style={{ fontSize: 11 }}>{new Date(v).toLocaleString()}</Text>,
    },
    {
      title: '',
      key: 'detail',
      width: 40,
      render: (_: any, r: DirtyBufferLog) => (
        <Button type="link" size="small" onClick={() => setDetailDrawer(r)}>
          Details
        </Button>
      ),
    },
  ];

  return (
    <div>
      <Title level={4} style={{ color: '#E6E6E6', marginBottom: 24 }}>
        <ThunderboltOutlined style={{ marginRight: 8 }} />
        Dirty Buffer Logs
      </Title>

      {stats && (
        <Row gutter={16} style={{ marginBottom: 24 }}>
          <Col span={4}>
            <Card size="small" style={{ background: 'rgba(255,255,255,0.04)', border: '1px solid rgba(255,255,255,0.08)' }}>
              <Statistic title={<Text style={{ color: '#888' }}>Total Entries</Text>} value={stats.total_entries} prefix={<DatabaseOutlined />} />
            </Card>
          </Col>
          <Col span={4}>
            <Card size="small" style={{ background: 'rgba(255,255,255,0.04)', border: '1px solid rgba(255,255,255,0.08)' }}>
              <Statistic title={<Text style={{ color: '#888' }}>Flushed</Text>} value={stats.total_flushed} valueStyle={{ color: '#52c41a' }} prefix={<CheckCircleOutlined />} />
            </Card>
          </Col>
          <Col span={4}>
            <Card size="small" style={{ background: 'rgba(255,255,255,0.04)', border: '1px solid rgba(255,255,255,0.08)' }}>
              <Statistic title={<Text style={{ color: '#888' }}>Failed</Text>} value={stats.total_failed} valueStyle={{ color: '#ff4d4f' }} prefix={<CloseCircleOutlined />} />
            </Card>
          </Col>
          <Col span={4}>
            <Card size="small" style={{ background: 'rgba(255,255,255,0.04)', border: '1px solid rgba(255,255,255,0.08)' }}>
              <Statistic title={<Text style={{ color: '#888' }}>Inconsistent</Text>} value={stats.total_inconsistent} valueStyle={{ color: '#faad14' }} prefix={<WarningOutlined />} />
            </Card>
          </Col>
          <Col span={4}>
            <Card size="small" style={{ background: 'rgba(255,255,255,0.04)', border: '1px solid rgba(255,255,255,0.08)' }}>
              <Statistic title={<Text style={{ color: '#888' }}>Avg Flush</Text>} value={stats.avg_flush_ms} suffix="ms" prefix={<ClockCircleOutlined />} />
            </Card>
          </Col>
          <Col span={4}>
            <Card size="small" style={{ background: 'rgba(255,255,255,0.04)', border: '1px solid rgba(255,255,255,0.08)' }}>
              <Statistic
                title={<Text style={{ color: '#888' }}>Success Rate</Text>}
                value={stats.total_entries > 0 ? ((stats.total_flushed / stats.total_entries) * 100).toFixed(1) : 0}
                suffix="%"
                valueStyle={{ color: stats.total_failed > 0 ? '#faad14' : '#52c41a' }}
              />
            </Card>
          </Col>
        </Row>
      )}

      <Space style={{ marginBottom: 16 }} wrap>
        <Select
          placeholder="Plugin"
          style={{ width: 150 }}
          allowClear
          onChange={(v) => setFilters((f) => ({ ...f, plugin_name: v }))}
          options={[
            { value: 'esxi', label: 'ESXi' },
            { value: 'hyperv', label: 'Hyper-V' },
            { value: 'mssql', label: 'MS SQL' },
            { value: 'postgresql', label: 'PostgreSQL' },
            { value: 'mysql', label: 'MySQL' },
            { value: 'oracle', label: 'Oracle' },
            { value: 'exchange', label: 'Exchange' },
            { value: 'generic', label: 'Generic' },
          ]}
        />
        <Select
          placeholder="Status"
          style={{ width: 140 }}
          allowClear
          onChange={(v) => setFilters((f) => ({ ...f, flush_status: v }))}
          options={[
            { value: 'flushed', label: 'Flushed' },
            { value: 'failed', label: 'Failed' },
            { value: 'timeout', label: 'Timeout' },
            { value: 'not_flushed', label: 'Not Flushed' },
          ]}
        />
        <Select
          placeholder="Consistency"
          style={{ width: 180 }}
          allowClear
          onChange={(v) => setFilters((f) => ({ ...f, is_consistent: v === 'true' ? true : v === 'false' ? false : undefined }))}
          options={[
            { value: 'true', label: 'Consistent' },
            { value: 'false', label: 'Inconsistent' },
          ]}
        />
        <Button icon={<ReloadOutlined />} onClick={fetchData}>Refresh</Button>
      </Space>

      <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.3 }}>
        <Table
          dataSource={logs}
          columns={columns}
          rowKey="id"
          loading={loading}
          pagination={{ pageSize: 20, showSizeChanger: true }}
          size="small"
          rowClassName={(r) => r.flush_status === 'failed' ? 'ant-table-row-error' : r.is_consistent === false ? 'ant-table-row-warn' : ''}
        />
      </motion.div>

      <Drawer
        title="Dirty Buffer Log Detail"
        open={!!detailDrawer}
        onClose={() => setDetailDrawer(null)}
        width={600}
      >
        {detailDrawer && (
          <Descriptions bordered column={1} size="small">
            <Descriptions.Item label="Job ID">{detailDrawer.backup_job_id}</Descriptions.Item>
            <Descriptions.Item label="Agent">{detailDrawer.agent_id}</Descriptions.Item>
            <Descriptions.Item label="Plugin">{detailDrawer.plugin_name}</Descriptions.Item>
            <Descriptions.Item label="Flush Status">
              <Tag color={statusColors[detailDrawer.flush_status]}>{detailDrawer.flush_status}</Tag>
            </Descriptions.Item>
            <Descriptions.Item label="Consistency">
              <Tag color={consistencyColors[detailDrawer.consistency_level]}>{detailDrawer.consistency_level}</Tag>
            </Descriptions.Item>
            <Descriptions.Item label="Before (pages)">{detailDrawer.before_page_count}</Descriptions.Item>
            <Descriptions.Item label="Before (bytes)">{formatBytes(detailDrawer.before_size_bytes)}</Descriptions.Item>
            <Descriptions.Item label="Before (% RAM)">{detailDrawer.before_percent_ram.toFixed(2)}%</Descriptions.Item>
            <Descriptions.Item label="After (pages)">{detailDrawer.after_page_count}</Descriptions.Item>
            <Descriptions.Item label="After (bytes)">{formatBytes(detailDrawer.after_size_bytes)}</Descriptions.Item>
            <Descriptions.Item label="After (% RAM)">{detailDrawer.after_percent_ram.toFixed(2)}%</Descriptions.Item>
            <Descriptions.Item label="Flush Duration">{detailDrawer.flush_duration_ms} ms</Descriptions.Item>
            <Descriptions.Item label="Total RAM">{formatBytes(detailDrawer.total_ram_bytes)}</Descriptions.Item>
            <Descriptions.Item label="Buffer Pool">{formatBytes(detailDrawer.buffer_pool_size)}</Descriptions.Item>
            <Descriptions.Item label="Flush Started">{detailDrawer.flush_started_at}</Descriptions.Item>
            <Descriptions.Item label="Flush Finished">{detailDrawer.flush_finished_at}</Descriptions.Item>
            <Descriptions.Item label="Error">
              {detailDrawer.error_message || <Text type="secondary">None</Text>}
            </Descriptions.Item>
            <Descriptions.Item label="Component Details">
              <pre style={{ fontSize: 11, margin: 0, whiteSpace: 'pre-wrap' }}>
                {JSON.stringify(JSON.parse(detailDrawer.component_details_json || '[]'), null, 2)}
              </pre>
            </Descriptions.Item>
          </Descriptions>
        )}
      </Drawer>
    </div>
  );
}
