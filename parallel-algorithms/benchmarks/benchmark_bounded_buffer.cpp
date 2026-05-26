#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include "parallel/bounded_buffer.h"

namespace {

template<typename Buffer>
double measure_throughput(Buffer& buffer, int producers, int consumers, int items_per_producer) {
    std::atomic<int> producers_done{0};
    std::atomic<long long> checksum{0};
    std::vector<std::thread> producer_threads;
    std::vector<std::thread> consumer_threads;

    const auto start = std::chrono::steady_clock::now();

    for (int p = 0; p < producers; ++p) {
        producer_threads.emplace_back([&, p]() {
            for (int i = 0; i < items_per_producer; ++i) {
                buffer.push(p * items_per_producer + i);
            }
            producers_done.fetch_add(1, std::memory_order_release);
        });
    }

    for (int c = 0; c < consumers; ++c) {
        consumer_threads.emplace_back([&]() {
            for (;;) {
                int value = 0;
                if (buffer.pop(value)) {
                    checksum.fetch_add(value, std::memory_order_relaxed);
                    continue;
                }
                if (producers_done.load(std::memory_order_acquire) == producers) {
                    break;
                }
            }
        });
    }

    for (auto& producer : producer_threads) {
        producer.join();
    }
    buffer.close();
    for (auto& consumer : consumer_threads) {
        consumer.join();
    }

    const auto elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - start).count();
    const double total_ops = static_cast<double>(producers) * items_per_producer;
    return total_ops / elapsed;
}

template<typename Buffer>
double average_throughput(std::size_t capacity, int producers, int consumers,
                          int items_per_producer, int runs) {
    double total = 0.0;
    for (int run = 0; run < runs; ++run) {
        Buffer buffer(capacity);
        total += measure_throughput(buffer, producers, consumers, items_per_producer);
    }
    return total / runs;
}

void print_row(const std::string& label, std::size_t capacity, int producers, int consumers,
               double throughput) {
    std::cout << std::left << std::setw(12) << label
              << " | cap=" << std::setw(5) << capacity
              << " | P:C=" << producers << ":" << consumers
              << " | throughput=" << std::fixed << std::setprecision(2)
              << throughput / 1000000.0 << " Mops/s\n";
}

} // namespace

void benchmark_bounded_buffer() {
    std::cout << "\n=== bounded_buffer Benchmark ===\n";

    const std::vector<std::size_t> capacities = {8, 64, 1024, 8192};
    const std::vector<std::pair<int, int>> ratios = {{1, 1}, {2, 1}, {1, 2}};
    const int items_per_producer = 200000;
    const int runs = 3;

    for (const auto& ratio : ratios) {
        std::cout << "\nProducer:Consumer = " << ratio.first << ":" << ratio.second << "\n";
        for (std::size_t capacity : capacities) {
            const double lock_tp = average_throughput<parallel::bounded_buffer_lock<int>>(
                capacity, ratio.first, ratio.second, items_per_producer, runs);
            const double atomic_tp = average_throughput<parallel::bounded_buffer_atomic<int>>(
                capacity, ratio.first, ratio.second, items_per_producer, runs);

            print_row("lock", capacity, ratio.first, ratio.second, lock_tp);
            print_row("atomic", capacity, ratio.first, ratio.second, atomic_tp);
        }
    }
}
