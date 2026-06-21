import React, { useEffect, useState } from 'react';
import { Row, Col, Card, Statistic, Table, Tag, Typography, Space } from 'antd';
import {
  CloudServerOutlined,
  CheckCircleOutlined,
  SyncOutlined,
  SaveOutlined,
  ArrowUpOutlined,
  ArrowDownOutlined,
} from '@ant-design/icons';
import { motion } from 'framer-motion';
import ReactECharts from 'echarts-for-react';
import CountUp from 'react-countup';
import axios from 'axios';
import { useDashboardStore } from '../stores/dashboardStore';
import { useWebSocket } from '../hooks/useWebSocket';

const { Title, Text } = Typography;

const formatter = (value: any) => `${value}`;

const fadeUp = {
  initial: { opacity: 0, y: 20 },
  animate: { opacity: 1, y: 0 },
  transition: { duration: 0.3 },
};

export default function Dashboard() {
  const { summary, chartData, recentJobs, setSummary, setChartData, setRecentJobs } = useDashboardStore();
  useWebSocket('dashboard');

  useEffect(() => {
    const loadData = async () => {
      try {
        const [s, c, j] = await Promise.all([
          axios.get('/api/dashboard/summary'),
          axios.get('/api/dashboard/charts'),
          axios.get('/api/dashboard/recent-jobs'),
        ]);
        setSummary(s.data);
        setChartData(c.data);
        setRecentJobs(j.data);
      } catch {}
    };
    loadData();
    const interval = setInterval(loadData, 30000);
    return () => clearInterval(interval);
  }, []);

  const jobColumns = [
    {
      title: 'ID',
      dataIndex: 'id',
      key: 'id',
      render: (id: number) => <Text code>#{id}</Text>,
    },
    {
      title: 'Type',
      dataIndex: 'type',
      key: 'type',
      render: (type: string) => (
        <Tag color={type === 'full' ? 'blue' : type === 'incremental' ? 'cyan' : 'purple'}>
          {type?.toUpperCase()}
        </Tag>
      ),
    },
    {
      title: 'Status',
      dataIndex: 'status',
      key: 'status',
      render: (status: string) => {
        const colors: Record<string, string> = {
          completed: 'success', running: 'processing', failed: 'error', pending: 'warning',
        };
        const icons: Record<string, React.ReactNode> = {
          running: <SyncOutlined spin />,
          completed: <CheckCircleOutlined />,
        };
        return <Tag color={colors[status]} icon={icons[status]}>{status?.toUpperCase()}</Tag>;
      },
    },
    {
      title: 'Size',
      dataIndex: 'size_bytes',
      key: 'size',
      render: (v: number) => `${(v / 1073741824).toFixed(2)} GB`,
    },
  ];

  return (
    <div>
      <Title level={4} style={{ color: '#E6E6E6', marginBottom: 24 }}>Dashboard</Title>

      <motion.div {...fadeUp}>
        <Row gutter={[16, 16]}>
          <Col xs={24} sm={12} lg={6}>
            <Card className="widget-card" style={{ borderLeft: '3px solid #00FF88' }}>
              <Statistic
                title={<Text style={{ color: '#8B949E' }}>Agents Online</Text>}
                value={summary?.agents_online || 0}
                formatter={formatter}
                prefix={<CloudServerOutlined style={{ color: '#00FF88' }} />}
              />
            </Card>
          </Col>
          <Col xs={24} sm={12} lg={6}>
            <Card className="widget-card" style={{ borderLeft: '3px solid #00E5FF' }}>
              <Statistic
                title={<Text style={{ color: '#8B949E' }}>Successful (24h)</Text>}
                value={summary?.jobs_successful_24h || 0}
                formatter={formatter}
                prefix={<CheckCircleOutlined style={{ color: '#00E5FF' }} />}
              />
            </Card>
          </Col>
          <Col xs={24} sm={12} lg={6}>
            <Card className="widget-card" style={{ borderLeft: '3px solid #00B4D8' }}>
              <Statistic
                title={<Text style={{ color: '#8B949E' }}>CDP Active</Text>}
                value={summary?.cdp_sessions_active || 0}
                formatter={formatter}
                prefix={<SyncOutlined style={{ color: '#00B4D8' }} />}
              />
            </Card>
          </Col>
          <Col xs={24} sm={12} lg={6}>
            <Card className="widget-card" style={{ borderLeft: '3px solid #7C3AED' }}>
              <Statistic
                title={<Text style={{ color: '#8B949E' }}>Traffic Saved (TB)</Text>}
                value={((summary?.traffic_saved_bytes || 0) / 1099511627776).toFixed(2)}
                formatter={(v) => v}
                prefix={<SaveOutlined style={{ color: '#7C3AED' }} />}
              />
            </Card>
          </Col>
        </Row>
      </motion.div>

      <Row gutter={[16, 16]} style={{ marginTop: 16 }}>
        <Col xs={24} lg={16}>
          <motion.div {...fadeUp} transition={{ delay: 0.1 }}>
            <Card className="widget-card" title={<Text style={{ color: '#E6E6E6' }}>Backup Volumes</Text>}>
              <ReactECharts
                style={{ height: 300 }}
                option={{
                  backgroundColor: 'transparent',
                  tooltip: { trigger: 'axis' },
                  xAxis: { type: 'category', data: chartData?.backup_volumes?.map((v: any) => v.label) || [], axisLine: { lineStyle: { color: '#333' } } },
                  yAxis: { type: 'value', axisLine: { lineStyle: { color: '#333' } }, splitLine: { lineStyle: { color: 'rgba(255,255,255,0.06)' } } },
                  series: [
                    {
                      name: 'Original',
                      type: 'line',
                      data: chartData?.backup_volumes?.map((v: any) => v.value) || [],
                      smooth: true,
                      areaStyle: { color: { type: 'linear', x: 0, y: 0, x2: 0, y2: 1, colorStops: [{ offset: 0, color: 'rgba(0,229,255,0.3)' }, { offset: 1, color: 'rgba(0,229,255,0)' }] } },
                      lineStyle: { color: '#00E5FF' },
                      itemStyle: { color: '#00E5FF' },
                    },
                    {
                      name: 'Transferred',
                      type: 'line',
                      data: chartData?.transfer_volumes?.map((v: any) => v.value) || [],
                      smooth: true,
                      areaStyle: { color: { type: 'linear', x: 0, y: 0, x2: 0, y2: 1, colorStops: [{ offset: 0, color: 'rgba(0,255,136,0.3)' }, { offset: 1, color: 'rgba(0,255,136,0)' }] } },
                      lineStyle: { color: '#00FF88' },
                      itemStyle: { color: '#00FF88' },
                    },
                  ],
                }}
              />
            </Card>
          </motion.div>
        </Col>
        <Col xs={24} lg={8}>
          <motion.div {...fadeUp} transition={{ delay: 0.15 }}>
            <Card className="widget-card" title={<Text style={{ color: '#E6E6E6' }}>Cache Hit Ratio</Text>}>
              <ReactECharts
                style={{ height: 300 }}
                option={{
                  backgroundColor: 'transparent',
                  series: [{
                    type: 'gauge',
                    startAngle: 200,
                    endAngle: -20,
                    min: 0,
                    max: 100,
                    progress: { show: true, width: 18 },
                    axisLine: { lineStyle: { width: 18, color: [[1, 'rgba(255,255,255,0.1)']] } },
                    axisTick: { show: false },
                    splitLine: { show: false },
                    axisLabel: { show: false },
                    pointer: { show: false },
                    title: { show: false },
                    detail: {
                      valueAnimation: true,
                      fontSize: 28,
                      formatter: '{value}%',
                      color: '#00E5FF',
                      offsetCenter: [0, '10%'],
                    },
                    data: [{ value: summary?.avg_cache_hit_ratio || 0 }],
                  }],
                }}
              />
            </Card>
          </motion.div>
        </Col>
      </Row>

      <motion.div {...fadeUp} transition={{ delay: 0.2 }} style={{ marginTop: 16 }}>
        <Card className="widget-card" title={<Text style={{ color: '#E6E6E6' }}>Recent Jobs</Text>}>
          <Table
            dataSource={recentJobs}
            columns={jobColumns}
            rowKey="id"
            pagination={false}
            size="small"
          />
        </Card>
      </motion.div>
    </div>
  );
}
