import React from 'react';
import { Routes, Route, Navigate } from 'react-router-dom';
import AppLayout from './components/AppLayout';
import Login from './pages/Login';
import Dashboard from './pages/Dashboard';
import Agents from './pages/Agents';
import Policies from './pages/Policies';
import Jobs from './pages/Jobs';
import Storages from './pages/Storages';
import ZabbixPage from './pages/Zabbix';
import TrafficPage from './pages/Traffic';
import AuditPage from './pages/Audit';
import ReplicationPage from './pages/Replication';
import RescuePage from './pages/Rescue';
import StorageTiersPage from './pages/StorageTiers';
import TapeLibraryPage from './pages/TapeLibrary';
import VirtualizationPage from './pages/Virtualization';
import DatabasesPage from './pages/Databases';
import MailSystemsPage from './pages/MailSystems';
import OsBackupPage from './pages/OsBackup';
import DirectoryServicesPage from './pages/DirectoryServices';
import GfsRetentionPage from './pages/GfsRetention';
import UsersPage from './pages/Users';
import AgentsDownloadPage from './pages/AgentsDownload';
import SureBackupPage from './pages/SureBackup';
import BackupCopyPage from './pages/BackupCopy';
import MalwarePage from './pages/Malware';
import DisasterRecoveryPage from './pages/DisasterRecovery';
import VMReplicationPage from './pages/VMReplication';
import { useAuthStore } from './stores/authStore';

function ProtectedRoute({ children }: { children: React.ReactNode }) {
  const token = useAuthStore((s) => s.accessToken);
  if (!token) return <Navigate to="/login" replace />;
  return <>{children}</>;
}

export default function App() {
  return (
    <Routes>
      <Route path="/login" element={<Login />} />
      <Route
        path="/*"
        element={
          <ProtectedRoute>
            <AppLayout>
              <Routes>
                <Route path="/dashboard" element={<Dashboard />} />
                <Route path="/agents" element={<Agents />} />
                <Route path="/policies" element={<Policies />} />
                <Route path="/jobs" element={<Jobs />} />
                <Route path="/storages" element={<Storages />} />
                <Route path="/zabbix" element={<ZabbixPage />} />
                <Route path="/traffic" element={<TrafficPage />} />
                <Route path="/audit" element={<AuditPage />} />
                <Route path="/replication" element={<ReplicationPage />} />
                <Route path="/rescue" element={<RescuePage />} />
                <Route path="/storage-tiers" element={<StorageTiersPage />} />
                <Route path="/tape" element={<TapeLibraryPage />} />
                <Route path="/virtualization" element={<VirtualizationPage />} />
                <Route path="/databases" element={<DatabasesPage />} />
                <Route path="/mail" element={<MailSystemsPage />} />
                <Route path="/os-backup" element={<OsBackupPage />} />
                <Route path="/directory" element={<DirectoryServicesPage />} />
                <Route path="/gfs" element={<GfsRetentionPage />} />
                <Route path="/users" element={<UsersPage />} />
                <Route path="/agents-download" element={<AgentsDownloadPage />} />
                <Route path="/surebackup" element={<SureBackupPage />} />
                <Route path="/backup-copy" element={<BackupCopyPage />} />
                <Route path="/malware" element={<MalwarePage />} />
                <Route path="/disaster-recovery" element={<DisasterRecoveryPage />} />
                <Route path="/vm-replication" element={<VMReplicationPage />} />
                <Route path="*" element={<Navigate to="/dashboard" replace />} />
              </Routes>
            </AppLayout>
          </ProtectedRoute>
        }
      />
    </Routes>
  );
}
