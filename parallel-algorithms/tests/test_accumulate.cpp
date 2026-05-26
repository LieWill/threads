#include <vector>
#include <numeric>
#include <random>

#include "test_assert.h"
#include "parallel/thread_pool.h"
#include "parallel/parallel_algorithms.h"

TEST_CASE(accumulate, basic_functionality) {
    parallel::thread_pool pool(4);
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    auto result = parallel::parallel_accumulate(data.begin(), data.end(), 0, pool);

    TEST_ASSERT_EQUAL(55, result, "Accumulate should sum all elements");
}

TEST_CASE(accumulate, empty_range) {
    parallel::thread_pool pool(4);
    std::vector<int> data;

    auto result = parallel::parallel_accumulate(data.begin(), data.end(), 100, pool);

    TEST_ASSERT_EQUAL(100, result, "Empty range should return initial value");
}

TEST_CASE(accumulate, single_element) {
    parallel::thread_pool pool(4);
    std::vector<int> data = {42};

    auto result = parallel::parallel_accumulate(data.begin(), data.end(), 0, pool);

    TEST_ASSERT_EQUAL(42, result, "Single element should be returned");
}

TEST_CASE(accumulate, result_matches_serial) {
    parallel::thread_pool pool(4);
    const std::size_t size = 100000;
    std::vector<int> data(size);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 1000);

    for (std::size_t i = 0; i < size; ++i) {
        data[i] = dist(gen);
    }

    int init = 1000;
    auto parallel_result = parallel::parallel_accumulate(data.begin(), data.end(), init, pool);
    auto serial_result = std::accumulate(data.begin(), data.end(), init);

    TEST_ASSERT_EQUAL(serial_result, parallel_result, "Parallel should match serial");
}

TEST_CASE(accumulate, v2_works_correctly) {
    parallel::thread_pool pool(4);
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    auto result = parallel::parallel_accumulate_v2(data.begin(), data.end(), 0, pool);

    TEST_ASSERT_EQUAL(55, result, "Accumulate v2 should sum all elements");
}

TEST_CASE(accumulate, double_type) {
    parallel::thread_pool pool(4);
    std::vector<double> data = {1.5, 2.5, 3.5, 4.5, 5.5};

    auto result = parallel::parallel_accumulate(data.begin(), data.end(), 0.0, pool);

    double expected = 17.5;
    TEST_ASSERT(result > expected - 0.001 && result < expected + 0.001, "Double accumulate should be correct");
}

TEST_CASE(accumulate, custom_initial_value) {
    parallel::thread_pool pool(4);
    std::vector<int> data = {10, 20, 30};

    auto result = parallel::parallel_accumulate(data.begin(), data.end(), 100, pool);

    TEST_ASSERT_EQUAL(160, result, "Initial value should be included");
}