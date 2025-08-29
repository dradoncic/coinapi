#pragma once
#include <string>

struct RawMessage {
    std::string channel;
    std::string product_id;
    std::string_view payload;
};