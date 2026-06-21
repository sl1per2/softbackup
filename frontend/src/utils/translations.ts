const translations: Record<string, Record<string, string>> = {
  en: {
    // Navigation
    'nav.dashboard': 'Dashboard',
    'nav.agents': 'Agents',
    'nav.policies': 'Policies',
    'nav.jobs': 'Jobs',
    'nav.storages': 'Storages',
    'nav.storageTiers': 'Storage Tiers',
    'nav.replication': 'Replication',
    'nav.tape': 'Tape Library',
    'nav.virtualization': 'Virtualization',
    'nav.databases': 'Databases',
    'nav.mail': 'Mail Systems',
    'nav.osBackup': 'OS & Filesystem',
    'nav.directory': 'Directory Services',
    'nav.gfs': 'GFS Retention',
    'nav.rescue': 'Rescue Image',
    'nav.traffic': 'Traffic',
    'nav.audit': 'Audit',
    'nav.zabbix': 'Zabbix',
    'nav.users': 'Users',
    'nav.agentsDownload': 'Download Agent',

    // Common
    'common.save': 'Save',
    'common.cancel': 'Cancel',
    'common.delete': 'Delete',
    'common.edit': 'Edit',
    'common.create': 'Create',
    'common.refresh': 'Refresh',
    'common.search': 'Search...',
    'common.loading': 'Loading...',
    'common.noData': 'No data',
    'common.actions': 'Actions',
    'common.status': 'Status',
    'common.name': 'Name',
    'common.host': 'Host',
    'common.type': 'Type',
    'common.version': 'Version',
    'common.size': 'Size',
    'common.date': 'Date',

    // Dashboard
    'dashboard.title': 'Dashboard',
    'dashboard.desc': 'Overview of your backup infrastructure. Real-time monitoring of agents, jobs, CDP sessions, and traffic statistics.',
    'dashboard.agentsOnline': 'Agents Online',
    'dashboard.successful24h': 'Successful (24h)',
    'dashboard.activeCDP': 'Active CDP',
    'dashboard.trafficSaved': 'Traffic Saved',
    'dashboard.recentJobs': 'Recent Jobs',
    'dashboard.backupVolumes': 'Backup Volumes (7 days)',

    // Agents
    'agents.title': 'Agents',
    'agents.desc': 'Manage backup agents installed on your servers. Agents perform backup/restore operations, communicate with the server via REST API, and report metrics. Install agents on each server you want to protect.',
    'agents.add': 'Add Agent',
    'agents.upgrade': 'Upgrade Agent',
    'agents.details': 'View agent details, system metrics, and CDP sessions.',

    // Policies
    'policies.title': 'Backup Policies',
    'policies.desc': 'Configure backup rules: source paths, schedules (cron), compression, encryption, transport modes, bandwidth limits, CDP settings. Policies define HOW and WHEN to backup your data.',
    'policies.create': 'New Policy',
    'policies.runNow': 'Run Now',
    'policies.cdpStart': 'Start CDP',
    'policies.cdpStop': 'Stop CDP',

    // Jobs
    'jobs.title': 'Backup Jobs',
    'jobs.desc': 'View history of all backup/restore jobs with status, duration, size, compression ratio, cache hit ratio, and transfer statistics. Filter by status, agent, policy, date range.',
    'jobs.cancel': 'Cancel Job',
    'jobs.details': 'View job overview, traffic graph, and detailed logs.',

    // Storages
    'storages.title': 'Storage Systems',
    'storages.desc': 'Manage backup storage destinations: local disks, NFS shares, S3/object storage. Test connections, monitor usage, configure Fast Clone support for instant VM recovery.',
    'storages.add': 'Add Storage',
    'storages.test': 'Test Connection',

    // Storage Tiers
    'storageTiers.title': 'Storage Tiers',
    'storageTiers.desc': 'Hierarchical storage management with automatic data migration between Hot (fast SSD), Warm (HDD), Cold (archive), and Tape tiers based on data age and access patterns.',
    'storageTiers.runTiering': 'Run Tiering',

    // Replication
    'replication.title': 'Remote Replication',
    'replication.desc': 'Synchronize backup copies between sites following the 3-2-1 rule. Supports incremental replication with digest exchange, bandwidth throttling, and encryption.',
    'replication.start': 'New Replication',

    // Tape Library
    'tape.title': 'Tape Library',
    'tape.desc': 'Manage LTO tape libraries: mount/unmount tapes, barcode inventory, multi-streaming, format tapes. Tape storage provides offline, air-gapped backup copies.',
    'tape.inventory': 'Inventory Scan',

    // Virtualization
    'virtualization.title': 'Virtualization',
    'virtualization.desc': 'Agentless backup of virtual machines from VMware vSphere, Hyper-V, Proxmox VE, KVM, OpenStack, and 12+ Russian platforms. Supports snapshots, CBT/RCT, Hot Add, NBD transport.',
    'virtualization.addHypervisor': 'Add Hypervisor',
    'virtualization.backupNow': 'Backup Now',

    // Databases
    'databases.title': 'Databases',
    'databases.desc': 'Backup and restore for 14+ database systems: PostgreSQL (+ Patroni), MySQL/MariaDB, SQL Server, Oracle, SAP HANA, Greenplum, Arenadata, YandexDB, and more. Supports hot backup, WAL/binlog archiving, point-in-time recovery.',
    'databases.connect': 'Connect',

    // Mail Systems
    'mail.title': 'Mail Systems',
    'mail.desc': 'Backup and restore for Microsoft Exchange, CommuniGate Pro, VK WorkSpace, RuPost, Mailion. Supports mailbox-level backup, granular item restore, public folders, CalDAV/CardDAV.',
    'mail.connect': 'Connect',
    'mail.backup': 'Backup',

    // OS & Filesystem
    'osBackup.title': 'OS & Filesystem Backup',
    'osBackup.desc': 'File-level backup for Linux (ext4, XFS, ZFS, BTRFS) and Windows (NTFS, ReFS). Supports LVM snapshots, ZFS/BTRFS snapshots, ACL preservation, incremental backup.',
    'osBackup.startBackup': 'Start Backup',

    // Directory Services
    'directory.title': 'Directory Services',
    'directory.desc': 'Backup and restore for ALD Pro, FreeIPA, and Microsoft Active Directory. Includes user/group backup, GPO, DNS, certificates, SYSVOL, system state.',
    'directory.connect': 'Connect',
    'directory.backup': 'Backup',

    // GFS Retention
    'gfs.title': 'GFS Retention Policy',
    'gfs.desc': 'Grandfather-Father-Son retention: automatically keeps Daily (7), Weekly (4), Monthly (12), Yearly (1) backups. Older backups beyond retention are deleted to save space.',
    'gfs.runCleanup': 'Run Cleanup',

    // Rescue Image
    'rescue.title': 'Rescue Image',
    'rescue.desc': 'Create bootable ISO for bare-metal recovery. Includes network tools and filesystem utilities. Boot from USB/CD to restore when the OS won\'t start.',
    'rescue.create': 'Create Image',

    // Traffic
    'traffic.title': 'Traffic Statistics',
    'traffic.desc': 'Monitor backup traffic optimization: cache hit ratios, compression ratios, zero-block savings, bandwidth usage. Analyze data transfer efficiency across all agents.',
    'traffic.globalCache': 'Global Cache Stats',

    // Audit
    'audit.title': 'Audit Log',
    'audit.desc': 'Complete audit trail of all user actions: logins, backup operations, configuration changes, restores. Filter by user, action, resource, date range. Export to CSV.',
    'audit.export': 'Export CSV',

    // Zabbix
    'zabbix.title': 'Zabbix Integration',
    'zabbix.desc': 'Export metrics to Zabbix monitoring: agent statuses, job results, CDP lag, cache hit ratios, traffic stats. Configure Zabbix server connection and Trapper port.',
    'zabbix.test': 'Test Connection',
    'zabbix.sync': 'Sync Metrics',

    // Users
    'users.title': 'User Management',
    'users.desc': 'Manage system users and access control. Create operators (backup management) and viewers (read-only). Change passwords, enable/disable accounts.',
    'users.add': 'Add User',
    'users.changePassword': 'Change My Password',
    'users.resetPassword': 'Reset Password',

    // Download Agent
    'agentsDownload.title': 'Download Agent',
    'agentsDownload.desc': 'Download the OBS Backup agent for Linux or Windows. Install on target servers to enable backup protection. Agents communicate via REST API or Unix Domain Socket.',
    'agentsDownload.download': 'Download',
    'agentsDownload.installScript': 'Install Script',
    'agentsDownload.linuxGuide': 'Linux Install Script',
    'agentsDownload.windowsGuide': 'Windows Guide',

    // Login
    'login.title': 'OBS Backup',
    'login.subtitle': 'Enterprise Backup System',
    'login.username': 'Username',
    'login.password': 'Password',
    'login.signIn': 'Sign In',
  },

  ru: {
    // Навигация
    'nav.dashboard': 'Панель управления',
    'nav.agents': 'Агенты',
    'nav.policies': 'Политики',
    'nav.jobs': 'Задания',
    'nav.storages': 'Хранилища',
    'nav.storageTiers': 'Уровни хранения',
    'nav.replication': 'Репликация',
    'nav.tape': 'Ленточная библиотека',
    'nav.virtualization': 'Виртуализация',
    'nav.databases': 'Базы данных',
    'nav.mail': 'Почтовые системы',
    'nav.osBackup': 'ОС и Файловые системы',
    'nav.directory': 'Каталоги (LDAP/AD)',
    'nav.gfs': 'Ретенция GFS',
    'nav.rescue': 'Спасательный образ',
    'nav.traffic': 'Статистика трафика',
    'nav.audit': 'Аудит',
    'nav.zabbix': 'Zabbix',
    'nav.users': 'Пользователи',
    'nav.agentsDownload': 'Скачать агент',

    // Общее
    'common.save': 'Сохранить',
    'common.cancel': 'Отмена',
    'common.delete': 'Удалить',
    'common.edit': 'Редактировать',
    'common.create': 'Создать',
    'common.refresh': 'Обновить',
    'common.search': 'Поиск...',
    'common.loading': 'Загрузка...',
    'common.noData': 'Нет данных',
    'common.actions': 'Действия',
    'common.status': 'Статус',
    'common.name': 'Название',
    'common.host': 'Хост',
    'common.type': 'Тип',
    'common.version': 'Версия',
    'common.size': 'Размер',
    'common.date': 'Дата',

    // Панель управления
    'dashboard.title': 'Панель управления',
    'dashboard.desc': 'Обзор инфраструктуры резервного копирования. Мониторинг агентов, заданий, CDP-сессий и статистики трафика в реальном времени.',
    'dashboard.agentsOnline': 'Агентов онлайн',
    'dashboard.successful24h': 'Успешно за 24ч',
    'dashboard.activeCDP': 'Активных CDP',
    'dashboard.trafficSaved': 'Сэкономлено трафика',
    'dashboard.recentJobs': 'Последние задания',
    'dashboard.backupVolumes': 'Объёмы бэкапов (7 дней)',

    // Агенты
    'agents.title': 'Агенты',
    'agents.desc': 'Управление агентами резервного копирования на серверах. Агенты выполняют операции бэкапа/восстановления, взаимодействуют с сервером через REST API и отправляют метрики. Установите агент на каждый защищаемый сервер.',
    'agents.add': 'Добавить агент',
    'agents.upgrade': 'Обновить агент',
    'agents.details': 'Просмотр деталей агента, метрик системы и CDP-сессий.',

    // Политики
    'policies.title': 'Политики резервного копирования',
    'policies.desc': 'Настройка правил бэкапа: исходные пути, расписания (cron), сжатие, шифрование, режимы передачи, ограничение канала, CDP. Политики определяют КАК и КОГДА резервировать данные.',
    'policies.create': 'Новая политика',
    'policies.runNow': 'Запустить сейчас',
    'policies.cdpStart': 'Запустить CDP',
    'policies.cdpStop': 'Остановить CDP',

    // Задания
    'jobs.title': 'Задания резервного копирования',
    'jobs.desc': 'История всех заданий бэкапа/восстановления с статусом, длительностью, размером, коэффициентом сжатия, коэффициентом попадания в кэш и статистикой передачи.',
    'jobs.cancel': 'Отменить задание',
    'jobs.details': 'Обзор задания, график трафика и детальные логи.',

    // Хранилища
    'storages.title': 'Системы хранения',
    'storages.desc': 'Управление хранилищами: локальные диски, NFS-шары, S3/объектное хранилище. Тестирование подключений, мониторинг использования, настройка Fast Clone для мгновенного восстановления VM.',
    'storages.add': 'Добавить хранилище',
    'storages.test': 'Тест подключения',

    // Уровни хранения
    'storageTiers.title': 'Уровни хранения',
    'storageTiers.desc': 'Иерархическое управление данными с автоматической миграцией между Hot (быстрые SSD), Warm (HDD), Cold (архив) и Tape (ленты) уровнями по возрасту данных.',
    'storageTiers.runTiering': 'Запустить миграцию',

    // Репликация
    'replication.title': 'Удалённая репликация',
    'replication.desc': 'Синхронизация резервных копий между сайтами по правилу 3-2-1. Поддерживает инкрементальную репликацию с обменом дайджестами, ограничение канала и шифрование.',
    'replication.start': 'Новая репликация',

    // Ленточная библиотека
    'tape.title': 'Ленточная библиотека',
    'tape.desc': 'Управление ленточными библиотеками LTO: монтирование/демонтирование кассет, инвентаризация по штрихкоду, мультистриминг, форматирование. Ленточное хранилище обеспечивает оффлайн-копии.',
    'tape.inventory': 'Инвентаризация',

    // Виртуализация
    'virtualization.title': 'Виртуализация',
    'virtualization.desc': 'Безагентное резервное копирование VM из VMware vSphere, Hyper-V, Proxmox VE, KVM, OpenStack и 12+ российских платформ. Поддержка снапшотов, CBT/RCT, Hot Add, NBD.',
    'virtualization.addHypervisor': 'Добавить гипервизор',
    'virtualization.backupNow': 'Бэкап сейчас',

    // Базы данных
    'databases.title': 'Базы данных',
    'databases.desc': 'Бэкап и восстановление 14+ СУБД: PostgreSQL (+ Patroni), MySQL/MariaDB, SQL Server, Oracle, SAP HANA, Greenplum, Arenadata, YandexDB и др. Поддержка горячего бэкапа, WAL/binlog архивации, восстановления на точку во времени.',
    'databases.connect': 'Подключить',

    // Почтовые системы
    'mail.title': 'Почтовые системы',
    'mail.desc': 'Бэкап и восстановление Microsoft Exchange, CommuniGate Pro, VK WorkSpace, RuPost, Mailion. Поддержка почтовых ящиков, поэлементного восстановления, общих папок, CalDAV/CardDAV.',
    'mail.connect': 'Подключить',
    'mail.backup': 'Бэкап',

    // ОС и Файловые системы
    'osBackup.title': 'Резервное копирование ОС и ФС',
    'osBackup.desc': 'Файловый бэкап для Linux (ext4, XFS, ZFS, BTRFS) и Windows (NTFS, ReFS). Поддержка LVM/ ZFS/BTRFS снапшотов, сохранение ACL, инкрементальный бэкап.',
    'osBackup.startBackup': 'Начать бэкап',

    // Каталоги
    'directory.title': 'Каталоги (LDAP/AD)',
    'directory.desc': 'Бэкап и восстановление ALD Pro, FreeIPA и Microsoft Active Directory. Включает резервное копирование пользователей/групп, GPO, DNS, сертификатов, SYSVOL, системного состояния.',
    'directory.connect': 'Подключить',
    'directory.backup': 'Бэкап',

    // Ретенция GFS
    'gfs.title': 'Политика ретенции GFS',
    'gfs.desc': 'Grandfather-Father-Son: автоматическое хранение Daily (7), Weekly (4), Monthly (12), Yearly (1) бэкапов. Старые копии удаляются для экономии места.',
    'gfs.runCleanup': 'Запустить очистку',

    // Спасательный образ
    'rescue.title': 'Спасательный образ',
    'rescue.desc': 'Создание загрузочного ISO для bare-metal восстановления. Включает сетевые утилиты и инструменты работы с файловыми системами. Загрузите с USB/CD при неработающей ОС.',
    'rescue.create': 'Создать образ',

    // Статистика трафика
    'traffic.title': 'Статистика трафика',
    'traffic.desc': 'Мониторинг оптимизации трафика: коэффициент попадания в кэш, сжатие, экономия за счёт нулевых блоков, утилизация канала. Анализ эффективности передачи данных.',
    'traffic.globalCache': 'Глобальный кэш',

    // Аудит
    'audit.title': 'Журнал аудита',
    'audit.desc': 'Полный аудит-трейл действий пользователей: входы, операции бэкапа, изменения настроек, восстановления. Фильтрация по пользователю, действию, ресурсу, дате. Экспорт в CSV.',
    'audit.export': 'Экспорт CSV',

    // Zabbix
    'zabbix.title': 'Интеграция с Zabbix',
    'zabbix.desc': 'Экспорт метрик в Zabbix: статусы агентов, результаты заданий, CDP lag, коэффициент кэша, статистика трафика. Настройка подключения к Zabbix серверу.',
    'zabbix.test': 'Тест подключения',
    'zabbix.sync': 'Синхронизация',

    // Пользователи
    'users.title': 'Управление пользователями',
    'users.desc': 'Управление пользователями и контроль доступа. Создание операторов (управление бэкапами) и наблюдателей (только чтение). Смена паролей, активация/деактивация аккаунтов.',
    'users.add': 'Добавить пользователя',
    'users.changePassword': 'Сменить мой пароль',
    'users.resetPassword': 'Сбросить пароль',

    // Скачать агент
    'agentsDownload.title': 'Скачать агент',
    'agentsDownload.desc': 'Скачать агент OBS Backup для Linux или Windows. Установите на целевые серверы для защиты данных. Агенты взаимодействуют через REST API или Unix Domain Socket.',
    'agentsDownload.download': 'Скачать',
    'agentsDownload.installScript': 'Скрипт установки',
    'agentsDownload.linuxGuide': 'Скрипт для Linux',
    'agentsDownload.windowsGuide': 'Инструкция для Windows',

    // Вход
    'login.title': 'OBS Backup',
    'login.subtitle': 'Корпоративная система резервного копирования',
    'login.username': 'Имя пользователя',
    'login.password': 'Пароль',
    'login.signIn': 'Войти',
  },
};

export default translations;
