import React, { useEffect, useState, useRef } from 'react';
import { Drawer, Select, Input, Button, Space, Switch, Typography, message } from 'antd';
import { ReloadOutlined, DownloadOutlined, SearchOutlined } from '@ant-design/icons';
import api from '../utils/api';

const { Text } = Typography;

interface Props {
  open: boolean;
  agentId?: number;
  agentName?: string;
  onClose: () => void;
}

export default function AgentLogViewer({ open, agentId, agentName, onClose }: Props) {
  const [logs, setLogs] = useState<string[]>([]);
  const [level, setLevel] = useState<string | undefined>();
  const [search, setSearch] = useState('');
  const [autoScroll, setAutoScroll] = useState(true);
  const [loading, setLoading] = useState(false);
  const logEndRef = useRef<HTMLDivElement>(null);

  const fetchLogs = async () => {
    if (!agentId) return;
    setLoading(true);
    try {
      const params: any = { tail: 1000 };
      if (level) params.level = level;
      if (search) params.search = search;
      const res = await api.get(`/api/log-viewer/${agentId}`, { params });
      setLogs(res.data.logs || []);
    } catch {
      message.error('Failed to load logs');
    }
    setLoading(false);
  };

  useEffect(() => {
    if (open && agentId) fetchLogs();
  }, [open, agentId, level, search]);

  useEffect(() => {
    if (autoScroll && logEndRef.current) {
      logEndRef.current.scrollIntoView({ behavior: 'smooth' });
    }
  }, [logs, autoScroll]);

  const handleDownload = () => {
    if (!agentId) return;
    window.open(`/api/log-viewer/${agentId}/download`, '_blank');
  };

  const getLineColor = (line: string) => {
    if (line.includes('[ERROR]')) return '#ff4d4f';
    if (line.includes('[WARN]')) return '#faad14';
    if (line.includes('[DEBUG]')) return '#8b949e';
    return '#52c41a';
  };

  return (
    <Drawer
      title={`Logs — ${agentName || 'Agent'}`}
      open={open}
      onClose={onClose}
      width={800}
      extra={
        <Space>
          <Select placeholder="Level" style={{ width: 100 }} allowClear value={level} onChange={setLevel}
            options={[
              { value: 'ERROR', label: 'ERROR' },
              { value: 'WARN', label: 'WARN' },
              { value: 'INFO', label: 'INFO' },
              { value: 'DEBUG', label: 'DEBUG' },
            ]} />
          <Input placeholder="Search..." style={{ width: 180 }} value={search}
            onChange={e => setSearch(e.target.value)} onPressEnter={fetchLogs}
            suffix={<SearchOutlined onClick={fetchLogs} style={{ cursor: 'pointer' }} />} />
          <Button icon={<ReloadOutlined />} onClick={fetchLogs} loading={loading} />
          <Button icon={<DownloadOutlined />} onClick={handleDownload}>Download</Button>
          <Switch checked={autoScroll} onChange={setAutoScroll} checkedChildren="Auto" unCheckedChildren="Manual" />
        </Space>
      }
    >
      <div style={{
        background: '#0d1117', border: '1px solid rgba(255,255,255,0.1)',
        borderRadius: 6, padding: 12, height: 'calc(100vh - 200px)',
        overflowY: 'auto', fontFamily: 'monospace', fontSize: 12, lineHeight: 1.6,
      }}>
        {logs.length === 0 ? (
          <Text style={{ color: '#8b949e' }}>No logs available. Agent may be offline.</Text>
        ) : (
          logs.map((line, i) => (
            <div key={i} style={{ color: getLineColor(line) }}>{line}</div>
          ))
        )}
        <div ref={logEndRef} />
      </div>
    </Drawer>
  );
}
