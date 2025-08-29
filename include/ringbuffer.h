#pragma once
#include <vector>
#include <atomic>
#include <optional>

template<typename T, size_t Size>
class RingBuffer {
public:
    RingBuffer();

    bool push(const T& item);
    std::optional<T> pop();

private:
    std::vector<T> buffer_;
    std::atomic<size_t> store_;
    std::atomic<size_t> remove_;
};
