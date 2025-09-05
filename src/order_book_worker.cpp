#include <string>
#include <simdjson.h>
#include "enums/order_side.h"
#include "states/order_book_state.h"
#include "structs/order_book.h"
#include "workers/order_book_worker.h"

OrderBookWorker::OrderBookWorker(OrderBookState& state) :
                    state_(state) {}

void OrderBookWorker::on_message(const RawMessage& msg) 
{
    simdjson::padded_string json (msg.payload);
    simdjson::ondemand::document doc = parser_.iterate(json);

    simdjson::ondemand::array events = doc["events"];
    simdjson::ondemand::object event = events.at(0).get_object();
    std::string_view msg_type = event["type"].get_string();
    
    switch (messageMap[msg_type]) {
        case Message::UPDATE:
            handle_update_message(msg);
            break;
        case Message::SNAPSHOT:
            handle_snapshot_message(msg);
            break;
        default:
            std::cerr << "Orderbook: unknown channel type '" << msg.channel << "'\n";
            return;
    }
}

void OrderBookWorker::handle_snapshot_message(const RawMessage& msg)
{
    simdjson::padded_string json(msg.payload);
    simdjson::ondemand::document doc = parser_.iterate(json);

    simdjson::ondemand::array events = doc["events"];
    simdjson::ondemand::object event = events.at(0).get_object();

    std::string_view product_sv = event["product_id"].get_string();
    auto product = std::string(product_sv);

    auto newBook = std::make_unique<OrderBook>();
    newBook->reserve_levels(2048, 2048);

    simdjson::ondemand::array orders = event["updates"];

    for (auto order : orders) {
        simdjson::ondemand::object order_obj = order.get_object();

        std::string_view side_sv;
        side_sv = order_obj["side"].get_string();
        Side side = (side_sv == "bid") ? Side::BID : Side::ASK;

        double price = 0, size = 0;
        std::string_view price_sv, qty_sv;
    
        price_sv = order_obj["price_level"].get_string();
        qty_sv = order_obj["new_quantity"].get_string();

        price = std::stod(std::string(price_sv));
        size  = std::stod(std::string(qty_sv));

        newBook->set_level(side, price, size);
    }

    state_.update_book(product, std::move(newBook));
}

void OrderBookWorker::handle_update_message(const RawMessage& msg) 
{
    simdjson::ondemand::parser parser;
    simdjson::padded_string json(msg.payload);
    simdjson::ondemand::document doc = parser.iterate(json);

    simdjson::ondemand::array events = doc["events"];
    simdjson::ondemand::object event = events.at(0).get_object();

    std::string_view product_sv = event["product_id"].get_string();
    auto product = std::string(product_sv);

    simdjson::ondemand::array updates = event["updates"];

    for (auto change : updates) {
        simdjson::ondemand::object change_obj = change.get_object();

        std::string_view side_sv;
        side_sv = change_obj["side"].get_string();
        Side side = (side_sv == "bid") ? Side::BID : Side::ASK;

        double price = 0, size = 0;
        std::string_view price_sv, qty_sv;
    
        price_sv = change_obj["price_level"].get_string();
        qty_sv = change_obj["new_quantity"].get_string();

        price = std::stod(std::string(price_sv));
        size  = std::stod(std::string(qty_sv));


        state_.add_order(product, side, price, size);
    }
}
