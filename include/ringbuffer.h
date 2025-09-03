#pragma once
#include <vector>
#include <atomic>
#include <optional>

template<typename T>
class RingBuffer {
public:
    RingBuffer(size_t size) : head_{0}, tail_{0}, size_{size}
    {
        buffer_.resize(size_);
    }

    bool push(const T& item)
    {
        size_t curr_head = head_.load(std::memory_order_relaxed);
        size_t next = (curr_head + 1) % size_;
    
        // if head wraps around back to the tail, the buffer is full
        if (next == tail_.load(std::memory_order_acquire)) return false;
    
        buffer_[curr_head] = item;
        head_.store(next, std::memory_order_release);
    
        return true;
    }

    bool pop(T& out)
    {
        size_t curr_tail = tail_.load(std::memory_order_relaxed);

        // if tail catches up to head, the buffer is empty
        if (curr_tail == head_.load(std::memory_order_acquire)) return false;
        
        out =  buffer_[curr_tail];
        tail_.store((curr_tail +  1) % size_, std::memory_order_release);
        return true;
    }

private:
    std::vector<T> buffer_;
    std::atomic<size_t> head_;
    std::atomic<size_t> tail_;
    size_t size_;
};
