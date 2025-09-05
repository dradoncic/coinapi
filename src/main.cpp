#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <boost/asio.hpp>
#include "websocket.h"
#include "dispatcher.h"
#include "ringbuffer.h"
#include "structs/raw_message.h"
#include "authentication.h"

int main(int argc, char** argv) {
    Authenticator auth{"/Users/deenradoncic/Desktop/code/coinapi/.env",
                        "/Users/deenradoncic/Desktop/code/coinapi/private_key.pem"};

    net::io_context ioc;
    net::ssl::context ssl_ctx(net::ssl::context::tls_client);
    
    // Set SSL options & configure TLS behavior
    ssl_ctx.set_options(
        net::ssl::context::default_workarounds |
        net::ssl::context::no_sslv2 |
        net::ssl::context::no_sslv3 |
        net::ssl::context::no_tlsv1 |
        net::ssl::context::no_tlsv1_1
    );

    ssl_ctx.set_default_verify_paths(); // use system CA
    ssl_ctx.set_verify_mode(net::ssl::verify_peer);

    SSL_CTX_set_tlsext_servername_callback(ssl_ctx.native_handle(), nullptr);

    WebSocket ws(auth, ioc, ssl_ctx);
    ws.set_message_handler([](const std::string& msg){
        std::cout << msg << "," << "\n";
    });

    std::string channel = argv[1];
    std::string ticker = argv[2];
    std::cout << "{" << "\n";
    ws.connect("advanced-trade-ws.coinbase.com", "443", channel, {ticker});
    ioc.run();
}