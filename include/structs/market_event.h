#pragma once
#include <string>

struct MarketEvent{
    std::string type;
    double price;
    double size;
    std::string side;
    std::string time;
};




