#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

// ---- your headers ----
#include "ring_buffer.hpp"        // RingBuffer<RawMessage, N> with push/pop
#include "dispatcher.hpp"         // Dispatcher(tickerQ, orderbookQ), handle_message(...)
#include "websocket_client.hpp"   // WebSocket with set_message_handler(std::function<void(std::string_view)>)
#include "TickerWorker.h"         // TickerState, TickerWorker
#include "OrderBookWorker.h"      // OrderBookState, OrderBookWorker
#include "market_events.hpp"      // RawMessage { std::string type, product_id, payload }

using boost::asio::ip::tcp;

static std::atomic<bool> g_running{true};

static void handle_signal(int) {
    g_running.store(false, std::memory_order_relaxed);
}

int main(int argc, char** argv) {
    // --- Signals for graceful shutdown ---
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    // --- ASIO contexts ---
    boost::asio::io_context ioc;
    boost::asio::ssl::context ssl_ctx(boost::asio::ssl::context::tlsv12_client);
    ssl_ctx.set_default_verify_paths();
    ssl_ctx.set_verify_mode(boost::asio::ssl::verify_peer);

    // --- Shared state for strategies ---
    TickerState     ticker_state;     // atomic<double> per product
    OrderBookState  orderbook_state;  // atomic shared_ptr<OrderBook> per product

    // --- Workers that will consume queues and update shared state ---
    TickerWorker    ticker_worker(ticker_state);
    OrderBookWorker orderbook_worker(orderbook_state);

    // --- Lock-free SPSC queues (producer: WS thread, consumer: worker thread) ---
    RingBuffer<RawMessage, 2048> ticker_queue;
    RingBuffer<RawMessage, 8192> orderbook_queue;

    // --- Dispatcher: routes raw JSON to the right queue with minimal peek ---
    Dispatcher dispatcher(ticker_queue, orderbook_queue);

    // --- WebSocket client ---
    WebSocket ws(ioc, ssl_ctx);

    // If Dispatcher::handle_message takes std::string_view:
    ws.set_message_handler([&dispatcher](std::string_view raw) {
        dispatcher.handle_message(raw);              // zero-copy path
    });

    // If your Dispatcher::handle_message currently takes std::string, use this instead:
    // ws.set_message_handler([&dispatcher](std::string_view raw) {
    //     dispatcher.handle_message(std::string(raw)); // one small copy to satisfy signature
    // });

    // --- Connect & subscribe (adjust as needed) ---
    const std::string host = "ws-feed.exchange.coinbase.com";
    const std::string port = "443";
    std::vector<std::string> products = {"BTC-USD", "ETH-USD"};
    // Your WebSocket::connect should send the subscribe message for ticker/level2/matches
    ws.connect("wss://" + host, port, products);

    // --- ASIO I/O thread ---
    std::thread io_thread([&ioc]() {
        try {
            ioc.run();
        } catch (const std::exception& e) {
            std::cerr << "[io_thread] Exception: " << e.what() << "\n";
        }
    });

    // --- Worker: ticker consumer ---
    std::thread ticker_thread([&]() {
        RawMessage msg;
        while (g_running.load(std::memory_order_relaxed)) {
            if (ticker_queue.pop(msg)) {
                // msg.payload contains the raw JSON for a ticker message
                // Parse & update shared state
                // e.g. ticker_worker.onMessage(msg.payload);
                ticker_worker.onMessage(msg.payload);
            } else {
                // avoid busy spin
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
        }
    });

    // --- Worker: orderbook consumer (level2 + match in arrival order) ---
    std::thread ob_thread([&]() {
        RawMessage msg;
        while (g_running.load(std::memory_order_relaxed)) {
            if (orderbook_queue.pop(msg)) {
                if (msg.type == "l2update" || msg.type == "snapshot") {
                    orderbook_worker.onLevel2Message(msg.payload);
                } else if (msg.type == "match") {
                    orderbook_worker.onMatchMessage(msg.payload);
                } else {
                    // ignore other types in this queue
                }
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
        }
    });

    // --- Simple test loop: read a few snapshots/prices while running ---
    std::thread monitor_thread([&]() {
        using namespace std::chrono_literals;
        while (g_running.load(std::memory_order_relaxed)) {
            auto ob = orderbook_state.getSnapshot("BTC-USD");
            double last = ticker_state.getLatestPrice("BTC-USD");
            if (ob) {
                double bb = ob->bestBid();
                double ba = ob->bestAsk();
                if (!std::isnan(bb) && !std::isnan(ba)) {
                    double mid = (bb + ba) * 0.5;
                    std::cout << "[MON] BTC-USD mid=" << mid << " bestBid=" << bb
                              << " bestAsk=" << ba << " last=" << last << "\n";
                }
            }
            std::this_thread::sleep_for(500ms);
        }
    });

    std::cout << "Press Enter to quit…\n";
    std::cin.get();

    // --- Shutdown ---
    g_running.store(false, std::memory_order_relaxed);
    try { ioc.stop(); } catch (...) {}

    if (io_thread.joinable())      io_thread.join();
    if (ticker_thread.joinable())  ticker_thread.join();
    if (ob_thread.joinable())      ob_thread.join();
    if (monitor_thread.joinable()) monitor_thread.join();

    return 0;
}
