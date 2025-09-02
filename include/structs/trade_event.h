#pragma once
#include <string>
#include "enums/order_side.h"

struct TradeEvent
{
    uint64_t trade_id;
    std::string time;
    Volume size;
    Price price;
    Side maker_side;
    std::string product_id;
    uint64_t sequence;
};