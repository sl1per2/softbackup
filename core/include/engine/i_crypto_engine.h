#pragma once
#include "common.h"
#include "framework/component.h"

namespace obs {

class ICryptoEngine : public IComponent {
public:
    virtual std::vector<uint8_t> encrypt(const uint8_t* data, size_t size, const std::string& key_hex) = 0;
    virtual std::vector<uint8_t> decrypt(const uint8_t* data, size_t size, const std::string& key_hex) = 0;
    virtual std::string generate_key() = 0;
    virtual std::string hash(const uint8_t* data, size_t size) = 0;
    virtual std::string wrap_key(const std::string& data_key_hex, const std::string& rsa_pub_pem) = 0;
    virtual std::string unwrap_key(const std::string& wrapped_hex, const std::string& rsa_priv_pem) = 0;
    virtual std::string generate_rsa_keypair(int bits = 4096) = 0;
    virtual std::vector<uint8_t> generate_salt() = 0;
};

} // namespace obs
