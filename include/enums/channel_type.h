#pragma once
#include <string>
#include <map> 

enum ChannelType {
    TICKER,
    SNAPSHOT,
    L2UPDATE,
    HEARTBEAT,
    STATUS
};

inline std::map<std::string, ChannelType> channelMap = {
    {"ticker", ChannelType::TICKER},
    {"snapshot", ChannelType::SNAPSHOT},
    {"l2update", ChannelType::L2UPDATE},
    {"heartbeat", ChannelType::HEARTBEAT},
    {"status", ChannelType::STATUS}
};