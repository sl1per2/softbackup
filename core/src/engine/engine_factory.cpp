#include "engine/engine_factory.h"
#include "engine/i_backup_engine.h"
#include "engine/i_dedup_engine.h"
#include "engine/i_crypto_engine.h"
#include "engine/i_compression.h"
#include "engine/i_transport.h"
#include <spdlog/spdlog.h>
#include <sqlite3.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <zlib.h>

namespace obs {

// ── BackupEngine implementation ──────────────────────────────────────────────

class DefaultBackupEngine : public IBackupEngine {
public:
    std::string component_name() const override { return "DefaultBackupEngine"; }

    void initialize() override { mark_initialized(); }

    BackupResult backup(const BackupRequest& request, EngineProgressCallback progress) override {
        BackupResult result;
        spdlog::info("DefaultBackupEngine: starting backup of {}", request.source_path);

        try {
            namespace fs = std::filesystem;
            if (!fs::exists(request.source_path)) {
                result.error = "Source path does not exist: " + request.source_path;
                return result;
            }

            uint64_t total_size = 0;
            std::vector<fs::path> files_to_backup;

            if (fs::is_regular_file(request.source_path)) {
                files_to_backup.push_back(request.source_path);
                total_size = fs::file_size(request.source_path);
            } else if (fs::is_directory(request.source_path)) {
                for (const auto& entry : fs::recursive_directory_iterator(request.source_path)) {
                    if (entry.is_regular_file()) {
                        bool excluded = false;
                        for (const auto& pattern : request.exclude_patterns) {
                            if (entry.path().string().find(pattern) != std::string::npos) {
                                excluded = true;
                                break;
                            }
                        }
                        if (!excluded) {
                            files_to_backup.push_back(entry.path());
                            total_size += entry.file_size();
                        }
                    }
                }
            }

            result.total_bytes = total_size;
            uint64_t processed = 0;

            for (const auto& file : files_to_backup) {
                std::ifstream ifs(file, std::ios::binary);
                if (!ifs) continue;

                std::vector<uint8_t> buffer(256 * 1024);
                while (ifs.read(reinterpret_cast<char*>(buffer.data()), buffer.size()) || ifs.gcount() > 0) {
                    size_t bytes_read = ifs.gcount();
                    processed += bytes_read;
                    result.transferred_bytes = processed;

                    if (progress) {
                        double pct = total_size > 0 ? (double(processed) / total_size) * 100.0 : 0.0;
                        progress(pct, processed);
                    }

                    if (!ifs) break;
                }
            }

            result.chunks_created = files_to_backup.size();
            result.success = true;

        } catch (const std::exception& e) {
            result.error = e.what();
            spdlog::error("DefaultBackupEngine error: {}", e.what());
        }

        return result;
    }

    bool cancel(const std::string& job_id) override {
        spdlog::info("DefaultBackupEngine: cancelling job {}", job_id);
        return true;
    }

    std::map<std::string, std::string> get_metrics() const override {
        return {
            {"engine", "default"},
            {"status", "ready"}
        };
    }
};

// ── DedupEngine implementation ───────────────────────────────────────────────

class DefaultDedupEngine : public IDedupEngine {
public:
    explicit DefaultDedupEngine(const std::string& db_path = "/tmp/obs_chunks.db")
        : db_path_(db_path) {
        int rc = sqlite3_open(db_path_.c_str(), &db_);
        if (rc != SQLITE_OK) {
            spdlog::error("Failed to open dedup database: {}", sqlite3_errmsg(db_));
            return;
        }
        const char* sql = "CREATE TABLE IF NOT EXISTS chunks ("
            "hash TEXT PRIMARY KEY, data BLOB, size INTEGER, ref_count INTEGER DEFAULT 1, "
            "first_seen TEXT)";
        sqlite3_exec(db_, sql, nullptr, nullptr, nullptr);
    }

    ~DefaultDedupEngine() override { close(); }

    std::string component_name() const override { return "DefaultDedupEngine"; }

    void initialize() override { mark_initialized(); }

    void shutdown() override { close(); }

    std::string hash_data(const uint8_t* data, size_t size) override {
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
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        return oss.str();
    }

    bool chunk_exists(const std::string& hash) override {
        std::lock_guard<std::mutex> lock(mutex_);
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db_, "SELECT 1 FROM chunks WHERE hash = ?", -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, hash.c_str(), -1, SQLITE_STATIC);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return rc == SQLITE_ROW;
    }

    void store_chunk(const std::string& hash, const uint8_t* data, size_t size) override {
        bool is_new = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            sqlite3_stmt* stmt;
            sqlite3_prepare_v2(db_,
                "INSERT OR IGNORE INTO chunks (hash, data, size, ref_count, first_seen) VALUES (?, ?, ?, 1, datetime('now'))",
                -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, hash.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_blob(stmt, 2, data, size, SQLITE_STATIC);
            sqlite3_bind_int64(stmt, 3, size);
            sqlite3_step(stmt);
            is_new = sqlite3_changes(db_) > 0;
            sqlite3_finalize(stmt);
        }
        total_chunks_++;
        if (is_new) unique_chunks_++;
        bytes_saved_ += size;
    }

    std::optional<ChunkData> retrieve_chunk(const std::string& hash) override {
        std::lock_guard<std::mutex> lock(mutex_);
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db_, "SELECT data, size FROM chunks WHERE hash = ?", -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, hash.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            ChunkData chunk;
            chunk.hash = hash;
            chunk.size = sqlite3_column_int(stmt, 1);
            const void* blob = sqlite3_column_blob(stmt, 0);
            chunk.data.assign(static_cast<const uint8_t*>(blob),
                             static_cast<const uint8_t*>(blob) + chunk.size);
            sqlite3_finalize(stmt);
            return chunk;
        }
        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    size_t total_chunks() const override { return total_chunks_; }
    size_t unique_chunks() const override { return unique_chunks_; }
    uint64_t bytes_saved() const override { return bytes_saved_; }

    void process_file(const std::string& path,
                      std::function<void(const uint8_t*, size_t, const std::string&)> callback) override {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs) return;

        std::vector<uint8_t> buffer(256 * 1024);
        while (ifs.read(reinterpret_cast<char*>(buffer.data()), buffer.size()) || ifs.gcount() > 0) {
            size_t bytes_read = ifs.gcount();
            std::string hash = hash_data(buffer.data(), bytes_read);
            callback(buffer.data(), bytes_read, hash);
            if (!ifs) break;
        }
    }

private:
    void close() {
        if (db_) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
    }

    std::string db_path_;
    sqlite3* db_ = nullptr;
    mutable std::mutex mutex_;
    size_t total_chunks_ = 0;
    size_t unique_chunks_ = 0;
    uint64_t bytes_saved_ = 0;
};

// ── CryptoEngine implementation ──────────────────────────────────────────────

class DefaultCryptoEngine : public ICryptoEngine {
public:
    std::string component_name() const override { return "DefaultCryptoEngine"; }
    void initialize() override { mark_initialized(); }

    std::vector<uint8_t> encrypt(const uint8_t* data, size_t size, const std::string& key_hex) override {
        auto key = hex_to_bytes(key_hex);
        if (key.size() != 32) throw std::runtime_error("AES key must be 256 bits");

        std::vector<uint8_t> iv(12);
        RAND_bytes(iv.data(), 12);

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr);
        EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data());

        std::vector<uint8_t> ciphertext(size + 16);
        int out_len = 0;
        EVP_EncryptUpdate(ctx, ciphertext.data(), &out_len, data, size);

        int final_len = 0;
        EVP_EncryptFinal_ex(ctx, ciphertext.data() + out_len, &final_len);
        out_len += final_len;

        std::vector<uint8_t> tag(16);
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag.data());
        EVP_CIPHER_CTX_free(ctx);

        std::vector<uint8_t> result;
        result.insert(result.end(), iv.begin(), iv.end());
        result.insert(result.end(), ciphertext.begin(), ciphertext.begin() + out_len);
        result.insert(result.end(), tag.begin(), tag.end());
        return result;
    }

    std::vector<uint8_t> decrypt(const uint8_t* data, size_t size, const std::string& key_hex) override {
        auto key = hex_to_bytes(key_hex);
        if (key.size() != 32) throw std::runtime_error("AES key must be 256 bits");
        if (size < 28) throw std::runtime_error("Ciphertext too short");

        const uint8_t* iv = data;
        const uint8_t* ciphertext = data + 12;
        size_t ct_len = size - 12 - 16;
        const uint8_t* tag = data + 12 + ct_len;

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr);
        EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), iv);

        std::vector<uint8_t> plaintext(ct_len + 16);
        int out_len = 0;
        EVP_DecryptUpdate(ctx, plaintext.data(), &out_len, ciphertext, ct_len);

        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, const_cast<uint8_t*>(tag));
        int ret = EVP_DecryptFinal_ex(ctx, plaintext.data() + out_len, &out_len);
        EVP_CIPHER_CTX_free(ctx);

        if (ret <= 0) throw std::runtime_error("Decryption failed - wrong key or corrupted data");
        plaintext.resize(out_len);
        return plaintext;
    }

    std::string generate_key() override {
        std::vector<uint8_t> key(32);
        RAND_bytes(key.data(), 32);
        return bytes_to_hex(key.data(), 32);
    }

    std::string hash(const uint8_t* data, size_t size) override {
        return blake2b512(data, size);
    }

    std::string wrap_key(const std::string& data_key_hex, const std::string& rsa_pub_pem) override {
        BIO* bio = BIO_new_mem_buf(rsa_pub_pem.c_str(), -1);
        EVP_PKEY* pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
        BIO_free(bio);
        if (!pkey) throw std::runtime_error("Failed to read RSA public key");

        auto data_key = hex_to_bytes(data_key_hex);
        std::vector<uint8_t> wrapped(EVP_PKEY_size(pkey));
        size_t wrapped_len = 0;

        EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
        EVP_PKEY_encrypt_init(ctx);
        EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);
        EVP_PKEY_encrypt(ctx, wrapped.data(), &wrapped_len, data_key.data(), data_key.size());
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);

        return bytes_to_hex(wrapped.data(), wrapped_len);
    }

    std::string unwrap_key(const std::string& wrapped_hex, const std::string& rsa_priv_pem) override {
        BIO* bio = BIO_new_mem_buf(rsa_priv_pem.c_str(), -1);
        EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
        BIO_free(bio);
        if (!pkey) throw std::runtime_error("Failed to read RSA private key");

        auto wrapped = hex_to_bytes(wrapped_hex);
        std::vector<uint8_t> unwrapped(EVP_PKEY_size(pkey));
        size_t unwrapped_len = 0;

        EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
        EVP_PKEY_decrypt_init(ctx);
        EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);
        EVP_PKEY_decrypt(ctx, unwrapped.data(), &unwrapped_len, wrapped.data(), wrapped.size());
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);

        return bytes_to_hex(unwrapped.data(), unwrapped_len);
    }

    std::string generate_rsa_keypair(int bits = 4096) override {
        EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
        EVP_PKEY_keygen_init(ctx);
        EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, bits);

        EVP_PKEY* pkey = nullptr;
        EVP_PKEY_keygen(ctx, &pkey);
        EVP_PKEY_CTX_free(ctx);

        BIO* pub_bio = BIO_new(BIO_s_mem());
        PEM_write_bio_PUBKEY(pub_bio, pkey);
        char* pub_data;
        long pub_len = BIO_get_mem_data(pub_bio, &pub_data);
        std::string pub_pem(pub_data, pub_len);
        BIO_free(pub_bio);

        BIO* priv_bio = BIO_new(BIO_s_mem());
        PEM_write_bio_PrivateKey(priv_bio, pkey, nullptr, nullptr, 0, nullptr, nullptr);
        char* priv_data;
        long priv_len = BIO_get_mem_data(priv_bio, &priv_data);
        std::string priv_pem(priv_data, priv_len);
        BIO_free(priv_bio);

        EVP_PKEY_free(pkey);
        return pub_pem + "\n---PRIVATE_KEY---\n" + priv_pem;
    }

    std::vector<uint8_t> generate_salt() override {
        std::vector<uint8_t> salt(16);
        RAND_bytes(salt.data(), 16);
        return salt;
    }

private:
    std::string bytes_to_hex(const uint8_t* data, size_t len) {
        std::ostringstream oss;
        for (size_t i = 0; i < len; i++) {
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
        }
        return oss.str();
    }

    std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
        std::vector<uint8_t> bytes;
        for (size_t i = 0; i < hex.size(); i += 2) {
            bytes.push_back(static_cast<uint8_t>(std::stoi(hex.substr(i, 2), nullptr, 16)));
        }
        return bytes;
    }

    std::string blake2b512(const uint8_t* data, size_t size) {
        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        EVP_DigestInit_ex(ctx, EVP_blake2b512(), nullptr);
        EVP_DigestUpdate(ctx, data, size);
        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int hash_len = 0;
        EVP_DigestFinal_ex(ctx, hash, &hash_len);
        EVP_MD_CTX_free(ctx);
        return bytes_to_hex(hash, hash_len);
    }
};

// ── CompressionEngine implementation ─────────────────────────────────────────

class ZstdCompressionEngine : public ICompressionEngine {
public:
    std::string component_name() const override { return "ZstdCompressionEngine"; }
    void initialize() override { mark_initialized(); }

    std::vector<uint8_t> compress(const uint8_t* data, size_t size, int level = 1) override {
        uLongf bound = compressBound(size);
        std::vector<uint8_t> output(bound);
        uLongf out_size = bound;
        int ret = compress2(output.data(), &out_size, data, size, level);
        if (ret != Z_OK) throw std::runtime_error("zlib compression failed");
        output.resize(out_size);
        return output;
    }

    std::vector<uint8_t> decompress(const uint8_t* data, size_t size) override {
        std::vector<uint8_t> output(size * 4);
        uLongf out_size = output.size();
        int ret = uncompress(output.data(), &out_size, data, size);
        if (ret != Z_OK) throw std::runtime_error("zlib decompression failed");
        output.resize(out_size);
        return output;
    }

    CompressionAlgorithm algorithm() const override { return CompressionAlgorithm::DEFLATE; }

    double estimate_ratio(const uint8_t* data, size_t size) override {
        auto compressed = compress(data, size, 1);
        return static_cast<double>(compressed.size()) / size;
    }
};

// ── TransportEngine implementation ───────────────────────────────────────────

class DefaultTransportEngine : public ITransportEngine {
public:
    std::string component_name() const override { return "DefaultTransportEngine"; }
    void initialize() override { mark_initialized(); }

    bool transfer(const TransferRequest& request, TransferProgressCallback callback) override {
        spdlog::info("DefaultTransportEngine: transferring {} -> {}",
            request.source_path, request.destination);

        namespace fs = std::filesystem;
        if (!fs::exists(request.source_path)) return false;

        try {
            uint64_t file_size = fs::file_size(request.source_path);
            std::ifstream ifs(request.source_path, std::ios::binary);
            std::ofstream ofs(request.destination, std::ios::binary);

            std::vector<uint8_t> buffer(request.stream_count > 0 ?
                request.stream_count * 64 * 1024 : 256 * 1024);
            uint64_t transferred = 0;

            while (ifs.read(reinterpret_cast<char*>(buffer.data()), buffer.size()) || ifs.gcount() > 0) {
                size_t bytes_read = ifs.gcount();
                ofs.write(reinterpret_cast<char*>(buffer.data()), bytes_read);
                transferred += bytes_read;

                if (callback) {
                    TransferProgress progress;
                    progress.total_bytes = file_size;
                    progress.transferred_bytes = transferred;
                    progress.speed_mbps = 0;
                    callback(progress);
                }

                if (!ifs) break;
            }

            return true;
        } catch (const std::exception& e) {
            spdlog::error("Transfer failed: {}", e.what());
            return false;
        }
    }

    bool cancel(const std::string& job_id) override {
        spdlog::info("DefaultTransportEngine: cancelling transfer {}", job_id);
        return true;
    }

    TransportMode detect_best_mode(const std::string& source, const std::string& dest) override {
        return TransportMode::NETWORK;
    }
};

// ── EngineFactory ────────────────────────────────────────────────────────────

std::shared_ptr<IBackupEngine> EngineFactory::create_backup_engine(const std::string& data_dir) {
    return std::make_shared<DefaultBackupEngine>();
}

std::shared_ptr<IDedupEngine> EngineFactory::create_dedup_engine(const std::string& cache_dir) {
    return std::make_shared<DefaultDedupEngine>(cache_dir + "/chunks.db");
}

std::shared_ptr<ICryptoEngine> EngineFactory::create_crypto_engine() {
    return std::make_shared<DefaultCryptoEngine>();
}

std::shared_ptr<ICompressionEngine> EngineFactory::create_compression_engine() {
    return std::make_shared<ZstdCompressionEngine>();
}

std::shared_ptr<ITransportEngine> EngineFactory::create_transport_engine() {
    return std::make_shared<DefaultTransportEngine>();
}

} // namespace obs
