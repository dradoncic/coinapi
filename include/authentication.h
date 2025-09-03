#include <string>
#include <unordered_map>


std::string base64UrlEncode(const std::string& input);

std::string signES256(const std::string& data, const std::string& pem_private_key);

std::string generateNonce();

std::string buildJWT(const std::string& api_key, const std::string& pem_key);

std::unordered_map<std::string, std::string> readEnvFile(const std::string& path);