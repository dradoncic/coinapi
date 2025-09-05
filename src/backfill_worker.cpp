#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cstdlib>
#include "simdjson.h"
#include "curl/curl.h"
#include "workers/backfill_worker.h"

BackFillWorker::BackFillWorker(TradeTape& tape, 
                                RingBuffer<BackFillRequest>& queue,
                                Authenticator& auth) :
                                trade_tape_{tape},
                                back_queue_{queue},
                                auth_{auth} {}

BackFillWorker::~BackFillWorker() { stop(); }

void BackFillWorker::start()
{
    running_.store(true);
    th_ =  std::thread(&BackFillWorker::loop, this);
}

void BackFillWorker::stop()
{
    running_.store(false);
    if (th_.joinable()) th_.join();
}

void BackFillWorker::loop()
{
    while (running_.load(std::memory_order_acquire)) {
        BackFillRequest req;
        if (back_queue_.pop(req)) {
            try { std::cout << "here" << "\n"; }    // process_request(req);
            catch (const std::exception& e) {
                std::cerr << "[BackFill] Error: " << e.what() << "\n";
            }
        } else {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    }
}