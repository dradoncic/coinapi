#pragma once
#include <thread>
#include <atomic>
#include <vector>
#include "structs/trade_event.h"
#include "structs/backfill_req.h"
#include "states/trade_tape.h"
#include "ringbuffer.h"
#include "authentication.h"

class BackFillWorker {
public:
    BackFillWorker(TradeTape& tape, 
                    RingBuffer<BackFillRequest>& queue, 
                    Authenticator& auth);
    ~BackFillWorker();

    void start();
    void stop();

private:
    void loop();
    void process_request(const BackFillRequest& req);
    TradeEvent fetch_trade_from_api(const BackFillRequest& req);

    TradeTape& trade_tape_;
    RingBuffer<BackFillRequest>& back_queue_;
    Authenticator& auth_;
    std::thread th_;
    std::atomic<bool> running_{false};
};