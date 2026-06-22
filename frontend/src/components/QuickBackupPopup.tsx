import React, { useState } from 'react';
import { Modal, Form, Select, Input, InputNumber, Button, Space, Typography, message } from 'antd';
import { PlayCircleOutlined } from '@ant-design/icons';
import api from '../utils/api';
import { useStorageOptions, usePolicyOptions } from '../hooks/useData';

const { Text } = Typography;

interface Props {
  open: boolean;
  agentId?: number;
  agentName?: string;
  onClose: () => void;
}

export default function QuickBackupPopup({ open, agentId, agentName, onClose }: Props) {
  const [loading, setLoading] = useState(false);
  const [form] = Form.useForm();
  const { options: storageOptions } = useStorageOptions();
  const { options: policyOptions } = usePolicyOptions();

  const handleBackup = async () => {
    try {
      const values = await form.validateFields();
      setLoading(true);
      await api.post(`/api/agents/${agentId}/command/start-backup`, {
        backup_type: values.backup_type,
        source_paths: values.source_paths?.split(',').map((s: string) => s.trim()).filter(Boolean),
        storage_id: values.storage_id,
        policy_id: values.policy_id,
      });
      message.success(`Backup started on ${agentName || 'agent'}`);
      form.resetFields();
      onClose();
    } catch (err: any) {
      message.error(err.response?.data?.detail || 'Failed to start backup');
    }
    setLoading(false);
  };

  return (
    <Modal
      title={`Quick Backup — ${agentName || 'Agent'}`}
      open={open}
      onCancel={onClose}
      footer={[
        <Button key="cancel" onClick={onClose}>Cancel</Button>,
        <Button key="start" type="primary" loading={loading} onClick={handleBackup}
          icon={<PlayCircleOutlined />}>
          Start Backup
        </Button>,
      ]}
      width={500}
    >
      <Form form={form} layout="vertical" initialValues={{ backup_type: 'full' }}>
        <Form.Item name="backup_type" label="Backup Type" rules={[{ required: true }]}>
          <Select options={[
            { value: 'full', label: 'Full Backup' },
            { value: 'incremental', label: 'Incremental Backup' },
            { value: 'differential', label: 'Differential Backup' },
          ]} />
        </Form.Item>
        <Form.Item name="source_paths" label="Source Paths (comma-separated)">
          <Input.TextArea rows={3} placeholder="/home, /var/data, C:\Users" />
        </Form.Item>
        <Form.Item name="storage_id" label="Storage Target">
          <Select placeholder="Select storage" allowClear options={storageOptions} />
        </Form.Item>
        <Form.Item name="policy_id" label="Apply Policy">
          <Select placeholder="Select policy" allowClear options={policyOptions} />
        </Form.Item>
      </Form>
    </Modal>
  );
}
