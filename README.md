# Parallel Summation

## Overview

This project demonstrates different techniques to compute the sum of an integer array using parallel programming in C++. The array is partitioned among multiple threads, each responsible for summing a portion of the data. The final result is aggregated using one of three methods:

1. Lock-based summation using `std::mutex`.
2. Atomic summation using `std::atomic<int>`.
3. Reduction-based summation using local thread-local sums combined after all threads complete.

Additionally, the single-threaded version is included as a baseline for performance and correctness comparison.

The primary objective is to highlight the trade-offs among these methods in terms of correctness, complexity, and speed.

## Build & Run

### Clone the repository

```bash
git clone https://github.com/AniDashyan/parallel_summation.git
cd parallel_summation
```

### Build the project

```bash
cmake -S . -B build
cmake --build build --config Release
```

### Run the executable

```bash
./build/sum --size [N] --thread [T]
```

Where:
- `N` is the size of the array (e.g., 10000)
- `T` is the number of threads to use (e.g., 4, 8, 16)

Note: If the --size or --thread options are not provided:

The default size is 1000.

The default thread count is automatically determined by the system's hardware using:
```
std::thread::hardware_concurrency()
```
This returns the number of concurrent threads the hardware can support (typically equal to the number of logical CPU cores). This allows the program to scale automatically with the system's available processing resources.

## Example Output

```
Array size     : 1000000
Thread count   : 8
 
------------------------------------------------------ 
Method                           Sum       Time (us) 
------------------------------------------------------ 
Single-threaded             54465885            1699 
Lock-based                  54465885            4652 
Atomic-based                54465885            1250 
Reduce-based                54465885            2622 
------------------------------------------------------ 
```

Each method produces the same result, demonstrating correctness, while execution times vary depending on the synchronization overhead and thread scalability.

---

## How It Works

The program initializes an array of random integers using the utility provided in the `kaizen.h` library. It then executes four different summation strategies:

### 1. **Single-threaded**
A straightforward loop over the entire array:
```cpp
for (size_t i = 0; i < arr.size(); ++i) {
    sum += arr[i];
}
```

### 2. **Lock-based multithreading**
Each thread calculates a local sum and then updates a shared sum variable using a mutex to avoid race conditions:
```cpp
std::lock_guard<std::mutex> lock(mtx);
sum += local_sum;
```

**Trade-off**: Simple and correct, but locks introduce contention that can degrade performance at higher thread counts.

### 3. **Atomic-based multithreading**
Local sums are added to a shared `std::atomic<int>` using relaxed memory ordering:
```cpp
sum.fetch_add(local_sum, std::memory_order_relaxed);
```

**Trade-off**: Eliminates mutex overhead, but atomic operations can still serialize updates to the shared variable. Offers better performance than locks in most scenarios.

### 4. **Reduce-based multithreading**
Each thread computes its partial sum independently in a thread-local variable. Once all threads complete, the main thread aggregates the partial sums:
```cpp
for (int ps : partial_sums) {
    total_sum += ps;
}
```

**Trade-off**: Offers minimal contention and best scalability. Ideal for compute-bound problems where threads can operate independently and synchronization is delayed until the end.

---

## Summary of Trade-offs

| Method         | Synchronization | Complexity | Scalability | Performance |
|----------------|------------------|------------|-------------|-------------|
| Single-threaded | None             | Very low   | None        | Baseline    |
| Lock-based      | High (mutex)     | Moderate   | Poor        | Slower with more threads |
| Atomic-based    | Medium (atomic)  | Moderate   | Good        | Improved over locks |
| Reduce-based    | None (delayed)   | Low        | Excellent   | Fastest and cleanest |

This comparative study demonstrates the importance of choosing the right synchronization mechanism in parallel programming. The reduce-based model is most efficient for this workload, but atomic and lock-based models still serve educational and practical value, especially in scenarios with different concurrency requirements.
