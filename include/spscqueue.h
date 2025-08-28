#pragma once
#include <vector>
#include <atomic>
#include <optional>

template<typename T, size_t Size>
class SPSCQueue {
public:
    SPSCQueue();

    bool push(const T& item);
    std::optional<T> pop();

private:
    std::vector<T> buffer_;
    std::atomic<size_t> store_;
    std::atomic<size_t> remove_;
};
