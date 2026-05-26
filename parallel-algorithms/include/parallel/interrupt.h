#pragma once

#include <atomic>

namespace parallel {

class interrupt_flag {
public:
    void interrupt() {
        flag_.store(true, std::memory_order_release);
    }

    bool is_set() const {
        return flag_.load(std::memory_order_acquire);
    }

private:
    std::atomic<bool> flag_{false};
};

} // namespace parallel
