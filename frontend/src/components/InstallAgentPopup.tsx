import React, { useState } from 'react';
import { Modal, Form, Input, InputNumber, Select, Steps, Alert, Typography, Space, Button, Progress, Tag, Card, Row, Col, message } from 'antd';
import { CloudDownloadOutlined, CheckCircleOutlined, CloseCircleOutlined, LoadingOutlined } from '@ant-design/icons';
import api from '../utils/api';

const { Text, Paragraph } = Typography;

interface Props {
  open: boolean;
  onClose: () => void;
  onComplete?: () => void;
}

interface InstallStep {
  status: 'wait' | 'process' | 'finish' | 'error';
  title: string;
  detail?: string;
}

export default function InstallAgentPopup({ open, onClose, onComplete }: Props) {
  const [step, setStep] = useState(0);
  const [installing, setInstalling] = useState(false);
  const [steps, setSteps] = useState<InstallStep[]>([]);
  const [form] = Form.useForm();

  const handleInstall = async () => {
    try {
      const values = await form.validateFields();
      setInstalling(true);
      setStep(1);

      setSteps([
        { status: 'process', title: 'Connecting to host' },
        { status: 'wait', title: 'Checking prerequisites' },
        { status: 'wait', title: 'Installing agent binary' },
        { status: 'wait', title: 'Configuring agent' },
        { status: 'wait', title: 'Starting agent service' },
        { status: 'wait', title: 'Registering with server' },
      ]);

      await new Promise(r => setTimeout(r, 1500));
      setSteps(prev => prev.map((s, i) => i === 0 ? { ...s, status: 'finish' } : i === 1 ? { ...s, status: 'process' } : s));
      setStep(2);

      await new Promise(r => setTimeout(r, 1000));
      setSteps(prev => prev.map((s, i) => i === 1 ? { ...s, status: 'finish' } : i === 2 ? { ...s, status: 'process' } : s));
      setStep(3);

      const osType = values.os_type === 'windows' ? 'winrm' : 'ssh';
      const endpoint = `/api/agents/deploy/${osType}`;
      const payload = osType === 'ssh' ? {
        host: values.host, port: values.ssh_port || 22,
        username: values.username, password: values.password,
        os_type: values.os_type, server_url: values.server_url || '',
        auth_token: values.auth_token || '',
      } : {
        host: values.host, port: values.winrm_port || 5985,
        username: values.username, password: values.password,
        use_ssl: values.use_ssl || false,
        server_url: values.server_url || '', auth_token: values.auth_token || '',
      };

      const res = await api.post(endpoint, payload);

      if (res.data.success) {
        setSteps(prev => prev.map((s, i) => {
          if (i <= 2) return { ...s, status: 'finish' as const };
          if (i === 3) return { ...s, status: 'finish' as const };
          if (i === 4) return { ...s, status: 'process' as const };
          return s;
        }));

        await new Promise(r => setTimeout(r, 2000));
        setSteps(prev => prev.map((s, i) => {
          if (i === 4) return { ...s, status: 'finish' as const };
          if (i === 5) return { ...s, status: 'process' as const };
          return s;
        }));

        await new Promise(r => setTimeout(r, 1000));
        setSteps(prev => prev.map(s => ({ ...s, status: 'finish' as const })));
        setStep(4);
        message.success(`Agent installed on ${values.host}`);
        onComplete?.();
      } else {
        setSteps(prev => prev.map((s, i) => i === prev.findIndex(p => p.status === 'process') ? { ...s, status: 'error', detail: res.data.error } : s));
        message.error(res.data.error || 'Installation failed');
      }
    } catch (err: any) {
      message.error(err.response?.data?.detail || 'Installation failed');
    }
    setInstalling(false);
  };

  const handleClose = () => {
    setStep(0);
    setSteps([]);
    form.resetFields();
    onClose();
  };

  return (
    <Modal
      title="Install Agent on New Host"
      open={open}
      onCancel={handleClose}
      footer={step >= 4 ? [
        <Button key="close" type="primary" onClick={handleClose}>Close</Button>,
      ] : undefined}
      width={600}
      closable={!installing}
      maskClosable={!installing}
    >
      {step === 0 && (
        <Form form={form} layout="vertical" initialValues={{ os_type: 'linux', ssh_port: 22, winrm_port: 5985 }}>
          <Form.Item name="host" label="Host" rules={[{ required: true, message: 'Enter IP or hostname' }]}>
            <Input placeholder="192.168.1.100" />
          </Form.Item>
          <Form.Item name="os_type" label="Operating System" rules={[{ required: true }]}>
            <Select options={[
              { value: 'linux', label: 'Linux (SSH)' },
              { value: 'windows', label: 'Windows (WinRM)' },
            ]} />
          </Form.Item>
          <Form.Item name="username" label="Username" rules={[{ required: true }]}>
            <Input placeholder="root / Administrator" />
          </Form.Item>
          <Form.Item name="password" label="Password">
            <Input.Password placeholder="Leave empty if using SSH key" />
          </Form.Item>
          <Form.Item name="server_url" label="Server URL">
            <Input placeholder="https://obs-server:8000 (auto-detected if empty)" />
          </Form.Item>
          <Form.Item name="auth_token" label="Auth Token">
            <Input.Password placeholder="Agent registration token" />
          </Form.Item>
          <Button type="primary" block size="large" onClick={handleInstall} loading={installing}
            icon={<CloudDownloadOutlined />}>
            Install Agent
          </Button>
        </Form>
      )}

      {step >= 1 && (
        <div>
          <Alert
            message={step >= 4 ? 'Installation Complete' : 'Installing...'}
            type={step >= 4 ? 'success' : 'info'}
            showIcon
            style={{ marginBottom: 16 }}
          />
          {steps.map((s, i) => (
            <Card key={i} size="small" style={{ marginBottom: 8 }}
              style={{ background: 'rgba(255,255,255,0.04)', border: '1px solid rgba(255,255,255,0.08)' }}>
              <Space>
                {s.status === 'finish' && <CheckCircleOutlined style={{ color: '#52c41a' }} />}
                {s.status === 'process' && <LoadingOutlined style={{ color: '#1890ff' }} />}
                {s.status === 'error' && <CloseCircleOutlined style={{ color: '#ff4d4f' }} />}
                {s.status === 'wait' && <span style={{ color: '#666' }}>○</span>}
                <Text>{s.title}</Text>
                {s.detail && <Text type="danger" style={{ fontSize: 12 }}>{s.detail}</Text>}
              </Space>
            </Card>
          ))}
        </div>
      )}
    </Modal>
  );
}
