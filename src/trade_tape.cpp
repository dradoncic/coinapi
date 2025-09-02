
#include "states/trade_tape.h"
#include "structs/trade_event.h"

/**
 * @brief synchronized store, multiple threads access this structure
 */
void TradeTape::add_trade(const TradeEvent& trade)
{
    std::lock_guard<std::mutex> lock(mtx_trade_);
    auto& vec = trades_[trade.product_id];

    auto it = std::upper_bound(vec.begin(), vec.end(), trade.trade_id,
                                [](uint64_t id, const TradeEvent& t) {
                                    return id < t.trade_id;
                                });
    if (it != vec.begin()) {
        auto prev = it - 1;
        if (prev->trade_id == trade.trade_id) return;
    }
    vec.insert(it, trade);
}

std::vector<TradeEvent> TradeTape::get_trades(const std::string& product_id) const
{
    std::lock_guard<std::mutex> lock(mtx_trade_);
    auto it = trades_.find(product_id);
    if (it == trades_.end()) return {};
    return it->second;
}