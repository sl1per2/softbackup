#pragma once
#include "common.h"
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/rand.h>

namespace obs {

class CryptoEngine {
public:
    CryptoEngine();
    ~CryptoEngine();

    std::vector<uint8_t> encrypt(const uint8_t* data, size_t size, const std::string& key_hex);
    std::vector<uint8_t> decrypt(const uint8_t* data, size_t size, const std::string& key_hex);
    std::string generate_key();
    std::string blake3_hash(const uint8_t* data, size_t size);
    std::string wrap_key(const std::string& aes_key_hex, const std::string& rsa_pub_pem);
    std::string unwrap_key(const std::string& wrapped_hex, const std::string& rsa_priv_pem);
    std::string generate_rsa_keypair(int bits = 4096);
    std::vector<uint8_t> generate_salt();

private:
    std::string bytes_to_hex(const uint8_t* data, size_t len);
    std::vector<uint8_t> hex_to_bytes(const std::string& hex);
};

} // namespace obs
