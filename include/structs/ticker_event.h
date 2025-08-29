#pragma once
#include <string>

struct TickerEvent {
    std::string type;
    uint64_t sequence;
    std::string product_id;
    double price;
    double open_24h;
    double volume_24h;
    double low_24h;
    double high_24h;
    double volume_30d;
    double best_bid;
    double best_bid_size;
    double best_ask;
    double best_ask_size;
    std::string side;      // "buy" or "sell"
    std::string time;      // ISO 8601 timestamp
    uint64_t trade_id;
    double last_size;
};