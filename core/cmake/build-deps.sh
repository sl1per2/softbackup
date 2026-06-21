#!/bin/bash
set -e

TARGET_ARCH=${1:-x86_64}

if [ "$TARGET_ARCH" = "x86_64" ]; then
    HOST=x86_64-w64-mingw32
    PREFIX=/deps/install/x86_64
else
    HOST=i686-w64-mingw32
    PREFIX=/deps/install/i686
fi

mkdir -p /deps/src $PREFIX

cd /deps/src

# ZLIB
wget -q https://zlib.net/zlib-1.3.1.tar.gz
tar xzf zlib-1.3.1.tar.gz
cd zlib-1.3.1
CC="${HOST}-gcc" AR="${HOST}-ar" RANLIB="${HOST}-ranlib" \
    ./configure --static --prefix=$PREFIX
make -j$(nproc) && make install
cd ..

# OpenSSL
wget -q https://www.openssl.org/source/openssl-3.2.1.tar.gz
tar xzf openssl-3.2.1.tar.gz
cd openssl-3.2.1
./Configure mingw64 \
    --cross-compile-prefix=${HOST}- \
    --prefix=$PREFIX \
    --openssldir=$PREFIX \
    no-shared no-ssl2 no-ssl3 no-dso
make -j$(nproc) && make install_sw
cd ..

# SQLite (static)
wget -q https://www.sqlite.org/2024/sqlite-amalgamation-3450100.zip
unzip -qo sqlite-amalgamation-3450100.zip
cd sqlite-amalgamation-3450100
${HOST}-gcc -c sqlite3.c -DSQLITE_THREADSAFE=1 -o sqlite3.o
${HOST}-ar rcs libsqlite3.a sqlite3.o
cp sqlite3.h $PREFIX/include/
cp libsqlite3.a $PREFIX/lib/
cd ..

# Boost (headers only, use system packages for libs)
wget -q https://boostorg.jfrog.io/artifactory/main/release/1.83.0/source/boost_1_83_0.tar.gz
tar xzf boost_1_83_0.tar.gz
cd boost_1_83_0
./bootstrap.sh --with-toolset=mingw
./b2 toolset=gcc-mingw \
    target-os=windows \
    binary-format=pe \
    architecture=x86 \
    address-model=$(if [ "$TARGET_ARCH" = "x86_64" ]; then echo 64; else echo 32; fi) \
    --prefix=$PREFIX \
    --with-system --with-filesystem --with-thread --with-program_options \
    link=static threading=multi variant=release \
    install -j$(nproc)
cd ..

# fmt
wget -q https://github.com/fmtlib/fmt/archive/refs/tags/10.2.1.tar.gz
tar xzf 10.2.1.tar.gz
cd fmt-10.2.1
mkdir build && cd build
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=/build/core/cmake/mingw-w64-${TARGET_ARCH}.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$PREFIX \
    -DFMT_TEST=OFF -DFMT_DOC=OFF
make -j$(nproc) && make install
cd ../..

# spdlog
wget -q https://github.com/gabime/spdlog/archive/refs/tags/v1.13.0.tar.gz
tar xzf v1.13.0.tar.gz
cd spdlog-1.13.0
mkdir build && cd build
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=/build/core/cmake/mingw-w64-${TARGET_ARCH}.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$PREFIX \
    -DSPDLOG_BUILD_EXAMPLE=OFF \
    -DSPDLOG_BUILD_TESTS=OFF \
    -DSPDLOG_FMT_EXTERNAL=ON
make -j$(nproc) && make install
cd ../..

# nlohmann-json (header only)
wget -q https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
mkdir -p $PREFIX/include/nlohmann
cp json.hpp $PREFIX/include/nlohmann/

# Protobuf (cross-compile host protoc first, then target)
wget -q https://github.com/protocolbuffers/protobuf/releases/download/v25.1/protobuf-25.1.tar.gz
tar xzf protobuf-25.1.tar.gz
cd protobuf-25.1
mkdir build-native && cd build-native
cmake ../cmake -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_PROTOC_BINARIES=ON
make -j$(nproc) && make install
cd ..
mkdir build-cross && cd build-cross
cmake ../cmake \
    -DCMAKE_TOOLCHAIN_FILE=/build/core/cmake/mingw-w64-${TARGET_ARCH}.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$PREFIX \
    -Dprotobuf_BUILD_TESTS=OFF \
    -Dprotobuf_BUILD_PROTOC_BINARIES=OFF \
    -Dprotobuf_BUILD_SHARED_LIBS=OFF \
    -Dprotobuf_WITH_ZLIB=ON \
    -DCMAKE_PREFIX_PATH=$PREFIX \
    -DCMAKE_FIND_ROOT_PATH="$PREFIX"
make -j$(nproc) && make install
cd ../..

# CURL
wget -q https://curl.se/download/curl-8.5.0.tar.gz
tar xzf curl-8.5.0.tar.gz
cd curl-8.5.0
mkdir build && cd build
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=/build/core/cmake/mingw-w64-${TARGET_ARCH}.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$PREFIX \
    -DCMAKE_PREFIX_PATH=$PREFIX \
    -DCMAKE_FIND_ROOT_PATH="$PREFIX" \
    -DCURL_STATICLIB=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DCURL_USE_OPENSSL=ON \
    -DCURL_ZLIB=ON
make -j$(nproc) && make install
cd ../..

# libsodium
wget -q https://github.com/jedisct1/libsodium/releases/download/1.0.19-RELEASE/libsodium-1.0.19.tar.gz
tar xzf libsodium-1.0.19.tar.gz
cd libsodium-1.0.19
./configure --host=$HOST --prefix=$PREFIX --disable-shared --enable-static
make -j$(nproc) && make install
cd ..

# zstd
wget -q https://github.com/facebook/zstd/releases/download/v1.5.5/zstd-1.5.5.tar.gz
tar xzf zstd-1.5.5.tar.gz
cd zstd-1.5.5
mkdir build && cd build
cmake ../build/cmake \
    -DCMAKE_TOOLCHAIN_FILE=/build/core/cmake/mingw-w64-${TARGET_ARCH}.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$PREFIX \
    -DCMAKE_PREFIX_PATH=$PREFIX \
    -DCMAKE_FIND_ROOT_PATH="$PREFIX" \
    -DZSTD_BUILD_PROGRAMS=OFF \
    -DZSTD_BUILD_SHARED=OFF \
    -DZSTD_BUILD_STATIC=ON
make -j$(nproc) && make install
cd ../..

# lz4
wget -q https://github.com/lz4/lz4/archive/refs/tags/v1.9.4.tar.gz
tar xzf v1.9.4.tar.gz
cd lz4-1.9.4
mkdir build && cd build
cmake ../build/cmake \
    -DCMAKE_TOOLCHAIN_FILE=/build/core/cmake/mingw-w64-${TARGET_ARCH}.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$PREFIX \
    -DCMAKE_PREFIX_PATH=$PREFIX \
    -DCMAKE_FIND_ROOT_PATH="$PREFIX" \
    -DLZ4_BUILD_PROGRAMS=OFF \
    -DBUILD_SHARED_LIBS=OFF
make -j$(nproc) && make install
cd ../..

# blake3
git clone --depth 1 https://github.com/BLAKE3-team/BLAKE3.git
cd BLAKE3
mkdir build && cd build
cmake ../c \
    -DCMAKE_TOOLCHAIN_FILE=/build/core/cmake/mingw-w64-${TARGET_ARCH}.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$PREFIX \
    -DBUILD_SHARED_LIBS=OFF
make -j$(nproc) && make install
cd ../..

echo "=== All dependencies built for ${TARGET_ARCH} ==="
ls -la $PREFIX/lib/
