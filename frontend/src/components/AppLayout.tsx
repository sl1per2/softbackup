import React, { useState } from 'react';
import { useNavigate, useLocation } from 'react-router-dom';
import { Layout, Menu, Avatar, Dropdown, Badge, Typography, Space, Button, Tooltip } from 'antd';
import {
  DashboardOutlined, CloudServerOutlined, SafetyCertificateOutlined,
  FileSearchOutlined, DatabaseOutlined, ApiOutlined, LineChartOutlined,
  AuditOutlined, SyncOutlined, CloudDownloadOutlined, SaveOutlined,
  HddOutlined, BellOutlined, UserOutlined, LogoutOutlined, MailOutlined,
  TeamOutlined, HistoryOutlined, DownloadOutlined, SafetyOutlined,
  ThunderboltOutlined, MenuFoldOutlined,
  MenuUnfoldOutlined, QuestionCircleOutlined, GlobalOutlined,
  ClearOutlined,
} from '@ant-design/icons';
import { useAuthStore } from '../stores/authStore';
import { useNotificationStore } from '../stores/notificationStore';
import { useLanguageStore } from '../stores/languageStore';
import HelpDrawer from './HelpDrawer';

const { Header, Sider, Content } = Layout;

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
  const { lang, setLang, t } = useLanguageStore();

  const currentSection = location.pathname.replace('/', '') || 'dashboard';

  const menuItems = [
    { key: '/dashboard', icon: <DashboardOutlined />, label: t('nav.dashboard') },
    { key: '/agents', icon: <CloudServerOutlined />, label: t('nav.agents') },
    { key: '/policies', icon: <SafetyCertificateOutlined />, label: t('nav.policies') },
    { key: '/jobs', icon: <FileSearchOutlined />, label: t('nav.jobs') },
    { key: '/storages', icon: <DatabaseOutlined />, label: t('nav.storages') },
    { key: '/storage-tiers', icon: <SaveOutlined />, label: t('nav.storageTiers') },
    { key: '/replication', icon: <SyncOutlined />, label: t('nav.replication') },
    { key: '/tape', icon: <HddOutlined />, label: t('nav.tape') },
    { key: '/virtualization', icon: <CloudServerOutlined />, label: t('nav.virtualization') },
    { key: '/databases', icon: <DatabaseOutlined />, label: t('nav.databases') },
    { key: '/mail', icon: <MailOutlined />, label: t('nav.mail') },
    { key: '/os-backup', icon: <HddOutlined />, label: t('nav.osBackup') },
    { key: '/directory', icon: <TeamOutlined />, label: t('nav.directory') },
    { key: '/gfs', icon: <HistoryOutlined />, label: t('nav.gfs') },
    { key: '/rescue', icon: <CloudDownloadOutlined />, label: t('nav.rescue') },
    { key: '/traffic', icon: <LineChartOutlined />, label: t('nav.traffic') },
    { key: '/audit', icon: <AuditOutlined />, label: t('nav.audit') },
    { key: '/zabbix', icon: <ApiOutlined />, label: t('nav.zabbix') },
  { key: '/users', icon: <TeamOutlined />, label: t('nav.users') },
  { key: '/agents-download', icon: <DownloadOutlined />, label: t('nav.agentsDownload') },
  { key: '/surebackup', icon: <SafetyCertificateOutlined />, label: 'SureBackup' },
  { key: '/backup-copy', icon: <SyncOutlined />, label: 'Backup Copy' },
  { key: '/malware', icon: <SafetyOutlined />, label: 'Malware' },
  { key: '/disaster-recovery', icon: <ThunderboltOutlined />, label: 'DR Plans' },
  { key: '/vm-replication', icon: <SyncOutlined />, label: 'VM Replication' },
  { key: '/dirty-buffers', icon: <ClearOutlined />, label: 'Dirty Buffers' },
];

  const userMenuItems = [
    { key: 'logout', icon: <LogoutOutlined />, label: t('nav.users') === 'Пользователи' ? 'Выйти' : 'Logout', onClick: () => { logout(); navigate('/login'); } },
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
          items={menuItems}
          onClick={({ key }) => navigate(key)}
          style={{ background: 'transparent', borderRight: 'none' }}
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
