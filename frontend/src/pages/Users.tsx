import { useEffect, useState } from 'react';
import { Table, Card, Button, Space, Tag, Modal, Form, Input, Select, Switch, message, Popconfirm, Tabs } from 'antd';
import { UserOutlined, PlusOutlined, DeleteOutlined, EditOutlined, KeyOutlined, ReloadOutlined } from '@ant-design/icons';
import { motion } from 'framer-motion';
import GlassModal from '../components/GlassModal';
import api from '../utils/api';
import { formatDate } from '../utils/format';

interface User {
  id: number;
  username: string;
  email: string;
  role: string;
  is_active: boolean;
  created_at?: string;
  last_login?: string;
}

export default function Users() {
  const [users, setUsers] = useState<User[]>([]);
  const [loading, setLoading] = useState(false);
  const [modalOpen, setModalOpen] = useState(false);
  const [editingUser, setEditingUser] = useState<User | null>(null);
  const [changePasswordOpen, setChangePasswordOpen] = useState(false);
  const [changePasswordUser, setChangePasswordUser] = useState<User | null>(null);
  const [myPasswordOpen, setMyPasswordOpen] = useState(false);
  const [form] = Form.useForm();
  const [passwordForm] = Form.useForm();
  const [myPasswordForm] = Form.useForm();

  const fetchUsers = async () => {
    setLoading(true);
    try {
      const resp = await api.get('/admin/users');
      setUsers(resp.data);
    } catch (err: any) {
      message.error(err.response?.data?.detail || 'Failed to load users');
    }
    setLoading(false);
  };

  useEffect(() => { fetchUsers(); }, []);

  const handleCreate = async () => {
    const values = await form.validateFields();
    try {
      await api.post('/admin/users', values);
      message.success('User created');
      setModalOpen(false);
      form.resetFields();
      fetchUsers();
    } catch (err: any) {
      message.error(err.response?.data?.detail || 'Failed to create user');
    }
  };

  const handleEdit = async () => {
    const values = await form.validateFields();
    if (!editingUser) return;
    try {
      await api.put(`/admin/users/${editingUser.id}`, values);
      message.success('User updated');
      setModalOpen(false);
      setEditingUser(null);
      form.resetFields();
      fetchUsers();
    } catch (err: any) {
      message.error(err.response?.data?.detail || 'Failed to update user');
    }
  };

  const handleDelete = async (userId: number) => {
    try {
      await api.delete(`/admin/users/${userId}`);
      message.success('User deleted');
      fetchUsers();
    } catch (err: any) {
      message.error(err.response?.data?.detail || 'Failed to delete user');
    }
  };

  const handleChangePassword = async () => {
    const values = await passwordForm.validateFields();
    if (!changePasswordUser) return;
    try {
      await api.post(`/admin/users/${changePasswordUser.id}/reset-password`, {
        new_password: values.new_password,
      });
      message.success('Password reset');
      setChangePasswordOpen(false);
      passwordForm.resetFields();
    } catch (err: any) {
      message.error(err.response?.data?.detail || 'Failed to reset password');
    }
  };

  const handleMyPassword = async () => {
    const values = await myPasswordForm.validateFields();
    try {
      await api.post('/admin/change-password', {
        old_password: values.old_password,
        new_password: values.new_password,
      });
      message.success('Password changed');
      setMyPasswordOpen(false);
      myPasswordForm.resetFields();
    } catch (err: any) {
      message.error(err.response?.data?.detail || 'Failed to change password');
    }
  };

  const roleColors: Record<string, string> = {
    admin: 'red',
    operator: 'blue',
    viewer: 'green',
  };

  const columns = [
    {
      title: 'User', key: 'user',
      render: (_: any, r: User) => (
        <Space>
          <UserOutlined style={{ fontSize: 18, color: r.role === 'admin' ? '#FF4757' : '#00E5FF' }} />
          <div>
            <div style={{ fontWeight: 600 }}>{r.username}</div>
            <div style={{ fontSize: 11, color: '#8B949E' }}>{r.email}</div>
          </div>
        </Space>
      ),
    },
    { title: 'Role', dataIndex: 'role', key: 'role', render: (r: string) => <Tag color={roleColors[r]}>{r.toUpperCase()}</Tag> },
    { title: 'Active', dataIndex: 'is_active', key: 'active', render: (v: boolean) => <Tag color={v ? 'green' : 'red'}>{v ? 'Active' : 'Disabled'}</Tag> },
    { title: 'Last Login', dataIndex: 'last_login', key: 'last', render: (v: string) => v ? formatDate(v) : 'Never' },
    { title: 'Created', dataIndex: 'created_at', key: 'created', render: (v: string) => formatDate(v) },
    {
      title: 'Actions', key: 'actions',
      render: (_: any, r: User) => (
        <Space>
          <Button size="small" icon={<EditOutlined />} onClick={() => {
            setEditingUser(r);
            form.setFieldsValue({ username: r.username, email: r.email, role: r.role, is_active: r.is_active });
            setModalOpen(true);
          }}>Edit</Button>
          <Button size="small" icon={<KeyOutlined />} onClick={() => {
            setChangePasswordUser(r);
            setChangePasswordOpen(true);
          }}>Reset Password</Button>
          {r.username !== 'admin' && (
            <Popconfirm title="Delete this user?" onConfirm={() => handleDelete(r.id)}>
              <Button size="small" danger icon={<DeleteOutlined />}>Delete</Button>
            </Popconfirm>
          )}
        </Space>
      ),
    },
  ];

  return (
    <motion.div initial={{ opacity: 0, y: 10 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.3 }}>
      <Card title="User Management"
        extra={
          <Space>
            <Button icon={<KeyOutlined />} onClick={() => setMyPasswordOpen(true)}>Change My Password</Button>
            <Button type="primary" icon={<PlusOutlined />} onClick={() => { setEditingUser(null); form.resetFields(); setModalOpen(true); }}>Add User</Button>
            <Button icon={<ReloadOutlined />} onClick={fetchUsers}>Refresh</Button>
          </Space>
        }>
        <Table dataSource={users} columns={columns} rowKey="id" loading={loading} pagination={false} />
      </Card>

      {/* Create/Edit User Modal */}
      <GlassModal
        open={modalOpen}
        onCancel={() => { setModalOpen(false); setEditingUser(null); form.resetFields(); }}
        title={editingUser ? 'Edit User' : 'Add User'}
        width={500}
        footer={
          <Space>
            <Button onClick={() => setModalOpen(false)}>Cancel</Button>
            <Button type="primary" onClick={editingUser ? handleEdit : handleCreate}>
              {editingUser ? 'Update' : 'Create'}
            </Button>
          </Space>
        }
      >
        <Form form={form} layout="vertical">
          <Form.Item name="username" label="Username" rules={[{ required: true, min: 3 }]}>
            <Input disabled={!!editingUser} />
          </Form.Item>
          <Form.Item name="email" label="Email" rules={[{ required: true, type: 'email' }]}>
            <Input />
          </Form.Item>
          {!editingUser && (
            <Form.Item name="password" label="Password" rules={[{ required: true, min: 6 }]}>
              <Input.Password />
            </Form.Item>
          )}
          {editingUser && (
            <Form.Item name="password" label="New Password (leave empty to keep)">
              <Input.Password placeholder="Leave empty to keep current" />
            </Form.Item>
          )}
          <Form.Item name="role" label="Role" rules={[{ required: true }]}>
            <Select options={[
              { value: 'admin', label: 'Admin — full access' },
              { value: 'operator', label: 'Operator — manage backups' },
              { value: 'viewer', label: 'Viewer — read only' },
            ]} />
          </Form.Item>
          {editingUser && (
            <Form.Item name="is_active" label="Active" valuePropName="checked">
              <Switch />
            </Form.Item>
          )}
        </Form>
      </GlassModal>

      {/* Reset Password Modal */}
      <GlassModal
        open={changePasswordOpen}
        onCancel={() => { setChangePasswordOpen(false); setChangePasswordUser(null); passwordForm.resetFields(); }}
        title={`Reset Password: ${changePasswordUser?.username}`}
        width={400}
        footer={
          <Space>
            <Button onClick={() => setChangePasswordOpen(false)}>Cancel</Button>
            <Button type="primary" onClick={handleChangePassword}>Reset</Button>
          </Space>
        }
      >
        <Form form={passwordForm} layout="vertical">
          <Form.Item name="new_password" label="New Password" rules={[{ required: true, min: 6 }]}>
            <Input.Password />
          </Form.Item>
        </Form>
      </GlassModal>

      {/* Change My Password Modal */}
      <GlassModal
        open={myPasswordOpen}
        onCancel={() => { setMyPasswordOpen(false); myPasswordForm.resetFields(); }}
        title="Change My Password"
        width={400}
        footer={
          <Space>
            <Button onClick={() => setMyPasswordOpen(false)}>Cancel</Button>
            <Button type="primary" onClick={handleMyPassword}>Change</Button>
          </Space>
        }
      >
        <Form form={myPasswordForm} layout="vertical">
          <Form.Item name="old_password" label="Current Password" rules={[{ required: true }]}>
            <Input.Password />
          </Form.Item>
          <Form.Item name="new_password" label="New Password" rules={[{ required: true, min: 6 }]}>
            <Input.Password />
          </Form.Item>
        </Form>
      </GlassModal>
    </motion.div>
  );
}
