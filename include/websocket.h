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

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ip = boost::asio::ip;

class WebSocket {
public:
    WebSocket(net::io_context& ioc, net::ssl::context& ssL_ctx);

    void connect(const std::string& host, const std::string& port, const std::vector<std::string>& products);

    using DispatcherHandler = std::function<void(std::string)>;
    void set_message_handler(DispatcherHandler handler);
    
private:
    void on_resolve(beast::error_code ec, ip::tcp::resolver::results_type results);
    void on_connect(beast::error_code ec);
    void on_ssl_handshake(beast::error_code ec);
    void on_handshake(beast::error_code ec);
    void on_read(beast::error_code ec, size_t bytes_transferred);

    net::io_context& ioc_; // schedules and executes all of the asynchronous network operations
    net::ssl::context& ssl_ctx_; // ssl/tls configuration context
    websocket::stream<beast::ssl_stream<ip::tcp::socket>> ws_; // websocket object
    ip::tcp::resolver resolver_; // resolves domains

    beast::flat_buffer read_buffer_;  // internally managed buffer
    std::string write_buffer_; // temporary write buffer

    std::string host_;
    std::string port_;
    std::vector<std::string> products_;

    DispatcherHandler handler_;
};