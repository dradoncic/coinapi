#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "enums/order_side.h"

struct TradeEvent
{
    int trade_id;
    std::string time;
    double size;
    double price;
    Side maker_side;
};

class TradeTape {
public:
    void add_trade(const TradeEvent& trade);
    const std::vector<TradeEvent&> get_trades() const;

private:
    std ::unordered_map<std::string, std::vector<TradeEvent>> trades_;
};
