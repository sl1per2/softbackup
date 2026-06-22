# OBS Backup — Полная документация
Версия 2.0.0 | 2026

## Оглавление

1. [Обзор системы](#1-обзор-системы)
2. [Быстрый старт](#2-быстрый-старт)
3. [Установка сервера](#3-установка-сервера)
4. [Установка агентов](#4-установка-агентов)
5. [Создание бэкапов](#5-создание-бэкапов)
6. [Восстановление](#6-восстановление)
7. [Непрерывная защита данных (CDP)](#7-непрерывная-защита-данных-cdp)
8. [Дедупликация](#8-дедупликация)
9. [Хранилища](#9-хранилища)
10. [Интеграция с Zabbix](#10-интеграция-с-zabbix)
11. [SureBackup — Автотестирование бэкапов](#11-surebackup)
12. [Backup Copy + GFS Archive](#12-backup-copy)
13. [Ленточные библиотеки (Tape)](#13-ленточные-библиотеки)
14. [Неизменяемость бэкапов (Immutability)](#14-неизменяемость)
15. [Обнаружение вредоносного ПО (Malware)](#15-malware)
16. [Оркестратор DR](#16-dr-orchestrator)
17. [Self-Service Portal](#17-self-service)
18. [LDAP / Active Directory](#18-ldap)
19. [Репликация VM](#19-репликация-vm)
20. [Storage Snapshots](#20-storage-snapshots)
21. [NAS Backup (NDMP)](#21-ndmp)
22. [SAP HANA / Oracle RMAN](#22-sap-oracle)
23. [Kubernetes Backup](#23-kubernetes)
24. [Scale-Out Repository](#24-scale-out)
25. [Отказоустойчивость сервера (HA)](#25-ha)
26. [Ad-hoc Backup](#26-ad-hoc)
27. [Веб-интерфейс](#27-веб-интерфейс)
28. [API Reference](#28-api-reference)
29. [Решение проблем](#29-решение-проблем)
30. [Безопасность](#30-безопасность)
31. [FAQ](#31-faq)
32. [Архитектура](#32-архитектура)
33. [История версий](#33-история-версий)
34. [Man-страницы](#34-man-страницы)
35. [OOP Архитектура ядра (v2.0)](#35-oop-архитектура-ядра)
36. [Dirty Buffer Logger](#36-dirty-buffer-logger)
37. [Удалённое управление агентами](#37-удалённое-управление-агентами)
38. [Деплой агентов через SSH/WinRM](#38-деплой-агентов-через-sshwinrm)

---

## 1. Обзор системы

### 1.1 Что такое OBS Backup

OBS Backup — корпоративная система резервного копирования с поддержкой инкрементального бэкапа на уровне блоков, непрерывной защиты данных (CDP), мгновенного восстановления виртуальных машин, дедупликации и WAN-ускорения. Разработана как альтернатива Veeam Backup & Replication, Cyber Protect (Acronis) и RuBackup.

### 1.2 Ключевые возможности

- Инкрементальный бэкап на уровне блоков (CBT) для Linux, Windows, VMware, Proxmox, Hyper-V
- Горячий бэкап баз данных: PostgreSQL, MySQL, MSSQL, Oracle, MongoDB, SAP HANA, 1С
- Непрерывная защита данных (CDP) с RPO от 1 секунды
- Мгновенное восстановление VM (Instant Recovery) за 30-120 секунд
- Автоматическое тестирование бэкапов (SureBackup) в изолированной среде
- Дедупликация на основе FastCDC + BLAKE3
- WAN-ускорение (Global Cache, Digest Exchange)
- Прямое чтение с SAN (Direct SAN Transport)
- Бэкап из снапшотов хранилищ (NetApp, Dell EMC, HPE, Pure Storage)
- Многоуровневая компрессия (LZ4, ZSTD)
- Шифрование AES-256-GCM (in-transit и at-rest)
- Неизменяемость бэкапов (Immutability, WORM, S3 Object Lock)
- Обнаружение вредоносного ПО (Malware Detection, интеграция с ClamAV)
- Репликация VM (непрерывная, RPO от 5 минут)
- Кросс-платформенное восстановление (VMDK↔QCOW2↔VHDX, P2V, V2P)
- Оркестратор аварийного восстановления (DR Plan, Failover, Failback)
- Резервное копирование на ленту (LTO-4/5/6/7/8/9, LTFS)
- Резервное копирование NAS (NDMP v4)
- Резервное копирование Kubernetes (etcd, PVC, Velero-совместимость)
- Scale-Out Repository (объединение хранилищ в пул)
- Портал самообслуживания (Self-Service Portal)
- Интеграция с LDAP/Active Directory
- Интеграция с Zabbix
- Отказоустойчивость сервера (HA Cluster)
- Ad-hoc Backup (VeeamZIP)
- Backup Copy + GFS Archive
- Веб-интерфейс с тёмной темой и real-time дашбордом

### 1.3 Архитектура

```
┌─────────────┐     ┌──────────────────┐     ┌─────────────┐
│  OBS Agent   │────▶│  OBS Server       │────▶│  Storage     │
│  (C++ core)  │     │  (Python FastAPI) │     │  (S3/NFS/...)│
│  Linux/Win/  │◀────│  React Frontend   │     │  Local/Dedup │
│  macOS/ARM   │     │  PostgreSQL+Redis │     │  Compressed  │
└─────────────┘     └──────────────────┘     └─────────────┘
```

#### 1.3.1 Компоненты (расширенный список)

| Компонент | Назначение | Технология |
|-----------|-----------|------------|
| Core | Ядро системы: агент, бэкап, восстановление, CDP | C++17/20 |
| Data Mover | Оптимизированная передача данных | C++ |
| Dedup Engine | Глобальная дедупликация | FastCDC + BLAKE3 |
| CDP Engine | Непрерывная защита данных | C++ |
| SureBackup | Автотестирование бэкапов | C++ + Virtual Lab |
| Backup Copy | Копирование бэкапов на вторичное хранилище | C++ |
| Tape Engine | Поддержка ленточных библиотек LTO | C++ + LTFS |
| Immutability | Защита от удаления/шифрования | C++ + S3 Object Lock |
| Malware Scanner | Обнаружение вредоносного ПО | C++ + ClamAV |
| DR Orchestrator | Оркестрация аварийного восстановления | C++ |
| VM Replication | Непрерывная репликация VM | C++ |
| Storage Snapshots | Бэкап из снапшотов SAN/NAS | C++ |
| NDMP | Бэкап NAS через NDMP v4 | C++ |
| SAP Backint | Бэкап SAP HANA | C++ |
| K8s Backup | Бэкап Kubernetes | C++ |
| Scale-Out Repo | Объединение хранилищ | C++ |
| HA Cluster | Отказоустойчивость сервера | Python + Patroni + Keepalived |
| Backend | REST API, планировщик, WebSocket | Python 3.11 FastAPI |
| Frontend | Веб-интерфейс | React 18 TypeScript |
| PostgreSQL | Хранение метаданных | PostgreSQL 16 |
| Redis | Очереди, кэш, WebSocket | Redis 7 |

Поток данных при бэкапе:

1. Agent получает задание от Server (REST API)
2. CBT определяет изменённые блоки
3. Чтение изменённых блоков (Direct SAN / Hot Add / Network)
4. Сжатие (LZ4/ZSTD) + Дедупликация (FastCDC + BLAKE3)
5. Шифрование (AES-256-GCM)
6. Поиск в Global Cache (Digest Exchange)
7. Отправка только уникальных блоков через Multi-stream TCP
8. Сохранение в Storage (S3/NFS/Local)
9. Обновление индекса чанков и каталога файлов
10. Отправка метрик в Zabbix

### 1.4 Поддерживаемые системы

**Операционные системы (агенты):**
- Linux: Ubuntu 20.04+, Debian 11+, RHEL 8+, CentOS 8+, Rocky 8+, Alma 8+
- Linux ARM64: Raspberry Pi OS, Ubuntu ARM, AWS Graviton
- Windows: Windows Server 2016/2019/2022, Windows 10/11 Pro
- macOS: 12 Monterey, 13 Ventura, 14 Sonoma

**Гипервизоры:**
- VMware vSphere 6.7, 7.0, 8.0 (vCenter, ESXi)
- Proxmox VE 7.x, 8.x
- Microsoft Hyper-V 2016, 2019, 2022
- KVM/QEMU, OpenStack, oVirt/zVirt
- Basis Dynamix, Альт Виртуализация, VMmanager, Р-Виртуализация, РУСТЭК, Space VM, Tionix

**Базы данных:**
- PostgreSQL 12-16 (+ Postgres Pro, Tantor, Arenadata, Patroni кластер)
- MySQL 5.7-8.0, MariaDB 10.5-10.11
- Microsoft SQL Server 2016-2022
- Oracle Database 12c-23ai (RMAN, BCT, Data Pump)
- SAP HANA 2.0, MongoDB 5.0-7.0, Greenplum 6-7
- YandexDB, РЕД База Данных, ПК СВ «Брест»

**Приложения:** 1С:Предприятие 8.3, Exchange 2016/2019, CommuniGate Pro, VK WorkSpace, RuPost, Mailion

**Облачные хранилища:** Яндекс Object Storage, VK Cloud, AWS S3, Ceph RGW, MinIO

**Каталоги:** Microsoft Active Directory, FreeIPA, ALD Pro

---

## 2. Быстрый старт

### 2.1 Установка сервера (3 команды)

```bash
curl -fsSL https://obs-backup.example.com/docker-compose.yml -o docker-compose.yml
curl -fsSL https://obs-backup.example.com/.env.example -o .env
nano .env && docker-compose up -d
```

Веб-интерфейс: http://<IP>:80 | API: http://<IP>:8000 | Логин: **admin** / Пароль: **admin**

### 2.2 Установка агента

```bash
curl -fsSL http://<server-ip>:80/api/agents-download/install-script/linux | sudo bash
```

### 2.3 Первый бэкап

1. Политики → Создать политику → Заполнить: название, cron, источники, хранилище
2. Запустить сейчас → Тип: Полный

### 2.4 Первое восстановление

Задания → Найти успешный бэкап → Восстановить → Выбрать файлы → Указать путь

---

## 3. Установка сервера

### 3.1 Системные требования

| Параметр | Минимальные | Рекомендуемые |
|----------|-------------|---------------|
| CPU | 4 ядра x86_64 | 16 ядер x86_64 |
| RAM | 8 GB | 32 GB |
| Диск | 100 GB SSD | 500 GB NVMe |
| Сеть | 1 Gbps | 10 Gbps |

### 3.2 Docker Compose

Сервисы: postgres:16, redis:7, backend (FastAPI + Celery worker + Celery beat), frontend (nginx + React)

### 3.3 Nginx + HTTPS

```nginx
server {
    listen 443 ssl http2;
    server_name backup.example.com;
    ssl_certificate /etc/letsencrypt/live/backup.example.com/fullchain.pem;
    location / { proxy_pass http://127.0.0.1:80; }
    location /api/ { proxy_pass http://127.0.0.1:8000; }
    location /ws/ { proxy_pass http://127.0.0.1:8000; proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade; proxy_set_header Connection "upgrade"; }
}
```

### 3.4 Firewall

```bash
sudo ufw allow 80/tcp
sudo ufw allow 443/tcp
sudo ufw allow from 10.0.0.0/8 to any port 8000
```

---

## 4. Установка агентов

### 4.1 Системные требования

CPU: 2+ ядра, RAM: 256MB+, Диск: 100MB + кэш

### 4.2 Linux

```bash
sudo dpkg -i obs-agent_1.0.0_amd64.deb  # DEB
sudo rpm -ivh obs-agent-1.0.0-1.x86_64.rpm  # RPM
```

### 4.3 Windows

```powershell
Start-Process msiexec.exe -ArgumentList "/i obs-agent.msi /qn SERVER_URL=https://<server> TOKEN=<token>" -Wait
```

### 4.4 Конфигурация

`/etc/obs-agent/config.json`:

```json
{
    "server_url": "https://backup.example.com",
    "token": "eyJhbGciOiJIUzI1NiIs...",
    "log_level": "info",
    "cache_size_mb": 2048,
    "max_concurrent_jobs": 2,
    "bandwidth_limit_kbps": 0,
    "bandwidth_schedule": [{"days":"1-5","hours":"9-18","limit":51200}]
}
```

### 4.5 Массовая установка

Ansible: загрузить install.sh, выполнить на всех хостах с --server и --token

### 4.6 Проверка

```bash
obs-agent --status
obs-agent --diag
sudo journalctl -u obs-agent -f
```

---

## 5. Создание бэкапов

### 5.1 Типы

| Тип | Размер | Скорость | Зависимость |
|-----|--------|----------|-------------|
| Full | 100% | Медленно | Независимый |
| Incremental | 1-10% | Быстро | От предыдущего |
| Differential | 5-30% | Средне | От последнего Full |

### 5.2 Инкрементальный бэкап

**Linux:** Btrfs send, ZFS send, LVM thin, dm-era, fanotify, tar --listed-incremental

**Windows:** VSS + Bitmap Differential, USN Journal, ReFS CBT

**VMware:** QueryChangedDiskAreas — Direct SAN / Hot Add / NBD

**Proxmox:** QEMU dirty bitmap, QGA fsfreeze, vzdump incremental

**Hyper-V:** RCT, VHDX checkpoints, VSS Writer

**БД:** pgBackRest (LSN), XtraBackup (LSN), RMAN LEVEL 1 (SCN), mongodump --oplog

---

## 6. Восстановление

### 6.1 Instant Recovery

VMware: NFS mount → VM за 30-120с → Storage vMotion в фоне

### 6.2 Восстановление файлов

File Browser → выбрать файлы → скачать/восстановить на агент

### 6.3 Восстановление БД

PostgreSQL PITR: pgBackRest --type=time --target="2026-06-21 14:30:00"
MySQL: xtrabackup --prepare + --copy-back
MSSQL: RESTORE DATABASE + RESTORE LOG с STOPAT

### 6.4 Кросс-платформенное

VMDK↔QCOW2↔VHDX через qemu-img + инъекция драйверов + адаптация конфига VM

---

## 7. Непрерывная защита данных (CDP)

### 7.1 Настройка

В политике: CDP → Включить → Интервал (1-300с) → Max Latency → Retention

### 7.2 Метрики

| Метрика | Норма | Тревога |
|---------|-------|---------|
| CDP Lag | < 1000 ms | > 5000 ms |

---

## 8. Дедупликация

FastCDC (64KB) + BLAKE3 + Global Cache (Digest Exchange) + Source/Target-side

---

## 9. Хранилища

| Тип | Fast Clone | Рекомендация |
|-----|------------|-------------|
| Local | XFS/Btrfs/ZFS/ReFS | Небольшие инсталляции |
| NFS | Нет | Средние |
| S3 | Нет | Облачное |

---

## 10. Интеграция с Zabbix

Настройка: Server URL, API Token, Trapper Port. Метрики: obs.jobs.*, obs.backup.*, obs.dedup.*, obs.cache.*, obs.cdp.*, obs.storage.*, obs.agents.*

---

## 11. SureBackup — Автоматическое тестирование бэкапов

### 11.1 Что такое SureBackup

SureBackup автоматически проверяет каждый созданный бэкап в изолированной среде (Virtual Lab). Это гарантирует, что бэкап действительно можно восстановить.

### 11.2 Как работает

1. Создаётся изолированная сеть (Virtual Lab): отдельный vSwitch/бридж без доступа к production
2. DNS-сервер-заглушка разрешает имена VM в изолированные IP
3. VM запускается из бэкапа в этой изолированной сети
4. Выполняются проверки (настраиваемый набор)
5. VM выключается, изолированная среда удаляется

### 11.3 Типы проверок

| Проверка | Описание | Критерий успеха |
|----------|----------|-----------------|
| Heartbeat | Проверка VMware Tools / QEMU Guest Agent | Агент отвечает за 60 секунд |
| Ping | Пинг IP внутри изолированной сети | 100% ответов |
| Application | Кастомный скрипт внутри VM | Код возврата 0 |
| CRC | Проверка контрольных сумм файлов | Все файлы совпадают |
| Boot Time | Время от старта до готовности | < 300 секунд |

### 11.4 Расписание

- После каждого Full-бэкапа
- После каждого N-го инкремента (настраивается, default 7)
- Раз в неделю для случайного бэкапа
- Вручную: кнопка "Проверить бэкап"

### 11.5 Результаты

- ✅ **Verified**: VM загрузилась, все тесты пройдены
- ⚠️ **Warning**: VM загрузилась, но некоторые тесты не пройдены
- ❌ **Failed**: VM не загрузилась или критическая ошибка

### 11.6 Уведомления

- Email: результат тестирования
- Zabbix: `obs.surebackup.status` (0=ok, 1=warning, 2=failed)
- Telegram: при Failed

### 11.7 Настройка

В веб-интерфейсе: "SureBackup" → "Настройки":
- Включить автоматическую проверку: Да
- Проверять после каждого: Full + каждый 7-й инкремент
- Virtual Lab Network: 192.168.255.0/24
- Таймаут загрузки: 300 секунд
- Приложения для проверки: указать скрипты

---

## 12. Backup Copy + GFS Archive

### 12.1 Назначение

Backup Copy создаёт копии бэкапов на вторичном хранилище для дополнительной защиты (правило 3-2-1). GFS Archive автоматически формирует долгосрочные точки хранения.

### 12.2 Режимы копирования

| Режим | Описание |
|-------|----------|
| Immediate | Копировать сразу после создания бэкапа |
| Scheduled | По расписанию (cron), например ежедневно в 6:00 |
| Mirror | Точная копия цепочки бэкапов (full + все инкременты) |

### 12.3 GFS Archive

Автоматическое создание долгосрочных точек из цепочки инкрементов:
- **Еженедельная точка**: каждый понедельник, хранить 4
- **Ежемесячная точка**: 1 число, хранить 12
- **Ежегодная точка**: 1 января, хранить 7

Точки создаются путём синтетического объединения (Synthetic Full) без передачи данных.

### 12.4 Создание Backup Copy Job

1. Перейти в "Backup Copy"
2. Нажать "Создать задание"
3. Выбрать исходную политику бэкапа
4. Выбрать целевое хранилище
5. Настроить расписание и GFS
6. Сохранить

---

## 13. Ленточные библиотеки (Tape)

### 13.1 Поддержка лент

OBS Backup поддерживает ленточные библиотеки LTO (Linear Tape-Open):
- LTO-4, LTO-5, LTO-6, LTO-7, LTO-8, LTO-9
- Производители: IBM, HP, Dell, Quantum, Oracle/StorageTek

### 13.2 Форматы записи

- **LTFS** (Linear Tape File System) — рекомендованный формат
- **tar** — fallback для совместимости
- **Мультиплексирование**: несколько бэкапов на одну ленту
- **Spanning**: один бэкап на несколько лент

### 13.3 Пул лент

| Пул | Описание |
|-----|----------|
| Free | Свободные ленты, готовы к использованию |
| Active | Ленты с актуальными бэкапами |
| Archive | Ленты с архивными GFS-точками |
| Retired | Ленты на списание |

### 13.4 Операции

- Инвентаризация: сканирование штрих-кодов всех лент
- Запись: бэкап → лента (автоматический выбор из Free Pool)
- Чтение: лента → восстановление
- Перемещение: смена пула
- Очистка: cleaning tape

### 13.5 Шифрование

LTO-4+ поддерживают аппаратное шифрование AES-256-GCM. Ключи управляются через встроенный KMS.

### 13.6 Уведомления

- Мало свободных лент (Free Pool < 5)
- Ошибка чтения/записи
- Необходима замена cleaning tape

### 13.7 Подключение

```bash
sudo apt install mtx sg3-utils
sudo lsscsi -g
sudo mtx -f /dev/sg2 status
```

В веб-интерфейсе: "Ленточная библиотека" → "Добавить": Устройство сменщика /dev/sg2, Устройство привода /dev/st0

---

## 14. Неизменяемость бэкапов (Immutability)

### 14.1 Зачем нужна

Защита от случайного удаления, атак шифровальщиков, злонамеренного удаления.

### 14.2 Режимы

**S3 Object Lock (для S3-хранилищ):**
- **Governance Mode**: блокировка с возможностью обхода (MFA + admin)
- **Compliance Mode**: абсолютная блокировка до истечения срока
- **Retention Period**: настраиваемый (дни/недели/месяцы)

**Файловая Immutability (для NFS/Local):**
- Linux: `chattr +i` (неизменяемый атрибут)
- Read-only mount для хранилища
- Storage Agent: принимает только запись

### 14.3 Настройка

В политике: Immutability → Включить → Период: 30 дней → Режим: Governance/Compliance

### 14.4 MFA Delete

Для удаления заблокированного бэкапа в Governance Mode:
1. Нажать "Удалить"
2. Запрос MFA-кода (Google Authenticator)
3. Ввести код → подтверждение → удаление

---

## 15. Обнаружение вредоносного ПО (Malware Detection)

### 15.1 Как работает

OBS Backup анализирует каждый бэкап на признаки вредоносной активности:

**Эвристики:**
- Резкое увеличение % изменённых блоков (>50% от среднего)
- Массовое изменение расширений (.encrypted, .lock, .crypt)
- Изменение энтропии файлов (энтропия > 7.5)
- Появление ransom notes (HOW_TO_DECRYPT.txt, README_FOR_DECRYPT.html)
- Массовое удаление VSS-снапшотов (Windows)

**Интеграция с антивирусами:**
- ClamAV (встроенный, open-source)
- Windows Defender (через MpCmdRun.exe)
- Kaspersky Endpoint Security (kav-cli)
- Dr.Web (drweb-ctl)

### 15.2 Действия при обнаружении

1. Бэкап помечается как SUSPICIOUS
2. Немедленное уведомление: Email + Telegram + Zabbix (DISASTER)
3. Автоматическая блокировка удаления чистых бэкапов
4. Автоматический запуск SureBackup для проверки
5. Администратор принимает решение

### 15.3 Настройка

"Malware Detection": Включить сканирование, Эвристический анализ, ClamAV, Действие при обнаружении

---

## 16. Оркестратор аварийного восстановления (DR)

### 16.1 Что такое DR Orchestrator

План аварийного восстановления: точный порядок восстановления VM, настройки сети, скрипты и зависимости. Запускается одной кнопкой.

### 16.2 Создание DR Plan

1. "Disaster Recovery" → "Создать план"
2. Добавить VM в порядке восстановления:
   - 1. Domain Controller (DC01)
   - 2. SQL Server (SQL01) — зависит от DC01
   - 3. 1С Сервер (1C01) — зависит от SQL01
   - 4. Web Server (WEB01) — зависит от 1C01
3. Network Mapping: Production → DR Network (192.168.10.0/24)
4. IP Mapping: DC01 10.0.0.1 → 192.168.10.1
5. Boot Delay: 30 секунд между VM
6. Pre/Post скрипты

### 16.3 Запуск

1. Красная кнопка "Launch DR"
2. Подтвердить: "Production будет остановлен"
3. Последовательное восстановление VM
4. Общий RTO отображается на дашборде
5. Отчёт (PDF) с деталями

### 16.4 Failback

Production восстановлен → изменения из реплики синхронизируются обратно → плановое переключение

---

## 17. Self-Service Portal

### 17.1 Назначение

Портал для пользователей, которые управляют бэкапами своих VM без администрирования системы.

### 17.2 Возможности

- Просмотр списка своих VM
- Запуск Ad-hoc бэкапа
- Восстановление отдельных файлов
- Просмотр истории бэкапов

### 17.3 Делегирование

Админ назначает права: Пользователи → {user} → Self-Service User → назначить VM

### 17.4 Квоты

- Ad-hoc бэкапов в день: 3
- Размер хранилища на пользователя: 500 GB

---

## 18. LDAP / Active Directory

### 18.1 Поддержка

Microsoft AD, OpenLDAP, FreeIPA, 389 Directory Server

### 18.2 Настройка

LDAP URL: ldaps://dc01.example.com:636, Base DN: DC=example,DC=com, Bind DN, TLS: Да

### 18.3 Маппинг групп

| AD Group | Роль OBS |
|----------|----------|
| Domain Admins | admin |
| Backup Operators | operator |
| IT Department | operator |
| Domain Users | self-service |

### 18.4 Аутентификация

1. Проверка локальной БД → 2. Если не найден → LDAP bind → 3. Создание/обновление пользователя → 4. Роль по группе AD

---

## 19. Репликация VM

### 19.1 Отличие от бэкапа

| | Бэкап | Репликация |
|---|-------|-----------|
| Куда | Хранилище (S3/NFS) | Гипервизор (готовая VM) |
| RPO | Часы | Минуты |
| Запуск | Instant Recovery | Мгновенный Failover |

### 19.2 Failover

1. Кнопка "Failover"
2. Подтвердить: "Production VM будет остановлена"
3. Реплика запускается на целевом хосте
4. DNS/IP обновляются

### 19.3 Failback

Production восстановлен → изменения синхронизируются обратно → плановое переключение

---

## 20. Storage Snapshots

### 20.1 Концепция

Бэкап из снапшотов SAN/NAS без нагрузки на production.

### 20.2 Поддерживаемые

NetApp ONTAP, Dell EMC, HPE Nimble/3PAR, Pure Storage, IBM FlashSystem, Linux LVM

### 20.3 Процесс

1. Создание снапшота (1-5 сек)
2. Монтирование к агенту (read-only)
3. Бэкап из снапшота
4. Удаление снапшота

---

## 21. NAS Backup (NDMP)

### 21.1 Поддержка

NetApp FAS/AFF, Dell EMC Isilon, QNAP, Synology, TrueNAS

### 21.2 Настройка

IP, порт (10000), NDMP credentials, режим: NDMP Direct

---

## 22. SAP HANA / Oracle RMAN

### 22.1 SAP HANA

Backint API: `hdbsql "BACKUP DATA USING BACKINT ('/path/to/obs_backint')"`

Full, Incremental, Differential, Log backup

### 22.2 Oracle RMAN

BCT: `ALTER DATABASE ENABLE BLOCK CHANGE TRACKING`
Level 1: `RMAN> BACKUP INCREMENTAL LEVEL 1 DATABASE PLUS ARCHIVELOG`
Multisection: `RMAN> BACKUP SECTION SIZE 1G DATABASE`

---

## 23. Kubernetes Backup

### 23.1 Компоненты

etcd + Манифесты + PVC

```bash
etcdctl snapshot save /backup/etcd.snap
kubectl get all --all-namespaces -o yaml > /backup/manifests.yaml
```

### 23.2 Velero-интеграция

```bash
velero plugin add obs-backup/velero-plugin
velero backup-location create obs --provider obs-backup --bucket obs-backup-k8s
```

---

## 24. Scale-Out Repository

### 24.1 Политики

| Политика | Описание |
|----------|----------|
| Round-Robin | Равномерно по extent'ам |
| Data Locality | Чанки VM на одном extent'е |
| Performance Tier | SSD для новых, HDD для старых |
| Capacity Tier | Холодные данные на S3 |

---

## 25. Отказоустойчивость сервера (HA)

### 25.1 Архитектура

Virtual IP (Keepalived) → OBS Server 1 (Active) + OBS Server 2 (Standby)
PostgreSQL Primary/Standby (Patroni + etcd), Redis Sentinel

### 25.2 Failover

Keepalived → Virtual IP на сервер 2, Patroni → Primary на standby, Redis Sentinel → Slave на Master. RTO: 30-60 секунд.

---

## 26. Ad-hoc Backup (VeeamZIP)

Быстрый разовый бэкап без политики. Агенты → Quick Backup → Путь → Retention → Запустить. Не влияет на цепочку инкрементов.

---

## 27. Веб-интерфейс

12 страниц: Дашборд, Агенты, Политики, Задания, Хранилища, Уровни хранения, Репликация, Ленточная библиотека, Виртуализация, Базы данных, Почта, ОС/ФС, Каталоги, GFS, Спасательный образ, Трафик, Аудит, Zabbix, Пользователи, Скачать агенты, SureBackup, Backup Copy, Malware, DR Plans, VM Replication

Тёмная тема: фон #0D1117, акцент #00E5FF, успех #00FF88, ошибка #FF4757

---

## 28. API Reference

### Аутентификация
- `POST /api/login`, `POST /api/refresh`, `POST /api/logout`

### Агенты
- `GET/POST/PUT/DELETE /api/agents`

### Политики
- `CRUD /api/policies`, `POST /api/policies/{id}/run`, `POST /api/policies/{id}/cdp/start|stop`

### Задания
- `GET /api/jobs`, `GET /api/jobs/{id}/log`

### SureBackup
- `GET/PUT /api/surebackup/config`, `POST /api/surebackup/run`, `GET /api/surebackup/results`

### Backup Copy
- `CRUD /api/backup-copy`, `POST /api/backup-copy/{id}/run`

### Tape Library
- `GET/POST /api/tape-lib/library|inventory|write|read|tapes`

### Immutability
- `PUT /api/policies/{id}/immutability`, `POST /api/jobs/{id}/delete`

### Malware
- `GET/PUT /api/malware/config`, `POST /api/malware/scan/{job_id}`, `GET /api/malware/alerts`

### DR
- `CRUD /api/dr/plans`, `POST /api/dr/plans/{id}/run`, `POST /api/dr/plans/{id}/failback`

### VM Replication
- `CRUD /api/replication-v2`, `POST .../start|failover|failback`

### LDAP
- `GET/PUT /api/admin/ldap/config`, `POST /api/admin/ldap/test|sync`

### Kubernetes
- `GET /api/k8s/clusters`, `POST /api/k8s/backup`

### Ad-hoc
- `POST /api/adhoc/backup`

### Виртуализация
- `GET /api/virtualization/vms`, `POST /api/virtualization/vms/{id}/backup`

### БД
- `GET /api/dbms/supported`, `POST /api/dbms/test`

### Дашборд
- `GET /api/dashboard/summary|charts|recent-jobs`, `WS /ws/dashboard`

### Пользователи
- `GET/POST/PUT/DELETE /api/admin/users`, `POST /api/admin/change-password`

---

## 29. Решение проблем

- Агент не подключается → firewall, token, server_url
- Бэкап упал → логи, хранилище, место
- Цепочка разорвана → автоматический Differential/Full
- VSS не работает → net stop vss & net start vss
- SureBackup Failed → проверить логи, пересоздать бэкап
- Шифровальщик → восстановить из Immutability-защищённого бэкапа
- DR не запускается → проверить network mapping, IP mapping

---

## 30. Безопасность

- Шифрование: AES-256-GCM (at-rest), TLS 1.3 (in-transit), RSA-4096
- Роли: admin, operator, viewer
- Аудит: все действия логируются
- Immutability: Governance/Compliance modes, MFA Delete
- Malware Detection: эвристики + ClamAV

---

## 31. FAQ

**В: Сколько агентов?** До 1000 на сервер.

**В: Как часто Full?** Рекомендуется раз в неделю.

**В: SureBackup Failed?** Проверить логи, пересоздать бэкап, проверить источник данных.

**В: Защита от шифровальщиков?** Immutability (30+ дней) + Malware Detection + SureBackup.

**В: Быстрое восстановление датацентра?** DR Orchestrator: настроить план → "Launch DR" → RTO от 10 минут.

**В: Ленточные библиотеки?** Да, LTO-4/5/6/7/8/9, LTFS/tar, IBM/HP/Dell/Quantum.

**В: Отказоустойчивость сервера?** HA Cluster: 2 сервера + Keepalived + Patroni + Redis Sentinel.

**В: Kubernetes?** Да, через Velero-плагин или напрямую: etcd + манифесты + PVC.

**В: Self-Service?** Да, назначить роль + привязать VM.

**В: Кросс-платформенное восстановление?** Да, VMDK↔QCOW2↔VHDX + инъекция драйверов.

---

## 32. Архитектура

```
OBS Agent (C++) ←→ OBS Server (FastAPI + Celery) ←→ PostgreSQL + Redis → Storage (S3/NFS/Local)
     ↕                                                          ↕
  CBT/CDP/Dedup/SureBackup                               Frontend (React + WebSocket)
```

---

## 33. История версий

### v2.0.0 (2026-06-22)
- **OOP переписка ядра** — Abstract Factory, Strategy, Observer, Command, DI Container, RAII
- **13 специализированных агентов** — Esxi, Hyper-V, MSSQL, PostgreSQL, Oracle, Proxmox, SAP HANA, K8s, Linux FS, Windows FS, NDMP, Exchange
- **DirtyBufferLogger** — фиксация грязных буферов перед каждым бэкапом, 6-шаговый цикл, SQLite, REST API
- **Удалённое управление агентами** — 11 команд (start-backup, restart, stop-jobs, update, flush, clear-cache, config, logs, metrics)
- **Деплой агентов** — SSH (Linux/macOS) и WinRM (Windows) из веб-интерфейса
- **Веб-интерфейс** — вкладка Dirty Buffers, модальное окно агента с 6 вкладками, live log viewer
- **AgentCommandHandler** — обработчик команд на стороне агента (Singleton)
- **EventBus** — подписка на события (JobStarted, JobCompleted, AgentConnected, ThreatDetected)
- **ServiceRegistry** — DI-контейнер для внедрения зависимостей
- **CommandQueue** — очередь заданий (Command-паттерн)
- **Тесты** — 10 OOP-тестов + 6 DirtyBufferLogger тестов

### v1.0.0 (2026-06-21)
- Первый публичный релиз
- Инкрементальный бэкап, CDP, Instant Recovery
- SureBackup: автотестирование бэкапов
- Backup Copy + GFS Archive
- Ленточные библиотеки LTO (LTFS)
- Неизменяемость (Immutability, S3 Object Lock, WORM)
- Обнаружение вредоносного ПО (эвристики + ClamAV)
- DR Orchestrator (план, failover, failback)
- Репликация VM (RPO от 5 минут)
- Бэкап из снапшотов хранилищ
- Self-Service Portal
- LDAP/AD интеграция
- NDMP (NAS backup)
- SAP HANA (Backint) + Oracle RMAN
- Kubernetes Backup (etcd + Velero)
- Scale-Out Repository
- HA Cluster (Patroni + Keepalived)
- Ad-hoc Backup
- Zabbix (15+ метрик)
- Веб-интерфейс, документация на русском (34 раздела)

---

## 34. Man-страницы

### obs-agent(1)
Команды: start, stop, restart, status, diag, version, upgrade, clear-cache

### obs-agent-config(5)
Параметры: server_url, token, log_level, cache_size_mb, max_concurrent_jobs, bandwidth_limit_kbps, bandwidth_schedule

---

## 35. OOP Архитектура ядра (v2.0)

### 35.1 Философия

Ядро системы (core/) полностью переписано в объектно-ориентированном стиле (C++17) с соблюдением принципов SOLID:

1. **Single Responsibility** — каждый класс имеет одну ответственность
2. **Open/Closed** — расширение через наследование, не модификация
3. **Liskov Substitution** — специализации заменяют базовые классы
4. **Interface Segregation** — мелкие интерфейсы (IBackupEngine, IDedupEngine, и т.д.)
5. **Dependency Inversion** — зависимости через абстракции

### 35.2 Паттерны

| Паттерн | Реализация | Файл |
|---------|-----------|------|
| Abstract Factory | AgentFactory — создание 13 типов агентов | `agent/agent_factory.h` |
| Abstract Factory | EngineFactory — создание движков | `engine/engine_factory.h` |
| Strategy | ICompressionEngine, ITransportEngine | `engine/i_compression.h` |
| Observer | EventBus — публикация событий | `framework/event_bus.h` |
| Command | CommandQueue — очередь заданий | `framework/command.h` |
| DI Container | ServiceRegistry — внедрение зависимостей | `framework/service_registry.h` |
| RAII | Все движки/агенты (конструктор=init, деструктор=shutdown) | все файлы |
| Composition | BackupPipeline = 5 движков | `engine/backup_pipeline.h` |

### 35.3 Иерархия классов

```
IComponent (базовый интерфейс)
├── IAgent (абстрактный агент)
│   └── GenericAgent (базовая реализация)
│       ├── EsxiAgent          — VMware ESXi/vSphere
│       ├── HypervAgent        — Microsoft Hyper-V
│       ├── ProxmoxAgent       — Proxmox VE
│       ├── MssqlAgent         — MS SQL Server
│       ├── PostgresqlAgent    — PostgreSQL + семейство
│       ├── OracleAgent        — Oracle (RMAN)
│       ├── SapHanaAgent       — SAP HANA
│       ├── KubernetesAgent    — Kubernetes
│       ├── LinuxFsAgent       — Btrfs/ZFS/LVM/dm-era
│       ├── WindowsFsAgent     — VSS/USN/ReFS
│       ├── NdmpAgent          — NAS (NDMP)
│       └── ExchangeAgent      — MS Exchange (EWS/Graph)
├── IBackupEngine → DefaultBackupEngine
├── IDedupEngine → DefaultDedupEngine (FastCDC + SQLite)
├── ICryptoEngine → DefaultCryptoEngine (OpenSSL AES-256-GCM)
├── ICompressionEngine → ZstdCompressionEngine
├── ITransportEngine → DefaultTransportEngine
├── ICdpEngine → CdpEngine
├── IIpcServer → UnixSocketServer / NamedPipeServer
└── DirtyBufferLogger
```

### 35.4 DI Контейнер

```cpp
// Создание всех движков через фабрику
auto pipeline = EngineFactory::instance().create_pipeline(data_dir, cache_dir);

// Регистрация в DI
service_registry_->register_instance<IBackupEngine>(pipeline->backup_engine());
service_registry_->register_instance<IDedupEngine>(pipeline->dedup_engine());
service_registry_->register_instance<ICryptoEngine>(pipeline->crypto_engine());

// Разрешение зависимостей
auto crypto = service_registry_->resolve<ICryptoEngine>();
```

### 35.5 EventBus (Observer)

```cpp
// Подписка на события
event_bus_->subscribe<JobStartedEvent>([](const JobStartedEvent& e) {
    spdlog::info("Job started: {}", e.job_id);
});

// Публикация событий
event_bus_->publish(JobStartedEvent{"job_001", "agent_001"});
event_bus_->publish(JobProgressEvent{"job_001", 75.0, 1024*1024});
event_bus_->publish(JobCompletedEvent{"job_001", true, ""});
```

### 35.6 Тестирование

```bash
cd core/build && ./test_oop
# 10 тестов: ServiceRegistry, EventBus, CommandQueue, AgentFactory,
#            EngineFactory, BackupPipeline, DedupEngine, CryptoEngine,
#            CompressionEngine, AgentCanHandle
```

---

## 36. Dirty Buffer Logger

### 36.1 Назначение

Перед каждым бэкапом система ОБЯЗАНА зафиксировать состояние грязных буферов, сбросить их на диск и записать отчёт в БД. Это даёт:

- **Доказательство консистентности** — был сброс или нет
- **Историю грязных буферов** — для анализа и поиска проблемных бэкапов
- **Аудит** — соответствие SLA и требованиям безопасности

### 36.2 6-шаговый цикл

```
Step 1/6: Capturing BEFORE state
  → Dirty buffers before: 2341 pages (9.1 MB, 5.7% of RAM)

Step 2/6: Quiescing application
  → Приложение приостановлено (если поддерживается)

Step 3/6: Flushing dirty buffers to disk
  → sync + drop_caches (Linux) /FlushFileBuffers (Windows)

Step 4/6: Unquiescing application
  → Приложение возобновлено

Step 5/6: Capturing AFTER state
  → Dirty buffers after: 0 pages (0.0 B, 0.0% of RAM)

Step 6/6: Evaluating consistency
  → APPLICATION_CONSISTENT / FILE_CONSISTENT / CRASH_CONSISTENT
```

### 36.3 Уровни согласованности

| До → После | Результат | Описание |
|------------|-----------|----------|
| 2341 → 0 | **APPLICATION_CONSISTENT** | Все грязные страницы сброшены |
| 5000 → 1200 | **FILE_CONSISTENT** | Часть страниц осталась грязной |
| 1000 → 1000 | **CRASH_CONSISTENT** | Сброс не выполнился |

### 36.4 API

```cpp
// Создание логгера
DirtyBufferLogger logger("/var/lib/obs/dirty_buffers.db", "https://server:8000");

// Полный цикл flush + log
PlatformBufferFlusher flusher("postgresql");
FlushResult result = logger.flush_and_log("job_001", "agent_001", &flusher);

// Запросы
auto logs = logger.get_logs("job_001");
auto failed = logger.get_failed_flushes();
auto inconsistent = logger.get_inconsistent_backups();
```

### 36.5 Таблица dirty_buffer_logs

| Поле | Тип | Описание |
|------|-----|----------|
| backup_job_id | TEXT | ID задания |
| agent_id | TEXT | ID агента |
| plugin_name | TEXT | esxi/hyperv/mssql/postgresql/... |
| before_page_count | INT | Грязные страницы ДО |
| before_size_bytes | INT | Размер ДО |
| before_percent_ram | REAL | % от RAM ДО |
| after_page_count | INT | Грязные страницы ПОСЛЕ |
| after_size_bytes | INT | Размер ПОСЛЕ |
| flush_status | TEXT | flushed/failed/timeout/not_flushed |
| flush_duration_ms | INT | Длительность сброса |
| consistency_level | TEXT | application/crash/file_consistent |
| is_consistent | BOOL | Консистентность |
| component_details_json | TEXT | JSON: [{name, dirty_pages, size_bytes}] |

### 36.6 REST API

```
GET  /api/dirty-buffers              — список с фильтрами
GET  /api/dirty-buffers/stats        — агрегированная статистика
GET  /api/dirty-buffers/failed       — неуспешные сбросы
GET  /api/dirty-buffers/inconsistent — неконсистентные бэкапы
POST /api/dirty-buffers/log          — запись лога (от агента)
```

### 36.7 Веб-интерфейс

Вкладка **Dirty Buffers** в веб-портале показывает:
- 6 карточек статистики (Total, Flushed, Failed, Inconsistent, Avg Flush, Success Rate)
- Таблицу с фильтрами по Plugin, Status, Consistency
- Drawer с полными деталями записи
- Цветовую индикацию: зелёный=flushed, красный=failed, жёлтый=inconsistent

---

## 37. Удалённое управление агентами

### 37.1 Возможности

Полное управление агентом удалённо через веб-интерфейс, без SSH на хост:

| Команда | Описание | API |
|---------|----------|-----|
| Quick Backup | Запуск бэкапа с выбором типа/источника | `POST /api/agents/{id}/command/start-backup` |
| Stop All Jobs | Остановка всех заданий | `POST /api/agents/{id}/command/stop-jobs` |
| Restart Agent | Перезапуск агента | `POST /api/agents/{id}/command/restart` |
| Update Agent | Самообновление | `POST /api/agents/{id}/command/update` |
| Flush Dirty Buffers | Сброс грязных буферов | `POST /api/agents/{id}/command/flush-dirty-buffers` |
| Clear Cache | Очистка кэша чанков | `POST /api/agents/{id}/command/clear-cache` |
| Get Config | Просмотр конфигурации | `GET /api/agents/{id}/config` |
| Update Config | Обновление конфигурации | `PUT /api/agents/{id}/config` |
| Get Logs | Просмотр логов | `GET /api/agents/{id}/logs` |
| Download Logs | Скачивание полного лога | `GET /api/agents/{id}/logs/download` |
| Get Metrics | Метрики CPU/RAM/Disk | `GET /api/agents/{id}/metrics` |

### 37.2 Модальное окно агента (6 вкладок)

| Вкладка | Содержимое |
|---------|-----------|
| **Overview** | CPU/RAM/Disk gauge, статус, инфо, uptime |
| **Control** | 6 кнопок: Quick Backup, Stop All, Restart, Update, Flush Buffers, Clear Cache |
| **Configuration** | Форма: log_level, heartbeat, cache, concurrent jobs, bandwidth |
| **Logs** | Терминальный вид, фильтр по уровню, поиск, auto-scroll, download |
| **Jobs** | Таблица заданий с прогрессом, кнопка запуска |
| **Dirty Buffers** | Статус, кнопка flush, история сбросов |

### 37.3 Архитектура

```
Web UI (React)
  │
  ├── POST /api/agents/{id}/command/start-backup
  ├── POST /api/agents/{id}/command/restart
  ├── GET  /api/agents/{id}/config
  ├── PUT  /api/agents/{id}/config
  ├── GET  /api/agents/{id}/logs
  │
  ▼
Backend (FastAPI)
  │
  ├── _send_agent_command(id, "restart")
  │     └── HTTP POST → http://{agent_ip}:9900/api/command
  │
  ▼
Agent Command Handler (C++)
  │
  ├── AgentCommandHandler::handle("restart", params)
  │     └── Выполнение команды
  │
  └── Ответ → Backend → Web UI
```

### 37.4 AgentCommandHandler (C++)

Обработчик команд на стороне агента (Singleton):

```cpp
// Регистрация обработчика
AgentCommandHandler::instance().register_handler("flush_dirty_buffers", [](const json&) {
    system("sync");
    return {true, "", {{"flushed", true}}};
});

// Обработка входящей команды
auto response = AgentCommandHandler::instance().handle("start_backup", params);
```

Поддерживаемые команды: `get_config`, `update_config`, `restart`, `stop_all_jobs`, `self_update`, `flush_dirty_buffers`, `clear_cache`, `get_logs`, `get_metrics`, `start_backup`.

---

## 38. Деплой агентов через SSH/WinRM

### 38.1 Деплой на Linux/macOS (SSH)

```bash
POST /api/agents/deploy/ssh
{
  "host": "192.168.1.100",
  "port": 22,
  "username": "root",
  "password": "secret",
  "os_type": "linux",
  "server_url": "https://obs-server:8000",
  "auth_token": "token123"
}
```

Процесс:
1. Подключение по SSH (paramiko)
2. Загрузка install.sh с сервера
3. Выполнение: `curl -fsSL https://get.obs-backup.example.com/install.sh | sudo bash -s -- --server <url> --token <token>`
4. Агент регистрируется на сервере автоматически

### 38.2 Деплой на Windows (WinRM)

```bash
POST /api/agents/deploy/winrm
{
  "host": "192.168.1.200",
  "port": 5985,
  "username": "Administrator",
  "password": "secret",
  "use_ssl": false,
  "server_url": "https://obs-server:8000",
  "auth_token": "token123"
}
```

Процесс:
1. Подключение по WinRM (pywinrm)
2. Загрузка install.ps1
3. Выполнение: `irm https://get.obs-backup.example.com/install.ps1 | iex -Args '--server','<url>','--token','<token>'`
4. Регистрация как Windows Service

### 38.3 Веб-интерфейс

Вкладка **Deploy New Agent** в разделе Agents:
- Поля: Host, Port, Username, Password, OS Type
- Опции: Private Key (для SSH), Server URL, Auth Token
- Кнопка **Deploy** — запуск автоматического деплоя

### 38.4 Требования

**Для SSH-деплоя:**
- Python библиотека: `pip install paramiko`
- Доступ: SSH на порт 22
- Права: root или sudo

**Для WinRM-деплоя:**
- Python библиотека: `pip install pywinrm`
- WinRM включён на целевом хосте
- Права: Administrator

---

## 39. Сетевое обнаружение (Network Discovery)

### 39.1 API

```
POST /api/discovery/scan     — сканирование подсети (IP scan, port check)
GET  /api/discovery/subnets  — список сохранённых подсетей
```

### 39.2 Фронтенд

Компонент `NetworkDiscovery` — ввод подсети (CIDR), сканирование порта 9900, таблица результатов с кнопкой "Install" / "Manage".

---

## 40. Деплой агентов (Agent Installer)

### 40.1 API

```
POST /api/agent-installer/install  — запуск установки
POST /api/agent-installer/check    — проверка наличия агента на хосте
```

### 40.2 Фронтенды

| Компонент | Описание |
|-----------|----------|
| `InstallAgentPopup` | Модальное окно установки: хост, OS, credentials → 6-шаговый прогресс |
| `BulkInstallProgress` | Массовая установка: таблица хостов, параллельный прогресс |
| `PendingAgents` | Список ожидающих регистрации агентов (approve/reject) |

---

## 41. Удалённое редактирование конфигурации

### 41.1 API

```
GET  /api/config-editor/schema/{agent_id}  — JSON Schema полей конфигурации
GET  /api/config-editor/{agent_id}         — текущие значения
PUT  /api/config-editor/{agent_id}         — обновление конфигурации
```

### 41.2 Фронтенд

Компонент `AgentConfigEditor` — Drawer с формой: log_level, heartbeat, cache, concurrent jobs, bandwidth, chunk_size. Кнопки "Save" и "Apply Now" (restart agent).

---

## 42. Просмотр логов агентов

### 42.1 API

```
GET  /api/log-viewer/{agent_id}           — логи с фильтрами (level, search, tail)
GET  /api/log-viewer/{agent_id}/download  — скачивание полного лога
```

### 42.2 WebSocket

```
/ws/agent-commands     — команды агентам в реальном времени
/ws/install-progress/{job_id} — прогресс установки агента
/ws/agent-logs/{agent_id}     — логи агента в реальном времени
```

### 42.3 Фронтенд

Компонент `AgentLogViewer` — Drawer с терминальным видом (чёрный фон, цветные строки), фильтр по уровню, поиск, auto-scroll, download.

---

## 43. Контекстное меню агента

Компонент `AgentContextMenu` — правый клик по агенту в таблице:
- Quick Backup
- Stop All Jobs
- Restart Agent
- Update Agent
- Edit Configuration
- View Logs
- Flush Dirty Buffers
- Clear Chunk Cache

---

## 44. Миграции Alembic

### 44.1 v002 — Dirty Buffers + Agent Control

Таблицы:
- `dirty_buffer_logs` — логи сброса грязных буферов
- `agent_install_jobs` — задания на установку агентов
- `agent_command_logs` — логи команд агентам

```bash
cd backend && alembic upgrade head
```

---

## 45. Тесты (37 тестов)

| Тест | Количество | Описание |
|------|-----------|----------|
| test_oop | 10 | ServiceRegistry, EventBus, CommandQueue, AgentFactory, EngineFactory, BackupPipeline, DedupEngine, CryptoEngine, CompressionEngine, AgentCanHandle |
| test_dirty_buffer | 6 | Basic flush, partial flush, failed flush, queries, failed queries, JSON output |
| test_esxi_agent | 7 | Creation, can_handle, config, list_vms, snapshot, changed_blocks, factory |
| test_mssql_agent | 5 | Creation, can_handle, connection, factory, types |
| test_pgsql_agent | 6 | Creation, can_handle, connection, continuous, factory, all_db_agents |

```bash
cd core/build && ctest --output-on-failure
```

---

*OBS Backup v2.0.0 | 2026-06-22 | support@obs-backup.example.com*
