#include "websocket.h"
#include <iostream>

using namespace boost::asio;
using namespace boost::beast;

WebSocket::WebSocket(boost::asio::io_context& ioc, boost::asio::ssl::context& ssl_ctx) :
                                ioc_(ioc),
                                ssl_ctx_(ssl_ctx),
                                ws_(ioc, ssl_ctx),
                                resolver_(ioc) {};

/**
 * @brief connect and resolve
 */
void WebSocket::connect(const std::string& host, const std::string& port, const std::vector<std::string>& products) 
{
    host_ = host;
    port_ = port;
    products_ = products;

    resolver_.async_resolve(host, port, [this](error_code ec, ip::tcp::resolver::results_type results) {
        on_resolve(ec, results);
    });
};

/**
 * @brief DNS resolution han
 */
void WebSocket::on_resolve(error_code ec, ip::tcp::resolver::results_type results) 
{
    if (ec) { std::cerr << "Resolve: " << ec.message() << "\n"; return; }
    async_connect(ws_.next_layer().next_layer(), results.begin(), results.end(),
        [this](error_code ec, auto) { on_connect(ec); });
};

/**
 * @brief TCP connected
 */
void WebSocket::on_connect(error_code ec) 
{
    if (ec) { std::cerr << "Connect: " << ec.message() << "\n"; return; }
    ws_.next_layer().async_handshake(ssl::stream_base::client,
        [this](error_code ec) { on_ssl_handshake(ec); });
};

/**
 * @brief SSL handshake complete
 */
void WebSocket::on_ssl_handshake(error_code ec) 
{
    if (ec) { std::cerr << "SSL Handshake: " << ec.message() << "\n"; return; }
    ws_.async_handshake(host_, "/",
        [this](error_code ec) { on_handshake(ec); });
};

/**
 * @brief subscribes to ticker channel & start reading
 */
void WebSocket::on_handshake(error_code ec) 
{
    if (ec) { std::cerr << "Handshake: " << ec.message() << "\n"; return; }

    nlohmann::json subscribe_msg;
    subscribe_msg["type"] = "suscribe";
    subscribe_msg["channels"] = {{{"name", "ticker"},{"product_ids", products_}}};

    write_buffer_ = subscribe_msg.dump();

    ws_.async_write(boost::asio::buffer(write_buffer_),
        [this](error_code ec, size_t bytes_transferred) { 
            if (ec) {std::cerr << "Subscribe: " << ec.message() << "\n"; return; }
            ws_.async_read(read_buffer_, [this](error_code ec, size_t bytes_transferred) {
                on_read(ec, bytes_transferred);
            });
        });
};

/**
 * @brief continously read messages
 */
void WebSocket::on_read(error_code ec , std::size_t bytes_transferred) 
{
    if (ec) { std::cerr << "Read: " << ec.message() << "\n"; return; }

    auto message = buffers_to_string(read_buffer_.data());
    read_buffer_.consume(read_buffer_.size());

    if (handler_) {
        handler_(message);
    }

    ws_.async_read(read_buffer_, [this](error_code ec, size_t bytes_transferred) {
        on_read(ec, bytes_transferred);
    });
};

/**
 * @brief set message handler callback
 */
void WebSocket::set_message_handler(DispatcherHandler handler)
{
    handler_ = std::move(handler);
};