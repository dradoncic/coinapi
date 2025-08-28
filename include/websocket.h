#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <string>
#include <functional>
#include <simdjson.h>
#include <nlohmann/json.hpp>
#include "spscqueue.h"
#include "structs/market_event.h"

namespace coinbase {

class WebSocket {
public:
    WebSocket(boost::asio::io_context& ioc,
                    boost::asio::ssl::context& ssL_ctx,
                    SPSCQueue<MarketEvent, 1024>& spsc_queue,
                    simdjson::ondemand::parser& parser
                );

    void connect(const std::string& host, const std::string& port, const std::vector<std::string>& products);
    
private:
    void on_resolve(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type results);
    void on_connect(boost::beast::error_code ec);
    void on_ssl_handshake(boost::beast::error_code ec);
    void on_handshake(boost::beast::error_code ec);
    void on_read(boost::beast::error_code ec, size_t bytes_transferred);

    boost::asio::io_context& ioc_; // schedules and executes all of the asynchronous network operations
    boost::asio::ssl::context& ssl_ctx_; // ssl/tls configuratio context
    boost::beast::websocket::stream<boost::beast::ssl_stream<boost::asio::ip::tcp::socket>> ws_; // websocket object
    boost::asio::ip::tcp::resolver resolver_; // resolves domains
    boost::beast::flat_buffer read_buffer_;  // internally managed buffer
    std::string write_buffer_; // temporary write buffer
    std::string host_;
    std::string port_;
    std::vector<std::string> products_;
    SPSCQueue<MarketEvent, 1024>& spsc_queue_;
    simdjson::ondemand::parser parser_;
};

};