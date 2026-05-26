#include "parallel/thread_pool.h"

#include <chrono>
#include <limits>
#include <utility>

namespace parallel {

namespace {

thread_local thread_pool* current_pool = nullptr;
thread_local std::size_t current_worker_index = std::numeric_limits<std::size_t>::max();

} // namespace

bool thread_pool::task_queue::try_push_front(std::function<void()> task) {
    std::lock_guard<std::mutex> lock(mutex);
    tasks.push_front(std::move(task));
    return true;
}

bool thread_pool::task_queue::try_pop_front(std::function<void()>& task) {
    std::lock_guard<std::mutex> lock(mutex);
    if (tasks.empty()) {
        return false;
    }
    task = std::move(tasks.front());
    tasks.pop_front();
    return true;
}

bool thread_pool::task_queue::try_steal_back(std::function<void()>& task) {
    std::lock_guard<std::mutex> lock(mutex);
    if (tasks.empty()) {
        return false;
    }
    task = std::move(tasks.back());
    tasks.pop_back();
    return true;
}

thread_pool::thread_pool(std::size_t num_threads)
    : joiner_(workers_) {
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) {
            num_threads = 2;
        }
    }

    local_queues_.reserve(num_threads);
    for (std::size_t i = 0; i < num_threads; ++i) {
        local_queues_.push_back(std::make_unique<task_queue>());
    }

    for (std::size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this, i]() {
            worker_thread(i);
        });
    }
}

thread_pool::~thread_pool() {
    done_.store(true, std::memory_order_release);
    condition_.notify_all();
}

void thread_pool::wait() {
    using namespace std::chrono_literals;
    while (pending_tasks_.load(std::memory_order_acquire) != 0) {
        if (!run_pending_task()) {
            std::unique_lock<std::mutex> lock(global_mutex_);
            condition_.wait_for(lock, 1ms, [this]() {
                return pending_tasks_.load(std::memory_order_acquire) == 0 ||
                       !global_tasks_.empty() ||
                       done_.load(std::memory_order_acquire);
            });
        }
    }
}

bool thread_pool::is_worker_thread() const {
    return current_pool == this;
}

std::size_t thread_pool::local_queue_index() const {
    return current_worker_index;
}

bool thread_pool::try_pop_local_task(std::function<void()>& task) {
    if (!is_worker_thread()) {
        return false;
    }
    return local_queues_[local_queue_index()]->try_pop_front(task);
}

bool thread_pool::try_pop_global_task(std::function<void()>& task) {
    std::lock_guard<std::mutex> lock(global_mutex_);
    if (global_tasks_.empty()) {
        return false;
    }
    task = std::move(global_tasks_.front());
    global_tasks_.pop();
    return true;
}

bool thread_pool::try_steal_task(std::function<void()>& task) {
    if (local_queues_.empty()) {
        return false;
    }

    const std::size_t start = is_worker_thread() ? local_queue_index() : 0;
    for (std::size_t offset = 1; offset <= local_queues_.size(); ++offset) {
        const std::size_t victim = (start + offset) % local_queues_.size();
        if (is_worker_thread() && victim == local_queue_index()) {
            continue;
        }
        if (local_queues_[victim]->try_steal_back(task)) {
            return true;
        }
    }

    return false;
}

bool thread_pool::run_pending_task() {
    std::function<void()> task;

    if (try_pop_local_task(task) || try_pop_global_task(task) || try_steal_task(task)) {
        execute_task(std::move(task));
        return true;
    }

    return false;
}

void thread_pool::execute_task(std::function<void()> task) {
    try {
        task();
    } catch (...) {
    }

    if (pending_tasks_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        condition_.notify_all();
    } else {
        condition_.notify_one();
    }
}

void thread_pool::worker_thread() {
    worker_thread(0);
}

void thread_pool::worker_thread(std::size_t index) {
    current_pool = this;
    current_worker_index = index;

    using namespace std::chrono_literals;
    while (true) {
        if (run_pending_task()) {
            continue;
        }

        if (done_.load(std::memory_order_acquire) &&
            pending_tasks_.load(std::memory_order_acquire) == 0) {
            break;
        }

        std::unique_lock<std::mutex> lock(global_mutex_);
        condition_.wait_for(lock, 1ms, [this]() {
            return done_.load(std::memory_order_acquire) ||
                   !global_tasks_.empty() ||
                   pending_tasks_.load(std::memory_order_acquire) == 0;
        });
    }

    current_pool = nullptr;
    current_worker_index = std::numeric_limits<std::size_t>::max();
}

} // namespace parallel
