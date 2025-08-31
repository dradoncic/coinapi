#pragma once
#include "structs/raw_message.h"
#include "structs/level_tape.h"
#include "states/order_book_state.h"
#include "enums/channel_type.h"

class OrderBookWorker {
    public:
        explicit OrderBookWorker(OrderBookState& state, LevelTape& tape);
    
        void on_message(const RawMessage& json);
    
    private:
        void on_level2_message(const RawMessage& json);
        void on_snapshot_message(const RawMessage& json); 
    
        OrderBookState& state_;
        LevelTape& tape_;

        mutable simdjson::ondemand::parser parser_;
    };