#pragma once

#include "thread_pool.h"
#include "join_threads.h"
#include "utils.h"

#include <algorithm>
#include <numeric>
#include <vector>
#include <iterator>
#include <future>
#include <type_traits>
#include <stack>
#include <random>
#include <cmath>

namespace parallel {

constexpr std::size_t min_parallel_size = 1000;
constexpr std::size_t grain_size = 100;

template<typename Iterator, typename Func>
void parallel_for_each(Iterator first, Iterator last, Func func, thread_pool& pool);

template<typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init, thread_pool& pool);

template<typename Iterator, typename T>
T parallel_accumulate_v2(Iterator first, Iterator last, T init, thread_pool& pool);

template<typename Iterator>
void parallel_partial_sum(Iterator first, Iterator last, thread_pool& pool);

template<typename Iterator, typename Compare>
void parallel_quick_sort(Iterator first, Iterator last, Compare comp, thread_pool& pool);

template<typename Iterator>
void parallel_quick_sort(Iterator first, Iterator last, thread_pool& pool);

namespace detail {

template<typename Iterator, typename T>
struct accumulate_chunk {
    T result;
    Iterator first;
    Iterator last;
};

template<typename Iterator, typename Func>
void for_each_chunk(Iterator first, Iterator last, Func func) {
    for (auto it = first; it != last; ++it) {
        func(*it);
    }
}

template<typename Iterator, typename T>
T accumulate_block(Iterator first, Iterator last, T init) {
    return std::accumulate(first, last, init);
}

template<typename Iterator, typename T>
std::vector<accumulate_chunk<Iterator, T>> partition_work(Iterator first, Iterator last, std::size_t num_partitions) {
    std::vector<accumulate_chunk<Iterator, T>> chunks;
    const std::size_t total_size = static_cast<std::size_t>(std::distance(first, last));

    if (total_size == 0) return chunks;

    const std::size_t chunk_size = std::max(total_size / num_partitions, static_cast<std::size_t>(1));

    Iterator chunk_start = first;
    while (chunk_start != last) {
        Iterator chunk_end = chunk_start;
        std::size_t current_chunk_size = 0;
        while (chunk_end != last && current_chunk_size < chunk_size) {
            ++chunk_end;
            ++current_chunk_size;
        }
        chunks.push_back({T{}, chunk_start, chunk_end});
        chunk_start = chunk_end;
    }

    return chunks;
}

template<typename Iterator>
void partial_sum_serial(Iterator first, Iterator last) {
    if (first == last) return;
    auto it = first;
    ++it;
    while (it != last) {
        *it = *it + *(it - 1);
        ++it;
    }
}

template<typename Iterator>
void parallel_partial_sum_impl(Iterator first, Iterator last, thread_pool& pool) {
    const std::size_t n = static_cast<std::size_t>(std::distance(first, last));
    if (n <= grain_size) {
        partial_sum_serial(first, last);
        return;
    }

    const std::size_t mid = n / 2;
    Iterator mid_iter = first;
    std::advance(mid_iter, static_cast<long>(mid));

    auto left_future = pool.submit([first, mid_iter, &pool]() {
        parallel_partial_sum_impl(first, mid_iter, pool);
    });

    parallel_partial_sum_impl(mid_iter, last, pool);
    wait_for_future(left_future, pool);

    auto mid_val = *(mid_iter - 1);

    auto update_right = [mid_val](typename std::iterator_traits<Iterator>::value_type& x) {
        x += mid_val;
    };

    for_each_chunk(mid_iter, last, update_right);
}

template<typename RandomIt>
RandomIt partition(RandomIt first, RandomIt last, typename std::iterator_traits<RandomIt>::value_type pivot) {
    auto mid = first + std::distance(first, last) / 2;
    auto pivot_iter = last - 1;

    std::swap(*mid, *pivot_iter);
    pivot = *pivot_iter;

    auto store_it = first;
    for (auto it = first; it != pivot_iter; ++it) {
        if (*it < pivot) {
            std::swap(*it, *store_it);
            ++store_it;
        }
    }
    std::swap(*store_it, *pivot_iter);
    return store_it;
}

template<typename RandomIt, typename Compare>
void quick_sort_serial(RandomIt first, RandomIt last, Compare comp) {
    if (first >= last || std::distance(first, last) <= 1) return;

    auto mid = first + std::distance(first, last) / 2;
    auto pivot_iter = last - 1;

    auto pivot = *mid;
    std::swap(*mid, *pivot_iter);

    auto store_it = first;
    for (auto it = first; it != pivot_iter; ++it) {
        if (comp(*it, pivot)) {
            std::swap(*it, *store_it);
            ++store_it;
        }
    }
    std::swap(*store_it, *pivot_iter);

    quick_sort_serial(first, store_it, comp);
    quick_sort_serial(store_it + 1, last, comp);
}

template<typename RandomIt, typename Compare>
void parallel_quick_sort_impl(RandomIt first, RandomIt last, Compare comp, thread_pool& pool, std::size_t depth = 0) {
    constexpr std::size_t max_depth = 4;
    constexpr std::size_t min_size_for_parallel = 10000;

    const auto n = static_cast<std::size_t>(std::distance(first, last));
    if (n <= 1) return;

    while (n >= min_size_for_parallel && depth < max_depth) {
        auto mid = first + n / 2;
        auto pivot = *mid;
        std::swap(*mid, *(last - 1));

        auto store_it = first;
        for (auto it = first; it != last - 1; ++it) {
            if (comp(*it, pivot)) {
                std::swap(*it, *store_it);
                ++store_it;
            }
        }
        std::swap(*store_it, *(last - 1));

        auto right_future = pool.submit([store_it, last, &comp, &pool, depth]() {
            parallel_quick_sort_impl(store_it + 1, last, comp, pool, depth + 1);
        });

        parallel_quick_sort_impl(first, store_it, comp, pool, depth + 1);
        wait_for_future(right_future, pool);
        return;
    }

    quick_sort_serial(first, last, comp);
}

} // namespace detail

template<typename Iterator, typename Func>
void parallel_for_each(Iterator first, Iterator last, Func func, thread_pool& pool) {
    const std::size_t n = static_cast<std::size_t>(std::distance(first, last));
    if (n == 0) return;

    const std::size_t num_threads = pool.size();
    const std::size_t chunk_size = std::max(n / num_threads, grain_size);

    std::vector<std::future<void>> futures;
    Iterator chunk_start = first;

    while (chunk_start < last) {
        Iterator chunk_end = chunk_start;
        std::size_t count = 0;
        while (chunk_end < last && count < chunk_size) {
            ++chunk_end;
            ++count;
        }

        futures.push_back(pool.submit([chunk_start, chunk_end, &func]() {
            detail::for_each_chunk(chunk_start, chunk_end, func);
        }));

        chunk_start = chunk_end;
    }

    for (auto& f : futures) {
        wait_for_future(f, pool);
    }
}

template<typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init, thread_pool& pool) {
    const std::size_t n = static_cast<std::size_t>(std::distance(first, last));
    if (n == 0) return init;

    const std::size_t num_threads = pool.size();
    const std::size_t chunk_size = std::max(n / num_threads, static_cast<std::size_t>(1));

    using ChunkType = detail::accumulate_chunk<Iterator, T>;
    std::vector<std::future<ChunkType>> futures;
    Iterator chunk_start = first;

    while (chunk_start < last) {
        Iterator chunk_end = chunk_start;
        std::size_t count = 0;
        while (chunk_end < last && count < chunk_size) {
            ++chunk_end;
            ++count;
        }

        futures.push_back(pool.submit([chunk_start, chunk_end]() {
            ChunkType chunk;
            chunk.first = chunk_start;
            chunk.last = chunk_end;
            chunk.result = detail::accumulate_block(chunk_start, chunk_end, T{});
            return chunk;
        }));

        chunk_start = chunk_end;
    }

    T result = init;
    for (auto& f : futures) {
        result += wait_for_future(f, pool).result;
    }

    return result;
}

template<typename Iterator, typename T>
T parallel_accumulate_v2(Iterator first, Iterator last, T init, thread_pool& pool) {
    const std::size_t n = static_cast<std::size_t>(std::distance(first, last));
    if (n == 0) return init;

    const std::size_t num_threads = pool.size();
    auto chunks = detail::partition_work<Iterator, T>(first, last, num_threads);

    std::vector<std::future<T>> futures;
    for (auto& chunk : chunks) {
        futures.push_back(pool.submit([chunk]() {
            return detail::accumulate_block(chunk.first, chunk.last, T{});
        }));
    }

    T result = init;
    for (auto& f : futures) {
        result += wait_for_future(f, pool);
    }

    return result;
}

template<typename Iterator>
void parallel_partial_sum(Iterator first, Iterator last, thread_pool& pool) {
    detail::parallel_partial_sum_impl(first, last, pool);
}

template<typename Iterator, typename Compare>
void parallel_quick_sort(Iterator first, Iterator last, Compare comp, thread_pool& pool) {
    detail::parallel_quick_sort_impl(first, last, comp, pool);
}

template<typename Iterator>
void parallel_quick_sort(Iterator first, Iterator last, thread_pool& pool) {
    using value_type = typename std::iterator_traits<Iterator>::value_type;
    parallel_quick_sort(first, last, std::less<value_type>(), pool);
}

} // namespace parallel
