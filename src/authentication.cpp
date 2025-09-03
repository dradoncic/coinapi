#include <ctime>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include "authentication.h"


std::string base64_url_encode(const std::string& input)
{
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO *bio = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bio);
    BIO_write(b64, input.data(), input.size());
    BIO_flush(b64);

    BUF_MEM *bufferPtr;
    BIO_get_mem_ptr(b64, &bufferPtr);

    std::string b64str(bufferPtr->data, bufferPtr->length);
    BIO_free_all(b64);

    // Replace + with -, / with _, remove =
    for(auto &c : b64str){
        if(c=='+') c='-';
        else if(c=='/') c='_';
    }
    b64str.erase(std::remove(b64str.begin(), b64str.end(), '='), b64str.end());
    return b64str;
}

std::string sign_ES256(const std::string& data, const std::string& pem_private_key) 
{
    BIO *bio = BIO_new_mem_buf(pem_private_key.data(), pem_private_key.size());
    EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if(!pkey) throw std::runtime_error("Failed to read private key");

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestSignInit(ctx, nullptr, EVP_sha256(), nullptr, pkey);
    EVP_DigestSignUpdate(ctx, data.data(), data.size());

    size_t siglen;
    EVP_DigestSignFinal(ctx, nullptr, &siglen);
    std::string signature(siglen, 0);
    EVP_DigestSignFinal(ctx, (unsigned char*)signature.data(), &siglen);
    signature.resize(siglen);

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);

    return base64_url_encode(signature);
}


std::string generate_nonce() 
{
    std::random_device rd;
    unsigned char bytes[16];
    for(int i=0;i<16;i++) bytes[i] = rd()%256;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(bytes,16,hash);
    std::ostringstream oss;
    for(int i=0;i<SHA256_DIGEST_LENGTH;i++)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return oss.str();
}

std::string build_JWT(const std::string& api_key, const std::string& pem_key) 
{
    nlohmann::json header = {
        {"alg", "ES256"},
        {"kid", api_key},
        {"nonce", generate_nonce()}
    };
    nlohmann::json payload = {
        {"iss", "coinbase-cloud"},
        {"sub", api_key},
        {"nbf", std::time(nullptr)},
        {"exp", std::time(nullptr)+120}
    };

    std::string headerStr = base64_url_encode(header.dump());
    std::string payloadStr = base64_url_encode(payload.dump());
    std::string signing_input = headerStr + "." + payloadStr;

    std::string signature = sign_ES256(signing_input, pem_key);

    return signing_input + "." + signature;
}

std::unordered_map<std::string, std::string> read_env_file(const std::string& path) {
    std::unordered_map<std::string, std::string> env;
    std::ifstream file(path);
    if (!file.is_open()) throw std::runtime_error("Cannot open .env file");

    std::string line;
    while (std::getline(file, line)) {
        // trim spaces
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

std::string read_pem_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) throw std::runtime_error("Cannot open PEM file");

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}