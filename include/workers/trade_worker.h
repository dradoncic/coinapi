#pragma once
#include <unordered_map>
#include <mutex>
#include "states/trade_tape.h"
#include "structs/raw_message.h"
#include "structs/backfill_req.h"
#include "ringbuffer.h"


class TradeWorker {
public:
    TradeWorker(TradeTape& tape, RingBuffer<BackFillRequest>& queue);
    
    void on_message(const RawMessage& msg);

private:
    void handle_match_message(const RawMessage& msg);
    void handle_heartbeat_message(const RawMessage& msg);

    TradeEvent parse_match(const RawMessage& msg);
    HeartbeatEvent parse_heartbeat(const RawMessage& msg);

    TradeTape& trade_tape_;
    RingBuffer<BackFillRequest>& back_queue_;

    mutable std::mutex mtx_state_;

    std::unordered_map<std::string, uint64_t> highest_match_trade_id_;;
    std::unordered_map<std::string, uint64_t> last_hb_trade_id_;

    std::unordered_map<std::string, uint64_t> last_match_seq_;
    std::unordered_map<std::string, uint64_t> last_hb_seq_;
};