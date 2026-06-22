cmake_minimum_required(VERSION 3.16)
project(vovqa-core VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Protobuf REQUIRED)
find_package(Boost REQUIRED COMPONENTS system filesystem thread program_options)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(SQLITE3 sqlite3)
endif()
if(NOT SQLITE3_FOUND)
    find_path(SQLITE3_INCLUDE_DIR sqlite3.h)
    find_library(SQLITE3_LIBRARY sqlite3)
    if(SQLITE3_INCLUDE_DIR AND SQLITE3_LIBRARY)
        set(SQLITE3_FOUND TRUE)
        set(SQLITE3_INCLUDE_DIRS ${SQLITE3_INCLUDE_DIR})
        set(SQLITE3_LIBRARIES ${SQLITE3_LIBRARY})
    endif()
endif()

find_path(SPDLOG_INCLUDE_DIR spdlog/spdlog.h)
find_library(SPDLOG_LIBRARY spdlog)
find_path(FMT_INCLUDE_DIR fmt/core.h)
find_library(FMT_LIBRARY fmt)

message(STATUS "spdlog: include=${SPDLOG_INCLUDE_DIR} lib=${SPDLOG_LIBRARY}")
message(STATUS "fmt: include=${FMT_INCLUDE_DIR} lib=${FMT_LIBRARY}")
message(STATUS "sqlite3: include=${SQLITE3_INCLUDE_DIRS} lib=${SQLITE3_LIBRARIES}")

set(PROTO_DIR ${CMAKE_SOURCE_DIR}/../proto)
file(GLOB PROTO_FILES ${PROTO_DIR}/*.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${Protobuf_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIRS}
    ${SPDLOG_INCLUDE_DIR}
    ${FMT_INCLUDE_DIR}
    ${SQLITE3_INCLUDE_DIRS}
)

# ── OOP Core Sources ─────────────────────────────────────────────────────────

set(CORE_SOURCES
    # Framework
    src/framework/application.cpp
    # Engine
    src/engine/engine_factory.cpp
    src/engine/backup_pipeline.cpp
    # Agents
    src/agent/generic_agent.cpp
    src/agent/esxi_agent.cpp
    src/agent/hyperv_agent.cpp
    src/agent/mssql_agent.cpp
    src/agent/postgresql_agent.cpp
    src/agent/oracle_agent.cpp
    src/agent/proxmox_agent.cpp
    src/agent/saphana_agent.cpp
    src/agent/kubernetes_agent.cpp
    src/agent/linux_fs_agent.cpp
    src/agent/windows_fs_agent.cpp
    src/agent/ndmp_agent.cpp
    src/agent/exchange_agent.cpp
    # IPC
    src/ipc/message_router.cpp
)

# Platform-specific IPC
if(WIN32)
    list(APPEND CORE_SOURCES src/ipc/named_pipe_server.cpp)
else()
    list(APPEND CORE_SOURCES src/ipc/unix_socket_server.cpp)
endif()

# Legacy engine implementations (kept for compatibility)
file(GLOB LEGACY_SOURCES
    src/backup_engine/backup_job.cpp
    src/backup_engine/cbt.cpp
    src/backup_engine/dirty_buffer_manager.cpp
    src/backup_engine/dirty_buffer_logger.cpp
    src/backup_engine/incremental/*.cpp
    src/backup_engine/replication.h
    src/backup_engine/anti_ransomware.h
    src/backup_engine/integrity_checker.h
    src/backup_engine/restore_engine.h
    src/crypto/crypto_engine.cpp
    src/dedup/dedup_engine.cpp
    src/cdp/cdp_engine.cpp
    src/data_mover/data_mover.cpp
    src/data_mover/tcp_stream.cpp
    src/traffic_optimizer/traffic_optimizer.cpp
    src/replication/remote_replication.cpp
    src/replication_v2/vm_replication.cpp
    src/restore/cross_platform_restore.cpp
    src/dr/dr_orchestrator.cpp
    src/verify/surebackup_engine.cpp
    src/ha/ha_cluster.cpp
    src/k8s/k8s_backup.cpp
    src/malware/malware_scan.cpp
    src/ndmp/ndmp_backup.cpp
    src/immutability/object_lock.cpp
    src/scaleout/scaleout_repository.cpp
    src/sap_ora/sap_backup.cpp
    src/storage_snapshot/storage_snapshot_backup.cpp
    src/backup_copy/backup_copy_job.cpp
    src/tape_lib/tape_engine.cpp
    src/tape/tape_library.cpp
    src/os_backup/os_storage_adapters.cpp
    src/mail/mail_adapter.cpp
    src/block_device/block_reader.cpp
    src/dbms/postgres_adapter.cpp
    src/dbms/mysql_adapter.cpp
    src/dbms/mssql_adapter.cpp
    src/dbms/oracle_adapter.cpp
    src/dbms/mongodb_1c_adapters.cpp
    src/dbms/other_dbms_adapters.cpp
    src/incremental/common/incremental_engine.cpp
    src/incremental/common/bitmap_manager.cpp
    src/incremental/common/chain_management.cpp
    src/incremental/vmware/vmware_backup.cpp
    src/incremental/hyperv/hyperv_backup.cpp
    src/incremental/proxmox/proxmox_backup.cpp
    src/incremental/linux/linux_fs_backup.cpp
    src/incremental/window/windows_fs_backup.cpp
    src/virtualization/vmware_adapter.cpp
    src/virtualization/hyperv_adapter.cpp
    src/virtualization/proxmox_adapter.cpp
    src/virtualization/vm_manager.cpp
    src/virtualization/russian_adapters.cpp
    src/fast_clone/fast_clone.cpp
    src/rescue/rescue_image.cpp
    src/verification/vm_verification.cpp
    src/storage_tier/tier_manager.cpp
    src/antivirus/av_scanner.cpp
)

list(APPEND CORE_SOURCES ${LEGACY_SOURCES})

add_executable(vovqa-core ${CORE_SOURCES} ${PROTO_SRCS})

if(WIN32)
    target_link_libraries(vovqa-core
        ${Protobuf_LIBRARIES}
        ${Boost_LIBRARIES}
        OpenSSL::SSL
        OpenSSL::Crypto
        ZLIB::ZLIB
        ${SQLITE3_LIBRARIES}
        CURL::libcurl_static
        ${FMT_LIBRARY}
        ${SPDLOG_LIBRARY}
        ws2_32
        crypt32
        advapi32
        user32
        kernel32
        iphlpapi
        wsock32
    )
    target_compile_definitions(vovqa-core PRIVATE CURL_STATICLIB)
else()
    target_link_libraries(vovqa-core
        ${Protobuf_LIBRARIES}
        ${Boost_LIBRARIES}
        OpenSSL::SSL
        OpenSSL::Crypto
        ZLIB::ZLIB
        Threads::Threads
        ${SQLITE3_LIBRARIES}
        CURL::libcurl
        ${FMT_LIBRARY}
        ${SPDLOG_LIBRARY}
    )
endif()

target_compile_definitions(vovqa-core PRIVATE
    SPDLOG_FMT_EXTERNAL
    SPDLOG_COMPILED_LIB
)

if(WIN32)
    target_compile_definitions(vovqa-core PRIVATE
        _WIN32
        WIN32
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# ── Tests ─────────────────────────────────────────────────────────────────────

enable_testing()

add_executable(test_oop
    tests/test_oop.cpp
    ${PROTO_SRCS}
)

target_include_directories(test_oop PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${Protobuf_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIRS}
    ${SPDLOG_INCLUDE_DIR}
    ${FMT_INCLUDE_DIR}
    ${SQLITE3_INCLUDE_DIRS}
)

target_link_libraries(test_oop
    ${Protobuf_LIBRARIES}
    ${Boost_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
    Threads::Threads
    ${SQLITE3_LIBRARIES}
    CURL::libcurl
    ${FMT_LIBRARY}
    ${SPDLOG_LIBRARY}
)

target_compile_definitions(test_oop PRIVATE
    SPDLOG_FMT_EXTERNAL
    SPDLOG_COMPILED_LIB
)

# Include source files for the test binary
target_sources(test_oop PRIVATE
    src/engine/engine_factory.cpp
    src/engine/backup_pipeline.cpp
    src/agent/generic_agent.cpp
    src/agent/esxi_agent.cpp
    src/agent/hyperv_agent.cpp
    src/agent/mssql_agent.cpp
    src/agent/postgresql_agent.cpp
    src/agent/oracle_agent.cpp
    src/agent/proxmox_agent.cpp
    src/agent/saphana_agent.cpp
    src/agent/kubernetes_agent.cpp
    src/agent/linux_fs_agent.cpp
    src/agent/windows_fs_agent.cpp
    src/agent/ndmp_agent.cpp
    src/agent/exchange_agent.cpp
    src/ipc/message_router.cpp
    src/cdp/cdp_engine.cpp
    src/dbms/postgres_adapter.cpp
    src/dbms/mysql_adapter.cpp
    src/dbms/mssql_adapter.cpp
    src/dbms/oracle_adapter.cpp
    src/dbms/mongodb_1c_adapters.cpp
    src/dbms/other_dbms_adapters.cpp
    src/virtualization/vmware_adapter.cpp
    src/virtualization/hyperv_adapter.cpp
    src/virtualization/proxmox_adapter.cpp
    src/virtualization/vm_manager.cpp
    src/virtualization/russian_adapters.cpp
)

if(NOT WIN32)
    target_sources(test_oop PRIVATE src/ipc/unix_socket_server.cpp)
else()
    target_sources(test_oop PRIVATE src/ipc/named_pipe_server.cpp)
endif()

add_test(NAME oop_tests COMMAND test_oop)

# ── Agent Tests ───────────────────────────────────────────────────────────────

foreach(agent_test esxi mssql pgsql)
    add_executable(test_${agent_test}_agent
        tests/test_${agent_test}_agent.cpp
        src/agent/generic_agent.cpp
        src/agent/esxi_agent.cpp
        src/agent/hyperv_agent.cpp
        src/agent/mssql_agent.cpp
        src/agent/postgresql_agent.cpp
        src/agent/oracle_agent.cpp
        src/agent/proxmox_agent.cpp
        src/agent/saphana_agent.cpp
        src/agent/kubernetes_agent.cpp
        src/agent/linux_fs_agent.cpp
        src/agent/windows_fs_agent.cpp
        src/agent/ndmp_agent.cpp
        src/agent/exchange_agent.cpp
        src/dbms/postgres_adapter.cpp
        src/dbms/mysql_adapter.cpp
        src/dbms/mssql_adapter.cpp
        src/dbms/oracle_adapter.cpp
        src/dbms/mongodb_1c_adapters.cpp
        src/dbms/other_dbms_adapters.cpp
    )

    target_include_directories(test_${agent_test}_agent PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}
        ${Protobuf_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
        ${ZLIB_INCLUDE_DIRS}
        ${SPDLOG_INCLUDE_DIR}
        ${FMT_INCLUDE_DIR}
        ${SQLITE3_INCLUDE_DIRS}
    )

    target_link_libraries(test_${agent_test}_agent
        OpenSSL::SSL
        OpenSSL::Crypto
        ZLIB::ZLIB
        Threads::Threads
        ${SQLITE3_LIBRARIES}
        CURL::libcurl
        ${FMT_LIBRARY}
        ${SPDLOG_LIBRARY}
    )

    target_compile_definitions(test_${agent_test}_agent PRIVATE
        SPDLOG_FMT_EXTERNAL
        SPDLOG_COMPILED_LIB
    )

    add_test(NAME ${agent_test}_agent_tests COMMAND test_${agent_test}_agent)
endforeach()

# ── DirtyBufferLogger Test ─────────────────────────────────────────────────────

add_executable(test_dirty_buffer
    tests/test_dirty_buffer.cpp
    src/backup_engine/dirty_buffer_logger.cpp
)

target_include_directories(test_dirty_buffer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${Protobuf_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIRS}
    ${SPDLOG_INCLUDE_DIR}
    ${FMT_INCLUDE_DIR}
    ${SQLITE3_INCLUDE_DIRS}
)

target_link_libraries(test_dirty_buffer
    ${Protobuf_LIBRARIES}
    ${Boost_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
    Threads::Threads
    ${SQLITE3_LIBRARIES}
    CURL::libcurl
    ${FMT_LIBRARY}
    ${SPDLOG_LIBRARY}
)

target_compile_definitions(test_dirty_buffer PRIVATE
    SPDLOG_FMT_EXTERNAL
    SPDLOG_COMPILED_LIB
)

add_test(NAME dirty_buffer_tests COMMAND test_dirty_buffer)
