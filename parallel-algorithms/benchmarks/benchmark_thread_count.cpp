#include <iostream>
#include <vector>
#include <numeric>
#include <random>
#include <chrono>
#include <iomanip>

#include "parallel/thread_pool.h"
#include "parallel/parallel_algorithms.h"

namespace {

constexpr int benchmark_runs = 5;

double measure_parallel_accumulate_with_threads(const std::vector<int>& data, int num_threads) {
    double total_ms = 0.0;
    long long checksum = 0;

    for (int run = 0; run < benchmark_runs; ++run) {
        parallel::thread_pool pool(static_cast<std::size_t>(num_threads));
        auto start = std::chrono::high_resolution_clock::now();
        checksum += parallel::parallel_accumulate(data.begin(), data.end(), 0LL, pool);
        auto end = std::chrono::high_resolution_clock::now();
        total_ms += std::chrono::duration<double, std::milli>(end - start).count();
    }

    if (checksum == 0) {
        std::cerr << "unexpected zero checksum\n";
    }

    return total_ms / benchmark_runs;
}

double measure_serial_accumulate(const std::vector<int>& data) {
    double total_ms = 0.0;
    long long checksum = 0;

    for (int run = 0; run < benchmark_runs; ++run) {
        auto start = std::chrono::high_resolution_clock::now();
        checksum += std::accumulate(data.begin(), data.end(), 0LL);
        auto end = std::chrono::high_resolution_clock::now();
        total_ms += std::chrono::duration<double, std::milli>(end - start).count();
    }

    if (checksum == 0) {
        std::cerr << "unexpected zero checksum\n";
    }

    return total_ms / benchmark_runs;
}

} // namespace

void benchmark_thread_count() {
    std::cout << "\n=== Thread Count Scaling Benchmark ===\n";

    const std::size_t data_size = 100000000;
    std::vector<int> base_data(data_size);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 1000);

    for (std::size_t i = 0; i < data_size; ++i) {
        base_data[i] = dist(gen);
    }

    std::cout << "\nData size: " << data_size << "\n";
    std::cout << std::string(70, '-') << "\n";

    const int hardware_threads = static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));
    const std::vector<int> thread_counts = {1, hardware_threads, hardware_threads * 2};
    const double serial_time = measure_serial_accumulate(base_data);

    for (int num_threads : thread_counts) {
        double parallel_time = measure_parallel_accumulate_with_threads(base_data, num_threads);
        const double speedup = serial_time / parallel_time;
        std::cout << "Threads: " << std::setw(3) << num_threads
                  << " | Time: " << std::fixed << std::setprecision(2)
                  << std::setw(8) << parallel_time << " ms"
                  << " | Speedup: " << std::setw(6) << speedup << "x\n";
    }

    std::cout << std::string(70, '-') << "\n";
    std::cout << "Serial avg (5 runs):           " << serial_time << " ms\n\n";

    std::cout << "Scaling Summary:\n";
    std::cout << std::string(50, '-') << "\n";
    std::cout << "Threads | Time (ms) | Speedup | Efficiency\n";
    std::cout << std::string(50, '-') << "\n";

    for (int num_threads : thread_counts) {
        double time = measure_parallel_accumulate_with_threads(base_data, num_threads);
        double speedup = serial_time / time;
        double efficiency = (speedup / num_threads) * 100.0;

        std::cout << std::setw(7) << num_threads << " | "
                  << std::setw(10) << std::fixed << std::setprecision(2) << time << " | "
                  << std::setw(7) << speedup << " | "
                  << std::setw(10) << efficiency << "%\n";
    }
    std::cout << std::endl;
}
