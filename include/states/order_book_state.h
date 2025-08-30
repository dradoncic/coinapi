#pragma once
#include <string>
#include <map>
#include <utility>
#include <shared_mutex>
#include <unordered_map>
#include "structs/order_book.h"

class OrderBookState {
    public:
        void update_book(const std::string& product, std::unique_ptr<OrderBook> newBook);
        void add_order(const std::string& product, Side side, Price price, Volume size);

        std::shared_ptr<const OrderBook> get_snapshot(const std::string& product) const;
    
    private:
        mutable std::shared_mutex mtx_;
        std::unordered_map<std::string, std::shared_ptr<OrderBook>> books_;
    };