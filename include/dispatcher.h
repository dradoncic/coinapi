#pragma once
#include <simdjson.h>
#include "ringbuffer.h"
#include "raw_message.h"
#include "channel_type.h"

class Dispatcher {
public:
    Dispatcher(RingBuffer<RawMessage, 1024>& ticker_queue,
                RingBuffer<RawMessage, 1024>& orderbook_queue);

    void handle_message(const std::string& raw);

private:
    RingBuffer<RawMessage, 1024>& ticker_queue_;
    RingBuffer<RawMessage, 1024>& orderbook_queue_;
};