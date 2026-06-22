import React, { useState, useCallback } from 'react';
import { Card, Table, Button, Input, Tag, Space, Typography, message } from 'antd';
import { SearchOutlined, ReloadOutlined, CloudServerOutlined } from '@ant-design/icons';
import api from '../utils/api';

const { Text } = Typography;

interface DiscoveredHost {
  ip: string;
  hostname: string;
  os_type: string;
  has_agent: boolean;
  agent_version?: string;
  port_open: boolean;
}

interface Props {
  onSelect?: (host: DiscoveredHost) => void;
}

export default function NetworkDiscovery({ onSelect }: Props) {
  const [hosts, setHosts] = useState<DiscoveredHost[]>([]);
  const [loading, setLoading] = useState(false);
  const [subnet, setSubnet] = useState('192.168.1.0/24');

  const handleScan = useCallback(async () => {
    setLoading(true);
    try {
      const res = await api.post('/api/discovery/scan', { subnet, port: 9900, timeout_ms: 500 });
      setHosts(res.data);
      message.info(`Found ${res.data.length} hosts`);
    } catch {
      message.error('Network scan failed');
    }
    setLoading(false);
  }, [subnet]);

  const columns = [
    {
      title: 'IP',
      dataIndex: 'ip',
      key: 'ip',
      render: (v: string) => <Text code>{v}</Text>,
    },
    {
      title: 'Hostname',
      dataIndex: 'hostname',
      key: 'hostname',
    },
    {
      title: 'Agent',
      key: 'agent',
      render: (_: any, r: DiscoveredHost) => r.has_agent
        ? <Tag color="success">Installed ({r.agent_version})</Tag>
        : <Tag color="default">Not installed</Tag>,
    },
    {
      title: 'Port 9900',
      dataIndex: 'port_open',
      key: 'port',
      render: (v: boolean) => v ? <Tag color="success">Open</Tag> : <Tag color="default">Closed</Tag>,
    },
    {
      title: 'Actions',
      key: 'actions',
      render: (_: any, r: DiscoveredHost) => (
        <Button size="small" type="primary" onClick={() => onSelect?.(r)}>
          {r.has_agent ? 'Manage' : 'Install'}
        </Button>
      ),
    },
  ];

  return (
    <Card title={<><CloudServerOutlined /> Network Discovery</>}
      extra={<Space>
        <Input value={subnet} onChange={e => setSubnet(e.target.value)}
          style={{ width: 200 }} placeholder="192.168.1.0/24" />
        <Button type="primary" icon={<SearchOutlined />} onClick={handleScan} loading={loading}>Scan</Button>
        <Button icon={<ReloadOutlined />} onClick={handleScan}>Refresh</Button>
      </Space>}
    >
      <Table dataSource={hosts} columns={columns} rowKey="ip" pagination={{ pageSize: 20 }} />
    </Card>
  );
}
