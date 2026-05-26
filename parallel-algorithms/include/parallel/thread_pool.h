#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <memory>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

#include "join_threads.h"

namespace parallel {

class thread_pool {
public:
    explicit thread_pool(std::size_t num_threads = std::thread::hardware_concurrency());
    ~thread_pool();

    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;

    thread_pool(thread_pool&&) = delete;
    thread_pool& operator=(thread_pool&&) = delete;

    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;

    void wait();
    bool run_pending_task();
    bool is_worker_thread() const;

    std::size_t size() const { return workers_.size(); }

private:
    struct task_queue {
        bool try_push_front(std::function<void()> task);
        bool try_pop_front(std::function<void()>& task);
        bool try_steal_back(std::function<void()>& task);

        std::mutex mutex;
        std::deque<std::function<void()>> tasks;
    };

    void worker_thread();
    void worker_thread(std::size_t index);
    bool try_pop_local_task(std::function<void()>& task);
    bool try_pop_global_task(std::function<void()>& task);
    bool try_steal_task(std::function<void()>& task);
    void execute_task(std::function<void()> task);
    std::size_t local_queue_index() const;

    std::queue<std::function<void()>> global_tasks_;
    std::vector<std::thread> workers_;
    std::vector<std::unique_ptr<task_queue>> local_queues_;

    mutable std::mutex global_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> done_{false};
    std::atomic<std::size_t> pending_tasks_{0};
    join_threads joiner_;
};

template<typename F, typename... Args>
auto thread_pool::submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
    using return_type = std::invoke_result_t<F, Args...>;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> result = task->get_future();
    std::function<void()> wrapped_task = [task]() { (*task)(); };

    if (done_.load(std::memory_order_acquire)) {
        throw std::runtime_error("cannot submit to stopped thread pool");
    }

    pending_tasks_.fetch_add(1, std::memory_order_relaxed);

    try {
        if (is_worker_thread()) {
            if (!local_queues_[local_queue_index()]->try_push_front(std::move(wrapped_task))) {
                throw std::runtime_error("failed to push task to local queue");
            }
        } else {
            std::lock_guard<std::mutex> lock(global_mutex_);
            if (done_.load(std::memory_order_acquire)) {
                pending_tasks_.fetch_sub(1, std::memory_order_relaxed);
                throw std::runtime_error("cannot submit to stopped thread pool");
            }
            global_tasks_.emplace(std::move(wrapped_task));
        }
    } catch (...) {
        if (!done_.load(std::memory_order_relaxed)) {
            pending_tasks_.fetch_sub(1, std::memory_order_relaxed);
        }
        throw;
    }

    condition_.notify_one();
    return result;
}

template<typename T>
T wait_for_future(std::future<T>& future, thread_pool& pool) {
    using namespace std::chrono_literals;
    while (future.wait_for(0ms) != std::future_status::ready) {
        if (!pool.run_pending_task()) {
            std::this_thread::yield();
        }
    }
    return future.get();
}

inline void wait_for_future(std::future<void>& future, thread_pool& pool) {
    using namespace std::chrono_literals;
    while (future.wait_for(0ms) != std::future_status::ready) {
        if (!pool.run_pending_task()) {
            std::this_thread::yield();
        }
    }
    future.get();
}

} // namespace parallel
