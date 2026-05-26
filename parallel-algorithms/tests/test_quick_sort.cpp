#include <vector>
#include <algorithm>
#include <random>
#include <functional>

#include "test_assert.h"
#include "parallel/thread_pool.h"
#include "parallel/parallel_algorithms.h"

TEST_CASE(quick_sort, basic_functionality) {
    parallel::thread_pool pool(4);
    std::vector<int> data = {5, 3, 8, 1, 9, 2, 7, 4, 6};

    parallel::parallel_quick_sort(data.begin(), data.end(), pool);

    TEST_ASSERT(std::is_sorted(data.begin(), data.end()), "Data should be sorted");
}

TEST_CASE(quick_sort, empty_range) {
    parallel::thread_pool pool(4);
    std::vector<int> data;

    parallel::parallel_quick_sort(data.begin(), data.end(), pool);

    TEST_ASSERT(data.empty(), "Empty range should remain empty");
}

TEST_CASE(quick_sort, single_element) {
    parallel::thread_pool pool(4);
    std::vector<int> data = {42};

    parallel::parallel_quick_sort(data.begin(), data.end(), pool);

    TEST_ASSERT_EQUAL(1, data.size(), "Single element should remain");
    TEST_ASSERT_EQUAL(42, data[0], "Single element should be unchanged");
}

TEST_CASE(quick_sort, already_sorted) {
    parallel::thread_pool pool(4);
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    parallel::parallel_quick_sort(data.begin(), data.end(), pool);

    TEST_ASSERT(std::is_sorted(data.begin(), data.end()), "Already sorted should remain sorted");
}

TEST_CASE(quick_sort, reverse_sorted) {
    parallel::thread_pool pool(4);
    std::vector<int> data = {9, 8, 7, 6, 5, 4, 3, 2, 1};

    parallel::parallel_quick_sort(data.begin(), data.end(), pool);

    TEST_ASSERT(std::is_sorted(data.begin(), data.end()), "Reverse sorted should become sorted");
}

TEST_CASE(quick_sort, duplicate_elements) {
    parallel::thread_pool pool(4);
    std::vector<int> data = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};

    parallel::parallel_quick_sort(data.begin(), data.end(), pool);

    TEST_ASSERT(std::is_sorted(data.begin(), data.end()), "Duplicates should be handled");
}

TEST_CASE(quick_sort, result_matches_serial) {
    parallel::thread_pool pool(4);
    const std::size_t size = 10000;
    std::vector<int> data1(size), data2(size);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 100000);

    for (std::size_t i = 0; i < size; ++i) {
        data1[i] = dist(gen);
        data2[i] = data1[i];
    }

    parallel::parallel_quick_sort(data1.begin(), data1.end(), pool);
    std::sort(data2.begin(), data2.end());

    TEST_ASSERT(data1 == data2, "Parallel should match serial");
}

TEST_CASE(quick_sort, custom_comparator) {
    parallel::thread_pool pool(4);
    std::vector<int> data = {5, 3, 8, 1, 9, 2, 7, 4, 6};

    parallel::parallel_quick_sort(data.begin(), data.end(), std::greater<int>(), pool);

    bool is_descending = true;
    for (size_t i = 1; i < data.size(); ++i) {
        if (data[i-1] < data[i]) {
            is_descending = false;
            break;
        }
    }
    TEST_ASSERT(is_descending, "Custom comparator (descending) should work");
}

TEST_CASE(quick_sort, large_dataset) {
    parallel::thread_pool pool(4);
    const std::size_t size = 10000;
    std::vector<int> data(size);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 100000);

    for (std::size_t i = 0; i < size; ++i) {
        data[i] = dist(gen);
    }

    parallel::parallel_quick_sort(data.begin(), data.end(), pool);

    TEST_ASSERT(std::is_sorted(data.begin(), data.end()), "Large dataset should be sorted");
}