#pragma once
#include <string>

struct BackFillRequest {
    std::string product_id;
    uint64_t trade_id;
};