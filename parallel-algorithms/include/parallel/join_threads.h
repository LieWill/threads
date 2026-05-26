#pragma once

#include <thread>
#include <vector>

namespace parallel {

class join_threads {
public:
    explicit join_threads(std::vector<std::thread>& threads) : threads_(threads) {}

    ~join_threads() {
        for (auto& t : threads_) {
            if (t.joinable()) {
                t.join();
            }
        }
    }

    join_threads(const join_threads&) = delete;
    join_threads& operator=(const join_threads&) = delete;

private:
    std::vector<std::thread>& threads_;
};

} // namespace parallel