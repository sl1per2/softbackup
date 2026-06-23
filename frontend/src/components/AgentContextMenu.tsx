import React, { useState } from 'react';
import { Dropdown, Menu, message, Modal } from 'antd';
import {
  PlayCircleOutlined, StopOutlined, ReloadOutlined, SettingOutlined,
  FileTextOutlined, ClearOutlined, ThunderboltOutlined,
  CloudDownloadOutlined, ExclamationCircleOutlined,
} from '@ant-design/icons';
import api from '../utils/api';

interface Props {
  agent: any;
  onAction: (action: string) => void;
}

export default function AgentContextMenu({ agent, onAction }: Props) {
  const [loading, setLoading] = useState<string | null>(null);

  const sendCommand = async (command: string, confirmMsg?: string) => {
    if (confirmMsg) {
      Modal.confirm({
        title: confirmMsg,
        icon: <ExclamationCircleOutlined />,
        okText: 'Yes', okType: 'danger', cancelText: 'Cancel',
        onOk: async () => {
          setLoading(command);
          try {
            await api.post(`/agents/${agent.id}/command/${command}`);
            message.success(`Command "${command}" sent`);
            onAction(command);
          } catch {
            message.error(`Failed to send "${command}"`);
          }
          setLoading(null);
        },
      });
    } else {
      setLoading(command);
      try {
        await api.post(`/agents/${agent.id}/command/${command}`);
        message.success(`Command "${command}" sent`);
        onAction(command);
      } catch {
        message.error(`Failed to send "${command}"`);
      }
      setLoading(null);
    }
  };

  const menuItems = [
    { key: 'backup', icon: <PlayCircleOutlined />, label: 'Quick Backup', onClick: () => onAction('backup') },
    { type: 'divider' as const },
    { key: 'stop', icon: <StopOutlined />, label: 'Stop All Jobs', onClick: () => sendCommand('stop-jobs', 'Stop all running jobs?') },
    { key: 'restart', icon: <ReloadOutlined />, label: 'Restart Agent', onClick: () => sendCommand('restart', 'Restart the agent?') },
    { key: 'update', icon: <CloudDownloadOutlined />, label: 'Update Agent', onClick: () => sendCommand('update', 'Check for updates?') },
    { type: 'divider' as const },
    { key: 'config', icon: <SettingOutlined />, label: 'Edit Configuration', onClick: () => onAction('config') },
    { key: 'logs', icon: <FileTextOutlined />, label: 'View Logs', onClick: () => onAction('logs') },
    { key: 'flush', icon: <ClearOutlined />, label: 'Flush Dirty Buffers', onClick: () => sendCommand('flush-dirty-buffers', 'Flush dirty buffers now?') },
    { key: 'clear-cache', icon: <ThunderboltOutlined />, label: 'Clear Chunk Cache', onClick: () => sendCommand('clear-cache', 'Clear dedup cache?') },
  ];

  return (
    <Dropdown menu={{ items: menuItems }} trigger={['contextMenu']}>
      <div onContextMenu={(e) => e.preventDefault()}>
        {agent?.hostname}
      </div>
    </Dropdown>
  );
}
