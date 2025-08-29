#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <string>
#include <functional>
#include <simdjson.h>
#include <nlohmann/json.hpp>
#include "structs/raw_message.h"

namespace coinbase {

class WebSocket {
public:
    WebSocket(boost::asio::io_context& ioc, boost::asio::ssl::context& ssL_ctx);

    void connect(const std::string& host, const std::string& port, const std::vector<std::string>& products);

    using DispatcherHandler = std::function<void(std::string_view)>;
    void set_message_handler(DispatcherHandler handler);
    
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

    DispatcherHandler handler_;
};

}