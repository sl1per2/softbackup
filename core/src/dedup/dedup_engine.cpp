#include "dedup/dedup_engine.h"
#include <spdlog/spdlog.h>
#include <openssl/evp.h>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace obs {

static const uint32_t GEAR_TABLE[256] = {
    0x85e1875e, 0xd5c7a3d2, 0x8e6e9a41, 0xb9f8c673, 0x3e5a1d4c, 0x71b2e8f9,
    0x4a8d3f67, 0xc2e5a19b, 0x5f3d7c82, 0x94a6b1e3, 0x6d2f4895, 0xe1c7b3a8,
    0x2b89f4d6, 0xa73e61c5, 0x58d4f2b9, 0xc1e7a384, 0x96b52d4f, 0x43a8e761,
    0x8d2f5c93, 0xb4e1a7d6, 0x6f9c3482, 0xe2a5d1b7, 0x38b4f6e9, 0x7d1ca543,
    0x95e2b8f4, 0x4a67c3d1, 0xd8f3e5a2, 0x61b94c87, 0xb2d4a695, 0x3e87f1c4,
    0xa54c2d86, 0x79e1b3f5, 0xc6a84d92, 0x53d7f1e8, 0x8b42a6c3, 0x2ef5d917,
    0xa1c87b34, 0xd469e2f5, 0x87b3a1c6, 0x5e2d94f8, 0xb1c47a36, 0x3d8ef2a5,
    0x69a5c4d7, 0xe2f81b43, 0x47c3a6d9, 0x95b2e8f1, 0x68d4a3c7, 0xb3e1f5a2,
    0x2a96c4d8, 0xd1f5b3a7, 0x84e2c693, 0x57a9b3d4, 0xc8d1e5f6, 0x3b64a2c9,
    0x92f7e4b1, 0x6da5c384, 0xe4b8f1d2, 0x73a2c5e6, 0x58d4f1a9, 0xb6e3c2d7,
    0x2f95a4c8, 0xa1d7e3b6, 0xd8c4f5a2, 0x47b6e1c3, 0x93a2d5f4, 0x65c8b1e7,
    0xe2f4a3d9, 0x3b87c6a5, 0x71d9e2f4, 0xa4c6b5e8, 0x5e2a3d7f, 0xc8b1d4e6,
    0x96f3a2c5, 0x4d87e1b6, 0xb5c2f3a8, 0x27e9d4c1, 0x6a3f5b82, 0xd1c4e7a5,
    0x89b6f2d3, 0x54e1a3c7, 0x38f5d9b6, 0xc2a7e4d8, 0x7b34c1a5, 0xe6d9f2b3,
    0x91a5c8e4, 0x46b7d2f3, 0xd3e8a5c1, 0x6a29f4b7, 0x85c4d6e3, 0x37b1a9f5,
    0xb8e2c4d6, 0x5f93a1c7, 0xc4d7e6b2, 0x2a85f3d1, 0x96e4b7a3, 0x71c8d5f6,
};

DedupEngine::DedupEngine(const std::string& db_path) {
    if (sqlite3_open(db_path.c_str(), &db_) != SQLITE_OK) {
        spdlog::error("Failed to open dedup db: {}", sqlite3_errmsg(db_));
        db_ = nullptr;
        return;
    }
    init_db();
}

DedupEngine::~DedupEngine() {
    if (db_) sqlite3_close(db_);
}

void DedupEngine::init_db() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS chunks (
            hash TEXT PRIMARY KEY,
            size INTEGER NOT NULL,
            ref_count INTEGER DEFAULT 1,
            first_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
        CREATE INDEX IF NOT EXISTS idx_chunks_size ON chunks(size);
    )";
    std::lock_guard<std::mutex> lock(db_mutex_);
    char* err = nullptr;
    sqlite3_exec(db_, sql, nullptr, nullptr, &err);
    if (err) {
        spdlog::error("DB init error: {}", err);
        sqlite3_free(err);
    }
}

std::string DedupEngine::hash_chunk(const uint8_t* data, size_t size) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) return "";

    EVP_DigestInit_ex(ctx, EVP_blake2b512(), nullptr);
    EVP_DigestUpdate(ctx, data, size);

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    EVP_DigestFinal_ex(ctx, hash, &hash_len);
    EVP_MD_CTX_free(ctx);

    std::ostringstream oss;
    for (unsigned int i = 0; i < hash_len; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return oss.str();
}

std::optional<ChunkInfo> DedupEngine::lookup(const std::string& hash) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "SELECT hash, size, ref_count, first_seen FROM chunks WHERE hash = ?";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, hash.c_str(), -1, SQLITE_STATIC);

    std::optional<ChunkInfo> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        ChunkInfo info;
        info.hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        info.size = sqlite3_column_int(stmt, 1);
        info.ref_count = sqlite3_column_int(stmt, 2);
        info.first_seen = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        result = info;

        const char* update = "UPDATE chunks SET ref_count = ref_count + 1 WHERE hash = ?";
        sqlite3_stmt* upd = nullptr;
        sqlite3_prepare_v2(db_, update, -1, &upd, nullptr);
        sqlite3_bind_text(upd, 1, hash.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(upd);
        sqlite3_finalize(upd);
    }
    sqlite3_finalize(stmt);
    return result;
}

void DedupEngine::add_chunk(const std::string& hash, uint32_t size) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "INSERT OR IGNORE INTO chunks (hash, size) VALUES (?, ?)";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, size);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

DedupStats DedupEngine::get_stats() {
    std::lock_guard<std::mutex> lock(db_mutex_);
    DedupStats stats;
    const char* sql = "SELECT COUNT(*), SUM(size * (ref_count - 1)) FROM chunks";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        stats.unique_chunks = sqlite3_column_int64(stmt, 0);
        stats.saved_bytes = sqlite3_column_int64(stmt, 1);
        stats.total_chunks = stats.unique_chunks;
    }
    sqlite3_finalize(stmt);
    return stats;
}

uint32_t DedupEngine::fastcdc_boundary(const uint8_t* data, size_t size) {
    uint32_t h = 0;
    for (size_t i = 0; i < size && i < 32; i++) {
        h = (h << 1) + GEAR_TABLE[data[i]];
    }
    return h;
}

void DedupEngine::process_file(const std::string& path,
                               std::function<void(const uint8_t*, size_t, const std::string&)> callback) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        spdlog::error("Cannot open file for dedup: {}", path);
        return;
    }

    std::vector<uint8_t> buffer(MAX_CHUNK);
    size_t total_read = 0;

    while (ifs) {
        ifs.read(reinterpret_cast<char*>(buffer.data()), MAX_CHUNK);
        size_t bytes = ifs.gcount();
        if (bytes == 0) break;

        size_t split = bytes;
        if (bytes >= MIN_CHUNK) {
            for (size_t i = MIN_CHUNK; i < bytes && i <= MAX_CHUNK; i++) {
                uint32_t h = fastcdc_boundary(buffer.data() + i - 32, 32);
                if ((h & (AVG_CHUNK - 1)) == 0) {
                    split = i;
                    break;
                }
            }
        }

        std::string hash = hash_chunk(buffer.data(), split);
        if (!hash.empty()) {
            callback(buffer.data(), split, hash);
        }

        if (split < bytes) {
            ifs.seekg(total_read + split);
        }
        total_read += split;
    }
}

} // namespace obs
