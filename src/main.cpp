#include <iostream>
#include <thread>
#include <vector>
#include <boost/asio.hpp>
#include "websocket.h"
#include "dispatcher.h"
#include "ringbuffer.h"
#include "structs/raw_message.h"
#include "authentication.h"

std::string API_KEY;
std::string PRIVATE_KEY;

int main(int argc, char** argv) {
    auto env = read_env_file("/Users/deenradoncic/Desktop/code/coinapi/.env");
    API_KEY = env["API_KEY"];
    PRIVATE_KEY = read_pem_file("/Users/deenradoncic/Desktop/code/coinapi/private_key.pem");

    std::cout << API_KEY << "\n";
    std::cout << PRIVATE_KEY << "\n";

    net::io_context ioc;
    net::ssl::context ssl_ctx(net::ssl::context::tls_client);
    
    // Set SSL options
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

    WebSocket ws(ioc, ssl_ctx);
    ws.set_message_handler([](const std::string& msg){
        std::cout << "Received: " << msg << std::endl;
    });

    ws.connect("advanced-trade-ws.coinbase.com", "443", {"BTC-USD"});
    ioc.run();
}