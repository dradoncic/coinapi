#pragma once
#include <string>


struct HeartbeatEvent {
    uint64_t sequence;
    uint64_t last_trade_id;
    std::string product_id;
};
