cmake_minimum_required(VERSION 3.16)
project(vovqa-core VERSION 1.0.0 LANGUAGES CXX)

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

file(GLOB_RECURSE CORE_SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)

if(WIN32)
    list(FILTER CORE_SOURCES EXCLUDE REGEX "unix_socket_server\\.cpp$")
else()
    list(FILTER CORE_SOURCES EXCLUDE REGEX "named_pipe_server\\.cpp$")
endif()

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
