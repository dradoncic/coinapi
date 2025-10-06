#pragma once
#include <string>
#include "enums/channel_type.h"

struct RawMessage {
    Channel channel;
    std::string payload;
};