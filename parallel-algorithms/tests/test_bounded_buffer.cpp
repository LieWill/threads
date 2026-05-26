#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "parallel/bounded_buffer.h"
#include "parallel/interrupt.h"
#include "test_assert.h"

namespace {

template<typename Buffer>
void run_mpmc_test(Buffer& buffer, int producers, int consumers, int items_per_producer,
                   long long& produced_sum, long long& consumed_sum, int& consumed_count) {
    std::atomic<long long> produced_atomic{0};
    std::atomic<long long> consumed_atomic{0};
    std::atomic<int> consumed_count_atomic{0};
    std::atomic<int> producers_done{0};

    std::vector<std::thread> producer_threads;
    std::vector<std::thread> consumer_threads;

    for (int p = 0; p < producers; ++p) {
        producer_threads.emplace_back([&, p]() {
            const int base = p * items_per_producer;
            for (int i = 1; i <= items_per_producer; ++i) {
                const int value = base + i;
                produced_atomic.fetch_add(value, std::memory_order_relaxed);
                buffer.push(value);
            }
            producers_done.fetch_add(1, std::memory_order_release);
        });
    }

    for (int c = 0; c < consumers; ++c) {
        consumer_threads.emplace_back([&]() {
            for (;;) {
                int value = 0;
                if (buffer.pop(value)) {
                    consumed_atomic.fetch_add(value, std::memory_order_relaxed);
                    consumed_count_atomic.fetch_add(1, std::memory_order_relaxed);
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

    produced_sum = produced_atomic.load(std::memory_order_relaxed);
    consumed_sum = consumed_atomic.load(std::memory_order_relaxed);
    consumed_count = consumed_count_atomic.load(std::memory_order_relaxed);
}

} // namespace

TEST_CASE(bounded_buffer_lock, basic_push_pop) {
    parallel::bounded_buffer_lock<int> buffer(4);
    TEST_ASSERT(buffer.push(1), "Push should succeed");
    TEST_ASSERT(buffer.push(2), "Push should succeed");

    int first = 0;
    int second = 0;
    TEST_ASSERT(buffer.pop(first), "Pop should succeed");
    TEST_ASSERT(buffer.pop(second), "Pop should succeed");

    TEST_ASSERT_EQUAL(1, first, "FIFO order should be preserved");
    TEST_ASSERT_EQUAL(2, second, "FIFO order should be preserved");
}

TEST_CASE(bounded_buffer_lock, multi_producer_multi_consumer) {
    parallel::bounded_buffer_lock<int> buffer(64);
    long long produced_sum = 0;
    long long consumed_sum = 0;
    int consumed_count = 0;

    run_mpmc_test(buffer, 2, 2, 50000, produced_sum, consumed_sum, consumed_count);

    TEST_ASSERT_EQUAL(produced_sum, consumed_sum, "Consumed values should match produced values");
    TEST_ASSERT_EQUAL(100000, consumed_count, "All values should be consumed");
}

TEST_CASE(bounded_buffer_atomic, multi_producer_multi_consumer) {
    parallel::bounded_buffer_atomic<int> buffer(1024);
    long long produced_sum = 0;
    long long consumed_sum = 0;
    int consumed_count = 0;

    run_mpmc_test(buffer, 2, 2, 50000, produced_sum, consumed_sum, consumed_count);

    TEST_ASSERT_EQUAL(produced_sum, consumed_sum, "Atomic buffer should preserve all values");
    TEST_ASSERT_EQUAL(100000, consumed_count, "Atomic buffer should consume all values");
}

TEST_CASE(bounded_buffer_lock, interrupt_stops_waiting_consumer_quickly) {
    parallel::bounded_buffer_lock<int> buffer(8);
    parallel::interrupt_flag flag;

    const auto start = std::chrono::steady_clock::now();
    std::thread consumer([&]() {
        int value = 0;
        buffer.pop(value, flag);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    flag.interrupt();
    consumer.join();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    TEST_ASSERT(elapsed.count() < 100, "Interrupt should stop waiting lock consumer within 100ms");
}

TEST_CASE(bounded_buffer_atomic, interrupt_stops_waiting_consumer_quickly) {
    parallel::bounded_buffer_atomic<int> buffer(32);
    parallel::interrupt_flag flag;

    const auto start = std::chrono::steady_clock::now();
    std::thread consumer([&]() {
        int value = 0;
        buffer.pop(value, flag);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    flag.interrupt();
    consumer.join();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    TEST_ASSERT(elapsed.count() < 100, "Interrupt should stop waiting atomic consumer within 100ms");
}
