#pragma once
#include <utility>
#include <vector>
#include <map>
#include "enums/order_side.h"

class OrderBook {
public:
    std::vector<std::pair<Price, Volume>> bids;
    std::vector<std::pair<Price, Volume>> asks;

    inline void add_order(Side side, Price price, Volume size);

    inline Price get_best_bid() const 
    {
        return bids.empty() ? 0.0 : bids.rbegin()->first;
    }

    inline Price get_best_ask() const
    {
        return asks.empty() ? 0.0 : asks.rbegin()->first;
    }

    inline std::pair<Price, Volume> get_best_bid_level() const 
    {
        return bids.empty() ? std::make_pair(0.0, 0.0) : *bids.rbegin();
    }

    inline std::pair<Price, Volume> get_best_ask_level() const 
    {
        return asks.empty() ? std::make_pair(0.0, 0.0) : *asks.rbegin();
    }

    inline void clear() {
        bids.clear();
        asks.clear();
    }

private:
    template<class T, class Compare>
    void insert_order(T& levels, Price price, Volume volume, Compare compare) 
    {
        auto it = std::lower_bound(levels.begin(), levels.end(), price,
            [compare] (const auto& p, Price price) { return compare(p.first, price); });
        
        if (it !=  levels.end() && it->first == price) {
            volume == 0 ? levels.erase(it) : it->second = volume;
        } else if (volume > 0) {
            levels.insert(it, std::make_pair(price, volume));
        }
    }
};