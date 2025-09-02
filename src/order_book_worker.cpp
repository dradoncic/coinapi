#include <string>
#include <simdjson.h>
#include "enums/order_side.h"
#include "states/order_book_state.h"
#include "structs/order_book.h"
#include "workers/order_book_worker.h"

OrderBookWorker::OrderBookWorker(OrderBookState& state, LevelTape& tape) :
                    state_(state),
                    tape_(tape) {}

void OrderBookWorker::on_message(const RawMessage& msg) 
{
    switch (channelMap[msg.channel]) {
        case ChannelType::L2UPDATE:
            handle_level2_message(msg);
            break;
        case ChannelType::SNAPSHOT:
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

    auto product_sv = doc["product"].get_string().value();
    auto product = std::string(product_sv);

    auto newBook = std::make_unique<OrderBook>();
    newBook->reserve_levels(1024, 1024);

    for (auto bid : doc["bids"]) {
        auto bid_arr = bid.get_array();
        auto it = bid_arr.begin();

        auto price = (*it).get_double().value();

        ++it;
        auto size  = (*it).get_double().value();
        
        newBook->set_level(Side::BID, price, size);
    }

    for (auto ask : doc["asks"]) {
        auto ask_arr = ask.get_array();
        auto it = ask_arr.begin();

        auto price = (*it).get_double().value();

        ++it;
        auto size = (*it).get_double().value();

        newBook->set_level(Side::ASK, price, size);
    }

    state_.update_book(product, std::move(newBook));
}

void OrderBookWorker::handle_level2_message(const RawMessage& msg) 
{
    simdjson::ondemand::parser parser;
    simdjson::padded_string json(msg.payload);
    simdjson::ondemand::document doc = parser.iterate(json);

    auto product_sv = doc["product_id"].get_string().value();
    auto product = std::string(product_sv);
    
    for (auto change : doc["changes"]) {
        auto change_arr = change.get_array();
        auto it = change_arr.begin();

        auto side_sv = (*it).get_string().value();
        Side side = std::string(side_sv) == "buy" ? Side::BID : Side::ASK;

        ++it;
        auto price  = (*it).get_double ().value();

        ++it;
        auto size = (*it).get_double().value();

        state_.add_order(product, side, price, size);
    }
}
