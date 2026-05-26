#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>
#include <thread>
#include <chrono>
#include <stdexcept>

#include "test_assert.h"
#include "parallel/thread_pool.h"
#include "parallel/parallel_algorithms.h"

TEST_CASE(thread_pool, creation_default) {
    parallel::thread_pool pool;
    TEST_ASSERT(pool.size() > 0, "Thread pool should have threads");
}

TEST_CASE(thread_pool, creation_with_size) {
    parallel::thread_pool pool(4);
    TEST_ASSERT(pool.size() == 4, "Thread pool should have specified size");
}

TEST_CASE(thread_pool, submit_returns_future) {
    parallel::thread_pool pool(2);
    auto future = pool.submit([]() { return 42; });
    TEST_ASSERT_EQUAL(42, future.get(), "Future should return correct value");
}

TEST_CASE(thread_pool, multiple_tasks) {
    parallel::thread_pool pool(4);
    std::vector<std::future<int>> futures;

    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool.submit([i]() { return i * i; }));
    }

    for (int i = 0; i < 10; ++i) {
        TEST_ASSERT_EQUAL(i * i, futures[i].get(), "Task should return correct result");
    }
}

TEST_CASE(thread_pool, wait_blocks_until_complete) {
    parallel::thread_pool pool(2);
    bool completed = false;

    pool.submit([&completed]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        completed = true;
    });

    pool.wait();
    TEST_ASSERT(completed, "Task should be completed after wait()");
}

TEST_CASE(thread_pool, pool_destruction) {
    bool completed = false;

    {
        parallel::thread_pool pool(2);
        pool.submit([&completed]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            completed = true;
        });
    }

    TEST_ASSERT(completed, "Pool destruction should wait for tasks");
}

TEST_CASE(thread_pool, nested_tasks_do_not_deadlock_single_worker) {
    parallel::thread_pool pool(1);

    auto outer = pool.submit([&pool]() {
        auto inner = pool.submit([]() {
            return 21;
        });
        return parallel::wait_for_future(inner, pool) * 2;
    });

    TEST_ASSERT_EQUAL(42, outer.get(), "Worker should execute pending tasks while waiting");
}
