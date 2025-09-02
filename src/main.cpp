#include <iostream>
#include <thread>
#include <vector>
#include <boost/asio.hpp>
#include "websocket.h"
#include "dispatcher.h"
#include "ringbuffer.h"
#include "structs/raw_message.h"

int main(int argc, char** argv) {
    boost::asio::io_context ioc;
    boost::asio::ssl::context ssl_ctx(boost::asio::ssl::context::tlsv12_client);
    ssl_ctx.set_default_verify_paths();
    ssl_ctx.set_verify_mode(boost::asio::ssl::verify_peer);

    RingBuffer<RawMessage> ticker_queue(2048);
    RingBuffer<RawMessage> orderbook_queue(8192);

    Dispatcher dispatcher(ticker_queue, orderbook_queue);
    WebSocket ws(ioc, ssl_ctx);

    ws.set_message_handler([&dispatcher](std::string raw) { dispatcher.handle_message(raw); });

    std::string host  = "ws-feed.exchange.coinbase.com";
    std::string port = "443";
    std::vector<std::string> products =  {"BTC_USD"};

    ws.connect("wss://" + host, port, products);

    std::thread io_thread([&ioc]() { ioc.run(); });

    std::cout << "Press Enter to quit...\n";
    std::cin.get();

    ioc.stop();
    io_thread.join();
}