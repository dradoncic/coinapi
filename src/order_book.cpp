#include <string>
#include <simdjson.h>
#include "enums/order_side.h"
#include "states/order_book_state.h"
#include "structs/order_book.h"
#include "order_book_worker.h"


inline void OrderBook::add_order(Side side, Price price, Volume size)
{
    if (side == Side::BID)
        return insert_order(bids, price, size, std::less<Price>());
    else
        return insert_order(asks, price, size, std::greater<Price>());
}

void OrderBookState::update_book(const std::string& product, std::unique_ptr<OrderBook> newBook)
{
    std::unique_lock<std::shared_mutex> lock(mtx_);
    books_[product] = std::shared_ptr<OrderBook>(std::move(newBook));

    update_fast_quote(product, *books_[product]);
}

void OrderBookState::add_order(const std::string& product, Side side, Price price, Volume size)
{
    std::unique_lock<std::shared_mutex> lock(mtx_);
    auto it = books_.find(product);
    if (it == books_.end()) return;

    if (it->second.use_count() == 1){
        it->second->add_order(side, price, size);
    } else {
        auto new_book = std::make_shared<OrderBook>(*it->second);
        new_book->add_order(side, price, size);
        it->second = new_book;
    }

    update_fast_quote(product, *it->second);
}

std::shared_ptr<const OrderBook> OrderBookState::get_snapshot(const std::string& product) const
{
    std::shared_lock<std::shared_mutex> mtx_;
    auto it = books_.find(product);
    return (it == books_.end()) ? nullptr : it->second;
}

OrderBookState::FastQuote OrderBookState::get_fast_quote(const std::string& product) const
{
    auto it = fast_quotes_.find(product);
    if (!(it == fast_quotes_.end())) {
        return it->second.load(std::memory_order_acquire);
    }
    return FastQuote{};
}

void OrderBookState::update_fast_quote(const std::string& product, const OrderBook& book) 
{
    FastQuote quote;
    quote.best_bid = book.get_best_bid ();
    quote.best_ask = book.get_best_ask();
    quote.sequence++;

    fast_quotes_[product].store(quote, std::memory_order_release);
}

OrderBookWorker::OrderBookWorker(OrderBookState& state) : state_(state) {}

void OrderBookWorker::on_message(const RawMessage& msg) 
{
    switch (channelMap[msg.channel]) {
        case ChannelType::L2UPDATE:
            on_level2_message(msg);
            break;
        case ChannelType::SNAPSHOT:
            on_snapshot_message(msg);
            break;
        default:
            std::cerr << "Orderbook: missing channel type." << "\n";
            return;
    }
}

void OrderBookWorker::on_snapshot_message(const RawMessage& msg)
{
    simdjson::ondemand::parser parser;
    simdjson::padded_string json(msg.payload);
    simdjson::ondemand::document doc = parser.iterate(json);

    auto product_sv = doc["product_id"].get_string().value();
    auto product = std::string(product_sv);
    auto newBook = std::make_unique<OrderBook>();

    for (auto bid : doc["bids"]) {
        auto bid_arr = bid.get_array();
        auto it = bid_arr.begin();

        auto price_sv = (*it).get_string().value();
        double price = std::stod(std::string(price_sv));

        ++it;
        auto size_sv  = (*it).get_string().value();
        double size = std::stod(std::string(size_sv));
        
        newBook->add_order(Side::BID, price, size);
    }

    for (auto ask : doc["asks"]) {
        auto ask_arr = ask.get_array();
        auto it = ask_arr.begin();

        auto price_sv = (*it).get_string().value();
        double price = std::stod(std::string(price_sv));

        ++it;
        auto size_sv = (*it).get_string().value();
        double size = std::stod(std::string(size_sv));

        newBook->add_order(Side::ASK, price, size);
    }

    state_.update_book(product, std::move(newBook));
}

void OrderBookWorker::on_level2_message(const RawMessage& msg) 
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
        auto price_sv  = (*it).get_string().value();
        double price = std::stod(std::string(price_sv));

        ++it;
        auto size_sv = (*it).get_string().value();
        double size = std::stod(std::string(size_sv));

        state_.add_order(product, side, price, size);
    }
}
