#pragma once
#include <string>
#include <map>
#include <utility>
#include <shared_mutex>
#include <mutex>
#include <memory>
#include <unordered_map>
#include "structs/order_book.h"

class OrderBookState {
public:
    OrderBookState() = default;

    void ensure_product(const std::string& product);
    void update_book(const std::string& product, std::unique_ptr<OrderBook> newBook);
    void add_order(const std::string& product, Side side, Price price, Volume size);

    std::shared_ptr<const OrderBook> get_snapshot(const std::string& product) const;

private:
    struct BookEntry {
        std::atomic<std::shared_ptr<OrderBook>> book;

        std::mutex writer_mutex;

        BookEntry() {
            book.store(std::make_shared<OrderBook>(), std::memory_order_relaxed);
        }
    };

    mutable std::shared_mutex map_mutex_;
    std::unordered_map<std::string, std::unique_ptr<BookEntry>> books_;
};