#include <iostream>
#include <vector>
#include <numeric>
#include <random>
#include <chrono>

#include "parallel/thread_pool.h"
#include "parallel/parallel_algorithms.h"

namespace {

constexpr int benchmark_runs = 5;

double measure_serial_accumulate(const std::vector<int>& data, long long& result_out) {
    double total_ms = 0.0;
    result_out = 0;

    for (int run = 0; run < benchmark_runs; ++run) {
        auto start = std::chrono::high_resolution_clock::now();
        long long result = std::accumulate(data.begin(), data.end(), 0LL);
        auto end = std::chrono::high_resolution_clock::now();
        result_out = result;
        total_ms += std::chrono::duration<double, std::milli>(end - start).count();
    }

    return total_ms / benchmark_runs;
}

double measure_parallel_accumulate(const std::vector<int>& data, std::size_t thread_count, long long& result_out) {
    double total_ms = 0.0;
    result_out = 0;

    for (int run = 0; run < benchmark_runs; ++run) {
        parallel::thread_pool pool(thread_count);
        auto start = std::chrono::high_resolution_clock::now();
        long long result = parallel::parallel_accumulate(data.begin(), data.end(), 0LL, pool);
        auto end = std::chrono::high_resolution_clock::now();
        result_out = result;
        total_ms += std::chrono::duration<double, std::milli>(end - start).count();
    }

    return total_ms / benchmark_runs;
}

double measure_parallel_accumulate_v2(const std::vector<int>& data, std::size_t thread_count, long long& result_out) {
    double total_ms = 0.0;
    result_out = 0;

    for (int run = 0; run < benchmark_runs; ++run) {
        parallel::thread_pool pool(thread_count);
        auto start = std::chrono::high_resolution_clock::now();
        long long result = parallel::parallel_accumulate_v2(data.begin(), data.end(), 0LL, pool);
        auto end = std::chrono::high_resolution_clock::now();
        result_out = result;
        total_ms += std::chrono::duration<double, std::milli>(end - start).count();
    }

    return total_ms / benchmark_runs;
}

} // namespace

void benchmark_accumulate() {
    std::cout << "\n=== accumulate Benchmark ===\n";

    const std::vector<std::size_t> sizes = {10000, 1000000, 100000000};
    const std::size_t hardware_threads = std::max(1u, std::thread::hardware_concurrency());

    for (std::size_t size : sizes) {
        std::cout << "\nData size: " << size << "\n";

        std::vector<int> data(size);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(1, 100);

        long long expected_sum = 0;
        for (std::size_t i = 0; i < size; ++i) {
            data[i] = dist(gen);
            expected_sum += data[i];
        }

        long long serial_result = 0;
        long long parallel_result = 0;
        long long parallel_result_v2 = 0;

        double serial_time = measure_serial_accumulate(data, serial_result);
        double parallel_time = measure_parallel_accumulate(data, hardware_threads, parallel_result);
        double parallel_time_v2 = measure_parallel_accumulate_v2(data, hardware_threads, parallel_result_v2);

        double speedup = serial_time / parallel_time;
        bool correct = (serial_result == expected_sum) &&
                       (parallel_result == expected_sum) &&
                       (parallel_result_v2 == expected_sum);

        std::cout << "  Serial avg (5 runs):             " << serial_time << " ms\n";
        std::cout << "  Parallel avg (block-based):      " << parallel_time << " ms\n";
        std::cout << "  Parallel avg (partition-based):  " << parallel_time_v2 << " ms\n";
        std::cout << "  Speedup:                         " << speedup << "x\n";
        std::cout << "  Correct:                         " << (correct ? "YES" : "NO") << "\n";
    }
}
