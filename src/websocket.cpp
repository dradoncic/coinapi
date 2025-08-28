#include "websocket.h"
#include <iostream>

namespace coinbase {

using namespace boost::asio;
using namespace boost::beast;

WebSocket::WebSocket(boost::asio::io_context& ioc, boost::asio::ssl::context& ssl_ctx,
                            SPSCQueue<MarketEvent, 1024>& spsc_queue,simdjson::ondemand::parser& parser) :
                                ioc_(ioc),
                                ssl_ctx_(ssl_ctx),
                                ws_(ioc, ssl_ctx),
                                resolver_(ioc),
                                spsc_queue_(spsc_queue) {};

/**
 * @brief connect and resolve
 */
void WebSocket::connect(const std::string& host, const std::string& port, const std::vector<std::string>& products) {
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
void WebSocket::on_resolve(error_code ec, ip::tcp::resolver::results_type results) {
    if (ec) { std::cerr << "Resolve: " << ec.message() << "\n"; return; }
    async_connect(ws_.next_layer().next_layer(), results.begin(), results.end(),
        [this](error_code ec) { on_connect(ec); });
};

/**
 * @brief TCP connected
 */
void WebSocket::on_connect(error_code ec) {
    if (ec) { std::cerr << "Connect: " << ec.message() << "\n"; return; }
    ws_.next_layer().async_handshake(ssl::stream_base::client,
        [this](error_code ec) { on_ssl_handshake(ec); });
};

/**
 * @brief SSL handshake complete
 */
void WebSocket::on_ssl_handshake(error_code ec) {
    if (ec) { std::cerr << "SSL Handshake: " << ec.message() << "\n"; return; }
    ws_.async_handshake(host_, "/",
        [this](error_code ec) { on_handshake(ec); });
};

/**
 * @brief subscribes to ticker channel & start reading
 */
void WebSocket::on_handshake(error_code ec) {
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
void WebSocket::on_read(error_code ec , std::size_t bytes_transferred) {
    if (ec) {std::cerr << "Read: " << ec.message() << "\n"; return; }

    auto message = buffers_to_string(read_buffer_.data());
    read_buffer_.consume(read_buffer_.size());

    simdjson::ondemand::parser parser;
    simdjson::ondemand::document doc;

    auto error = parser.iterate(message).get(doc);
    if (error) { std::cerr << "simdjson error \n"; return; }

    std::string_view type;
    if (doc["type"].get(type) == simdjson::SUCCESS && type == "ticker") {
        MarketEvent event;
        doc["product_id"].get(event.product_id);
        double price, size;
        doc["price"].get(price);
        doc["size"].get(size);
        event.price = price;
        event.size = size;
        doc["side"].get(event.side);
        doc["time"].get(event.time);

        spsc_queue_.push(event);
    }
    ws_.async_read(read_buffer_, [this](error_code ec, size_t bytes_transferred) { on_read(ec, bytes_transferred); });
};
