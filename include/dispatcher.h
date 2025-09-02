#pragma once
#include <simdjson.h>
#include "ringbuffer.h"
#include "structs/raw_message.h"
#include "enums/channel_type.h"

class Dispatcher {
public:
    Dispatcher(RingBuffer<RawMessage>& ticker_queue,
                RingBuffer<RawMessage>& orderbook_queue) :
                ticker_queue_{ticker_queue},
                orderbook_queue_{orderbook_queue} {};

    void handle_message(const std::string& raw)
    {
        simdjson::ondemand::parser parser;
        simdjson::padded_string json(raw);
        simdjson::ondemand::document doc = parser.iterate(json);
    
        std::string_view type;  // type-safe
        if (doc["type"].get(type)) return;
    
        if (type == "subscriptions") return;
    
        std::string_view product_id;  // type-safe
        if (doc["product_id"].get(product_id)) return;
    
        RawMessage msg;
        msg.channel = std::string(type);
        msg.product_id = std::string(product_id);
        msg.payload = raw;
    
        ChannelType channel_type = channelMap.at(std::string(type));
    
        switch (channel_type) {
            case ChannelType::TICKER:
                ticker_queue_.push(msg);
                break;
            case ChannelType::SNAPSHOT:
            case ChannelType::L2UPDATE:
                orderbook_queue_.push(msg);
                break;
            default:
                std::cerr << "WebSocket: channel not yet supported." << "\n";
                return;
        }
    }

private:
    RingBuffer<RawMessage, TickerSize>& ticker_queue_;
    RingBuffer<RawMessage, OrderbookSize>& orderbook_queue_;
};