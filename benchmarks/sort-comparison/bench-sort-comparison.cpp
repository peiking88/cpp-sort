/*
 * Benchmark: cpp-sort vs x86-simd-sort Performance Comparison
 * 
 * Compares various cpp-sort sorting algorithms against x86-simd-sort
 * for different array sizes and data types.
 */

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <iomanip>
#include <random>
#include <string>
#include <vector>
#include <cmath>

// cpp-sort includes
#include <cpp-sort/sorters.h>
#include <cpp-sort/sorters/std_sorter.h>
#include <cpp-sort/sorters/quick_sorter.h>
#include <cpp-sort/sorters/pdq_sorter.h>
#include <cpp-sort/sorters/merge_sorter.h>
#include <cpp-sort/sorters/tim_sorter.h>
#include <cpp-sort/sorters/heap_sorter.h>
#include <cpp-sort/sorters/spin_sorter.h>
#include <cpp-sort/sorters/grail_sorter.h>
#include <cpp-sort/sorters/spread_sorter.h>
#include <cpp-sort/sorters/ska_sorter.h>
#include <cpp-sort/sorters/simd_sorter.h>
#include <cpp-sort/sorters/parallel_sorter.h>

////////////////////////////////////////////////////////////
// Test configuration
////////////////////////////////////////////////////////////

constexpr int WARMUP_ITERATIONS = 10;
constexpr int BENCHMARK_ITERATIONS = 100;
constexpr int CORRECTNESS_ITERATIONS = 100;
constexpr int LARGE_WARMUP_ITERATIONS = 3;
constexpr int LARGE_BENCHMARK_ITERATIONS = 10;

////////////////////////////////////////////////////////////
// Timing utilities
////////////////////////////////////////////////////////////

template<typename Func>
auto measure_time(Func&& func, int iterations) -> double
{
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        func();
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double, std::micro> duration = end - start;
    return duration.count() / iterations;
}

////////////////////////////////////////////////////////////
// Test data generation
////////////////////////////////////////////////////////////

enum class DataPattern {
    RANDOM,
    SORTED,
    REVERSE,
    NEARLY_SORTED,
    FEW_UNIQUE
};

template<typename T>
auto generate_data(std::size_t n, DataPattern pattern, unsigned seed = 42) -> std::vector<T>
{
    std::mt19937 rng(seed);
    std::vector<T> data(n);
    
    switch (pattern) {
        case DataPattern::RANDOM:
            if constexpr (std::is_integral_v<T>) {
                std::uniform_int_distribution<T> dist(std::numeric_limits<T>::min(),
                                                       std::numeric_limits<T>::max());
                for (auto& x : data) { x = dist(rng); }
            } else {
                std::uniform_real_distribution<T> dist(T(-1000), T(1000));
                for (auto& x : data) { x = dist(rng); }
            }
            break;
            
        case DataPattern::SORTED:
            for (std::size_t i = 0; i < n; ++i) {
                data[i] = static_cast<T>(i);
            }
            break;
            
        case DataPattern::REVERSE:
            for (std::size_t i = 0; i < n; ++i) {
                data[i] = static_cast<T>(n - i - 1);
            }
            break;
            
        case DataPattern::NEARLY_SORTED:
            for (std::size_t i = 0; i < n; ++i) {
                data[i] = static_cast<T>(i);
            }
            // Swap ~5% of elements
            for (std::size_t i = 0; i < n / 20; ++i) {
                std::size_t j = rng() % n;
                std::size_t k = rng() % n;
                std::swap(data[j], data[k]);
            }
            break;
            
        case DataPattern::FEW_UNIQUE:
            std::vector<T> values;
            int num_values = std::max(10, static_cast<int>(n / 100));
            if constexpr (std::is_integral_v<T>) {
                std::uniform_int_distribution<T> dist(0, static_cast<T>(num_values));
                for (int i = 0; i < num_values; ++i) {
                    values.push_back(dist(rng));
                }
            } else {
                std::uniform_real_distribution<T> dist(T(0), T(num_values));
                for (int i = 0; i < num_values; ++i) {
                    values.push_back(dist(rng));
                }
            }
            for (auto& x : data) {
                x = values[rng() % num_values];
            }
            break;
    }
    
    return data;
}

////////////////////////////////////////////////////////////
// Sorting wrappers
////////////////////////////////////////////////////////////

// cpp-sort algorithms
template<typename T>
void cppsort_std_sort(T* arr, std::size_t n) {
    cppsort::std_sorter sorter;
    sorter(arr, arr + n);
}

template<typename T>
void cppsort_pdq_sort(T* arr, std::size_t n) {
    cppsort::pdq_sorter sorter;
    sorter(arr, arr + n);
}

template<typename T>
void cppsort_quick_sort(T* arr, std::size_t n) {
    cppsort::quick_sorter sorter;
    sorter(arr, arr + n);
}

template<typename T>
void cppsort_merge_sort(T* arr, std::size_t n) {
    cppsort::merge_sorter sorter;
    sorter(arr, arr + n);
}

template<typename T>
void cppsort_tim_sort(T* arr, std::size_t n) {
    cppsort::tim_sorter sorter;
    sorter(arr, arr + n);
}

template<typename T>
void cppsort_heap_sort(T* arr, std::size_t n) {
    cppsort::heap_sorter sorter;
    sorter(arr, arr + n);
}

template<typename T>
void cppsort_spin_sort(T* arr, std::size_t n) {
    cppsort::spin_sorter sorter;
    sorter(arr, arr + n);
}

template<typename T>
void cppsort_grail_sort(T* arr, std::size_t n) {
    cppsort::grail_sorter sorter;
    sorter(arr, arr + n);
}

template<typename T>
void cppsort_spread_sort(T* arr, std::size_t n) {
    cppsort::spread_sorter sorter;
    sorter(arr, arr + n);
}

template<typename T>
void cppsort_ska_sort(T* arr, std::size_t n) {
    cppsort::ska_sorter sorter;
    sorter(arr, arr + n);
}

// cpp-sort simd_sorter (wrapper for x86-simd-sort)
template<typename T>
void cppsort_simd_sort(T* arr, std::size_t n) {
    cppsort::simd_sorter sorter;
    sorter(arr, arr + n);
}

// cpp-sort parallel_sorter (uses libfork for large datasets)
template<typename T>
void cppsort_parallel_sort(T* arr, std::size_t n) {
    cppsort::parallel_sorter sorter;
    sorter(arr, arr + n);
}

// std::sort reference
template<typename T>
void std_sort(T* arr, std::size_t n) {
    std::sort(arr, arr + n);
}

////////////////////////////////////////////////////////////
// Correctness verification
////////////////////////////////////////////////////////////

template<typename SortFunc, typename T>
auto verify_correctness(SortFunc sort_func, std::size_t n, DataPattern pattern) -> bool
{
    auto original = generate_data<T>(n, pattern);
    auto data = original;
    auto reference = original;
    
    // Sort using reference
    std::sort(reference.begin(), reference.end());
    
    // Sort using test function
    sort_func(data.data(), n);
    
    // Compare
    return data == reference;
}

////////////////////////////////////////////////////////////
// Benchmark runner
////////////////////////////////////////////////////////////

struct BenchmarkResult {
    std::string algorithm;
    double time_us;
    bool correct;
};

template<typename T>
auto run_benchmark(std::size_t n, DataPattern pattern, 
                   int warmup_iters = WARMUP_ITERATIONS,
                   int bench_iters = BENCHMARK_ITERATIONS) -> std::vector<BenchmarkResult>
{
    std::vector<BenchmarkResult> results;
    
    auto original = generate_data<T>(n, pattern);
    
    // Define algorithms to test
    struct Algorithm {
        std::string name;
        std::function<void(T*, std::size_t)> func;
    };
    
    std::vector<Algorithm> algorithms = {
        {"std::sort", std_sort<T>},
        {"cpp-sort/simd_sorter", cppsort_simd_sort<T>},
        {"cpp-sort/parallel_sorter", cppsort_parallel_sort<T>},
        {"cpp-sort/std_sorter", cppsort_std_sort<T>},
        {"cpp-sort/pdq_sorter", cppsort_pdq_sort<T>},
        {"cpp-sort/quick_sorter", cppsort_quick_sort<T>},
        {"cpp-sort/merge_sorter", cppsort_merge_sort<T>},
        {"cpp-sort/tim_sorter", cppsort_tim_sort<T>},
        {"cpp-sort/heap_sorter", cppsort_heap_sort<T>},
        {"cpp-sort/spin_sorter", cppsort_spin_sort<T>},
        {"cpp-sort/grail_sorter", cppsort_grail_sort<T>},
        {"cpp-sort/spread_sorter", cppsort_spread_sort<T>},
        {"cpp-sort/ska_sorter", cppsort_ska_sort<T>},
    };
    
    // Adjust correctness iterations for large datasets
    int correctness_iters = (n >= 1000000) ? 10 : CORRECTNESS_ITERATIONS;
    
    for (const auto& algo : algorithms) {
        BenchmarkResult result;
        result.algorithm = algo.name;
        
        // Verify correctness
        result.correct = true;
        for (int i = 0; i < correctness_iters && result.correct; ++i) {
            result.correct = verify_correctness<decltype(algo.func), T>(algo.func, n, pattern);
        }
        
        if (!result.correct) {
            result.time_us = -1.0;
            results.push_back(result);
            continue;
        }
        
        // Measure performance
        auto data = original;
        
        // Warmup
        for (int i = 0; i < warmup_iters; ++i) {
            data = original;
            algo.func(data.data(), n);
        }
        
        // Benchmark
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < bench_iters; ++i) {
            data = original;
            algo.func(data.data(), n);
        }
        auto end = std::chrono::high_resolution_clock::now();
        
        std::chrono::duration<double, std::micro> duration = end - start;
        result.time_us = duration.count() / bench_iters;
        
        results.push_back(result);
    }
    
    return results;
}

////////////////////////////////////////////////////////////
// Pattern name helper
////////////////////////////////////////////////////////////

auto pattern_name(DataPattern pattern) -> std::string
{
    switch (pattern) {
        case DataPattern::RANDOM: return "Random";
        case DataPattern::SORTED: return "Sorted";
        case DataPattern::REVERSE: return "Reverse";
        case DataPattern::NEARLY_SORTED: return "Nearly Sorted";
        case DataPattern::FEW_UNIQUE: return "Few Unique";
        default: return "Unknown";
    }
}

////////////////////////////////////////////////////////////
// Print results
////////////////////////////////////////////////////////////

void print_results(const std::vector<BenchmarkResult>& results, 
                   const std::string& type_name,
                   std::size_t n,
                   DataPattern pattern)
{
    std::cout << "\n=== Type: " << type_name << ", N=" << n 
              << ", Pattern: " << pattern_name(pattern) << " ===\n";
    
    // Find minimum time for speedup calculation
    double min_time = std::numeric_limits<double>::max();
    for (const auto& r : results) {
        if (r.correct && r.time_us > 0 && r.time_us < min_time) {
            min_time = r.time_us;
        }
    }
    
    std::cout << std::left << std::setw(25) << "Algorithm"
              << std::right << std::setw(12) << "Time (us)"
              << std::setw(10) << "Speedup"
              << std::setw(10) << "Status" << "\n";
    std::cout << std::string(57, '-') << "\n";
    
    for (const auto& r : results) {
        std::cout << std::left << std::setw(25) << r.algorithm;
        
        if (!r.correct) {
            std::cout << std::right << std::setw(12) << "FAILED"
                      << std::setw(10) << "-"
                      << std::setw(10) << "FAIL\n";
        } else {
            std::cout << std::right << std::fixed << std::setprecision(2)
                      << std::setw(12) << r.time_us;
            
            if (r.time_us > 0 && min_time > 0) {
                double speedup = min_time / r.time_us;
                std::cout << std::setw(9) << speedup << "x";
            } else {
                std::cout << std::setw(10) << "-";
            }
            std::cout << std::setw(10) << "OK\n";
        }
    }
}

////////////////////////////////////////////////////////////
// Main
////////////////////////////////////////////////////////////

int main()
{
    std::cout << "==============================================\n";
    std::cout << "cpp-sort vs x86-simd-sort Performance Benchmark\n";
    std::cout << "==============================================\n";
    
#if defined(__AVX512F__)
    std::cout << "SIMD: AVX-512 detected\n";
#elif defined(__AVX2__)
    std::cout << "SIMD: AVX2 detected\n";
#elif defined(__AVX__)
    std::cout << "SIMD: AVX detected\n";
#else
    std::cout << "SIMD: No SIMD support detected\n";
#endif
    
    std::cout << "Warmup iterations: " << WARMUP_ITERATIONS << "\n";
    std::cout << "Benchmark iterations: " << BENCHMARK_ITERATIONS << "\n";
    std::cout << "Correctness iterations: " << CORRECTNESS_ITERATIONS << "\n";
    
    // Test sizes (include large datasets for parallel_sorter benchmark)
    std::vector<std::size_t> sizes = {100, 1000, 10000, 100000, 1000000, 10000000};
    
    // Test patterns
    std::vector<DataPattern> patterns = {
        DataPattern::RANDOM,
        DataPattern::SORTED,
        DataPattern::REVERSE,
        DataPattern::NEARLY_SORTED,
        DataPattern::FEW_UNIQUE
    };
    
    // Test types
    using TestTypes = std::tuple<float, double, int32_t, int64_t, uint32_t, uint64_t>;
    
    // Run benchmarks for each combination
    for (std::size_t n : sizes) {
        // Use fewer iterations for large datasets
        int warmup = (n >= 1000000) ? LARGE_WARMUP_ITERATIONS : WARMUP_ITERATIONS;
        int bench = (n >= 1000000) ? LARGE_BENCHMARK_ITERATIONS : BENCHMARK_ITERATIONS;
        
        for (auto pattern : patterns) {
            // Float
            auto results_float = run_benchmark<float>(n, pattern, warmup, bench);
            print_results(results_float, "float", n, pattern);
            
            // Double
            auto results_double = run_benchmark<double>(n, pattern, warmup, bench);
            print_results(results_double, "double", n, pattern);
            
            // int32_t
            auto results_i32 = run_benchmark<int32_t>(n, pattern, warmup, bench);
            print_results(results_i32, "int32_t", n, pattern);
            
            // int64_t
            auto results_i64 = run_benchmark<int64_t>(n, pattern, warmup, bench);
            print_results(results_i64, "int64_t", n, pattern);
        }
        
        std::cout << "\n";
    }
    
    std::cout << "\n==============================================\n";
    std::cout << "Benchmark complete\n";
    std::cout << "==============================================\n";
    
    return 0;
}
