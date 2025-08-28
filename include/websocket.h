#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <string>
#include <functional>
#include <simdjson.h>
#include "spscqueue.h"
#include "structs/market_event.h"

namespace coinbase {

class WebSocketClient {
public:
    WebSocketClient(boost::asio::io_context& ioc,
                    boost::asio::ssl::context& ssL_ctx,
                    SPSCQueue<MarketEvent, 1024>& spsc_queue
                );

    void connect(const std::string& host, const std::string& port, const std::vector<std::string>& products);
    
private:
    
};

};