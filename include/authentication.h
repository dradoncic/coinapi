#pragma once
#include <string>
#include <unordered_map>

class Authenticator {
public:
    Authenticator(const std::string& env_path, const std::string& pem_path);
    std::string build_jwt();
    void fetch_api_keys();

private:
    std::string generate_nonce();

    std::unordered_map<std::string, std::string> read_env_file(const std::string& path);
    std::string read_pem_file(const std::string& path);

    std::string api_key_;
    std::string pem_key_;

    std::string env_path_;
    std::string pem_path_;
};