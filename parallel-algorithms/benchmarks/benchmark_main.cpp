#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>
#include <functional>
#include <iomanip>
#include <thread>
#include <chrono>

#include "parallel/thread_pool.h"
#include "parallel/parallel_algorithms.h"

void benchmark_for_each();
void benchmark_accumulate();
void benchmark_quick_sort();
void benchmark_thread_count();
void benchmark_bounded_buffer();

int main() {
    std::cout << "===========================================\n";
    std::cout << "   Parallel Algorithms Benchmark Suite\n";
    std::cout << "===========================================\n\n";

    const std::size_t num_threads = std::thread::hardware_concurrency();
    std::cout << "Detected " << num_threads << " CPU cores\n\n";

    benchmark_for_each();
    benchmark_accumulate();
    benchmark_quick_sort();
    benchmark_thread_count();
    benchmark_bounded_buffer();

    std::cout << "===========================================\n";
    std::cout << "          Benchmark Complete\n";
    std::cout << "===========================================\n";

    return 0;
}
