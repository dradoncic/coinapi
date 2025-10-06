#include <utility>
#include <iostream>
#include "states/order_book_state.h"

void OrderBookState::ensure_product(const std::string& product) 
{
    {
        std::shared_lock<std::shared_mutex> shared_lock(mtx_orderbooks_);
        if (books_.find(product) != books_.end()) return;
    }

    std::unique_lock<std::shared_mutex> write_lock(mtx_orderbooks_);
    if (books_.find(product) == books_.end()) {
        books_.emplace(product, std::make_unique<BookEntry>());
    }
}

void OrderBookState::update_book(const std::string& product, std::unique_ptr<OrderBook> newBook)
{
    ensure_product(product);

    std::shared_lock<std::shared_mutex> shared_lock(mtx_orderbooks_);
    auto it = books_.find(product);
    if (it == books_.end()) return;

    BookEntry* entry = it->second.get();
    shared_lock.unlock();

    auto shared_book = std::shared_ptr<OrderBook>(std::move(newBook));

    std::unique_lock<std::shared_mutex> write_lock(entry->mtx_book);
    entry->book = shared_book;
}

void OrderBookState::add_order(const std::string& product, Side side, Price price, Volume size)
{
    std::shared_lock<std::shared_mutex> shared_lock(mtx_orderbooks_);
    auto it = books_.find(product);
    if (it == books_.end()) return;

    BookEntry* entry = it->second.get();
    shared_lock.unlock();

    std::unique_lock<std::shared_mutex> write_lock(entry->mtx_book);

    std::shared_ptr<OrderBook> curr_book = entry->book;

    if (curr_book.use_count() > 1) {
        curr_book = std::make_shared<OrderBook>(*curr_book);
        entry->book = curr_book;
    }

    curr_book->set_level(side, price, size);
}

std::shared_ptr<const OrderBook> OrderBookState::get_snapshot(const std::string& product) const
{
    std::shared_lock<std::shared_mutex> hash_lock(mtx_orderbooks_);
    auto it = books_.find(product);
    if (it == books_.end()) return nullptr;

    BookEntry* entry = it->second.get();
    hash_lock.unlock();

    std::shared_lock<std::shared_mutex> book_lock(entry->mtx_book);
    return entry->book;
}

void OrderBookState::view_books(int levels) const
{
    std::shared_lock<std::shared_mutex> book_lock(mtx_orderbooks_);
    for (const auto& [product, entry] : books_)
    {
        std::shared_lock entry_lock(entry->mtx_book);
        std::cout << "Product: " << product << "\n";
        if (entry->book) {
            entry->book->print(levels);
        } else {
            std:: cout << " (empty)\n";
        }
    }
}