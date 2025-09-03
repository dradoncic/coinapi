#pragma once
#include <string>
#include <unordered_map>

std::string generate_nonce();

std::string build_jwt(const std::string& api_key, const std::string& pem_key);

std::unordered_map<std::string, std::string> read_env_file(const std::string& path);

std::string read_pem_file(const std::string& path);