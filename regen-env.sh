#!/bin/bash
set -e

JWT_SECRET=$(openssl rand -hex 32)
JWT_REFRESH_SECRET=$(openssl rand -hex 32)
DB_PASS=$(openssl rand -base64 16 | tr -d '/+=' | head -c 16)

# Root .env
cat > .env <<EOF
DATABASE_URL=postgresql+asyncpg://obs:${DB_PASS}@postgres:5432/obs_backup
REDIS_URL=redis://redis:6379
JWT_SECRET=${JWT_SECRET}
JWT_REFRESH_SECRET=${JWT_REFRESH_SECRET}
CORE_SOCKET_PATH=/tmp/obs-core.sock
EOF

# Backend .env
cat > backend/.env <<EOF
DATABASE_URL=postgresql+asyncpg://obs:${DB_PASS}@localhost:5432/obs_backup
REDIS_URL=redis://localhost:6379
JWT_SECRET=${JWT_SECRET}
JWT_REFRESH_SECRET=${JWT_REFRESH_SECRET}
CORE_SOCKET_PATH=/tmp/obs-core.sock
DEBUG=true
EOF

echo "Done. New secrets generated:"
echo "  DB_PASS:        ${DB_PASS}"
echo "  JWT_SECRET:     ${JWT_SECRET}"
echo "  JWT_REFRESH:    ${JWT_REFRESH_SECRET}"
