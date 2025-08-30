#include "structs/order_book.h"

void OrderBook::set_level(Side side, Price price, Volume size)
{
    if (side == Side::BID) {
        insert_or_erase(bids_, price, size, true);
    } else {
        insert_or_erase(asks_, price, size, false);
    }

    refresh_bests();
    ++sequence_;
}

std::vector<Level> OrderBook::top_bids(size_t n) const 
{
    size_t m = std::min(n, bids_.size());
    return std::vector<Level>(bids_.rbegin(), bids_.rbegin() + m);
}

std::vector<Level> OrderBook::top_asks(size_t n) const 
{
    size_t m = std::min(n, asks_.size());
    return std::vector<Level>(asks_.rbegin(), asks_.rbegin() + m);
}

void OrderBook::reserve_levels(size_t bids, size_t asks) 
{
    bids_.reserve(bids);
    asks_.reserve(asks);
}

void OrderBook::insert_or_erase(std::vector<Level>& vec, Price price, Volume size, bool descending)
{
    auto compare = [descending](Price p1, Price p2) {
        return descending ? (p1 < p2) : (p1 > p2);
    };

    auto it = std::lower_bound(vec.begin(), vec.end(), price, 
                [compare](const Level& l, Price p) {
                    return compare(l.price, p);
                });

    if (it != vec.end() && it->price == price) {
        if (size == 0.0)
            vec.erase(it);
        else
            it->size = size;
        return;
    }

    if (size == 0.0) return;

    vec.insert(it, Level{price, size});
}

void OrderBook::refresh_bests()
{
    if (!bids_.empty()) best_bid_ = bids_.rbegin()->price;
    else best_bid_ = std::numeric_limits<double>::quiet_NaN();

    if (!asks_.empty()) best_ask_ = asks_.rbegin()->price;
    else best_ask_ = std::numeric_limits<double>::quiet_NaN();
}