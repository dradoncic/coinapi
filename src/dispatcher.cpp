#include "dispatcher.h"
#include <string>

Dispatcher::Dispatcher(RingBuffer<RawMessage, 1024>& ticker_queue,
                        RingBuffer<RawMessage, 1024>& orderbook_queue) :
                        ticker_queue_{ticker_queue},
                        orderbook_queue_{orderbook_queue} {};

void Dispatcher::handle_message(const std::string& raw)
{
    simdjson::ondemand::parser parser;
    auto doc = parser.iterate(raw);

    std::string_view type;
    if (doc["type"].get(type)) return;

    if (type == "subscriptions") return;

    std::string_view product_id;
    if (doc["product_id"].get(product_id)) return;

    RawMessage msg;
    msg.channel = std::string(type);
    msg.product_id = std::string(product_id);
    msg.payload = raw;

    if (type == "ticker") {
        ticker_queue_.push(msg);
    } else if (type == "snapshot" || type == "l2update") {
        orderbook_queue_.push(msg);
    }
}