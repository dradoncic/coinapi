#include "websocket.h"
#include "authentication.h"
#include <iostream>

WebSocket::WebSocket(Authenticator& authenticator, boost::asio::io_context& ioc, boost::asio::ssl::context& ssl_ctx) :
                                auth_(authenticator),
                                ioc_(ioc),
                                ssl_ctx_(ssl_ctx),
                                ws_(ioc, ssl_ctx),
                                resolver_(ioc) {};

/**
 * @brief asynchronous DNS resolution
 */
void WebSocket::connect(const std::string& host, const std::string& port, 
                        const std::string& channel, 
                        const std::vector<std::string>& products) 
{
    host_ = host;
    port_ = port;
    products_ = products;
    channel_ =  channel;

    resolver_.async_resolve(host, port, [this](beast::error_code ec, net::ip::tcp::resolver::results_type results) {
        on_resolve(ec, results);
    });
};

/**
 * @brief initiates non-blocking TCP connection of the underlying socket to the resolved endpoints
 */
void WebSocket::on_resolve(beast::error_code ec, ip::tcp::resolver::results_type results) 
{
    if (ec) { std::cerr << "Resolve: " << ec.message() << "\n"; return; }
    async_connect(ws_.next_layer().next_layer(), results.begin(), results.end(),
        [this](beast::error_code ec, auto) { on_connect(ec); });
};

/**
 * @brief sets SNI on the SSL object & performs the TLS handshake
 */
void WebSocket::on_connect(beast::error_code ec) 
{
    if (ec) { std::cerr << "Connect: " << ec.message() << "\n"; return; }

    if (!SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host_.c_str())) {
        beast::error_code ec_sni{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
        std::cerr << "SNI: " << ec_sni.message() << "\n";
        return;
    }

    ws_.next_layer().async_handshake(net::ssl::stream_base::client,
        [this](beast::error_code ec) { on_ssl_handshake(ec); });
};

/**
 * @brief performs WebSocket handshake using the already-establish TLS connection
 */
void WebSocket::on_ssl_handshake(beast::error_code ec) 
{
    if (ec) { 
        std::cerr << "SSL Handshake error: " << ec.message() << std::endl;
        std::cerr << "Error code: " << ec.value() << std::endl;
        std::cerr << "Error category: " << ec.category().name() << std::endl;
        
        unsigned long ssl_err;
        while ((ssl_err = ERR_get_error()) != 0) {
            char err_buf[256];
            ERR_error_string_n(ssl_err, err_buf, sizeof(err_buf));
            std::cerr << "OpenSSL error: " << err_buf << std::endl;
        }
        return; 
    }

    ws_.async_handshake(host_, "/",
        [this](beast::error_code ec) { on_handshake(ec); });
};

/**
 * @brief sends subscribe message & starts to read the loop
 */
void WebSocket::on_handshake(beast::error_code ec) 
{
    if (ec) { std::cerr << "Handshake: " << ec.message() << "\n"; return; }

    nlohmann::json subscribe_msg;
    subscribe_msg["type"] = "subscribe";
    subscribe_msg["channel"] = channel_;
    subscribe_msg["product_ids"] = products_;
    subscribe_msg["jwt"] = auth_.build_jwt();

    write_buffer_ = subscribe_msg.dump();

    ws_.async_write(net::buffer(write_buffer_),
        [this](beast::error_code ec, size_t bytes_transferred) { 
            if (ec) {std::cerr << "Subscribe: " << ec.message() << "\n"; return; }
            ws_.async_read(read_buffer_, [this](beast::error_code ec, size_t bytes_transferred) {
                on_read(ec, bytes_transferred);
            });
        });
};

/**
 * @brief continously recieves & read messages
 */
void WebSocket::on_read(beast::error_code ec , std::size_t bytes_transferred) 
{
    if (ec) { std::cerr << "Read: " << ec.message() << "\n"; return; }

    auto message = beast::buffers_to_string(read_buffer_.data());
    read_buffer_.consume(bytes_transferred);

    if (handler_) {
        handler_(message);
    }

    message_count_++;

    if (message_count_ >= max_messages_) {
        std::cout << "}" << "\n";
        ws_.async_close(websocket::close_code::normal,
            [this](beast::error_code ec) {
                if (ec) std::cerr << "Close: " << ec.message() << "\n";
            });
        return;
    }

    ws_.async_read(read_buffer_, [this](beast::error_code ec, size_t bytes_transferred) {
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