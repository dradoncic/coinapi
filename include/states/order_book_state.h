#pragma once
#include <string>
#include <map>
#include <utility>
#include "structs/order_book.h"

class OrderBookState {
    public:
        void update_book(const std::string& product, std::unique_ptr<OrderBook> newBook);
        std::shared_ptr<const OrderBook> getSnapshot(const std::string& product) const;
        
        void add_order(const std::string& product, Side side, Price price, Volume size);
        void remove_order(const std::string& product, Side side, Price price, Volume size);
    
    private:
        std::shared_ptr<OrderBook> get_book(const std::string& product);
    
        std::unordered_map<std::string, std::shared_ptr<OrderBook>> books_;
    };