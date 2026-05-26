#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <new>
#include <optional>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "interrupt.h"

namespace parallel {

template<typename T>
class bounded_buffer_lock {
public:
    explicit bounded_buffer_lock(std::size_t capacity)
        : buffer_(capacity), capacity_(capacity) {}

    bounded_buffer_lock(const bounded_buffer_lock&) = delete;
    bounded_buffer_lock& operator=(const bounded_buffer_lock&) = delete;

    bool push(T value) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_full_.wait(lock, [this]() { return closed_ || size_ < capacity_; });
        if (closed_) {
            return false;
        }
        buffer_[tail_] = std::move(value);
        tail_ = (tail_ + 1) % capacity_;
        ++size_;
        lock.unlock();
        not_empty_.notify_one();
        return true;
    }

    bool pop(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_.wait(lock, [this]() { return closed_ || size_ > 0; });
        if (size_ == 0) {
            return false;
        }
        value = std::move(*buffer_[head_]);
        buffer_[head_].reset();
        head_ = (head_ + 1) % capacity_;
        --size_;
        lock.unlock();
        not_full_.notify_one();
        return true;
    }

    bool push(T value, const interrupt_flag& flag,
              std::chrono::milliseconds poll = std::chrono::milliseconds(10)) {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!closed_ && size_ == capacity_) {
            if (flag.is_set()) {
                return false;
            }
            not_full_.wait_for(lock, poll);
        }
        if (closed_ || flag.is_set()) {
            return false;
        }
        buffer_[tail_] = std::move(value);
        tail_ = (tail_ + 1) % capacity_;
        ++size_;
        lock.unlock();
        not_empty_.notify_one();
        return true;
    }

    bool pop(T& value, const interrupt_flag& flag,
             std::chrono::milliseconds poll = std::chrono::milliseconds(10)) {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!closed_ && size_ == 0) {
            if (flag.is_set()) {
                return false;
            }
            not_empty_.wait_for(lock, poll);
        }
        if (size_ == 0) {
            return false;
        }
        value = std::move(*buffer_[head_]);
        buffer_[head_].reset();
        head_ = (head_ + 1) % capacity_;
        --size_;
        lock.unlock();
        not_full_.notify_one();
        return true;
    }

    void close() {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
        not_empty_.notify_all();
        not_full_.notify_all();
    }

    std::size_t capacity() const {
        return capacity_;
    }

private:
    std::vector<std::optional<T>> buffer_;
    const std::size_t capacity_;
    std::size_t head_ = 0;
    std::size_t tail_ = 0;
    std::size_t size_ = 0;
    bool closed_ = false;
    mutable std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
};

template<typename T>
class bounded_buffer_atomic {
public:
    explicit bounded_buffer_atomic(std::size_t capacity)
        : capacity_(capacity),
          cells_(capacity) {
        for (std::size_t i = 0; i < capacity_; ++i) {
            cells_[i].sequence.store(i, std::memory_order_relaxed);
        }
    }

    bounded_buffer_atomic(const bounded_buffer_atomic&) = delete;
    bounded_buffer_atomic& operator=(const bounded_buffer_atomic&) = delete;

    ~bounded_buffer_atomic() {
        close();
        T value;
        while (try_pop(value)) {
        }
    }

    bool push(T value) {
        while (!closed_.load(std::memory_order_acquire)) {
            if (try_push(std::move(value))) {
                return true;
            }
            std::this_thread::yield();
        }
        return false;
    }

    bool pop(T& value) {
        while (!closed_.load(std::memory_order_acquire)) {
            if (try_pop(value)) {
                return true;
            }
            std::this_thread::yield();
        }
        return try_pop(value);
    }

    bool push(T value, const interrupt_flag& flag,
              std::chrono::milliseconds poll = std::chrono::milliseconds(1)) {
        while (!closed_.load(std::memory_order_acquire) && !flag.is_set()) {
            if (try_push(std::move(value))) {
                return true;
            }
            std::this_thread::sleep_for(poll);
        }
        return false;
    }

    bool pop(T& value, const interrupt_flag& flag,
             std::chrono::milliseconds poll = std::chrono::milliseconds(1)) {
        while (!closed_.load(std::memory_order_acquire) && !flag.is_set()) {
            if (try_pop(value)) {
                return true;
            }
            std::this_thread::sleep_for(poll);
        }
        return try_pop(value);
    }

    void close() {
        closed_.store(true, std::memory_order_release);
    }

    std::size_t capacity() const {
        return capacity_;
    }

private:
    struct cell {
        std::atomic<std::size_t> sequence;
        typename std::aligned_storage<sizeof(T), alignof(T)>::type storage;
    };

    static T* value_ptr(cell& slot) {
        return reinterpret_cast<T*>(&slot.storage);
    }

    bool try_push(T&& value) {
        cell* slot = nullptr;
        std::size_t pos = enqueue_pos_.load(std::memory_order_relaxed);
        for (;;) {
            slot = &cells_[pos % capacity_];
            const std::size_t sequence = slot->sequence.load(std::memory_order_acquire);
            const std::intptr_t diff = static_cast<std::intptr_t>(sequence) -
                                       static_cast<std::intptr_t>(pos);

            if (diff == 0) {
                if (enqueue_pos_.compare_exchange_weak(
                        pos, pos + 1, std::memory_order_relaxed, std::memory_order_relaxed)) {
                    break;
                }
            } else if (diff < 0) {
                return false;
            } else {
                pos = enqueue_pos_.load(std::memory_order_relaxed);
            }
        }

        new (&slot->storage) T(std::move(value));
        slot->sequence.store(pos + 1, std::memory_order_release);
        return true;
    }

    bool try_pop(T& value) {
        cell* slot = nullptr;
        std::size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
        for (;;) {
            slot = &cells_[pos % capacity_];
            const std::size_t sequence = slot->sequence.load(std::memory_order_acquire);
            const std::intptr_t diff = static_cast<std::intptr_t>(sequence) -
                                       static_cast<std::intptr_t>(pos + 1);

            if (diff == 0) {
                if (dequeue_pos_.compare_exchange_weak(
                        pos, pos + 1, std::memory_order_relaxed, std::memory_order_relaxed)) {
                    break;
                }
            } else if (diff < 0) {
                return false;
            } else {
                pos = dequeue_pos_.load(std::memory_order_relaxed);
            }
        }

        T* stored = value_ptr(*slot);
        value = std::move(*stored);
        stored->~T();
        slot->sequence.store(pos + capacity_, std::memory_order_release);
        return true;
    }

    const std::size_t capacity_;
    std::vector<cell> cells_;
    alignas(64) std::atomic<std::size_t> enqueue_pos_{0};
    alignas(64) std::atomic<std::size_t> dequeue_pos_{0};
    std::atomic<bool> closed_{false};
};

} // namespace parallel
