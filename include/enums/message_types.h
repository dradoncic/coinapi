#pragma once
#include <string>
#include <map>
#include "channel_type.h"


enum Message {
    SNAPSHOT, 
    UPDATE
};

inline std::map<std::string_view, Message> messageMap = {
    {"snapshot", Message::SNAPSHOT},
    {"update", Message::UPDATE}
};