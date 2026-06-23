import { useEffect, useState, useCallback } from 'react';
import { Table, Card, Button, Space, Tag, Form, Input, Select, Switch, message, Row, Col } from 'antd';
import { FolderOutlined, LockOutlined, ReloadOutlined } from '@ant-design/icons';
import { motion } from 'framer-motion';
import api from '../utils/api';

export default function OsBackup() {
  const [fsData, setFsData] = useState<any>(null);
  const [loading, setLoading] = useState(false);

  const fetchData = useCallback(async () => {
    setLoading(true);
    try {
      const resp = await api.get('/os-backup/supported-filesystems');
      setFsData(resp.data);
    } catch {
      message.error('Failed to load filesystem data');
    }
    setLoading(false);
  }, []);

  useEffect(() => { fetchData(); }, []);

  const columns = [
    { title: 'Filesystem', dataIndex: 'fs', key: 'fs', render: (t: string) => <strong>{t.toUpperCase()}</strong> },
    { title: 'Snapshots', dataIndex: 'snapshots', key: 'snap', render: (v: boolean) => v ? <Tag color="green">Yes</Tag> : <Tag>No</Tag> },
    { title: 'Reflink', dataIndex: 'reflink', key: 'reflink', render: (v: boolean) => v ? <Tag color="blue">Yes</Tag> : <Tag>No</Tag> },
    { title: 'CoW', dataIndex: 'cow', key: 'cow', render: (v: boolean) => v ? <Tag color="cyan">Yes</Tag> : <Tag>No</Tag> },
    { title: 'VSS', dataIndex: 'vss', key: 'vss', render: (v: boolean) => v ? <Tag color="green">Yes</Tag> : <Tag>-</Tag> },
    { title: 'Block Clone', dataIndex: 'block_clone', key: 'bc', render: (v: boolean) => v ? <Tag color="purple">Yes</Tag> : <Tag>-</Tag> },
  ];

  return (
    <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.3 }}>
      <Row gutter={[16, 16]}>
        <Col xs={24} lg={12}>
          <Card title={<><FolderOutlined /> Linux Filesystems</>} size="small">
            <Table dataSource={fsData?.linux || []} columns={columns} rowKey="fs" loading={loading} pagination={false} size="small" />
          </Card>
        </Col>
        <Col xs={24} lg={12}>
          <Card title={<><LockOutlined /> Windows Filesystems</>} size="small">
            <Table dataSource={fsData?.windows || []} columns={columns} rowKey="fs" loading={loading} pagination={false} size="small" />
          </Card>
        </Col>
      </Row>

      <Card title="Quick Backup" style={{ marginTop: 16 }} size="small">
        <Form layout="inline">
          <Form.Item label="Source"><Input placeholder="/path/to/backup" style={{ width: 300 }} /></Form.Item>
          <Form.Item label="Output"><Input placeholder="/backup/output" style={{ width: 300 }} /></Form.Item>
          <Form.Item label="Compress"><Switch defaultChecked /></Form.Item>
          <Form.Item label="Encrypt"><Switch /></Form.Item>
          <Form.Item><Button type="primary">Start Backup</Button></Form.Item>
        </Form>
      </Card>
    </motion.div>
  );
}
