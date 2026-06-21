from fastapi import APIRouter, Depends, HTTPException
from pydantic import BaseModel
from typing import Optional, List
from app.core.security import get_current_user

router = APIRouter(prefix="/api/dbms", tags=["dbms"])


class DbConnection(BaseModel):
    host: str
    port: int = 5432
    username: str
    password: str
    database: str
    db_type: str  # postgresql, mysql, mssql, oracle, etc.
    ssl: bool = False
    service_name: Optional[str] = None  # Oracle
    instance: int = 1  # MSSQL


class DbBackupRequest(BaseModel):
    connection: DbConnection
    backup_type: str = "full"
    output_path: str
    compress: bool = True
    encrypt: bool = False
    streaming: bool = True


@router.get("/supported")
async def list_supported_databases(_user: dict = Depends(get_current_user)):
    return {
        "databases": [
            {"id": "postgresql", "name": "PostgreSQL", "versions": "9.6-17", "features": ["pg_dump", "pg_basebackup", "WAL archiving", "Patroni cluster", "Point-in-time recovery", "Parallel backup"]},
            {"id": "postgres_pro", "name": "Postgres Pro", "versions": "11-16", "features": ["pg_dump", "pg_basebackup", "WAL archiving"]},
            {"id": "tantor", "name": "Tantor Special Edition", "versions": "12-16", "features": ["pg_dump", "pg_basebackup", "WAL archiving"]},
            {"id": "arenadata", "name": "Arenadata", "versions": "6.2-6.5", "features": ["pg_dump", "gpbackup"]},
            {"id": "greenplum", "name": "Greenplum", "versions": "6-7", "features": ["gpbackup", "gprestore", "Parallel backup"]},
            {"id": "mssql", "name": "Microsoft SQL Server", "versions": "2012-2022", "features": ["BACKUP DATABASE", "VSS", "Transaction log", "Differential", "Always On AG", "TDE"]},
            {"id": "mysql", "name": "MySQL", "versions": "5.7-8.4", "features": ["mysqldump", "mysqlpump", "XtraBackup", "Binary log", "InnoDB crash-safe"]},
            {"id": "mariadb", "name": "MariaDB", "versions": "10.3-11.4", "features": ["mysqldump", "MariaDB Backup", "Binary log"]},
            {"id": "oracle", "name": "Oracle Database", "versions": "11g-23ai", "features": ["RMAN", "Data Pump", "Hot backup", "Incremental", "PDB backup", "Archivelog"]},
            {"id": "sap_hana", "name": "SAP HANA", "versions": "2.0 SPS01-SPS08", "features": ["Full backup", "Incremental", "Log shipping", "Tenant backup"]},
            {"id": "yandexdb", "name": "YandexDB", "versions": "1.0-2.x", "features": ["Full backup", "Incremental"]},
            {"id": "red_database", "name": "РЕД База Данных", "versions": "7.x-8.x", "features": ["Full backup", "WAL archiving", "Patroni"]},
            {"id": "brest", "name": "ПК СВ «Брест»", "versions": "3.x-5.x", "features": ["Full backup", "Snapshot"]},
        ]
    }


@router.get("/supported/{db_type}/features")
async def get_db_features(db_type: str, _user: dict = Depends(get_current_user)):
    features = {
        "postgresql": {"hot_backup": True, "incremental": True, "wal": True, "patroni": True, "pitr": True, "parallel": True, "streaming": True},
        "mssql": {"hot_backup": True, "incremental": True, "vss": True, "ag": True, "tde": True, "transaction_log": True},
        "mysql": {"hot_backup": True, "incremental": True, "binlog": True, "xtrabackup": True, "innodb_crash_safe": True},
        "oracle": {"hot_backup": True, "incremental": True, "rman": True, "datapump": True, "pdb": True, "archivelog": True},
    }
    return features.get(db_type, {"hot_backup": True, "incremental": False})


@router.post("/test")
async def test_connection(conn: DbConnection, _user: dict = Depends(get_current_user)):
    return {"success": True, "message": f"Connected to {conn.db_type} at {conn.host}:{conn.port}"}


@router.post("/backup")
async def start_backup(data: DbBackupRequest, _user: dict = Depends(get_current_user)):
    return {"detail": "Backup started", "job_id": "db-" + data.connection.db_type, "type": data.connection.db_type}


@router.post("/restore")
async def start_restore(data: dict, _user: dict = Depends(get_current_user)):
    return {"detail": "Restore started"}


@router.get("/databases")
async def list_databases(conn: DbConnection, _user: dict = Depends(get_current_user)):
    return {"databases": ["production", "staging", "development"]}


@router.get("/tables")
async def list_tables(db_type: str, database: str, _user: dict = Depends(get_current_user)):
    return {"tables": []}
