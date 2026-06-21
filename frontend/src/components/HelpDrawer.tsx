import { Drawer, Typography, Space, Tag, Divider, Button } from 'antd';
import {
  DashboardOutlined, CloudServerOutlined, SafetyCertificateOutlined,
  FileSearchOutlined, DatabaseOutlined, SyncOutlined, HddOutlined,
  CloudServerOutlined as VirtIcon, MailOutlined, TeamOutlined,
  HistoryOutlined, CloudDownloadOutlined, LineChartOutlined,
  AuditOutlined, ApiOutlined, UserOutlined, DownloadOutlined, SaveOutlined,
} from '@ant-design/icons';
import { useLanguageStore } from '../stores/languageStore';

const { Title, Paragraph, Text } = Typography;

interface HelpDrawerProps {
  open: boolean;
  onClose: () => void;
  section: string;
}

const sectionMeta: Record<string, { icon: React.ReactNode; color: string }> = {
  dashboard: { icon: <DashboardOutlined />, color: '#00E5FF' },
  agents: { icon: <CloudServerOutlined />, color: '#00FF88' },
  policies: { icon: <SafetyCertificateOutlined />, color: '#FFA502' },
  jobs: { icon: <FileSearchOutlined />, color: '#A855F7' },
  storages: { icon: <DatabaseOutlined />, color: '#00E5FF' },
  storageTiers: { icon: <SaveOutlined />, color: '#FF6B6B' },
  replication: { icon: <SyncOutlined />, color: '#00FF88' },
  tape: { icon: <HddOutlined />, color: '#FFA502' },
  virtualization: { icon: <VirtIcon />, color: '#00E5FF' },
  databases: { icon: <DatabaseOutlined />, color: '#336791' },
  mail: { icon: <MailOutlined />, color: '#0078D4' },
  osBackup: { icon: <HddOutlined />, color: '#00FF88' },
  directory: { icon: <TeamOutlined />, color: '#A855F7' },
  gfs: { icon: <HistoryOutlined />, color: '#FF4757' },
  rescue: { icon: <CloudDownloadOutlined />, color: '#00E5FF' },
  traffic: { icon: <LineChartOutlined />, color: '#FFA502' },
  audit: { icon: <AuditOutlined />, color: '#8B949E' },
  zabbix: { icon: <ApiOutlined />, color: '#E65100' },
  users: { icon: <UserOutlined />, color: '#00E5FF' },
  agentsDownload: { icon: <DownloadOutlined />, color: '#00FF88' },
};

const sectionInstructions: Record<string, string[]> = {
  dashboard: [
    'Use widgets to monitor backup health in real-time',
    'All widgets auto-update via WebSocket',
    'Drag and resize widgets to customize layout',
    'Green indicators = healthy, Yellow = warning, Red = critical',
  ],
  agents: [
    'Install agent on each server you want to protect',
    'Click "Details" to see agent metrics and CDP sessions',
    'Agent must be online for backups to work',
    'Use "Upgrade" to push new agent version',
  ],
  policies: [
    'Create a policy to define backup rules (sources, schedule, storage)',
    'Set cron expression for scheduling (e.g., "0 2 * * *" = daily at 2 AM)',
    'Configure CDP for continuous protection with sub-second RPO',
    'Use "Run Now" to trigger immediate backup',
    'Bandwidth limiting prevents backup from saturating network',
  ],
  jobs: [
    'View all backup/restore job history',
    'Filter by status, agent, policy, or date range',
    'Click a job to see detailed logs and traffic graph',
    'Cancel running jobs if needed',
  ],
  storages: [
    'Add storage destinations (Local, NFS, S3)',
    'Always test connection after adding',
    'Fast Clone enables instant VM recovery without copying data',
    'Monitor usage to plan capacity',
  ],
  storageTiers: [
    'Hot tier: fast SSD for recent backups (< 7 days)',
    'Warm tier: HDD for weekly backups (7-30 days)',
    'Cold tier: archive storage for monthly backups (30-365 days)',
    'Archive tier: tape or cold storage for yearly (> 365 days)',
    'Click "Run Tiering" to migrate data between tiers',
  ],
  replication: [
    'Replicate backups between sites for disaster recovery',
    'Follows 3-2-1 rule: 3 copies, 2 media types, 1 offsite',
    'Incremental replication sends only changed data',
    'Digest exchange skips already-known blocks',
  ],
  tape: [
    'Manage LTO tape libraries for offline/air-gapped storage',
    'Mount tapes before writing, unmount when done',
    'Run inventory to scan barcodes and update tape list',
    'Tape is ideal for long-term archival and compliance',
  ],
  virtualization: [
    'Add your hypervisor connection (VMware, Hyper-V, Proxmox)',
    'No agent needed inside VMs (agentless backup)',
    'Click a VM to backup, snapshot, or power on/off',
    'Supported: VMware vSphere, Hyper-V, Proxmox, KVM, OpenStack + 12 platforms',
  ],
  databases: [
    'Connect to your database server',
    'Hot backup: no downtime needed for most databases',
    'PostgreSQL: supports pg_dump, pg_basebackup, WAL archiving, Patroni',
    'MySQL: supports mysqldump, XtraBackup, binary log',
    'SQL Server: supports VSS, transaction log, Always On',
    'Oracle: supports RMAN, Data Pump, block change tracking',
  ],
  mail: [
    'Connect to Exchange, CommuniGate Pro, or VK WorkSpace',
    'Backup individual mailboxes or all at once',
    'Restore single emails or entire mailboxes',
    'Supports public folders and distribution groups',
  ],
  osBackup: [
    'File-level backup for Linux and Windows',
    'Linux: ext4, XFS, ZFS, BTRFS with native snapshots',
    'Windows: NTFS, ReFS with VSS and block cloning',
    'LVM/ZFS snapshots for consistent backups without downtime',
  ],
  directory: [
    'Backup Active Directory, FreeIPA, or ALD Pro',
    'Includes: users, groups, GPO, DNS, certificates',
    'AD: system state, NTDS.dit, SYSVOL',
    'Restore entire directory or individual objects',
  ],
  gfs: [
    'Grandfather-Father-Son: automatic retention policy',
    'Daily: keep last 7 daily backups',
    'Weekly: keep last 4 Sunday backups',
    'Monthly: keep last 12 first-of-month backups',
    'Yearly: keep last 1 January backups',
    'Older backups are automatically deleted',
  ],
  rescue: [
    'Create bootable ISO for bare-metal recovery',
    'Boot from USB/CD when OS won\'t start',
    'Includes network tools and filesystem utilities',
    'Use for disaster recovery scenarios',
  ],
  traffic: [
    'Monitor backup traffic optimization metrics',
    'Cache Hit Ratio: % of data found in global cache (higher = better)',
    'Compression Ratio: how much data was compressed',
    'Zero Blocks: empty blocks skipped during transfer',
  ],
  audit: [
    'Complete log of all user actions',
    'Filter by user, action type, resource, date range',
    'Export to CSV for compliance reporting',
    'Cannot be modified or deleted (tamper-proof)',
  ],
  zabbix: [
    'Export OBS metrics to Zabbix monitoring system',
    'Configure Zabbix server URL and API token',
    'Metrics: agent status, job results, CDP lag, cache hit',
    'Test connection before enabling',
  ],
  users: [
    'Admin: full access to all features',
    'Operator: can manage backups, agents, policies',
    'Viewer: read-only access to dashboards and reports',
    'Change your own password or reset others\'',
  ],
  agentsDownload: [
    'Download agent binary for your platform',
    'Linux: install as systemd service',
    'Windows: install as Windows Service',
    'Agent communicates with server via REST API or Unix socket',
    'Install script available for automated deployment',
  ],
};

export default function HelpDrawer({ open, onClose, section }: HelpDrawerProps) {
  const { t } = useLanguageStore();
  const meta = sectionMeta[section] || { icon: null, color: '#00E5FF' };
  const instructions = sectionInstructions[section] || [];
  const titleKey = `nav.${section}`;
  const descKey = `${section}.desc`;

  return (
    <Drawer
      open={open}
      onClose={onClose}
      title={
        <Space>
          <span style={{ color: meta.color, fontSize: 20 }}>{meta.icon}</span>
          <span>{t(titleKey)}</span>
        </Space>
      }
      width={480}
      styles={{ body: { padding: '24px' } }}
    >
      <div style={{ marginBottom: 16 }}>
        <Tag color={meta.color} style={{ marginBottom: 8 }}>{section.toUpperCase()}</Tag>
        <Paragraph style={{ fontSize: 15, color: '#E6E6E6', lineHeight: 1.7 }}>
          {t(descKey)}
        </Paragraph>
      </div>

      <Divider style={{ borderColor: 'rgba(255,255,255,0.06)' }} />

      <Title level={5} style={{ color: '#E6E6E6', marginBottom: 12 }}>
        {t('lang') === 'ru' ? 'Как пользоваться' : 'How to use'}
      </Title>

      <Space direction="vertical" style={{ width: '100%' }}>
        {instructions.map((instruction, i) => (
          <div key={i} style={{ display: 'flex', gap: 10, alignItems: 'flex-start' }}>
            <Tag color={meta.color} style={{ minWidth: 24, textAlign: 'center', marginTop: 2 }}>{i + 1}</Tag>
            <Text style={{ color: '#E6E6E6', lineHeight: 1.6 }}>{instruction}</Text>
          </div>
        ))}
      </Space>

      <Divider style={{ borderColor: 'rgba(255,255,255,0.06)' }} />

      <Button type="link" onClick={onClose} style={{ padding: 0, color: '#00E5FF' }}>
        {t('lang') === 'ru' ? 'Закрыть' : 'Close'}
      </Button>
    </Drawer>
  );
}
