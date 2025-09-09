#pragma once
#include <vector>
#include <string>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include "enums/order_side.h"
#include "structs/trade_event.h"
#include "structs/heartbeat_event.h"

class TradeState {
public:
    void add_trade(const TradeEvent& trade);
    std::vector<TradeEvent> get_trades(const std::string& product_id) const;

private:
    mutable std::mutex mtx_trade_;
    std::unordered_map<std::string, std::vector<TradeEvent>> trades_;
};
