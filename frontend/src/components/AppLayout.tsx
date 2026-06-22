import React, { useState } from 'react';
import { useNavigate, useLocation } from 'react-router-dom';
import { Layout, Menu, Avatar, Dropdown, Badge, Typography, Space, Button, Tooltip } from 'antd';
import {
  DashboardOutlined, CloudServerOutlined, SafetyCertificateOutlined,
  FileSearchOutlined, DatabaseOutlined, ApiOutlined, LineChartOutlined,
  AuditOutlined, SyncOutlined, CloudDownloadOutlined, SaveOutlined,
  HddOutlined, BellOutlined, UserOutlined, LogoutOutlined, MailOutlined,
  TeamOutlined, HistoryOutlined, DownloadOutlined, SafetyOutlined,
  ThunderboltOutlined, MenuFoldOutlined, MenuUnfoldOutlined,
  QuestionCircleOutlined, GlobalOutlined, ClearOutlined,
  DesktopOutlined, SettingOutlined, PartitionOutlined,
  ToolOutlined, ApiOutlined as ApiIcon, SafetyCertificateOutlined as ShieldIcon,
} from '@ant-design/icons';
import { useAuthStore } from '../stores/authStore';
import { useNotificationStore } from '../stores/notificationStore';
import { useLanguageStore } from '../stores/languageStore';
import HelpDrawer from './HelpDrawer';

const { Header, Sider, Content } = Layout;
const { Text } = Typography;

interface Props {
  children: React.ReactNode;
}

export default function AppLayout({ children }: Props) {
  const [collapsed, setCollapsed] = useState(false);
  const [helpOpen, setHelpOpen] = useState(false);
  const navigate = useNavigate();
  const location = useLocation();
  const logout = useAuthStore((s) => s.logout);
  const unreadCount = useNotificationStore((s) => s.unreadCount);
  const { lang, setLang } = useLanguageStore();

  const currentSection = location.pathname.replace('/', '') || 'dashboard';

  const menuItems = [
    {
      key: 'overview',
      type: 'group' as const,
      label: collapsed ? '' : 'OVERVIEW',
      children: [
        { key: '/dashboard', icon: <DashboardOutlined />, label: 'Dashboard' },
      ],
    },
    {
      key: 'agents-section',
      type: 'group' as const,
      label: collapsed ? '' : 'AGENTS',
      children: [
        { key: '/agents', icon: <CloudServerOutlined />, label: 'Agent Management' },
        { key: '/agents-download', icon: <DownloadOutlined />, label: 'Agent Deployment' },
        { key: '/dirty-buffers', icon: <ClearOutlined />, label: 'Dirty Buffers' },
      ],
    },
    {
      key: 'backup-section',
      type: 'group' as const,
      label: collapsed ? '' : 'BACKUP & POLICIES',
      children: [
        { key: '/policies', icon: <SafetyCertificateOutlined />, label: 'Backup Policies' },
        { key: '/jobs', icon: <FileSearchOutlined />, label: 'Backup Jobs' },
        { key: '/backup-copy', icon: <SyncOutlined />, label: 'Backup Copy' },
        { key: '/gfs', icon: <HistoryOutlined />, label: 'GFS Retention' },
      ],
    },
    {
      key: 'storage-section',
      type: 'group' as const,
      label: collapsed ? '' : 'STORAGE',
      children: [
        { key: '/storages', icon: <DatabaseOutlined />, label: 'Storage Targets' },
        { key: '/storage-tiers', icon: <SaveOutlined />, label: 'Storage Tiers' },
        { key: '/tape', icon: <HddOutlined />, label: 'Tape Library' },
      ],
    },
    {
      key: 'sources-section',
      type: 'group' as const,
      label: collapsed ? '' : 'DATA SOURCES',
      children: [
        { key: '/virtualization', icon: <CloudServerOutlined />, label: 'Virtualization' },
        { key: '/databases', icon: <DatabaseOutlined />, label: 'Databases' },
        { key: '/mail', icon: <MailOutlined />, label: 'Mail Systems' },
        { key: '/os-backup', icon: <DesktopOutlined />, label: 'OS / Filesystem' },
        { key: '/directory', icon: <TeamOutlined />, label: 'Directory Services' },
      ],
    },
    {
      key: 'replication-section',
      type: 'group' as const,
      label: collapsed ? '' : 'REPLICATION & DR',
      children: [
        { key: '/replication', icon: <SyncOutlined />, label: 'Storage Replication' },
        { key: '/vm-replication', icon: <SyncOutlined />, label: 'VM Replication' },
        { key: '/disaster-recovery', icon: <ThunderboltOutlined />, label: 'DR Plans' },
        { key: '/rescue', icon: <CloudDownloadOutlined />, label: 'Rescue Media' },
      ],
    },
    {
      key: 'security-section',
      type: 'group' as const,
      label: collapsed ? '' : 'SECURITY',
      children: [
        { key: '/surebackup', icon: <SafetyCertificateOutlined />, label: 'SureBackup' },
        { key: '/malware', icon: <SafetyOutlined />, label: 'Malware Detection' },
      ],
    },
    {
      key: 'monitoring-section',
      type: 'group' as const,
      label: collapsed ? '' : 'MONITORING & AUDIT',
      children: [
        { key: '/traffic', icon: <LineChartOutlined />, label: 'Traffic Analytics' },
        { key: '/zabbix', icon: <ApiOutlined />, label: 'Zabbix Integration' },
        { key: '/audit', icon: <AuditOutlined />, label: 'Audit Log' },
      ],
    },
    {
      key: 'admin-section',
      type: 'group' as const,
      label: collapsed ? '' : 'ADMINISTRATION',
      children: [
        { key: '/users', icon: <UserOutlined />, label: 'Users' },
      ],
    },
  ];

  const userMenuItems = [
    { key: 'logout', icon: <LogoutOutlined />, label: 'Logout', onClick: () => { logout(); navigate('/login'); } },
  ];

  const langMenuItems = [
    { key: 'ru', label: '🇷🇺 Русский' },
    { key: 'en', label: '🇬🇧 English' },
  ];

  return (
    <Layout style={{ minHeight: '100vh' }}>
      <Sider
        trigger={null}
        collapsible
        collapsed={collapsed}
        width={240}
        style={{
          background: 'rgba(13,17,23,0.95)',
          borderRight: '1px solid rgba(255,255,255,0.06)',
          overflow: 'auto',
        }}
      >
        <div style={{ padding: '16px', textAlign: 'center' }}>
          <span
            className="neon-text"
            style={{
              fontSize: collapsed ? 16 : 20,
              fontFamily: 'monospace',
              fontWeight: 700,
            }}
          >
            {collapsed ? 'OB' : 'OBS Backup'}
          </span>
        </div>
        <Menu
          theme="dark"
          mode="inline"
          selectedKeys={[location.pathname]}
          defaultOpenKeys={[
            'overview', 'agents-section', 'backup-section', 'storage-section',
            'sources-section', 'replication-section', 'security-section',
            'monitoring-section', 'admin-section',
          ]}
          items={menuItems}
          onClick={({ key }) => navigate(key)}
          style={{
            background: 'transparent',
            borderRight: 'none',
          }}
        />
      </Sider>
      <Layout>
        <Header
          style={{
            padding: '0 24px',
            background: 'rgba(13,17,23,0.95)',
            borderBottom: '1px solid rgba(255,255,255,0.06)',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'space-between',
          }}
        >
          <Space>
            {React.createElement(collapsed ? MenuUnfoldOutlined : MenuFoldOutlined, {
              onClick: () => setCollapsed(!collapsed),
              style: { fontSize: 18, cursor: 'pointer', color: '#E6E6E6' },
            })}
          </Space>
          <Space size="middle">
            <Tooltip title={lang === 'ru' ? 'Помощь по разделу' : 'Help for this section'}>
              <QuestionCircleOutlined
                style={{ fontSize: 18, cursor: 'pointer', color: '#00E5FF' }}
                onClick={() => setHelpOpen(true)}
              />
            </Tooltip>

            <Dropdown
              menu={{
                items: langMenuItems,
                onClick: ({ key }) => setLang(key as 'en' | 'ru'),
              }}
              placement="bottomRight"
            >
              <Button
                size="small"
                icon={<GlobalOutlined />}
                style={{ borderColor: 'rgba(255,255,255,0.15)', color: '#E6E6E6' }}
              >
                {lang.toUpperCase()}
              </Button>
            </Dropdown>

            <Badge count={unreadCount} size="small">
              <BellOutlined style={{ fontSize: 18, cursor: 'pointer', color: '#E6E6E6' }} />
            </Badge>
            <Dropdown menu={{ items: userMenuItems }} placement="bottomRight">
              <Avatar
                icon={<UserOutlined />}
                style={{ backgroundColor: '#00E5FF', cursor: 'pointer' }}
              />
            </Dropdown>
          </Space>
        </Header>
        <Content
          style={{
            margin: '24px',
            padding: '24px',
            background: 'rgba(13,17,23,0.5)',
            minHeight: 280,
          }}
        >
          {children}
        </Content>
      </Layout>

      <HelpDrawer
        open={helpOpen}
        onClose={() => setHelpOpen(false)}
        section={currentSection}
      />
    </Layout>
  );
}
