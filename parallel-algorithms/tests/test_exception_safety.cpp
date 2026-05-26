#include <vector>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <chrono>

#include "test_assert.h"
#include "parallel/thread_pool.h"
#include "parallel/parallel_algorithms.h"

TEST_CASE(exception, tasks_handle_exceptions) {
    parallel::thread_pool pool(2);

    auto future1 = pool.submit([]() { throw std::runtime_error("test error"); });
    auto future2 = pool.submit([]() { return 42; });

    bool threw = false;
    try {
        future1.get();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    TEST_ASSERT(threw, "Exception in task should propagate");

    TEST_ASSERT_EQUAL(42, future2.get(), "Other tasks should still complete");
}

TEST_CASE(exception, for_each_handles_throwing_functors) {
    parallel::thread_pool pool(4);
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    bool threw = false;

    try {
        parallel::parallel_for_each(data.begin(), data.end(),
            [&threw](int& x) {
                if (x == 5) throw std::runtime_error("test");
                x *= 2;
            }, pool);
    } catch (const std::runtime_error&) {
        threw = true;
    }

    TEST_ASSERT(threw, "Exception in functor should propagate");
}

TEST_CASE(exception, accumulate_empty_container) {
    parallel::thread_pool pool(4);
    std::vector<int> empty_data;

    auto result = parallel::parallel_accumulate(empty_data.begin(), empty_data.end(), 100, pool);

    TEST_ASSERT_EQUAL(100, result, "Empty container should return initial value");
}

TEST_CASE(exception, quick_sort_empty_container) {
    parallel::thread_pool pool(4);
    std::vector<int> empty_data;

    parallel::parallel_quick_sort(empty_data.begin(), empty_data.end(), pool);

    TEST_ASSERT(empty_data.empty(), "Empty container should remain empty");
}

TEST_CASE(exception, partial_sum_empty_range) {
    parallel::thread_pool pool(4);
    std::vector<int> data;
    parallel::parallel_partial_sum(data.begin(), data.end(), pool);
    TEST_ASSERT(data.empty(), "Empty range should remain empty");
}

TEST_CASE(exception, partial_sum_single_element) {
    parallel::thread_pool pool(4);
    std::vector<int> data = {42};
    parallel::parallel_partial_sum(data.begin(), data.end(), pool);
    TEST_ASSERT_EQUAL(42, data[0], "Single element should be unchanged");
}

TEST_CASE(exception, pool_destruction_waits) {
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

TEST_CASE(exception, multiple_algorithms_in_sequence) {
    parallel::thread_pool pool(4);

    std::vector<int> data1 = {1, 2, 3, 4, 5};
    parallel::parallel_for_each(data1.begin(), data1.end(), [](int& x) { x *= 2; }, pool);

    auto sum = parallel::parallel_accumulate(data1.begin(), data1.end(), 0, pool);
    TEST_ASSERT_EQUAL(30, sum, "Sum after for_each should be correct");

    parallel::parallel_quick_sort(data1.begin(), data1.end(), pool);
    std::vector<int> expected = {2, 4, 6, 8, 10};
    TEST_ASSERT(data1 == expected, "Data should be sorted");
}