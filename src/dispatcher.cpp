#include "dispatcher.h"
#include <string>

template<size_t TickerSize, size_t OrderbookSize>
Dispatcher<TickerSize, OrderbookSize>::Dispatcher(RingBuffer<RawMessage, TickerSize>& ticker_queue,
                        RingBuffer<RawMessage, OrderbookSize>& orderbook_queue) :
                        ticker_queue_{ticker_queue},
                        orderbook_queue_{orderbook_queue} {};

/**
 * @brief dispatches incoming messages to correct queue
 */
template<size_t TickerSize, size_t OrderbookSize>
void Dispatcher<TickerSize, OrderbookSize>::handle_message(const std::string& raw)
{
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