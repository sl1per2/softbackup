import { Card } from 'antd';
import CountUp from 'react-countup';

interface StatCardProps {
  icon: React.ReactNode;
  label: string;
  value: number;
  color: string;
  suffix?: string;
}

export default function StatCard({ icon, label, value, color, suffix }: StatCardProps) {
  return (
    <Card
      style={{
        borderLeft: `3px solid ${color}`,
        background: 'rgba(255,255,255,0.03)',
      }}
      bodyStyle={{ padding: '16px 20px' }}
    >
      <div style={{ display: 'flex', alignItems: 'center', gap: 12 }}>
        <div style={{ fontSize: 24, color }}>{icon}</div>
        <div>
          <div style={{ fontSize: 12, color: '#8B949E', marginBottom: 4 }}>{label}</div>
          <div style={{ fontSize: 24, fontWeight: 700, color: '#E6E6E6' }}>
            <CountUp end={value} duration={1.5} separator="," />
            {suffix && <span style={{ fontSize: 14, marginLeft: 4, color: '#8B949E' }}>{suffix}</span>}
          </div>
        </div>
      </div>
    </Card>
  );
}
