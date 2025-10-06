#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <csignal>
#include <boost/asio.hpp>
#include "websocket.h"
#include "dispatcher.h"
#include "ringbuffer.h"
#include "structs/raw_message.h"
#include "authentication.h"
#include "states/order_book_state.h"
#include "workers/order_book_worker.h"
#include "states/trade_state.h"
#include "workers/trade_worker.h"
#include "structs/backfill_req.h"

static std::atomic<bool> g_running{true};

static void handle_signal(int) {
    g_running.store(false, std::memory_order_relaxed);
}

int main(int argc, char** argv) {
    // --- Signals for graceful shutdown ---
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    // --- Authenticator for JWT generation ---
    Authenticator auth{"/Users/deenradoncic/Desktop/code/coinapi/.env",
                        "/Users/deenradoncic/Desktop/code/coinapi/private_key.pem"};

    // --- Set SSL options & configure TLS behavior ---
    net::io_context ioc;
    net::ssl::context ssl_ctx(net::ssl::context::tls_client);
    
    ssl_ctx.set_options(
        net::ssl::context::default_workarounds |
        net::ssl::context::no_sslv2 |
        net::ssl::context::no_sslv3 |
        net::ssl::context::no_tlsv1 |
        net::ssl::context::no_tlsv1_1
    );

    ssl_ctx.set_default_verify_paths();
    ssl_ctx.set_verify_mode(net::ssl::verify_peer);

    SSL_CTX_set_tlsext_servername_callback(ssl_ctx.native_handle(), nullptr);

    // --- Lock-free SPSC queues (producer: WS thread, consumer: worker thread) ---
    RingBuffer<RawMessage> orderbook_queue(8192);
    RingBuffer<RawMessage> ticker_queue(8192);
    RingBuffer<RawMessage> trade_queue(8192);
    // RingBuffer<BackFillRequest> backfill_queue(8192);

    // --- Shared state for strategies ---
    OrderBookState orderbook_state;
    // TradeState trade_state;

    // --- Workers that will consume queues and update shared state ---
    OrderBookWorker orderbook_worker(orderbook_state);
    // TradeWorker trade_worker(trade_state, backfill_queue);

    // --- Dispatcher: routes raw JSON to the right queue with minimal peek ---
    Dispatcher dispatcher(ticker_queue, orderbook_queue, trade_queue);

    // --- WebSocket client ---
    WebSocket ws(auth, ioc, ssl_ctx);
    ws.set_message_handler([&dispatcher](const std::string_view msg){
        dispatcher.handle_message(msg);
    });

    std::string channel = argv[1];
    std::string ticker = argv[2];
    std::cout << "{" << "\n";
    ws.connect("advanced-trade-ws.coinbase.com", "443", channel, {ticker});

    // --- ASIO I/O thread ---
    std::thread io_thread([&ioc]() {
        try {
            ioc.run();
        } catch (const std::exception& e) {
            std::cerr << "[io_thread] Exception: " << e.what() << "\n";
        }
    });

    std::thread ob_thread([&](){
        RawMessage msg;
        while (g_running.load(std::memory_order_relaxed)) {
            if (orderbook_queue.pop(msg)) {
                orderbook_worker.on_message(msg);
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
        }
    });

    while (g_running.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    ioc.stop();
    if (io_thread.joinable()) io_thread.join();
    if (ob_thread.joinable()) ob_thread.join();

    std::cout << "Final Order Books:\n";
    orderbook_state.view_books(100);

    return 0;




    // --- Worker: orderbook consumer (level2 + match in arrival order) ---
    // std::thread ob_thread([&]() {
    //     RawMessage msg;
    //     while (g_running.load(std::memory_order_relaxed)) {
    //         if (orderbook_queue.pop(msg)) {
    //             orderbook_worker.on_message(msg);
    //         } else {
    //             std::this_thread::sleep_for(std::chrono::microseconds(50));
    //         }
    //     }
    // });

    // --- Shutdown ---
    // std::cout << "Press Enter to quit…\n";
    // std::cin.get();
    // g_running.store(false, std::memory_order_relaxed);
    // try { ioc.stop(); } catch (...) {}

    // if (io_thread.joinable())      io_thread.join();

    // return 0;
}