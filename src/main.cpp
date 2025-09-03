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
    auto env = readEnvFile("/Users/deenradoncic/Desktop/code/coinapi/.env");
    API_KEY = env["API_KEY"];
    PRIVATE_KEY = env["PRIVATE_KEY"];

    boost::asio::io_context ioc;
    boost::asio::ssl::context ssl_ctx(boost::asio::ssl::context::tlsv12_client);

    ssl_ctx.set_default_verify_paths(); // use system CA
    ssl_ctx.set_verify_mode(boost::asio::ssl::verify_peer);

    WebSocket ws(ioc, ssl_ctx);
    ws.set_message_handler([](const std::string& msg){
        std::cout << "Received: " << msg << std::endl;
    });

    ws.connect("advanced-trade-ws.coinbase.com", "443", {"BTC-USD"});
    ioc.run();
}