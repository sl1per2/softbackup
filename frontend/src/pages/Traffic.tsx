import React, { useEffect, useState } from 'react';
import { Card, Row, Col, Select, DatePicker, Table, Typography, Space, Statistic } from 'antd';
import { ArrowDownOutlined, ArrowUpOutlined } from '@ant-design/icons';
import { motion } from 'framer-motion';
import ReactECharts from 'echarts-for-react';
import axios from 'axios';

const { Title, Text } = Typography;
const { RangePicker } = DatePicker;

export default function TrafficPage() {
  const [stats, setStats] = useState<any[]>([]);
  const [cacheStats, setCacheStats] = useState<any>(null);
  const [bandwidth, setBandwidth] = useState<any>(null);
  const [filters, setFilters] = useState<Record<string, any>>({});

  useEffect(() => {
    const load = async () => {
      try {
        const [s, c, b] = await Promise.all([
          axios.get('/api/traffic/stats', { params: filters }),
          axios.get('/api/traffic/global-cache-stats'),
          axios.get('/api/traffic/bandwidth-usage'),
        ]);
        setStats(s.data);
        setCacheStats(c.data);
        setBandwidth(b.data);
      } catch {}
    };
    load();
  }, [filters]);

  const totalOriginal = stats.reduce((s, r) => s + (r.original_size || 0), 0);
  const totalTransferred = stats.reduce((s, r) => s + (r.transferred_size || 0), 0);

  const columns = [
    { title: 'Agent', dataIndex: 'agent_id', key: 'agent' },
    { title: 'Original', dataIndex: 'original_size', key: 'orig', render: (v: number) => `${(v / 1073741824).toFixed(2)} GB` },
    { title: 'Transferred', dataIndex: 'transferred_size', key: 'trans', render: (v: number) => `${(v / 1073741824).toFixed(2)} GB` },
    { title: 'Compression', dataIndex: 'compression_ratio', key: 'comp', render: (v: number) => `${((1 - 1 / v) * 100).toFixed(1)}%` },
    { title: 'Cache Hits', dataIndex: 'cache_hits', key: 'cache' },
    { title: 'Zero Blocks', dataIndex: 'zero_blocks', key: 'zero' },
    { title: 'Throughput', dataIndex: 'throughput_mbps', key: 'tp', render: (v: number) => `${v.toFixed(1)} MB/s` },
  ];

  return (
    <div>
      <Title level={4} style={{ color: '#E6E6E6', marginBottom: 24 }}>Traffic</Title>
      <Space style={{ marginBottom: 16 }}>
        <Select placeholder="Agent" style={{ width: 200 }} allowClear onChange={(v) => setFilters((f) => ({ ...f, agent_id: v }))} />
        <RangePicker onChange={(d) => setFilters((f) => ({ ...f, date_from: d?.[0]?.toISOString(), date_to: d?.[1]?.toISOString() }))} />
      </Space>

      <Row gutter={[16, 16]} style={{ marginBottom: 16 }}>
        <Col xs={24} sm={12} lg={8}>
          <Card className="widget-card">
            <Statistic
              title="Total Original"
              value={(totalOriginal / 1073741824).toFixed(2)}
              suffix="GB"
              valueStyle={{ color: '#00E5FF' }}
            />
          </Card>
        </Col>
        <Col xs={24} sm={12} lg={8}>
          <Card className="widget-card">
            <Statistic
              title="Total Transferred"
              value={(totalTransferred / 1073741824).toFixed(2)}
              suffix="GB"
              valueStyle={{ color: '#00FF88' }}
            />
          </Card>
        </Col>
        <Col xs={24} sm={12} lg={8}>
          <Card className="widget-card">
            <Statistic
              title="Traffic Saved"
              value={((totalOriginal - totalTransferred) / 1073741824).toFixed(2)}
              suffix="GB"
              valueStyle={{ color: '#7C3AED' }}
            />
          </Card>
        </Col>
      </Row>

      <Row gutter={[16, 16]} style={{ marginBottom: 16 }}>
        <Col xs={24} lg={16}>
          <Card className="widget-card" title="Traffic Comparison">
            <ReactECharts
              style={{ height: 300 }}
              option={{
                backgroundColor: 'transparent',
                tooltip: { trigger: 'axis' },
                xAxis: { type: 'category', data: ['Original', 'Transferred'] },
                yAxis: { type: 'value' },
                series: [{
                  type: 'bar',
                  data: [
                    { value: totalOriginal, itemStyle: { color: '#00E5FF' } },
                    { value: totalTransferred, itemStyle: { color: '#00FF88' } },
                  ],
                  barWidth: 80,
                }],
              }}
            />
          </Card>
        </Col>
        <Col xs={24} lg={8}>
          <Card className="widget-card" title="Cache Stats">
            <Space direction="vertical" style={{ width: '100%' }}>
              <Statistic title="Total Entries" value={cacheStats?.total_entries || 0} />
              <Statistic title="Hit Ratio" value={cacheStats?.hit_ratio ? `${(cacheStats.hit_ratio * 100).toFixed(1)}%` : '0%'} />
              <Statistic title="Cache Size" value={`${((cacheStats?.size_bytes || 0) / 1073741824).toFixed(2)} GB`} />
            </Space>
          </Card>
        </Col>
      </Row>

      <Card className="widget-card" title="Traffic Stats">
        <Table dataSource={stats} columns={columns} rowKey="id" pagination={{ pageSize: 20 }} />
      </Card>
    </div>
  );
}
