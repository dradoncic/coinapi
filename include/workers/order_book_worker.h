#pragma once
#include "structs/raw_message.h"
#include "structs/level_tape.h"
#include "states/order_book_state.h"
#include "enums/channel_type.h"
#include "enums/message_types.h"

class OrderBookWorker {
    public:
        explicit OrderBookWorker(OrderBookState& state);
    
        void on_message(const RawMessage& json);
    
    private:
        void handle_update_message(const RawMessage& json);
        void handle_snapshot_message(const RawMessage& json); 
    
        OrderBookState& state_;

        mutable simdjson::ondemand::parser parser_;
    };