#include <format>
#include <atomic>
#include <vector>
#include <thread>
#include <utility>
#include <string>
#include <mutex>
#include "kaizen.h"

int single_thread_sum(const std::vector<int>& arr) {
    int sum = 0;
    for (size_t i = 0; i < arr.size(); ++i) {
        sum += arr[i];
    }
    return sum;
}

void thread_sum_lock(size_t thread_idx, const std::vector<int>& arr, int& sum, std::mutex& mtx, size_t num_threads) {
    size_t n = arr.size();
    size_t chunk_size = n / num_threads;
    size_t start = thread_idx * chunk_size;
    size_t end = (thread_idx == num_threads - 1) ? n : (thread_idx + 1) * chunk_size;

    int local_sum = 0;
    for (size_t i = start; i < end; ++i) {
        local_sum += arr[i];
    }
    std::lock_guard<std::mutex> lock(mtx);
    sum += local_sum;
}

int multi_thread_sum_lock(const std::vector<int>& arr, size_t num_threads) {
    std::vector<std::thread> threads;
    std::mutex mtx;
    int sum = 0;

    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back([&arr, &sum, &mtx, num_threads](size_t idx) {
            thread_sum_lock(idx, arr, sum, mtx, num_threads);
        }, i);
    }

    for (auto& t : threads) {
        t.join();
    }
    return sum;
}

void thread_sum_atomic(size_t thread_idx, const std::vector<int>& arr, std::atomic<int>& sum, size_t num_threads) {
    size_t n = arr.size();
    size_t chunk_size = n / num_threads;
    size_t start = thread_idx * chunk_size;
    size_t end = (thread_idx == num_threads - 1) ? n : (thread_idx + 1) * chunk_size;

    int local_sum = 0;
    for (size_t i = start; i < end; ++i) {
        local_sum += arr[i];
    }
    sum.fetch_add(local_sum, std::memory_order_relaxed);
}

int multi_thread_sum_atomic(const std::vector<int>& arr, size_t num_threads) {
    std::vector<std::thread> threads;
    std::atomic<int> sum(0);
    size_t n = arr.size();
    size_t chunk_size = n / num_threads;

    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back([&arr, &sum, num_threads](size_t idx) {
            thread_sum_atomic(idx, arr, sum, num_threads);
        }, i);
    }

    for (auto& t : threads) {
        t.join();
    }
    return sum.load();
}

void thread_sum_reduce(size_t thread_idx, const std::vector<int>& arr, int& partial_sum, size_t num_threads) {
    size_t n = arr.size();
    size_t chunk_size = n / num_threads;
    size_t start = thread_idx * chunk_size;
    size_t end = (thread_idx == num_threads - 1) ? n : (thread_idx + 1) * chunk_size;

    partial_sum = 0;
    for (size_t i = start; i < end; ++i) {
        partial_sum += arr[i];
    }
}

int multi_thread_sum_reduce(const std::vector<int>& arr, size_t num_threads) {
    std::vector<std::thread> threads;
    std::vector<int> partial_sums(num_threads, 0);
    size_t n = arr.size();
    size_t chunk_size = n / num_threads;

    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back([&arr, &partial_sums, num_threads, i](size_t idx) {
            thread_sum_reduce(idx, arr, partial_sums[i], num_threads);
        }, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    int total_sum = 0;
    for (int ps : partial_sums) {
        total_sum += ps;
    }
    return total_sum;
}

std::pair<int, int> parse_args(int argc, char** argv) {
    zen::cmd_args args(argv, argc);
    if (!args.is_present("--size") || !args.is_present("--thread")) {
        zen::log(zen::color::yellow("Warning: "), "Missing required arguments. Using default values.");
        return {1'000'000, std::thread::hardware_concurrency()};
    }
    int size = std::stoi(args.get_options("--size")[0]);
    int threads = std::stoi(args.get_options("--thread")[0]);
    return {size, threads};
}

template <typename Func>
auto measure_time(Func&& func) {
    zen::timer t;
    t.start();
    func();
    t.stop();
    return t.duration<zen::timer::usec>().count();
}

int main(int argc, char** argv) {
    auto [size, num_threads] = parse_args(argc, argv);

    std::vector<int> arr(size);
    zen::generate_random(arr, size);

    int single_sum = 0, lock_sum = 0, atomic_sum = 0, reduce_sum = 0;

    auto single_time = measure_time([&]() {
        single_sum = single_thread_sum(arr);
    });

    auto lock_time = measure_time([&]() {
        lock_sum = multi_thread_sum_lock(arr, num_threads);
    });

    auto atomic_time = measure_time([&]() {
        atomic_sum = multi_thread_sum_atomic(arr, num_threads);
    });

    auto reduce_time = measure_time([&]() {
        reduce_sum = multi_thread_sum_reduce(arr, num_threads);
    });

    zen::print(std::format("Array size     : {}\n", size));
    zen::log(std::format("Thread count   : {}\n", num_threads));
    zen::log("------------------------------------------------------");
    zen::log(std::format("{:<20} {:>15} {:>15}", "Method", "Sum", "Time (us)"));
    zen::log("------------------------------------------------------");
    zen::log(std::format("{:<20} {:>15} {:>15}", "Single-threaded", single_sum, single_time));
    zen::log(std::format("{:<20} {:>15} {:>15}", "Lock-based", lock_sum, lock_time));
    zen::log(std::format("{:<20} {:>15} {:>15}", "Atomic-based", atomic_sum, atomic_time));
    zen::log(std::format("{:<20} {:>15} {:>15}", "Reduce-based", reduce_sum, reduce_time));
    zen::log("------------------------------------------------------");

    return 0;
}