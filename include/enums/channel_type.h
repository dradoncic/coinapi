#pragma once
#include <string>
#include <map> 

enum Channel {
    TICKER,
    TRADE,
    LEVEL2,
};

inline std::map<std::string_view, Channel> channelMap = {
    {"ticker", Channel::TICKER},
    {"l2_data", Channel::LEVEL2},
    {"market_trades", Channel::TRADE},
};