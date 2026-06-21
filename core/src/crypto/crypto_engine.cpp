#include "crypto/crypto_engine.h"
#include <spdlog/spdlog.h>
#include <sstream>
#include <iomanip>

namespace obs {

CryptoEngine::CryptoEngine() {}
CryptoEngine::~CryptoEngine() {}

std::string CryptoEngine::bytes_to_hex(const uint8_t* data, size_t len) {
    std::ostringstream oss;
    for (size_t i = 0; i < len; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    }
    return oss.str();
}

std::vector<uint8_t> CryptoEngine::hex_to_bytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        bytes.push_back(static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
    }
    return bytes;
}

std::string CryptoEngine::generate_key() {
    uint8_t key[32];
    RAND_bytes(key, sizeof(key));
    return bytes_to_hex(key, sizeof(key));
}

std::vector<uint8_t> CryptoEngine::generate_salt() {
    std::vector<uint8_t> salt(16);
    RAND_bytes(salt.data(), salt.size());
    return salt;
}

std::vector<uint8_t> CryptoEngine::encrypt(const uint8_t* data, size_t size, const std::string& key_hex) {
    auto key = hex_to_bytes(key_hex);
    if (key.size() != 32) throw std::runtime_error("Invalid key size");

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Failed to create cipher context");

    std::vector<uint8_t> iv(12);
    RAND_bytes(iv.data(), iv.size());

    std::vector<uint8_t> ciphertext(size + 16);
    int out_len = 0, total_len = 0;

    EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr);
    EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data());
    EVP_EncryptUpdate(ctx, ciphertext.data(), &out_len, data, size);
    total_len = out_len;
    EVP_EncryptFinal_ex(ctx, ciphertext.data() + total_len, &out_len);
    total_len += out_len;

    std::vector<uint8_t> tag(16);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag.data());
    EVP_CIPHER_CTX_free(ctx);

    std::vector<uint8_t> result;
    result.insert(result.end(), iv.begin(), iv.end());
    result.insert(result.end(), ciphertext.begin(), ciphertext.begin() + total_len);
    result.insert(result.end(), tag.begin(), tag.end());
    return result;
}

std::vector<uint8_t> CryptoEngine::decrypt(const uint8_t* data, size_t size, const std::string& key_hex) {
    auto key = hex_to_bytes(key_hex);
    if (key.size() != 32) throw std::runtime_error("Invalid key size");
    if (size < 28) throw std::runtime_error("Invalid encrypted data");

    const uint8_t* iv = data;
    const uint8_t* ciphertext = data + 12;
    const uint8_t* tag = data + size - 16;
    size_t ct_len = size - 12 - 16;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    std::vector<uint8_t> plaintext(ct_len + 16);
    int out_len = 0, total_len = 0;

    EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr);
    EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), iv);
    EVP_DecryptUpdate(ctx, plaintext.data(), &out_len, ciphertext, ct_len);
    total_len = out_len;

    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, const_cast<uint8_t*>(tag));
    int ret = EVP_DecryptFinal_ex(ctx, plaintext.data() + total_len, &out_len);
    EVP_CIPHER_CTX_free(ctx);

    if (ret <= 0) throw std::runtime_error("Decryption failed (authentication)");
    total_len += out_len;
    plaintext.resize(total_len);
    return plaintext;
}

std::string CryptoEngine::wrap_key(const std::string& aes_key_hex, const std::string& rsa_pub_pem) {
    auto aes_key = hex_to_bytes(aes_key_hex);

    BIO* bio = BIO_new_mem_buf(rsa_pub_pem.data(), rsa_pub_pem.size());
    EVP_PKEY* pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if (!pkey) throw std::runtime_error("Failed to parse RSA public key");

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
    EVP_PKEY_encrypt_init(ctx);
    size_t out_len = 0;
    EVP_PKEY_encrypt(ctx, nullptr, &out_len, aes_key.data(), aes_key.size());
    std::vector<uint8_t> wrapped(out_len);
    EVP_PKEY_encrypt(ctx, wrapped.data(), &out_len, aes_key.data(), aes_key.size());
    wrapped.resize(out_len);

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    return bytes_to_hex(wrapped.data(), wrapped.size());
}

std::string CryptoEngine::unwrap_key(const std::string& wrapped_hex, const std::string& rsa_priv_pem) {
    auto wrapped = hex_to_bytes(wrapped_hex);

    BIO* bio = BIO_new_mem_buf(rsa_priv_pem.data(), rsa_priv_pem.size());
    EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if (!pkey) throw std::runtime_error("Failed to parse RSA private key");

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
    EVP_PKEY_decrypt_init(ctx);
    size_t out_len = 0;
    EVP_PKEY_decrypt(ctx, nullptr, &out_len, wrapped.data(), wrapped.size());
    std::vector<uint8_t> key(out_len);
    EVP_PKEY_decrypt(ctx, key.data(), &out_len, wrapped.data(), wrapped.size());
    key.resize(out_len);

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    return bytes_to_hex(key.data(), key.size());
}

std::string CryptoEngine::generate_rsa_keypair(int bits) {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    EVP_PKEY_keygen_init(ctx);
    EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, bits);
    EVP_PKEY* pkey = nullptr;
    EVP_PKEY_keygen(ctx, &pkey);
    EVP_PKEY_CTX_free(ctx);

    BIO* bio = BIO_new(BIO_s_mem());
    PEM_write_bio_PrivateKey(bio, pkey, nullptr, nullptr, 0, nullptr, nullptr);
    char* priv_data;
    long priv_len = BIO_get_mem_data(bio, &priv_data);
    std::string priv_pem(priv_data, priv_len);
    BIO_free(bio);

    EVP_PKEY_free(pkey);
    return priv_pem;
}

std::string CryptoEngine::blake3_hash(const uint8_t* data, size_t size) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_blake2b512(), nullptr);
    EVP_DigestUpdate(ctx, data, size);
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    EVP_DigestFinal_ex(ctx, hash, &hash_len);
    EVP_MD_CTX_free(ctx);
    return bytes_to_hex(hash, hash_len);
}

} // namespace obs
