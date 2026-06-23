import React, { useEffect, useState } from 'react';
import { Drawer, Form, Input, InputNumber, Select, Switch, Button, Space, Typography, Card, Row, Col, message } from 'antd';
import { SaveOutlined, ReloadOutlined } from '@ant-design/icons';
import api from '../utils/api';

const { Text, Title } = Typography;

interface Props {
  open: boolean;
  agentId?: number;
  agentName?: string;
  onClose: () => void;
}

interface ConfigField {
  key: string;
  value: any;
  type: string;
  description: string;
  readonly: boolean;
  options?: number[];
}

export default function AgentConfigEditor({ open, agentId, agentName, onClose }: Props) {
  const [fields, setFields] = useState<ConfigField[]>([]);
  const [loading, setLoading] = useState(false);
  const [saving, setSaving] = useState(false);
  const [form] = Form.useForm();

  const fetchConfig = async () => {
    if (!agentId) return;
    setLoading(true);
    try {
      const [schemaRes, configRes] = await Promise.all([
        api.get(`/config-editor/schema/${agentId}`),
        api.get(`/config-editor/${agentId}`),
      ]);
      setFields(schemaRes.data.schema || []);
      form.setFieldsValue(configRes.data);
    } catch {
      message.error('Failed to load configuration');
    }
    setLoading(false);
  };

  useEffect(() => {
    if (open && agentId) fetchConfig();
  }, [open, agentId]);

  const handleSave = async () => {
    try {
      const values = await form.validateFields();
      setSaving(true);
      await api.put(`/config-editor/${agentId}`, values);
      message.success('Configuration saved. Agent will apply changes within 30 seconds.');
      onClose();
    } catch {
      message.error('Failed to save configuration');
    }
    setSaving(false);
  };

  const handleApplyNow = async () => {
    try {
      await api.post(`/agents/${agentId}/command/apply-config`);
      message.success('Agent restarting to apply configuration');
    } catch {
      message.error('Failed to apply configuration');
    }
  };

  const renderField = (field: ConfigField) => {
    const disabled = field.readonly;
    switch (field.type) {
      case 'select':
        return (
          <Form.Item key={field.key} name={field.key} label={field.description}
            extra={disabled ? 'Read-only' : undefined}>
            <Select disabled={disabled} options={field.options?.map(o => ({ value: o, label: String(o) }))} />
          </Form.Item>
        );
      case 'number':
        return (
          <Form.Item key={field.key} name={field.key} label={field.description}
            extra={disabled ? 'Read-only' : undefined}>
            <InputNumber disabled={disabled} style={{ width: '100%' }} />
          </Form.Item>
        );
      default:
        return (
          <Form.Item key={field.key} name={field.key} label={field.description}
            extra={disabled ? 'Read-only' : undefined}>
            <Input disabled={disabled} />
          </Form.Item>
        );
    }
  };

  return (
    <Drawer
      title={`Configuration — ${agentName || 'Agent'}`}
      open={open}
      onClose={onClose}
      width={500}
      extra={
        <Space>
          <Button icon={<ReloadOutlined />} onClick={fetchConfig} loading={loading}>Reload</Button>
          <Button icon={<ReloadOutlined />} onClick={handleApplyNow}>Apply Now</Button>
          <Button type="primary" icon={<SaveOutlined />} onClick={handleSave} loading={saving}>Save</Button>
        </Space>
      }
    >
      <Form form={form} layout="vertical">
        {fields.map(renderField)}
      </Form>
    </Drawer>
  );
}
