import { useEffect, useState } from 'react';
import { Card, Table, Button, Space, Tag, Row, Col, Statistic, Select, Typography, Divider, message, Tabs } from 'antd';
import { DownloadOutlined, ReloadOutlined, CloudServerOutlined, WindowsOutlined, AppleOutlined, LinuxOutlined, CopyOutlined, CodeOutlined, InfoCircleOutlined } from '@ant-design/icons';
import { motion } from 'framer-motion';
import api from '../utils/api';
import { formatBytes } from '../utils/format';

const { Title, Text, Paragraph } = Typography;

interface AgentBinary {
  filename: string;
  os: string;
  arch: string;
  version: string;
  description: string;
  size: number;
  available: boolean;
}

const osIcons: Record<string, React.ReactNode> = {
  Linux: <LinuxOutlined style={{ fontSize: 24, color: '#00E5FF' }} />,
  Windows: <WindowsOutlined style={{ fontSize: 24, color: '#A855F7' }} />,
  macOS: <AppleOutlined style={{ fontSize: 24, color: '#8B949E' }} />,
};

const osColors: Record<string, string> = {
  Linux: '#00E5FF',
  Windows: '#A855F7',
  macOS: '#8B949E',
};

export default function AgentsDownload() {
  const [agents, setAgents] = useState<AgentBinary[]>([]);
  const [loading, setLoading] = useState(false);
  const [selectedOS, setSelectedOS] = useState<string | null>(null);
  const [installScript, setInstallScript] = useState('');

  const fetchAgents = async () => {
    setLoading(true);
    try {
      const resp = await api.get('/agents-download/list');
      setAgents(resp.data.agents || []);
    } catch {}
    setLoading(false);
  };

  useEffect(() => { fetchAgents(); }, []);

  const filteredAgents = selectedOS
    ? agents.filter(a => a.os === selectedOS)
    : agents;

  const handleDownload = (filename: string) => {
    window.open(`/api/agents-download/download/${filename}`, '_blank');
  };

  const fetchInstallScript = async (osType: string) => {
    try {
      const resp = await api.get(`/agents-download/install-script/${osType}`);
      setInstallScript(resp.data);
    } catch {}
  };

  const copyScript = () => {
    navigator.clipboard.writeText(installScript);
    message.success('Copied to clipboard');
  };

  const columns = [
    {
      title: 'Agent', key: 'agent',
      render: (_: any, r: AgentBinary) => (
        <Space>
          {osIcons[r.os] || <CloudServerOutlined />}
          <div>
            <div style={{ fontWeight: 600 }}>{r.description}</div>
            <div style={{ fontSize: 11, color: '#8B949E' }}>{r.filename}</div>
          </div>
        </Space>
      ),
    },
    { title: 'OS', dataIndex: 'os', key: 'os', render: (o: string) => <Tag color={osColors[o]}>{o}</Tag> },
    { title: 'Arch', dataIndex: 'arch', key: 'arch', render: (a: string) => <Tag color="blue">{a}</Tag> },
    { title: 'Version', dataIndex: 'version', key: 'version', render: (v: string) => <Tag>v{v}</Tag> },
    { title: 'Size', dataIndex: 'size', key: 'size', render: (s: number) => s > 0 ? <Text strong>{formatBytes(s)}</Text> : <Text type="secondary">Not built</Text> },
    {
      title: '', key: 'actions', width: 140,
      render: (_: any, r: AgentBinary) => (
        <Button
          type="primary"
          icon={<DownloadOutlined />}
          onClick={() => handleDownload(r.filename)}
          disabled={!r.available}
          block
        >
          {r.available ? 'Download' : 'Not built'}
        </Button>
      ),
    },
  ];

  return (
    <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.3 }}>
      {/* OS Selector */}
      <Card style={{ marginBottom: 16 }}>
        <Row align="middle" justify="space-between">
          <Col>
            <Space>
              <Title level={4} style={{ margin: 0 }}>
                <DownloadOutlined style={{ color: '#00E5FF', marginRight: 8 }} />
                Download OBS Backup Agent
              </Title>
              <Text type="secondary">Select your operating system to download the agent</Text>
            </Space>
          </Col>
          <Col>
            <Space>
              <Select
                placeholder="Select OS"
                allowClear
                style={{ width: 200 }}
                value={selectedOS}
                onChange={setSelectedOS}
                options={[
                  { value: 'Linux', label: <Space><LinuxOutlined /> Linux</Space> },
                  { value: 'Windows', label: <Space><WindowsOutlined /> Windows</Space> },
                  { value: 'macOS', label: <Space><AppleOutlined /> macOS</Space> },
                ]}
              />
              <Button icon={<ReloadOutlined />} onClick={fetchAgents}>Refresh</Button>
            </Space>
          </Col>
        </Row>
      </Card>

      {/* Stats */}
      <Row gutter={[16, 16]} style={{ marginBottom: 16 }}>
        <Col xs={24} sm={8}>
          <Card size="small">
            <Statistic
              title="Available Agents"
              value={agents.filter(a => a.available).length}
              suffix={`/ ${agents.length}`}
              prefix={<CheckCircleIcon />}
              valueStyle={{ color: '#00FF88' }}
            />
          </Card>
        </Col>
        <Col xs={24} sm={8}>
          <Card size="small">
            <Statistic
              title="Platforms"
              value={3}
              prefix={<CloudServerOutlined style={{ color: '#00E5FF' }} />}
              suffix="OS"
            />
          </Card>
        </Col>
        <Col xs={24} sm={8}>
          <Card size="small">
            <Statistic
              title="Version"
              value={agents[0]?.version || '1.0.0'}
              prefix={<Tag color="green">Latest</Tag>}
            />
          </Card>
        </Col>
      </Row>

      {/* Agent table */}
      <Card
        title={
          <Space>
            <InfoCircleOutlined style={{ color: '#00E5FF' }} />
            <span>Agent Downloads</span>
            {selectedOS && <Tag color={osColors[selectedOS]}>{selectedOS}</Tag>}
          </Space>
        }
      >
        <Paragraph type="secondary" style={{ marginBottom: 16 }}>
          Download the OBS Backup agent for your platform. Install on target servers to enable backup protection.
          The agent communicates with the server via REST API or Unix Domain Socket.
        </Paragraph>
        <Table
          dataSource={filteredAgents}
          columns={columns}
          rowKey="filename"
          loading={loading}
          pagination={false}
        />
      </Card>

      {/* Quick Install */}
      <Card title={<><CodeOutlined /> Quick Install Commands</>} style={{ marginTop: 16 }}>
        <Tabs items={[
          {
            key: 'linux',
            label: <Space><LinuxOutlined /> Linux</Space>,
            children: (
              <Space direction="vertical" style={{ width: '100%' }}>
                <Paragraph copyable>curl -fsSL http://192.168.88.80/api/agents-download/download/obs-agent-linux-x64 -o /usr/local/bin/obs-agent && chmod +x /usr/local/bin/obs-agent</Paragraph>
                <Button size="small" onClick={() => fetchInstallScript('linux')} icon={<CodeOutlined />}>Full install script</Button>
              </Space>
            ),
          },
          {
            key: 'windows',
            label: <Space><WindowsOutlined /> Windows</Space>,
            children: (
              <Space direction="vertical" style={{ width: '100%' }}>
                <Paragraph copyable>Invoke-WebRequest -Uri "http://192.168.88.80/api/agents-download/download/obs-agent-windows-x64.exe" -OutFile "$env:TEMP\obs-agent.exe"</Paragraph>
                <Button size="small" onClick={() => fetchInstallScript('windows')} icon={<CodeOutlined />}>Full install guide</Button>
              </Space>
            ),
          },
          {
            key: 'macos',
            label: <Space><AppleOutlined /> macOS</Space>,
            children: (
              <Space direction="vertical" style={{ width: '100%' }}>
                <Paragraph copyable>curl -fsSL http://192.168.88.80/api/agents-download/download/obs-agent-macos-x64 -o /usr/local/bin/obs-agent && chmod +x /usr/local/bin/obs-agent</Paragraph>
                <Button size="small" onClick={() => fetchInstallScript('macos')} icon={<CodeOutlined />}>Full install script</Button>
              </Space>
            ),
          },
        ]} />

        {installScript && (
          <div style={{ marginTop: 12 }}>
            <Space style={{ marginBottom: 8 }}>
              <Button icon={<CopyOutlined />} onClick={copyScript}>Copy Script</Button>
            </Space>
            <pre style={{
              background: 'rgba(0,0,0,0.3)',
              padding: 12,
              borderRadius: 8,
              maxHeight: 300,
              overflow: 'auto',
              fontFamily: 'monospace',
              fontSize: 11,
              color: '#E6E6E6',
            }}>
              {installScript}
            </pre>
          </div>
        )}
      </Card>
    </motion.div>
  );
}

function CheckCircleIcon() {
  return <span style={{ color: '#00FF88' }}>✓</span>;
}
