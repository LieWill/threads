#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>

#include "parallel/thread_pool.h"
#include "parallel/parallel_algorithms.h"

namespace {

constexpr int benchmark_runs = 5;

double measure_serial_quick_sort(const std::vector<int>& input, long long& checksum) {
    double total_ms = 0.0;
    checksum = 0;

    for (int run = 0; run < benchmark_runs; ++run) {
        auto data = input;
        auto start = std::chrono::high_resolution_clock::now();
        std::sort(data.begin(), data.end());
        auto end = std::chrono::high_resolution_clock::now();
        checksum += data.front() + data.back();
        total_ms += std::chrono::duration<double, std::milli>(end - start).count();
    }

    return total_ms / benchmark_runs;
}

double measure_parallel_quick_sort(const std::vector<int>& input, std::size_t thread_count, long long& checksum) {
    double total_ms = 0.0;
    checksum = 0;

    for (int run = 0; run < benchmark_runs; ++run) {
        auto data = input;
        parallel::thread_pool pool(thread_count);
        auto start = std::chrono::high_resolution_clock::now();
        parallel::parallel_quick_sort(data.begin(), data.end(), pool);
        auto end = std::chrono::high_resolution_clock::now();
        checksum += data.front() + data.back();
        total_ms += std::chrono::duration<double, std::milli>(end - start).count();
    }

    return total_ms / benchmark_runs;
}

} // namespace

void benchmark_quick_sort() {
    std::cout << "\n=== quick_sort Benchmark ===\n";

    const std::vector<std::size_t> sizes = {10000, 1000000, 10000000};
    const std::size_t hardware_threads = std::max(1u, std::thread::hardware_concurrency());

    for (std::size_t size : sizes) {
        std::cout << "\nData size: " << size << "\n";

        std::vector<int> data(size);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(1, 1000000);

        for (std::size_t i = 0; i < size; ++i) {
            data[i] = dist(gen);
        }

        long long serial_checksum = 0;
        long long parallel_checksum = 0;
        double serial_time = measure_serial_quick_sort(data, serial_checksum);
        double parallel_time = measure_parallel_quick_sort(data, hardware_threads, parallel_checksum);

        double speedup = serial_time / parallel_time;
        bool correct = (serial_checksum == parallel_checksum);

        std::cout << "  Serial avg (5 runs):  " << serial_time << " ms\n";
        std::cout << "  Parallel avg (5 runs): " << parallel_time << " ms\n";
        std::cout << "  Speedup:               " << speedup << "x\n";
        std::cout << "  Correct:               " << (correct ? "YES" : "NO") << "\n";
    }
}
