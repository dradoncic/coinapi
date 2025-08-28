#pragma once
#include <string>

struct MarketEvent{
    std::string product_id;
    double price;
    double size;
    std::string side;
    std::string time;
};




