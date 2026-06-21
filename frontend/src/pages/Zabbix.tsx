import React, { useEffect, useState } from 'react';
import { Form, Input, InputNumber, Switch, Button, Card, Table, Tag, Typography, Space, message } from 'antd';
import { ApiOutlined, CheckCircleOutlined, CloseCircleOutlined, SyncOutlined } from '@ant-design/icons';
import { motion } from 'framer-motion';
import axios from 'axios';

const { Title, Text } = Typography;

export default function ZabbixPage() {
  const [config, setConfig] = useState<any>(null);
  const [triggers, setTriggers] = useState<any[]>([]);
  const [testResult, setTestResult] = useState<any>(null);
  const [testing, setTesting] = useState(false);
  const [form] = Form.useForm();

  useEffect(() => {
    const load = async () => {
      try {
        const [cfg, trig] = await Promise.all([
          axios.get('/api/zabbix/config'),
          axios.get('/api/zabbix/triggers'),
        ]);
        setConfig(cfg.data);
        form.setFieldsValue(cfg.data);
        setTriggers(trig.data?.triggers || []);
      } catch {}
    };
    load();
  }, []);

  const handleSave = async () => {
    try {
      const values = await form.validateFields();
      await axios.put('/api/zabbix/config', values);
      message.success('Zabbix config saved');
    } catch {}
  };

  const handleTest = async () => {
    setTesting(true);
    setTestResult(null);
    try {
      const res = await axios.post('/api/zabbix/test');
      setTestResult(res.data);
    } catch {
      setTestResult({ success: false, message: 'Connection failed' });
    }
    setTesting(false);
  };

  const triggerColumns = [
    { title: 'Problem', dataIndex: 'description', key: 'desc' },
    { title: 'Host', dataIndex: 'host', key: 'host' },
    {
      title: 'Severity',
      dataIndex: 'severity',
      key: 'severity',
      render: (s: number) => {
        const colors: Record<number, string> = { 1: 'default', 2: 'warning', 3: 'warning', 4: 'error', 5: 'error' };
        return <Tag color={colors[s]}>{['Not classified', 'Information', 'Warning', 'Average', 'High', 'Disaster'][s]}</Tag>;
      },
    },
    { title: 'Time', dataIndex: 'timestamp', key: 'time' },
  ];

  return (
    <div>
      <Title level={4} style={{ color: '#E6E6E6', marginBottom: 24 }}>Zabbix Integration</Title>
      <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.3 }}>
        <Card className="widget-card" style={{ marginBottom: 16 }}>
          <Form form={form} layout="vertical">
            <Form.Item name="server_url" label="Server URL">
              <Input placeholder="http://zabbix-server/api_jsonrpc.php" />
            </Form.Item>
            <Form.Item name="trapper_port" label="Trapper Port">
              <InputNumber min={1} max={65535} style={{ width: 200 }} />
            </Form.Item>
            <Form.Item name="api_token" label="API Token">
              <Input.Password placeholder="API Token" />
            </Form.Item>
            <Form.Item name="enabled" label="Enabled" valuePropName="checked">
              <Switch />
            </Form.Item>
            <Form.Item name="metrics_interval_seconds" label="Metrics Interval (seconds)">
              <InputNumber min={10} max={3600} />
            </Form.Item>
            <Space>
              <Button type="primary" onClick={handleSave}>Save</Button>
              <Button icon={<SyncOutlined />} loading={testing} onClick={handleTest}>Test Connection</Button>
            </Space>
          </Form>
          {testResult && (
            <div style={{ marginTop: 16, padding: 12, background: '#161B22', borderRadius: 8 }}>
              {testResult.success ? (
                <Space>
                  <CheckCircleOutlined style={{ color: '#00FF88' }} />
                  <Text style={{ color: '#00FF88' }}>Connection successful</Text>
                </Space>
              ) : (
                <Space>
                  <CloseCircleOutlined style={{ color: '#FF4757' }} />
                  <Text style={{ color: '#FF4757' }}>{testResult.message || 'Connection failed'}</Text>
                </Space>
              )}
            </div>
          )}
        </Card>

        <Card className="widget-card" title={<Text style={{ color: '#E6E6E6' }}>Latest Triggers</Text>}>
          <Table
            dataSource={triggers}
            columns={triggerColumns}
            rowKey="triggerid"
            pagination={false}
            size="small"
            locale={{ emptyText: 'No triggers found' }}
          />
        </Card>
      </motion.div>
    </div>
  );
}
