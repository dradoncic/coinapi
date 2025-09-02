#pragma once
#include <string>

struct BackFillRequest {
    std::string product_id;
    uint64_t from_trade_id;
    uint64_t to_trade_id;
};