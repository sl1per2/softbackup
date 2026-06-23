import { useEffect, useState } from 'react';
import { Table, Card, Button, Space, Tag, Tabs, Modal, Form, Input, InputNumber, Select, Switch, Progress, Badge, Tooltip, message, Popconfirm, Row, Col } from 'antd';
import {
  CloudServerOutlined, HddOutlined, DesktopOutlined,
  PlayCircleOutlined, PauseCircleOutlined, CameraOutlined,
  SyncOutlined, DeleteOutlined, PlusOutlined, ReloadOutlined,
  ExportOutlined, ImportOutlined, ThunderboltOutlined,
  CheckCircleOutlined, CloseCircleOutlined,
} from '@ant-design/icons';
import { motion } from 'framer-motion';
import GlassModal from '../components/GlassModal';
import StatusBadge from '../components/StatusBadge';
import api from '../utils/api';
import { formatBytes, formatDate } from '../utils/format';

const hypervisorIcons: Record<string, React.ReactNode> = {
  vmware: <CloudServerOutlined style={{ color: '#00E5FF' }} />,
  hyperv: <DesktopOutlined style={{ color: '#A855F7' }} />,
  proxmox: <HddOutlined style={{ color: '#FFA502' }} />,
};

const hypervisorColors: Record<string, string> = {
  vmware: '#00E5FF',
  hyperv: '#A855F7',
  proxmox: '#FFA502',
};

interface VM {
  vm_id: string;
  name: string;
  host: string;
  type: string;
  power_state: string;
  cpu: number;
  memory_mb: number;
  disk_gb: number;
  os: string;
  ip: string;
  tools: boolean;
}

interface Hypervisor {
  id: string;
  name: string;
  type: string;
  host: string;
  connected: boolean;
  vm_count: number;
}

interface BackupJob {
  job_id: string;
  vm_id: string;
  status: string;
  transport: string;
  started_at: string;
  total_bytes: number;
  transferred_bytes: number;
  speed_mbps: number;
}

export default function Virtualization() {
  const [vms, setVMs] = useState<VM[]>([]);
  const [hypervisors, setHypervisors] = useState<Hypervisor[]>([]);
  const [backupJobs, setBackupJobs] = useState<BackupJob[]>([]);
  const [loading, setLoading] = useState(false);
  const [addConnOpen, setAddConnOpen] = useState(false);
  const [selectedVM, setSelectedVM] = useState<VM | null>(null);
  const [detailOpen, setDetailOpen] = useState(false);
  const [features, setFeatures] = useState<any>(null);
  const [form] = Form.useForm();
  const [backupForm] = Form.useForm();

  const fetchAll = async () => {
    setLoading(true);
    try {
      const [vmResp, connResp, jobResp, featResp] = await Promise.all([
        api.get('/virtualization/vms'),
        api.get('/virtualization/hypervisors'),
        api.get('/virtualization/backup-jobs'),
        api.get('/virtualization/features'),
      ]);
      setVMs(vmResp.data);
      setHypervisors(connResp.data);
      setBackupJobs(jobResp.data);
      setFeatures(featResp.data);
    } catch {
      message.error('Failed to load virtualization data');
    }
    setLoading(false);
  };

  useEffect(() => { fetchAll(); }, []);

  const handleAddConnection = async () => {
    const values = await form.validateFields();
    try {
      await api.post('/virtualization/hypervisors', values);
      message.success('Hypervisor connected');
      setAddConnOpen(false);
      form.resetFields();
      fetchAll();
    } catch (err: any) {
      message.error(err.response?.data?.detail || 'Connection failed');
    }
  };

  const handleBackup = async (vmId: string) => {
    try {
      await api.post(`/virtualization/vms/${vmId}/backup`, {
        vm_id: vmId,
        storage_id: 'local',
        transport: 'NETWORK',
        quiesce: true,
      });
      message.success('Backup started');
      fetchAll();
    } catch (err: any) {
      message.error(err.response?.data?.detail || 'Backup failed');
    }
  };

  const handlePowerAction = async (vmId: string, action: string) => {
    try {
      await api.post(`/virtualization/vms/${vmId}/power/${action}`);
      message.success(`VM ${action === 'on' ? 'started' : action === 'off' ? 'stopped' : 'suspended'}`);
      fetchAll();
    } catch (err: any) {
      message.error('Action failed');
    }
  };

  const handleSnapshot = async (vmId: string) => {
    try {
      await api.post(`/virtualization/vms/${vmId}/snapshot?name=obs-snapshot&quiesce=true`);
      message.success('Snapshot created');
    } catch (err: any) {
      message.error('Snapshot failed');
    }
  };

  const vmColumns = [
    {
      title: 'VM', key: 'vm',
      render: (_: any, r: VM) => (
        <Space>
          <span style={{ color: hypervisorColors[r.type] }}>{hypervisorIcons[r.type]}</span>
          <div>
            <div style={{ fontWeight: 600 }}>{r.name}</div>
            <div style={{ fontSize: 11, color: '#8B949E' }}>{r.vm_id}</div>
          </div>
        </Space>
      ),
    },
    { title: 'Type', dataIndex: 'type', key: 'type', render: (t: string) => <Tag color={hypervisorColors[t]}>{t.toUpperCase()}</Tag> },
    { title: 'Host', dataIndex: 'host', key: 'host' },
    { title: 'State', dataIndex: 'power_state', key: 'state', render: (s: string) =>
      <Badge status={s === 'on' ? 'success' : 'default'} text={s.toUpperCase()} />
    },
    { title: 'CPU', dataIndex: 'cpu', key: 'cpu', render: (v: number) => `${v} vCPU` },
    { title: 'RAM', dataIndex: 'memory_mb', key: 'ram', render: (v: number) => `${(v / 1024).toFixed(0)} GB` },
    { title: 'Disk', dataIndex: 'disk_gb', key: 'disk', render: (v: number) => `${v} GB` },
    { title: 'OS', dataIndex: 'os', key: 'os', render: (t: string) => <Tag>{t}</Tag> },
    { title: 'Tools', dataIndex: 'tools', key: 'tools', render: (v: boolean) =>
      v ? <CheckCircleOutlined style={{ color: '#00FF88' }} /> : <CloseCircleOutlined style={{ color: '#FF4757' }} />
    },
    {
      title: 'Actions', key: 'actions', render: (_: any, r: VM) => (
        <Space size="small">
          <Tooltip title="Backup"><Button size="small" type="primary" icon={<SyncOutlined />} onClick={() => handleBackup(r.vm_id)} /></Tooltip>
          <Tooltip title="Snapshot"><Button size="small" icon={<CameraOutlined />} onClick={() => handleSnapshot(r.vm_id)} /></Tooltip>
          <Tooltip title="Power On"><Button size="small" icon={<PlayCircleOutlined />} onClick={() => handlePowerAction(r.vm_id, 'on')} /></Tooltip>
          <Tooltip title="Power Off"><Button size="small" danger icon={<PauseCircleOutlined />} onClick={() => handlePowerAction(r.vm_id, 'off')} /></Tooltip>
          <Button size="small" onClick={() => { setSelectedVM(r); setDetailOpen(true); }}>Details</Button>
        </Space>
      ),
    },
  ];

  const jobColumns = [
    { title: 'Job ID', dataIndex: 'job_id', key: 'id' },
    { title: 'VM', dataIndex: 'vm_id', key: 'vm' },
    { title: 'Status', dataIndex: 'status', key: 'status', render: (s: string) => <StatusBadge status={s} /> },
    { title: 'Transport', dataIndex: 'transport', key: 'transport', render: (t: string) => <Tag color="blue">{t}</Tag> },
    { title: 'Progress', key: 'progress', render: (_: any, r: BackupJob) => (
      <Progress percent={r.total_bytes > 0 ? Math.round(r.transferred_bytes / r.total_bytes * 100) : 0} size="small" />
    )},
    { title: 'Speed', dataIndex: 'speed_mbps', key: 'speed', render: (v: number) => v > 0 ? `${v} MB/s` : '-' },
    { title: 'Started', dataIndex: 'started_at', key: 'started', render: (v: string) => formatDate(v) },
  ];

  return (
    <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.3 }}>
      {/* Stats row */}
      <Row gutter={[16, 16]} style={{ marginBottom: 16 }}>
        {Object.entries(features || {}).map(([type, info]: [string, any]) => (
          <Col xs={24} sm={8} key={type}>
            <Card size="small" style={{ borderTop: `3px solid ${hypervisorColors[type]}` }}>
              <Space>
                <span style={{ fontSize: 24, color: hypervisorColors[type] }}>{hypervisorIcons[type]}</span>
                <div>
                  <div style={{ fontWeight: 600 }}>{info.versions?.[info.versions.length - 1] || type}</div>
                  <div style={{ fontSize: 11, color: '#8B949E' }}>
                    {info.features?.length || 0} features | {info.transports?.length || 0} transports
                  </div>
                </div>
              </Space>
            </Card>
          </Col>
        ))}
      </Row>

      <Tabs items={[
        {
          key: 'vms', label: `Virtual Machines (${vms.length})`,
          children: (
            <>
              <Space style={{ marginBottom: 16 }}>
                <Button type="primary" icon={<PlusOutlined />} onClick={() => setAddConnOpen(true)}>Add Hypervisor</Button>
                <Button icon={<ReloadOutlined />} onClick={fetchAll}>Refresh</Button>
              </Space>
              <Table dataSource={vms} columns={vmColumns} rowKey="vm_id" loading={loading}
                onRow={(record) => ({
                  onClick: () => { setSelectedVM(record); setDetailOpen(true); },
                  style: { cursor: 'pointer' },
                })}
                pagination={{ pageSize: 20 }} />
            </>
          ),
        },
        {
          key: 'connections', label: `Hypervisors (${hypervisors.length})`,
          children: (
            <Table dataSource={hypervisors} rowKey="id" loading={loading}
              columns={[
                { title: 'Name', dataIndex: 'name', key: 'name', render: (t: string) => <strong>{t}</strong> },
                { title: 'Type', dataIndex: 'type', key: 'type', render: (t: string) => <Tag color={hypervisorColors[t]}>{t.toUpperCase()}</Tag> },
                { title: 'Host', dataIndex: 'host', key: 'host' },
                { title: 'Status', dataIndex: 'connected', key: 'status', render: (v: boolean) =>
                  <Badge status={v ? 'success' : 'error'} text={v ? 'Connected' : 'Disconnected'} />
                },
                { title: 'VMs', dataIndex: 'vm_count', key: 'vms' },
                { title: 'Actions', key: 'actions', render: (_: any, r: Hypervisor) => (
                  <Space>
                    <Button size="small">Test</Button>
                    <Popconfirm title="Disconnect?" onConfirm={() => {
                      api.delete(`/virtualization/hypervisors/${r.id}`);
                      fetchAll();
                    }}>
                      <Button size="small" danger>Disconnect</Button>
                    </Popconfirm>
                  </Space>
                )},
              ]}
              pagination={false} />
          ),
        },
        {
          key: 'jobs', label: `Backup Jobs (${backupJobs.length})`,
          children: (
            <Table dataSource={backupJobs} columns={jobColumns} rowKey="job_id" loading={loading} pagination={{ pageSize: 10 }} />
          ),
        },
        {
          key: 'features', label: 'Supported Features',
          children: (
            <div>
              {features && Object.entries(features).map(([type, info]: [string, any]) => (
                <Card key={type} title={<Space><span style={{ color: hypervisorColors[type] }}>{hypervisorIcons[type]}</span>{type.toUpperCase()}</Space>}
                  size="small" style={{ marginBottom: 16, borderTop: `3px solid ${hypervisorColors[type]}` }}>
                  <Row gutter={[16, 16]}>
                    <Col span={8}>
                      <div style={{ fontWeight: 600, marginBottom: 8 }}>Versions</div>
                      {info.versions?.map((v: string) => <Tag key={v}>{v}</Tag>)}
                    </Col>
                    <Col span={8}>
                      <div style={{ fontWeight: 600, marginBottom: 8 }}>Transport Modes</div>
                      {info.transports?.map((t: string) => <Tag key={t} color="blue">{t}</Tag>)}
                    </Col>
                    <Col span={8}>
                      <div style={{ fontWeight: 600, marginBottom: 8 }}>Features</div>
                      {info.features?.map((f: string) => <div key={f} style={{ fontSize: 12, marginBottom: 2 }}>✓ {f}</div>)}
                    </Col>
                  </Row>
                </Card>
              ))}
            </div>
          ),
        },
      ]} />

      {/* Add Hypervisor Modal */}
      <GlassModal open={addConnOpen} onCancel={() => setAddConnOpen(false)} title="Add Hypervisor Connection" width={600}
        footer={<Space><Button onClick={() => setAddConnOpen(false)}>Cancel</Button><Button type="primary" onClick={handleAddConnection}>Connect</Button></Space>}>
        <Form form={form} layout="vertical">
          <Form.Item name="name" label="Name" rules={[{ required: true }]}><Input placeholder="My vCenter" /></Form.Item>
          <Form.Item name="type" label="Type" rules={[{ required: true }]}>
            <Select options={[
              { value: 'vmware', label: 'VMware vSphere/ESXi' },
              { value: 'hyperv', label: 'Microsoft Hyper-V' },
              { value: 'proxmox', label: 'Proxmox VE' },
            ]} />
          </Form.Item>
          <Form.Item name="host" label="Host" rules={[{ required: true }]}><Input placeholder="vcenter.example.com" /></Form.Item>
          <Form.Item name="port" label="Port"><InputNumber defaultValue={443} style={{ width: '100%' }} /></Form.Item>
          <Form.Item name="username" label="Username" rules={[{ required: true }]}><Input placeholder="administrator@vsphere.local" /></Form.Item>
          <Form.Item name="password" label="Password" rules={[{ required: true }]}><Input.Password /></Form.Item>
          <Form.Item name="verify_ssl" label="Verify SSL" valuePropName="checked"><Switch defaultChecked /></Form.Item>
        </Form>
      </GlassModal>

      {/* VM Detail Modal */}
      <GlassModal open={detailOpen} onCancel={() => { setDetailOpen(false); setSelectedVM(null); }}
        title={selectedVM?.name || 'VM Details'} width={800}>
        {selectedVM && (
          <Tabs items={[
            {
              key: 'info', label: 'Info',
              children: (
                <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 12 }}>
                  <div><strong>VM ID:</strong> {selectedVM.vm_id}</div>
                  <div><strong>Type:</strong> <Tag color={hypervisorColors[selectedVM.type]}>{selectedVM.type.toUpperCase()}</Tag></div>
                  <div><strong>Host:</strong> {selectedVM.host}</div>
                  <div><strong>State:</strong> <Badge status={selectedVM.power_state === 'on' ? 'success' : 'default'} text={selectedVM.power_state.toUpperCase()} /></div>
                  <div><strong>CPU:</strong> {selectedVM.cpu} vCPU</div>
                  <div><strong>RAM:</strong> {selectedVM.memory_mb} MB</div>
                  <div><strong>Disk:</strong> {selectedVM.disk_gb} GB</div>
                  <div><strong>OS:</strong> {selectedVM.os}</div>
                  <div><strong>IP:</strong> {selectedVM.ip}</div>
                  <div><strong>Tools:</strong> {selectedVM.tools ? '✓ Installed' : '✗ Not installed'}</div>
                </div>
              ),
            },
            {
              key: 'actions', label: 'Actions',
              children: (
                <Space wrap>
                  <Button type="primary" icon={<SyncOutlined />} onClick={() => handleBackup(selectedVM.vm_id)}>Backup Now</Button>
                  <Button icon={<CameraOutlined />} onClick={() => handleSnapshot(selectedVM.vm_id)}>Take Snapshot</Button>
                  {selectedVM.power_state === 'on' ? (
                    <Button danger icon={<PauseCircleOutlined />} onClick={() => handlePowerAction(selectedVM.vm_id, 'off')}>Power Off</Button>
                  ) : (
                    <Button type="primary" icon={<PlayCircleOutlined />} onClick={() => handlePowerAction(selectedVM.vm_id, 'on')}>Power On</Button>
                  )}
                  <Button icon={<ExportOutlined />}>Export</Button>
                </Space>
              ),
            },
          ]} />
        )}
      </GlassModal>
    </motion.div>
  );
}
