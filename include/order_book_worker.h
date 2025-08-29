#pragma once
#include <string>
#include "states/order_book_state.h"

class OrderBookWorker {
    public:
        explicit OrderBookWorker(OrderBookState& state);
    
        void on_message(const std::string& json);
    
    private:
        void on_level2_message(const std::string& json);
        void on_snapshot_message(const std::string &json); 
    
        OrderBookState& state_;
    };