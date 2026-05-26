#pragma once

#include <algorithm>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <random>
#include <vector>

namespace parallel {
namespace utils {

class timer {
public:
    using clock = std::chrono::high_resolution_clock;
    using duration = std::chrono::duration<double, std::milli>;

    timer() : start_(clock::now()) {}

    void reset() { start_ = clock::now(); }

    double elapsed_ms() const {
        return std::chrono::duration_cast<duration>(clock::now() - start_).count();
    }

    double elapsed_seconds() const {
        return elapsed_ms() / 1000.0;
    }

private:
    clock::time_point start_;
};

template<typename Func>
double benchmark(Func&& func, int iterations = 10) {
    timer t;
    for (int i = 0; i < iterations; ++i) {
        func();
    }
    return t.elapsed_ms() / iterations;
}

template<typename T>
void print_result(const std::string& name, const T& serial_time, const T& parallel_time, int num_threads) {
    double speedup = serial_time / parallel_time;
    double efficiency = speedup / num_threads * 100.0;

    std::cout << name << ":\n";
    std::cout << "  Serial time:    " << serial_time << " ms\n";
    std::cout << "  Parallel time:  " << parallel_time << " ms\n";
    std::cout << "  Speedup:        " << speedup << "\n";
    std::cout << "  Efficiency:     " << efficiency << "%\n";
    std::cout << "  Threads:        " << num_threads << "\n";
    std::cout << std::endl;
}

template<typename T>
void print_comparison(const std::string& name, const std::vector<T>& times_by_threads,
                     const std::vector<int>& thread_counts, const T& serial_time) {
    std::cout << name << " Comparison:\n";
    std::cout << "Serial time: " << serial_time << " ms\n";
    std::cout << std::string(60, '-') << "\n";
    std::cout << "Threads | Time (ms) | Speedup | Efficiency\n";
    std::cout << std::string(60, '-') << "\n";

    for (size_t i = 0; i < thread_counts.size(); ++i) {
        double speedup = serial_time / times_by_threads[i];
        double efficiency = speedup / thread_counts[i] * 100.0;
        std::cout << std::setw(7) << thread_counts[i] << " | "
                  << std::setw(10) << times_by_threads[i] << " | "
                  << std::setw(7) << std::fixed << std::setprecision(2) << speedup << " | "
                  << std::setw(10) << efficiency << "%\n";
    }
    std::cout << std::endl;
}

template<typename T>
bool is_sorted(const std::vector<T>& v) {
    return std::is_sorted(v.begin(), v.end());
}

template<typename T>
std::vector<T> generate_random_vector(std::size_t n, T min_val = 0, T max_val = 1000000) {
    std::vector<T> v(n);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<T> dist(min_val, max_val);

    for (std::size_t i = 0; i < n; ++i) {
        v[i] = dist(gen);
    }
    return v;
}

} // namespace utils
} // namespace parallel