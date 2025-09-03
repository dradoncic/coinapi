#include <string>
#include <unordered_map>
#include <openssl/sha.h>


std::string base64_url_encode(const std::string& input);

std::string sign_ES256(const std::string& data, const std::string& pem_private_key);

std::string generate_nonce();

std::string build_JWT(const std::string& api_key, const std::string& pem_key);

std::unordered_map<std::string, std::string> read_env_file(const std::string& path);

std::string read_pem_file(const std::string& path);