#include "spscqueue.h"

template<typename T, size_t Size>
SPSCQueue<T, Size>::SPSCQueue() : head_{0}, tail_{0}
{
    buffer_.resize(Size);
}

template<typename T, size_t Size>
bool SPSCQueue<T, Size>::push(const T& item)
{
    size_t curr_head = head_.load(std::memory_order_relaxed);
    size_t next = (curr_head + 1) % Size;

    // if head wraps around back to the tail, the buffer is full
    if (next == tail_.load(std::memory_order_acquire)) return false;

    buffer_[curr_head] = item;
    head.store(next, std::memory_order_release);

    return true;
}

template<typename T, size_t Size>
std::optional<T> SPSCQueue<T, Size>::pop()
{
    size_t curr_tail = tail_.load(std::memory_order_relaxed);

    // if tail catches up to head, the buffer is empty
    if (curr_tail == head_.load(std::memory_order_acquire)) return {};
    
    T item =  buffer_[curr_tail];
    tail.store((curr_tail +  1) % Size, std::memory_order_release);
    return item;
}