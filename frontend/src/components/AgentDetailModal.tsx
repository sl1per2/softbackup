import React, { useEffect, useState, useRef } from 'react';
import {
  Modal, Tabs, Button, Space, Typography, Descriptions, Badge, Form, Input,
  Select, InputNumber, Table, Tag, Card, Row, Col, Statistic, Popconfirm,
  message, Drawer, Switch, Slider, Spin, Alert, Tooltip, Progress,
} from 'antd';
import {
  PlayCircleOutlined, StopOutlined, ReloadOutlined, CloudDownloadOutlined,
  DeleteOutlined, SettingOutlined, FileTextOutlined, ClearOutlined,
  ThunderboltOutlined, SyncOutlined, SaveOutlined, DownloadOutlined,
  SearchOutlined, ExclamationCircleOutlined, CheckCircleOutlined,
  ClockCircleOutlined, DashboardOutlined, DatabaseOutlined,
} from '@ant-design/icons';
import ReactECharts from 'echarts-for-react';
import axios from 'axios';
import { useStorageOptions, usePolicyOptions } from '../hooks/useData';

const { Title, Text, Paragraph } = Typography;
const { TextArea } = Input;

interface AgentData {
  id: number;
  hostname: string;
  ip: string;
  os_type: string;
  version: string | null;
  status: string;
  last_seen: string | null;
  core_version: string | null;
  transport_mode: string;
  cache_size_bytes: number;
  cache_hit_ratio: number;
}

interface AgentMetrics {
  cpu_usage: number;
  memory_usage_mb: number;
  disk_usage_percent: number;
  uptime_seconds: number;
  active_jobs: number;
}

interface AgentConfig {
  server_url: string;
  log_level: string;
  heartbeat_interval_sec: number;
  cache_size_mb: number;
  max_concurrent_jobs: number;
  bandwidth_limit_kbps: number;
}

interface Props {
  agent: AgentData | null;
  open: boolean;
  onClose: () => void;
}

export default function AgentDetailModal({ agent, open, onClose }: Props) {
  const [metrics, setMetrics] = useState<AgentMetrics | null>(null);
  const [config, setConfig] = useState<AgentConfig | null>(null);
  const [logs, setLogs] = useState<string[]>([]);
  const [logLevel, setLogLevel] = useState<string | undefined>();
  const [logSearch, setLogSearch] = useState('');
  const [loading, setLoading] = useState(false);
  const [commandLoading, setCommandLoading] = useState<string | null>(null);
  const [backupDrawerOpen, setBackupDrawerOpen] = useState(false);
  const [deployDrawerOpen, setDeployDrawerOpen] = useState(false);
  const [configForm] = Form.useForm();
  const [backupForm] = Form.useForm();
  const [deployForm] = Form.useForm();
  const logEndRef = useRef<HTMLDivElement>(null);
  const [autoScroll, setAutoScroll] = useState(true);
  const { options: storageOptions } = useStorageOptions();
  const { options: policyOptions } = usePolicyOptions();

  useEffect(() => {
    if (open && agent) {
      fetchMetrics();
      fetchConfig();
      fetchLogs();
    }
  }, [open, agent]);

  useEffect(() => {
    if (autoScroll && logEndRef.current) {
      logEndRef.current.scrollIntoView({ behavior: 'smooth' });
    }
  }, [logs, autoScroll]);

  const fetchMetrics = async () => {
    if (!agent) return;
    try {
      const res = await axios.get(`/api/agents/${agent.id}/metrics`);
      setMetrics(res.data);
    } catch {
      message.error('Failed to load agent metrics');
    }
  };

  const fetchConfig = async () => {
    if (!agent) return;
    try {
      const res = await axios.get(`/api/agents/${agent.id}/config`);
      setConfig(res.data);
      configForm.setFieldsValue(res.data);
    } catch {
      message.error('Failed to load agent configuration');
    }
  };

  const fetchLogs = async () => {
    if (!agent) return;
    try {
      const params: any = { tail: 500 };
      if (logLevel) params.level = logLevel;
      if (logSearch) params.search = logSearch;
      const res = await axios.get(`/api/agents/${agent.id}/logs`, { params });
      setLogs(res.data.logs || []);
    } catch {
      message.error('Failed to load agent logs');
    }
  };

  const sendCommand = async (command: string, confirmMsg?: string) => {
    if (!agent) return;
    if (confirmMsg) {
      Modal.confirm({
        title: confirmMsg,
        icon: <ExclamationCircleOutlined />,
        okText: 'Yes',
        okType: 'danger',
        cancelText: 'Cancel',
        onOk: () => executeCommand(command),
      });
    } else {
      await executeCommand(command);
    }
  };

  const executeCommand = async (command: string) => {
    if (!agent) return;
    setCommandLoading(command);
    try {
      await axios.post(`/api/agents/${agent.id}/command/${command}`);
      message.success(`Command "${command}" sent successfully`);
      setTimeout(fetchMetrics, 2000);
    } catch {
      message.error(`Failed to send command "${command}"`);
    }
    setCommandLoading(null);
  };

  const handleQuickBackup = async (values: any) => {
    if (!agent) return;
    setCommandLoading('start-backup');
    try {
      await axios.post(`/api/agents/${agent.id}/command/start-backup`, {
        backup_type: values.backup_type,
        source_paths: values.source_paths?.split(',').map((s: string) => s.trim()) || [],
        policy_id: values.policy_id,
      });
      message.success('Backup started');
      setBackupDrawerOpen(false);
    } catch {
      message.error('Failed to start backup');
    }
    setCommandLoading(null);
  };

  const handleConfigSave = async (values: any) => {
    if (!agent) return;
    setCommandLoading('save-config');
    try {
      await axios.put(`/api/agents/${agent.id}/config`, values);
      message.success('Configuration saved. Agent will apply changes within 30 seconds.');
    } catch {
      message.error('Failed to save configuration');
    }
    setCommandLoading(null);
  };

  const handleDeploy = async (values: any) => {
    setCommandLoading('deploy');
    try {
      const endpoint = values.os_type === 'windows' ? '/api/agents/deploy/winrm' : '/api/agents/deploy/ssh';
      await axios.post(endpoint, values);
      message.success(`Deploy command sent to ${values.host}`);
      setDeployDrawerOpen(false);
    } catch {
      message.error('Deployment failed');
    }
    setCommandLoading(null);
  };

  const handleDownloadLogs = () => {
    if (!agent) return;
    window.open(`/api/agents/${agent.id}/logs/download`, '_blank');
  };

  if (!agent) return null;

  const isOnline = agent.status === 'online' || agent.status === 'backing_up' || agent.status === 'cdp_active';

  const overviewTab = (
    <div>
      <Row gutter={16} style={{ marginBottom: 16 }}>
        <Col span={6}>
          <Card size="small" style={{ background: 'rgba(255,255,255,0.04)' }}>
            <Statistic
              title="Status"
              value={agent.status.toUpperCase()}
              valueStyle={{ color: isOnline ? '#52c41a' : '#ff4d4f', fontSize: 16 }}
              prefix={isOnline ? <CheckCircleOutlined /> : <ExclamationCircleOutlined />}
            />
          </Card>
        </Col>
        <Col span={6}>
          <Card size="small" style={{ background: 'rgba(255,255,255,0.04)' }}>
            <Statistic title="CPU" value={metrics?.cpu_usage || 0} suffix="%" prefix={<DashboardOutlined />} />
          </Card>
        </Col>
        <Col span={6}>
          <Card size="small" style={{ background: 'rgba(255,255,255,0.04)' }}>
            <Statistic title="Memory" value={metrics?.memory_usage_mb || 0} suffix="MB" prefix={<DatabaseOutlined />} />
          </Card>
        </Col>
        <Col span={6}>
          <Card size="small" style={{ background: 'rgba(255,255,255,0.04)' }}>
            <Statistic title="Disk" value={metrics?.disk_usage_percent || 0} suffix="%" prefix={<CloudDownloadOutlined />} />
          </Card>
        </Col>
      </Row>
      <Descriptions bordered column={2} size="small">
        <Descriptions.Item label="Hostname">{agent.hostname}</Descriptions.Item>
        <Descriptions.Item label="IP Address">{agent.ip}</Descriptions.Item>
        <Descriptions.Item label="OS">{agent.os_type}</Descriptions.Item>
        <Descriptions.Item label="Agent Version">{agent.version || 'N/A'}</Descriptions.Item>
        <Descriptions.Item label="Core Version">{agent.core_version || 'N/A'}</Descriptions.Item>
        <Descriptions.Item label="Transport">{agent.transport_mode}</Descriptions.Item>
        <Descriptions.Item label="Cache Hit">{(agent.cache_hit_ratio * 100).toFixed(1)}%</Descriptions.Item>
        <Descriptions.Item label="Last Seen">{agent.last_seen ? new Date(agent.last_seen).toLocaleString() : 'Never'}</Descriptions.Item>
      </Descriptions>
    </div>
  );

  const controlTab = (
    <div>
      <Alert
        message="Remote Agent Control"
        description="These commands are sent to the agent over the network. The agent must be online."
        type="info"
        showIcon
        style={{ marginBottom: 16 }}
      />
      <Row gutter={[16, 16]}>
        <Col span={8}>
          <Card hoverable style={{ textAlign: 'center' }}>
            <Button
              type="primary"
              size="large"
              icon={<PlayCircleOutlined />}
              loading={commandLoading === 'start-backup'}
              onClick={() => setBackupDrawerOpen(true)}
              style={{ width: '100%', height: 60, fontSize: 16 }}
            >
              Quick Backup
            </Button>
          </Card>
        </Col>
        <Col span={8}>
          <Card hoverable style={{ textAlign: 'center' }}>
            <Popconfirm title="Stop ALL running jobs?" onConfirm={() => sendCommand('stop-jobs')} okText="Stop All" cancelText="Cancel">
              <Button
                danger
                size="large"
                icon={<StopOutlined />}
                loading={commandLoading === 'stop-jobs'}
                style={{ width: '100%', height: 60, fontSize: 16 }}
              >
                Stop All Jobs
              </Button>
            </Popconfirm>
          </Card>
        </Col>
        <Col span={8}>
          <Card hoverable style={{ textAlign: 'center' }}>
            <Popconfirm title="Restart the agent? This will briefly interrupt running jobs." onConfirm={() => sendCommand('restart')} okText="Restart" cancelText="Cancel">
              <Button
                size="large"
                icon={<ReloadOutlined />}
                loading={commandLoading === 'restart'}
                style={{ width: '100%', height: 60, fontSize: 16, borderColor: '#faad14', color: '#faad14' }}
              >
                Restart Agent
              </Button>
            </Popconfirm>
          </Card>
        </Col>
        <Col span={8}>
          <Card hoverable style={{ textAlign: 'center' }}>
            <Button
              size="large"
              icon={<CloudDownloadOutlined />}
              loading={commandLoading === 'update'}
              onClick={() => sendCommand('update', 'Check for and install agent updates?')}
              style={{ width: '100%', height: 60, fontSize: 16 }}
            >
              Update Agent
            </Button>
          </Card>
        </Col>
        <Col span={8}>
          <Card hoverable style={{ textAlign: 'center' }}>
            <Button
              size="large"
              icon={<ThunderboltOutlined />}
              loading={commandLoading === 'flush-dirty-buffers'}
              onClick={() => sendCommand('flush-dirty-buffers', 'Flush dirty buffers now?')}
              style={{ width: '100%', height: 60, fontSize: 16 }}
            >
              Flush Dirty Buffers
            </Button>
          </Card>
        </Col>
        <Col span={8}>
          <Card hoverable style={{ textAlign: 'center' }}>
            <Popconfirm title="Clear chunk dedup cache? This increases storage usage until new chunks are created." onConfirm={() => sendCommand('clear-cache')} okText="Clear" cancelText="Cancel">
              <Button
                size="large"
                icon={<DeleteOutlined />}
                loading={commandLoading === 'clear-cache'}
                style={{ width: '100%', height: 60, fontSize: 16 }}
              >
                Clear Chunk Cache
              </Button>
            </Popconfirm>
          </Card>
        </Col>
      </Row>
    </div>
  );

  const configTab = (
    <div>
      <Form
        form={configForm}
        layout="vertical"
        onFinish={handleConfigSave}
        initialValues={{
          log_level: 'info',
          heartbeat_interval_sec: 30,
          cache_size_mb: 1024,
          max_concurrent_jobs: 2,
          bandwidth_limit_kbps: 0,
        }}
      >
        <Form.Item label="Server URL">
          <Input disabled value={config?.server_url || ''} />
        </Form.Item>
        <Row gutter={16}>
          <Col span={8}>
            <Form.Item label="Log Level" name="log_level">
              <Select options={[
                { value: 'debug', label: 'DEBUG' },
                { value: 'info', label: 'INFO' },
                { value: 'warn', label: 'WARN' },
                { value: 'error', label: 'ERROR' },
              ]} />
            </Form.Item>
          </Col>
          <Col span={8}>
            <Form.Item label="Heartbeat Interval (sec)" name="heartbeat_interval_sec">
              <InputNumber min={5} max={300} style={{ width: '100%' }} />
            </Form.Item>
          </Col>
          <Col span={8}>
            <Form.Item label="Cache Size (MB)" name="cache_size_mb">
              <InputNumber min={64} max={102400} style={{ width: '100%' }} />
            </Form.Item>
          </Col>
        </Row>
        <Row gutter={16}>
          <Col span={8}>
            <Form.Item label="Max Concurrent Jobs" name="max_concurrent_jobs">
              <InputNumber min={1} max={16} style={{ width: '100%' }} />
            </Form.Item>
          </Col>
          <Col span={8}>
            <Form.Item label="Bandwidth Limit (KB/s, 0=unlimited)" name="bandwidth_limit_kbps">
              <InputNumber min={0} max={1000000} style={{ width: '100%' }} />
            </Form.Item>
          </Col>
        </Row>
        <Space>
          <Button type="primary" htmlType="submit" icon={<SaveOutlined />} loading={commandLoading === 'save-config'}>
            Save Configuration
          </Button>
          <Button icon={<ReloadOutlined />} onClick={() => sendCommand('restart', 'Apply configuration now by restarting agent?')}>
            Apply Immediately
          </Button>
        </Space>
      </Form>
    </div>
  );

  const logsTab = (
    <div>
      <Space style={{ marginBottom: 12 }} wrap>
        <Select
          placeholder="Level"
          style={{ width: 120 }}
          allowClear
          value={logLevel}
          onChange={(v) => { setLogLevel(v); }}
          options={[
            { value: 'ERROR', label: 'ERROR' },
            { value: 'WARN', label: 'WARN' },
            { value: 'INFO', label: 'INFO' },
            { value: 'DEBUG', label: 'DEBUG' },
          ]}
        />
        <Input
          placeholder="Search logs..."
          style={{ width: 200 }}
          value={logSearch}
          onChange={(e) => setLogSearch(e.target.value)}
          onPressEnter={fetchLogs}
          suffix={<SearchOutlined onClick={fetchLogs} style={{ cursor: 'pointer' }} />}
        />
        <Button icon={<ReloadOutlined />} onClick={fetchLogs}>Refresh</Button>
        <Button icon={<DownloadOutlined />} onClick={handleDownloadLogs}>Download Full Log</Button>
        <Switch
          checked={autoScroll}
          onChange={setAutoScroll}
          checkedChildren="Auto-scroll"
          unCheckedChildren="Manual"
        />
      </Space>
      <div style={{
        background: '#0d1117',
        border: '1px solid rgba(255,255,255,0.1)',
        borderRadius: 6,
        padding: 12,
        height: 400,
        overflowY: 'auto',
        fontFamily: 'monospace',
        fontSize: 12,
        lineHeight: 1.6,
      }}>
        {logs.length === 0 ? (
          <Text style={{ color: '#8b949e' }}>No logs available. Agent may be offline.</Text>
        ) : (
          logs.map((line, i) => {
            const color = line.includes('ERROR') ? '#ff4d4f'
              : line.includes('WARN') ? '#faad14'
              : line.includes('DEBUG') ? '#8b949e'
              : '#52c41a';
            return <div key={i} style={{ color }}>{line}</div>;
          })
        )}
        <div ref={logEndRef} />
      </div>
    </div>
  );

  const jobsTab = (
    <div>
      <Button type="primary" icon={<PlayCircleOutlined />} onClick={() => setBackupDrawerOpen(true)} style={{ marginBottom: 16 }}>
        Start Backup Now
      </Button>
      <Table
        dataSource={[]}
        columns={[
          { title: 'Job ID', dataIndex: 'job_id', key: 'job_id', render: (v: string) => <Text code>{v}</Text> },
          { title: 'Type', dataIndex: 'type', key: 'type', render: (v: string) => <Tag color="blue">{v}</Tag> },
          { title: 'Status', dataIndex: 'status', key: 'status' },
          { title: 'Progress', dataIndex: 'progress', key: 'progress', render: (v: number) => <Progress percent={v} size="small" /> },
          { title: 'Started', dataIndex: 'started_at', key: 'started', render: (v: string) => new Date(v).toLocaleString() },
          { title: '', key: 'cancel', render: () => <Button danger size="small" icon={<StopOutlined />} /> },
        ]}
        rowKey="job_id"
        pagination={{ pageSize: 10 }}
        locale={{ emptyText: 'No active jobs' }}
      />
    </div>
  );

  const dirtyBuffersTab = (
    <div>
      <Row gutter={16} style={{ marginBottom: 16 }}>
        <Col span={8}>
          <Card size="small">
            <Statistic title="Dirty Pages" value={metrics?.active_jobs || 0} prefix={<DatabaseOutlined />} />
          </Card>
        </Col>
        <Col span={8}>
          <Card size="small">
            <Statistic title="Status" value="Ready" valueStyle={{ color: '#52c41a' }} />
          </Card>
        </Col>
        <Col span={8}>
          <Card size="small">
            <Statistic title="Last Flush" value="OK" valueStyle={{ color: '#52c41a' }} />
          </Card>
        </Col>
      </Row>
      <Button
        type="primary"
        icon={<ThunderboltOutlined />}
        loading={commandLoading === 'flush-dirty-buffers'}
        onClick={() => sendCommand('flush-dirty-buffers')}
        style={{ marginBottom: 16 }}
      >
        Flush Dirty Buffers Now
      </Button>
      <Table
        dataSource={[]}
        columns={[
          { title: 'Time', dataIndex: 'time', key: 'time' },
          { title: 'Before', dataIndex: 'before', key: 'before' },
          { title: 'After', dataIndex: 'after', key: 'after' },
          { title: 'Status', dataIndex: 'status', key: 'status', render: (v: string) => <Tag color={v === 'flushed' ? 'success' : 'error'}>{v}</Tag> },
          { title: 'Duration', dataIndex: 'duration', key: 'duration' },
        ]}
        rowKey="time"
        pagination={{ pageSize: 10 }}
        locale={{ emptyText: 'No flush history' }}
      />
    </div>
  );

  return (
    <>
      <Modal
        title={
          <Space>
            <Badge status={isOnline ? 'success' : 'error'} />
            <Text strong>{agent.hostname}</Text>
            <Tag>{agent.ip}</Tag>
          </Space>
        }
        open={open}
        onCancel={onClose}
        footer={null}
        width={900}
        styles={{ body: { background: 'transparent' } }}
      >
        <Tabs
          defaultActiveKey="overview"
          items={[
            { key: 'overview', label: 'Overview', children: overviewTab },
            { key: 'control', label: 'Control', children: controlTab },
            { key: 'config', label: 'Configuration', children: configTab },
            { key: 'logs', label: 'Logs', children: logsTab },
            { key: 'jobs', label: 'Jobs', children: jobsTab },
            { key: 'dirty', label: 'Dirty Buffers', children: dirtyBuffersTab },
          ]}
        />
      </Modal>

      <Drawer
        title="Quick Backup"
        open={backupDrawerOpen}
        onClose={() => setBackupDrawerOpen(false)}
        width={450}
      >
        <Form form={backupForm} layout="vertical" onFinish={handleQuickBackup} initialValues={{ backup_type: 'full' }}>
          <Form.Item label="Backup Type" name="backup_type" rules={[{ required: true }]}>
            <Select options={[
              { value: 'full', label: 'Full Backup' },
              { value: 'incremental', label: 'Incremental Backup' },
              { value: 'differential', label: 'Differential Backup' },
            ]} />
          </Form.Item>
          <Form.Item label="Source Paths (comma-separated)" name="source_paths">
            <TextArea rows={3} placeholder="/home, /var/data, C:\Users" />
          </Form.Item>
          <Form.Item label="Storage" name="storage_id">
            <Select placeholder="Select storage" allowClear options={storageOptions} />
          </Form.Item>
          <Form.Item label="Policy" name="policy_id">
            <Select placeholder="Select policy" allowClear options={policyOptions} />
          </Form.Item>
          <Button type="primary" htmlType="submit" block size="large" loading={commandLoading === 'start-backup'} icon={<PlayCircleOutlined />}>
            Start Backup
          </Button>
        </Form>
      </Drawer>

      <Drawer
        title="Deploy Agent to New Host"
        open={deployDrawerOpen}
        onClose={() => setDeployDrawerOpen(false)}
        width={500}
      >
        <Alert message="Deploy agent via SSH or WinRM" description="Provide host credentials to install the agent remotely." type="info" showIcon style={{ marginBottom: 16 }} />
        <Form form={deployForm} layout="vertical" onFinish={handleDeploy}>
          <Row gutter={16}>
            <Col span={16}>
              <Form.Item label="Host" name="host" rules={[{ required: true }]}>
                <Input placeholder="192.168.1.100 or hostname" />
              </Form.Item>
            </Col>
            <Col span={8}>
              <Form.Item label="Port" name="port" initialValue={22}>
                <InputNumber min={1} max={65535} style={{ width: '100%' }} />
              </Form.Item>
            </Col>
          </Row>
          <Row gutter={16}>
            <Col span={12}>
              <Form.Item label="Username" name="username" rules={[{ required: true }]}>
                <Input placeholder="root" />
              </Form.Item>
            </Col>
            <Col span={12}>
              <Form.Item label="Password" name="password">
                <Input.Password placeholder="or use private key" />
              </Form.Item>
            </Col>
          </Row>
          <Form.Item label="OS Type" name="os_type" initialValue="linux">
            <Select options={[
              { value: 'linux', label: 'Linux' },
              { value: 'windows', label: 'Windows' },
              { value: 'macos', label: 'macOS' },
            ]} />
          </Form.Item>
          <Form.Item label="Private Key (optional)" name="private_key">
            <TextArea rows={3} placeholder="-----BEGIN RSA PRIVATE KEY-----" />
          </Form.Item>
          <Row gutter={16}>
            <Col span={16}>
              <Form.Item label="Server URL" name="server_url">
                <Input placeholder="https://obs-server:8000" />
              </Form.Item>
            </Col>
            <Col span={8}>
              <Form.Item label="Auth Token" name="auth_token">
                <Input.Password />
              </Form.Item>
            </Col>
          </Row>
          <Button type="primary" htmlType="submit" block size="large" loading={commandLoading === 'deploy'} icon={<CloudDownloadOutlined />}>
            Deploy Agent
          </Button>
        </Form>
      </Drawer>
    </>
  );
}
