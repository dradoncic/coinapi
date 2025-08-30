#include <utility>
#include "states/order_book_state.h"

void OrderBookState::ensure_product(const std::string& product) 
{
    std::shared_lock<std::shared_mutex> lock(map_mutex_);
    if (books_.find(product) != books_.end()) return;
    lock.unlock();

    std::unique_lock<std::shared_mutex> lock(map_mutex_);
    if (books_.find(product) == books_.end()) {
        books_.emplace(product, std::make_unique<BookEntry>());
    }
}

void OrderBookState::update_book(const std::string& product, std::unique_ptr<OrderBook> newBook)
{
    ensure_product(product);

    auto entry = books_.at(product).get();
    
    std::shared_ptr<OrderBook> book = std::shared_ptr<OrderBook>(std::move(newBook));
    std::lock_guard<std::mutex> lock(entry->writer_mutex);
    entry->book.store(book, std::memory_order_release);
}

void OrderBookState::add_order(const std::string& product, Side side, Price price, Volume size)
{
    std::shared_lock<std::shared_mutex> lock(map_mutex_);

    auto it = books_.find(product);
    if (it == books_.end()) return;

    BookEntry* entry = it->second.get();

    std::lock_guard<std::mutex> lock(entry->writer_mutex);
    
    std::shared_ptr<OrderBook> cur = entry->book.load(std::memory_order_acquire);

    if (cur.unique()) {
        cur->set_level(side, price, size);
        entry->book.store(cur, std::memory_order_release);
    } else {
        std::shared_ptr<OrderBook> clone = std::make_shared<OrderBook>(*cur);
        clone->set_level(side, price, size);
        entry->book.store(clone, std::memory_order_release);
    }
}

std::shared_ptr<const OrderBook> OrderBookState::get_snapshot(const std::string& product) const
{
    std::shared_lock<std::shared_mutex> lock(map_mutex_);
    auto it = books_.find(product);
    if (it == books_.end()) return nullptr;

    std::shared_ptr<OrderBook> cur = it->second->book.load(std::memory_order_acquire);
    return cur;
}