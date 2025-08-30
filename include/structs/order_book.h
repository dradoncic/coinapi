#pragma once
#include <utility>
#include <vector>
#include <map>
#include <algorithm>
#include <limits>
#include <optional>
#include <cassert>
#include <cstdint> 
#include "enums/order_side.h"

class OrderBook {
public:
    OrderBook() {
        bids_.reserve(1000);
        asks_.reserve(1000);
        sequence_ = 0;
        best_bid_ = std::numeric_limits<double>::quiet_NaN();
        best_ask_ = std::numeric_limits<double>::quiet_NaN();
    }

    OrderBook(const OrderBook& book) = default;
    OrderBook& operator=(const OrderBook& book) = default;

    void set_level(Side side, Price price, Volume size);

    inline std::vector<Level> get_bids() const { return bids_; };
    inline std::vector<Level> get_asks() const { return asks_; };

    inline Price best_bid() const { return best_bid_; };
    inline Price best_ask() const { return best_ask_; };
    inline uint64_t sequence() const { return sequence_; };

    std::vector<Level> top_bids(size_t n) const;
    std::vector<Level> top_asks(size_t n) const;

    void reserve_levels(size_t bids, size_t asks);

private:
    std::vector<Level> bids_;
    std::vector<Level> asks_;

    Price best_bid_;
    Price best_ask_;
    uint64_t sequence_;

    static void insert_or_erase(std::vector<Level>& vec, Price price, Volume volume, bool descencding);
    void refresh_bests();
};