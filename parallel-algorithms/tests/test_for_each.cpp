#include <vector>
#include <algorithm>
#include <numeric>
#include <random>

#include "test_assert.h"
#include "parallel/thread_pool.h"
#include "parallel/parallel_algorithms.h"

TEST_CASE(for_each, basic_functionality) {
    parallel::thread_pool pool(4);
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    parallel::parallel_for_each(data.begin(), data.end(), [](int& x) { x *= 2; }, pool);

    std::vector<int> expected = {2, 4, 6, 8, 10, 12, 14, 16, 18, 20};
    TEST_ASSERT(data == expected, "for_each should transform all elements");
}

TEST_CASE(for_each, empty_range) {
    parallel::thread_pool pool(4);
    std::vector<int> data;

    parallel::parallel_for_each(data.begin(), data.end(), [](int& x) { x *= 2; }, pool);

    TEST_ASSERT(data.empty(), "Empty range should remain empty");
}

TEST_CASE(for_each, single_element) {
    parallel::thread_pool pool(4);
    std::vector<int> data = {5};

    parallel::parallel_for_each(data.begin(), data.end(), [](int& x) { x += 10; }, pool);

    TEST_ASSERT_EQUAL(15, data[0], "Single element should be transformed");
}

TEST_CASE(for_each, result_matches_serial) {
    parallel::thread_pool pool(4);
    const std::size_t size = 10000;
    std::vector<int> data1(size), data2(size);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 1000);

    for (std::size_t i = 0; i < size; ++i) {
        data1[i] = dist(gen);
        data2[i] = data1[i];
    }

    parallel::parallel_for_each(data1.begin(), data1.end(), [](int& x) { x = x * 2 + 1; }, pool);
    std::for_each(data2.begin(), data2.end(), [](int& x) { x = x * 2 + 1; });

    TEST_ASSERT(data1 == data2, "Parallel for_each should match serial");
}

TEST_CASE(for_each, with_state_capture) {
    parallel::thread_pool pool(4);
    std::vector<int> data = {1, 2, 3, 4, 5};
    int sum = 0;

    parallel::parallel_for_each(data.begin(), data.end(), [&sum](int x) { sum += x; }, pool);

    int expected_sum = std::accumulate(data.begin(), data.end(), 0);
    TEST_ASSERT_EQUAL(expected_sum, sum, "Sum should match expected");
}