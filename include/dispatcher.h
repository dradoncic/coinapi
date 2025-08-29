#pragma once
#include <simdjson.h>
#include "ringbuffer.h"
#include "structs/raw_message.h"
#include "enums/channel_type.h"

template<size_t TickerSize, size_t OrderbookSize>
class Dispatcher {
public:
    Dispatcher(RingBuffer<RawMessage, TickerSize>& ticker_queue,
                RingBuffer<RawMessage, OrderbookSize>& orderbook_queue);

    void handle_message(const std::string& raw);

private:
    RingBuffer<RawMessage, TickerSize>& ticker_queue_;
    RingBuffer<RawMessage, OrderbookSize>& orderbook_queue_;
    simdjson::ondemand::parser parser_;
};