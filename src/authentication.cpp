#include <fstream>
#include <sstream>
#include <ctime>
#include <chrono>
#include <random>
#include <jwt-cpp/jwt.h>
#include <openssl/sha.h>
#include "authentication.h"

Authenticator::Authenticator(const std::string& env_path, const std::string& pem_path) :
                                env_path_(env_path),
                                pem_path_(pem_path)
{
    fetch_api_keys();
}

std::string Authenticator::generate_nonce() 
{
    unsigned char bytes[16];
    std::random_device rd;
    for (int i = 0; i < 16; i++) {
        bytes[i] = rd() % 256;
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(bytes, 16, hash);

    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return oss.str();
}

std::string Authenticator::build_jwt() 
{

    auto token = jwt::create()
        .set_issuer("coinbase-cloud")
        .set_subject(api_key_)
        .set_not_before(std::chrono::system_clock::now())
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::seconds{120})
        .set_header_claim("kid", jwt::claim(std::string(api_key_)))
        .set_header_claim("nonce", jwt::claim(generate_nonce()))
        .sign(jwt::algorithm::es256{"", pem_key_});

    return token;
}

void Authenticator::fetch_api_keys()
{
    api_key_ = read_env_file(env_path_)["API_KEY"];
    pem_key_ = read_pem_file(pem_path_);
}


std::unordered_map<std::string, std::string> Authenticator::read_env_file(const std::string& path) 
{
    std::unordered_map<std::string, std::string> env;
    std::ifstream file(path);
    if (!file.is_open()) throw std::runtime_error("Cannot open .env file");

    std::string line;
    while (std::getline(file, line)) {

        line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
        if (line.empty() || line[0]=='#') continue;

        auto pos = line.find('=');
        if (pos == std::string::npos) continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos+1);

        // remove quotes if present
        if (!value.empty() && value.front()=='"' && value.back()=='"')
            value = value.substr(1, value.size()-2);

        env[key] = value;
    }
    return env;
}

std::string Authenticator::read_pem_file(const std::string& path) 
{
    std::ifstream file(path);
    if (!file.is_open()) throw std::runtime_error("Cannot open PEM file");

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}