#include <string>
#include <iostream>
#include <stdexcept>
#include "workers/trade_worker.h"
#include "enums/channel_type.h"
#include "enums/order_side.h"

TradeWorker::TradeWorker(TradeState& tape, RingBuffer<BackFillRequest>& queue) :
                            trade_tape_{tape},
                            back_queue_{queue} {}

void TradeWorker::on_message(const RawMessage& msg)
{
    return;
    // switch (channelMap[msg.channel]) {
    //     case ChannelType::MATCH:
    //         handle_match_message(msg);
    //         break;
    //     case ChannelType::HEARTBEAT:
    //         handle_heartbeat_message(msg);
    //         break;
    //     default:
    //         std::cerr << "Trades: unknown channel type '" << msg.channel << "'\n";
    //         return;
    // }
}

TradeEvent TradeWorker::parse_match_event(const RawMessage& msg)
{
    simdjson::padded_string json(msg.payload);
    simdjson::ondemand::document doc = parser_.iterate(json);

    TradeEvent trade;
    trade.trade_id = doc["trade_id"].get_uint64().value();
    trade.sequence = doc["sequence"].get_uint64().value();
    trade.product_id = std::string(doc["product_id"].get_string().value());
    trade.time = std::string(doc["time"].get_string().value());
    trade.size = doc["size"].get_double().value();
    trade.price = doc["price"].get_double().value();
    
    auto side_sv = doc["side"].get_string().value();
    trade.maker_side = (side_sv == "buy") ? Side::BID : Side::ASK;

    return trade;
}   

HeartbeatEvent TradeWorker::parse_heartbeat_event(const RawMessage& msg)
{
    simdjson::padded_string json(msg.payload);
    simdjson::ondemand::document doc = parser_.iterate(json);

    HeartbeatEvent heartbeat;
    heartbeat.product_id = std::string(doc["product_id"].get_string().value());
    heartbeat.last_trade_id = doc["last_trade_id"].get_uint64().value();
    heartbeat.sequence = doc["sequence"].get_uint64().value();

    return heartbeat;
}

void TradeWorker::handle_match_message(const RawMessage& msg)
{
    TradeEvent t;
    try {
        t = parse_match_event(msg);
    } catch (const std::exception& e) {
        std::cerr << "[TradeWorker] Failed to parse match: " << e.what() << "\n";
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mtx_state_);
        auto& last_seq = last_match_seq_[t.product_id];
        if (last_seq != 0 && t.sequence > last_seq + 1) {
            std::cerr << "[TradeWorker] MATCH sequence gap for " << t.product_id
                        << " expected " << (last_seq + 1) << " got " << t.sequence << "\n";
        }

        if (t.sequence > last_seq) last_seq = t.sequence;

        auto& highest = highest_match_trade_id_[t.product_id];
        if (t.trade_id > highest) highest = t.trade_id;
    }
}

void TradeWorker::handle_heartbeat_message(const RawMessage& msg)
{
    HeartbeatEvent h;
    try {
        h = parse_heartbeat_event(msg);
    } catch (const std::exception& e) {
        std::cerr << "[TradeWorker] Failed to parse heartbeat: " << e.what() << "\n";
        return;
    }

    uint64_t to_backfill_from;
    uint64_t to_backfill_to;

    {
        std::lock_guard<std::mutex> lock(mtx_state_);

        auto& last_seq = last_hb_seq_[h.product_id];
        if (last_seq != 0 && h.sequence > last_seq + 1) {
            std::cerr << "[TradeWorker] HEARTBEAT sequence gap for " << h.product_id
                        << " expected " << (last_seq + 1) << " got " << h.sequence << "\n";
        }

        if (h.sequence > last_seq) last_seq = h.sequence;

        const uint64_t highest_match = highest_match_trade_id_[h.product_id];
        if (h.last_trade_id > highest_match) {
            to_backfill_from = highest_match ? (highest_match + 1) : h.last_trade_id;
            to_backfill_to = h.last_trade_id;
        }

        last_hb_trade_id_[h.product_id] = h.last_trade_id;
    }

    if (to_backfill_to >= to_backfill_from) {
        for (uint64_t i = to_backfill_from; i <= to_backfill_to; i++) {
            BackFillRequest req{h.product_id, i};
            if (!back_queue_.push(req)) {
                std::cerr << "[TradeWorker] Backfill queue FULL for " << h.product_id
                            << " " << req.trade_id << "\n";
            } else {
                std::cout << "[TradeWorker] Queued backfill for " << h.product_id
                            << " " << req.trade_id << "\n";
            }
        }
    }
}