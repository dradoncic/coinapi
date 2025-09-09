#pragma once
#include <simdjson.h>
#include "ringbuffer.h"
#include "structs/raw_message.h"
#include "enums/channel_type.h"

class Dispatcher {
public:
    Dispatcher(RingBuffer<RawMessage>& ticker_queue,
                RingBuffer<RawMessage>& orderbook_queue,
                RingBuffer<RawMessage>& trade_queue) :
                ticker_queue_{ticker_queue},
                orderbook_queue_{orderbook_queue},
                trade_queue_{trade_queue} {};

    void handle_message(const std::string_view& raw)
    {
        simdjson::ondemand::parser parser;
        simdjson::padded_string json(raw);
        simdjson::ondemand::document doc = parser.iterate(json);

        std::string_view channel = doc["channel"];

        if (channel == "subscriptions" || "heartbeats") return;
        
        Channel channel_type = channelMap.at(channel);

        RawMessage msg;
        msg.channel = channel_type;  
        msg.payload = raw;
    
        switch (channel_type) {
            case Channel::TICKER:
                ticker_queue_.push(msg);
                break;
            case Channel::LEVEL2:
                orderbook_queue_.push(msg);
                break;
            case Channel::TRADE:
                trade_queue_.push(msg);
                break;
            default:
                std::cerr << "WebSocket: channel not yet supported." << "\n";
                return;
        }
    }

private:
    RingBuffer<RawMessage>& ticker_queue_;
    RingBuffer<RawMessage>& orderbook_queue_;
    RingBuffer<RawMessage>& trade_queue_;
};