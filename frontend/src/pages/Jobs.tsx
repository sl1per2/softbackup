import React, { useEffect, useState, useCallback } from 'react';
import { Table, Tag, Select, DatePicker, Button, Modal, Tabs, Input, Space, Typography, message } from 'antd';
import { ReloadOutlined, SearchOutlined } from '@ant-design/icons';
import { motion } from 'framer-motion';
import axios from 'axios';
import ReactECharts from 'echarts-for-react';

const { Title, Text } = Typography;
const { RangePicker } = DatePicker;

export default function Jobs() {
  const [jobs, setJobs] = useState<any[]>([]);
  const [loading, setLoading] = useState(false);
  const [modalOpen, setModalOpen] = useState(false);
  const [selectedJob, setSelectedJob] = useState<any>(null);
  const [filters, setFilters] = useState<Record<string, any>>({});

  const fetchJobs = useCallback(async () => {
    setLoading(true);
    try {
      const res = await axios.get('/api/jobs', { params: filters });
      setJobs(res.data);
    } catch {
      message.error('Failed to load jobs');
    }
    setLoading(false);
  }, [filters]);

  useEffect(() => { fetchJobs(); }, [fetchJobs]);

  const columns = [
    { title: 'ID', dataIndex: 'id', key: 'id', render: (id: number) => <Text code>#{id}</Text> },
    {
      title: 'Type',
      dataIndex: 'type',
      key: 'type',
      render: (t: string) => {
        const c: Record<string, string> = { full: 'blue', incremental: 'cyan', differential: 'purple' };
        return <Tag color={c[t]}>{t?.toUpperCase()}</Tag>;
      },
    },
    {
      title: 'Status',
      dataIndex: 'status',
      key: 'status',
      render: (s: string) => {
        const c: Record<string, string> = { completed: 'success', running: 'processing', failed: 'error', pending: 'warning', cancelled: 'default' };
        return <Tag color={c[s]}>{s?.toUpperCase()}</Tag>;
      },
    },
    { title: 'Transport', dataIndex: 'transport_mode_used', key: 'transport' },
    {
      title: 'Size',
      key: 'size',
      render: (_: any, r: any) => (
        <Space>
          <Text>{(r.size_bytes / 1073741824).toFixed(2)} GB</Text>
          <Text type="secondary">→</Text>
          <Text style={{ color: '#00FF88' }}>{(r.size_transferred_bytes / 1073741824).toFixed(2)} GB</Text>
        </Space>
      ),
    },
    {
      title: 'Compression',
      dataIndex: 'compression_ratio',
      key: 'compression',
      render: (v: number) => `${((1 - 1 / v) * 100).toFixed(1)}%`,
    },
    {
      title: 'Cache Hit',
      dataIndex: 'cache_hit_ratio',
      key: 'cache',
      render: (v: number) => `${(v * 100).toFixed(1)}%`,
    },
    {
      title: 'Started',
      dataIndex: 'started_at',
      key: 'started',
      render: (t: string) => t ? new Date(t).toLocaleString() : '-',
    },
    {
      title: 'Actions',
      key: 'actions',
      render: (_: any, record: any) => (
        <Space>
          <Button size="small" onClick={() => { setSelectedJob(record); setModalOpen(true); }}>Details</Button>
          {['pending', 'running'].includes(record.status) && (
            <Button size="small" danger onClick={() => axios.post(`/api/jobs/${record.id}/cancel`)}>Cancel</Button>
          )}
        </Space>
      ),
    },
  ];

  return (
    <div>
      <Title level={4} style={{ color: '#E6E6E6', marginBottom: 24 }}>Jobs</Title>
      <Space style={{ marginBottom: 16 }} wrap>
        <Select
          mode="multiple"
          placeholder="Status"
          style={{ minWidth: 200 }}
          onChange={(v) => setFilters((f) => ({ ...f, status: v.join(',') }))}
          options={[
            { value: 'completed', label: 'Completed' },
            { value: 'running', label: 'Running' },
            { value: 'failed', label: 'Failed' },
            { value: 'pending', label: 'Pending' },
            { value: 'cancelled', label: 'Cancelled' },
          ]}
        />
        <Select
          placeholder="Type"
          style={{ width: 150 }}
          allowClear
          onChange={(v) => setFilters((f) => ({ ...f, job_type: v }))}
          options={[
            { value: 'full', label: 'Full' },
            { value: 'incremental', label: 'Incremental' },
            { value: 'differential', label: 'Differential' },
          ]}
        />
        <RangePicker
          onChange={(dates) => {
            setFilters((f) => ({
              ...f,
              date_from: dates?.[0]?.toISOString(),
              date_to: dates?.[1]?.toISOString(),
            }));
          }}
        />
        <Button icon={<ReloadOutlined />} onClick={fetchJobs} />
      </Space>

      <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.3 }}>
        <Table
          dataSource={jobs}
          columns={columns}
          rowKey="id"
          loading={loading}
          pagination={{ pageSize: 20 }}
        />
      </motion.div>

      <Modal
        title={`Job #${selectedJob?.id}`}
        open={modalOpen}
        onCancel={() => setModalOpen(false)}
        footer={null}
        width={700}
      >
        {selectedJob && (
          <Tabs items={[
            {
              key: 'overview',
              label: 'Overview',
              children: (
                <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 16 }}>
                  {[
                    ['Status', selectedJob.status],
                    ['Type', selectedJob.type],
                    ['Started', selectedJob.started_at ? new Date(selectedJob.started_at).toLocaleString() : '-'],
                    ['Finished', selectedJob.finished_at ? new Date(selectedJob.finished_at).toLocaleString() : '-'],
                    ['Original Size', `${(selectedJob.size_bytes / 1073741824).toFixed(2)} GB`],
                    ['Transferred', `${(selectedJob.size_transferred_bytes / 1073741824).toFixed(2)} GB`],
                    ['Dedup Saved', `${(selectedJob.dedup_saved_bytes / 1073741824).toFixed(2)} GB`],
                    ['Zero Blocks', selectedJob.zero_blocks_skipped],
                    ['Chunks Total', selectedJob.chunks_total],
                    ['Chunks Cached', selectedJob.chunks_cached],
                  ].map(([label, value]) => (
                    <div key={String(label)}>
                      <Text type="secondary">{label}</Text>
                      <br />
                      <Text strong>{value}</Text>
                    </div>
                  ))}
                </div>
              ),
            },
            {
              key: 'traffic',
              label: 'Traffic',
              children: (
                <ReactECharts
                  style={{ height: 250 }}
                  option={{
                    backgroundColor: 'transparent',
                    tooltip: { trigger: 'axis' },
                    xAxis: { type: 'category', data: ['Original', 'Transferred'] },
                    yAxis: { type: 'value' },
                    series: [{
                      type: 'bar',
                      data: [
                        { value: selectedJob.size_bytes, itemStyle: { color: '#00E5FF' } },
                        { value: selectedJob.size_transferred_bytes, itemStyle: { color: '#00FF88' } },
                      ],
                      barWidth: 60,
                    }],
                  }}
                />
              ),
            },
            {
              key: 'log',
              label: 'Log',
              children: (
                <div style={{ background: '#161B22', padding: 16, borderRadius: 8, maxHeight: 300, overflow: 'auto', fontFamily: 'monospace', fontSize: 12 }}>
                  {selectedJob.error_log || 'No log available'}
                </div>
              ),
            },
          ]} />
        )}
      </Modal>
    </div>
  );
}
