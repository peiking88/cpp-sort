/*
 * Benchmark: Serial vs Parallel Sorters Performance Comparison
 * 
 * Compares serial sorters against their parallel counterparts:
 *   - merge_sorter vs parallel_merge_sorter
 *   - quick_sorter vs parallel_quick_sorter
 *   - pdq_sorter vs parallel_pdq_sorter
 *   - tim_sorter vs parallel_tim_sorter
 *   - heap_sorter vs parallel_heap_sorter
 *   - grail_sorter vs parallel_grail_sorter
 *   - simd_sorter vs parallel_simd_sorter
 */

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <random>
#include <string>
#include <vector>
#include <thread>
#include <map>
#include <cstring>

// cpp-sort serial sorters
#include <cpp-sort/sorters/merge_sorter.h>
#include <cpp-sort/sorters/quick_sorter.h>
#include <cpp-sort/sorters/pdq_sorter.h>
#include <cpp-sort/sorters/tim_sorter.h>
#include <cpp-sort/sorters/heap_sorter.h>
#include <cpp-sort/sorters/grail_sorter.h>
#include <cpp-sort/sorters/simd_sorter.h>

// cpp-sort parallel sorters
#include <cpp-sort/sorters/parallel_merge_sorter.h>
#include <cpp-sort/sorters/parallel_quick_sorter.h>
#include <cpp-sort/sorters/parallel_pdq_sorter.h>
#include <cpp-sort/sorters/parallel_tim_sorter.h>
#include <cpp-sort/sorters/parallel_heap_sorter.h>
#include <cpp-sort/sorters/parallel_grail_sorter.h>
#include <cpp-sort/sorters/parallel_simd_sorter.h>

////////////////////////////////////////////////////////////
// Test configuration
////////////////////////////////////////////////////////////

constexpr int WARMUP_ITERATIONS = 3;
constexpr int BENCHMARK_ITERATIONS = 10;
constexpr int LARGE_WARMUP_ITERATIONS = 1;
constexpr int LARGE_BENCHMARK_ITERATIONS = 3;

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

// Serial sorters
template<typename T>
void merge_sort_func(T* arr, std::size_t n) {
    cppsort::merge_sorter sorter;
    sorter(arr, arr + n);
}

template<typename T>
void quick_sort_func(T* arr, std::size_t n) {
    cppsort::quick_sorter sorter;
    sorter(arr, arr + n);
}

template<typename T>
void pdq_sort_func(T* arr, std::size_t n) {
    cppsort::pdq_sorter sorter;
    sorter(arr, arr + n);
}

template<typename T>
void tim_sort_func(T* arr, std::size_t n) {
    cppsort::tim_sorter sorter;
    sorter(arr, arr + n);
}

template<typename T>
void heap_sort_func(T* arr, std::size_t n) {
    cppsort::heap_sorter sorter;
    sorter(arr, arr + n);
}

template<typename T>
void grail_sort_func(T* arr, std::size_t n) {
    cppsort::grail_sorter sorter;
    sorter(arr, arr + n);
}

template<typename T>
void simd_sort_func(T* arr, std::size_t n) {
    cppsort::simd_sorter sorter;
    sorter(arr, arr + n);
}

// Parallel sorters
template<typename T>
void parallel_merge_sort_func(T* arr, std::size_t n) {
    cppsort::parallel_merge_sorter sorter;
    sorter(arr, arr + n);
}

template<typename T>
void parallel_quick_sort_func(T* arr, std::size_t n) {
    cppsort::parallel_quick_sorter sorter;
    sorter(arr, arr + n);
}

template<typename T>
void parallel_pdq_sort_func(T* arr, std::size_t n) {
    cppsort::parallel_pdq_sorter sorter;
    sorter(arr, arr + n);
}

template<typename T>
void parallel_tim_sort_func(T* arr, std::size_t n) {
    cppsort::parallel_tim_sorter sorter;
    sorter(arr, arr + n);
}

template<typename T>
void parallel_heap_sort_func(T* arr, std::size_t n) {
    cppsort::parallel_heap_sorter sorter;
    sorter(arr, arr + n);
}

template<typename T>
void parallel_grail_sort_func(T* arr, std::size_t n) {
    cppsort::parallel_grail_sorter sorter;
    sorter(arr, arr + n);
}

template<typename T>
void parallel_simd_sort_func(T* arr, std::size_t n) {
    cppsort::parallel_simd_sorter sorter;
    sorter(arr, arr + n);
}

// std::sort reference
template<typename T>
void std_sort_func(T* arr, std::size_t n) {
    std::sort(arr, arr + n);
}

////////////////////////////////////////////////////////////
// Algorithm registry
////////////////////////////////////////////////////////////

struct AlgorithmPair {
    std::string serial_name;
    std::string parallel_name;
    std::function<void(int*, std::size_t)> serial_func_int;
    std::function<void(int*, std::size_t)> parallel_func_int;
    std::function<void(float*, std::size_t)> serial_func_float;
    std::function<void(float*, std::size_t)> parallel_func_float;
    std::function<void(double*, std::size_t)> serial_func_double;
    std::function<void(double*, std::size_t)> parallel_func_double;
};

std::map<std::string, AlgorithmPair> algorithm_registry = {
    {"merge", {
        "merge_sorter", "parallel_merge_sorter",
        merge_sort_func<int>, parallel_merge_sort_func<int>,
        merge_sort_func<float>, parallel_merge_sort_func<float>,
        merge_sort_func<double>, parallel_merge_sort_func<double>
    }},
    {"quick", {
        "quick_sorter", "parallel_quick_sorter",
        quick_sort_func<int>, parallel_quick_sort_func<int>,
        quick_sort_func<float>, parallel_quick_sort_func<float>,
        quick_sort_func<double>, parallel_quick_sort_func<double>
    }},
    {"pdq", {
        "pdq_sorter", "parallel_pdq_sorter",
        pdq_sort_func<int>, parallel_pdq_sort_func<int>,
        pdq_sort_func<float>, parallel_pdq_sort_func<float>,
        pdq_sort_func<double>, parallel_pdq_sort_func<double>
    }},
    {"tim", {
        "tim_sorter", "parallel_tim_sorter",
        tim_sort_func<int>, parallel_tim_sort_func<int>,
        tim_sort_func<float>, parallel_tim_sort_func<float>,
        tim_sort_func<double>, parallel_tim_sort_func<double>
    }},
    {"heap", {
        "heap_sorter", "parallel_heap_sorter",
        heap_sort_func<int>, parallel_heap_sort_func<int>,
        heap_sort_func<float>, parallel_heap_sort_func<float>,
        heap_sort_func<double>, parallel_heap_sort_func<double>
    }},
    {"grail", {
        "grail_sorter", "parallel_grail_sorter",
        grail_sort_func<int>, parallel_grail_sort_func<int>,
        grail_sort_func<float>, parallel_grail_sort_func<float>,
        grail_sort_func<double>, parallel_grail_sort_func<double>
    }},
    {"simd", {
        "simd_sorter", "parallel_simd_sorter",
        simd_sort_func<int>, parallel_simd_sort_func<int>,
        simd_sort_func<float>, parallel_simd_sort_func<float>,
        simd_sort_func<double>, parallel_simd_sort_func<double>
    }},
};

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
    double speedup_vs_serial;  // Speedup of parallel vs serial
};

template<typename T>
auto run_benchmark_pair(const AlgorithmPair& algo_pair,
                        std::size_t n, DataPattern pattern,
                        int warmup_iters = WARMUP_ITERATIONS,
                        int bench_iters = BENCHMARK_ITERATIONS) -> std::vector<BenchmarkResult>
{
    std::vector<BenchmarkResult> results;
    
    auto original = generate_data<T>(n, pattern);
    
    // Select the appropriate function based on type
    auto serial_func = [](const AlgorithmPair& pair) {
        if constexpr (std::is_same_v<T, int>) return pair.serial_func_int;
        else if constexpr (std::is_same_v<T, float>) return pair.serial_func_float;
        else return pair.serial_func_double;
    }(algo_pair);
    
    auto parallel_func = [](const AlgorithmPair& pair) {
        if constexpr (std::is_same_v<T, int>) return pair.parallel_func_int;
        else if constexpr (std::is_same_v<T, float>) return pair.parallel_func_float;
        else return pair.parallel_func_double;
    }(algo_pair);
    
    // Test serial
    {
        BenchmarkResult result;
        result.algorithm = algo_pair.serial_name;
        
        // Verify correctness
        result.correct = verify_correctness<decltype(serial_func), T>(serial_func, n, pattern);
        
        if (result.correct) {
            auto data = original;
            
            // Warmup
            for (int i = 0; i < warmup_iters; ++i) {
                data = original;
                serial_func(data.data(), n);
            }
            
            // Benchmark
            auto start = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < bench_iters; ++i) {
                data = original;
                serial_func(data.data(), n);
            }
            auto end = std::chrono::high_resolution_clock::now();
            
            std::chrono::duration<double, std::micro> duration = end - start;
            result.time_us = duration.count() / bench_iters;
        } else {
            result.time_us = -1.0;
        }
        result.speedup_vs_serial = 1.0;  // Serial is baseline
        results.push_back(result);
    }
    
    // Test parallel
    {
        BenchmarkResult result;
        result.algorithm = algo_pair.parallel_name;
        
        // Verify correctness
        result.correct = verify_correctness<decltype(parallel_func), T>(parallel_func, n, pattern);
        
        if (result.correct) {
            auto data = original;
            
            // Warmup
            for (int i = 0; i < warmup_iters; ++i) {
                data = original;
                parallel_func(data.data(), n);
            }
            
            // Benchmark
            auto start = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < bench_iters; ++i) {
                data = original;
                parallel_func(data.data(), n);
            }
            auto end = std::chrono::high_resolution_clock::now();
            
            std::chrono::duration<double, std::micro> duration = end - start;
            result.time_us = duration.count() / bench_iters;
            
            // Calculate speedup vs serial
            if (results[0].time_us > 0) {
                result.speedup_vs_serial = results[0].time_us / result.time_us;
            } else {
                result.speedup_vs_serial = 0.0;
            }
        } else {
            result.time_us = -1.0;
            result.speedup_vs_serial = 0.0;
        }
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
    
    std::cout << std::left << std::setw(25) << "Algorithm"
              << std::right << std::setw(14) << "Time (us)"
              << std::setw(14) << "Speedup"
              << std::setw(10) << "Status" << "\n";
    std::cout << std::string(63, '-') << "\n";
    
    for (const auto& r : results) {
        std::cout << std::left << std::setw(25) << r.algorithm;
        
        if (!r.correct) {
            std::cout << std::right << std::setw(14) << "FAILED"
                      << std::setw(14) << "-"
                      << std::setw(10) << "FAIL\n";
        } else {
            std::cout << std::right << std::fixed << std::setprecision(2)
                      << std::setw(14) << r.time_us;
            
            if (r.speedup_vs_serial >= 1.0) {
                // Parallel is faster
                std::cout << std::setw(13) << r.speedup_vs_serial << "x";
            } else if (r.speedup_vs_serial > 0) {
                // Serial (baseline) or parallel is slower
                std::cout << std::setw(13) << r.speedup_vs_serial << "x";
            } else {
                std::cout << std::setw(14) << "-";
            }
            std::cout << std::setw(10) << "OK\n";
        }
    }
}

////////////////////////////////////////////////////////////
// Print summary table
////////////////////////////////////////////////////////////

void print_summary_header() {
    std::cout << "\n";
    std::cout << std::string(100, '=') << "\n";
    std::cout << std::left << std::setw(25) << "Algorithm Pair"
              << std::right << std::setw(15) << "Serial (us)"
              << std::setw(15) << "Parallel (us)"
              << std::setw(12) << "Speedup"
              << std::setw(12) << "Pattern"
              << std::setw(12) << "Type"
              << std::setw(10) << "Size" << "\n";
    std::cout << std::string(100, '-') << "\n";
}

void print_summary_row(const std::string& algo_name,
                       const BenchmarkResult& serial_result,
                       const BenchmarkResult& parallel_result,
                       const std::string& type_name,
                       std::size_t n,
                       DataPattern pattern)
{
    std::cout << std::left << std::setw(25) << algo_name;
    
    if (serial_result.correct && parallel_result.correct) {
        std::cout << std::right << std::fixed << std::setprecision(2)
                  << std::setw(15) << serial_result.time_us
                  << std::setw(15) << parallel_result.time_us
                  << std::setw(11) << parallel_result.speedup_vs_serial << "x";
    } else {
        std::cout << std::right 
                  << std::setw(15) << (serial_result.correct ? std::to_string(serial_result.time_us) : "FAIL")
                  << std::setw(15) << (parallel_result.correct ? std::to_string(parallel_result.time_us) : "FAIL")
                  << std::setw(12) << "-";
    }
    
    std::cout << std::setw(12) << pattern_name(pattern).substr(0, 10)
              << std::setw(12) << type_name
              << std::setw(10) << n << "\n";
}

////////////////////////////////////////////////////////////
// Show help
////////////////////////////////////////////////////////////

void show_help(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help           Show this help message\n";
    std::cout << "  -c <algo>            Compare specific algorithm pair (all, merge, quick, pdq, tim, heap, grail, simd)\n";
    std::cout << "                       Default: all\n";
    std::cout << "\nAvailable algorithm pairs:\n";
    for (const auto& [key, pair] : algorithm_registry) {
        std::cout << "  " << std::left << std::setw(10) << key 
                  << " -> " << pair.serial_name << " vs " << pair.parallel_name << "\n";
    }
    std::cout << "\nExamples:\n";
    std::cout << "  " << program_name << "              # Compare all algorithm pairs\n";
    std::cout << "  " << program_name << " -c merge    # Compare merge_sorter vs parallel_merge_sorter\n";
    std::cout << "  " << program_name << " -c quick    # Compare quick_sorter vs parallel_quick_sorter\n";
}

////////////////////////////////////////////////////////////
// Main
////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
    std::string selected_algo = "all";
    
    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
            show_help(argv[0]);
            return 0;
        } else if (std::strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            selected_algo = argv[++i];
            // Convert to lowercase
            std::transform(selected_algo.begin(), selected_algo.end(), selected_algo.begin(), ::tolower);
        }
    }
    
    // Validate algorithm selection
    std::vector<std::string> algos_to_run;
    if (selected_algo == "all") {
        for (const auto& [key, pair] : algorithm_registry) {
            algos_to_run.push_back(key);
        }
    } else if (algorithm_registry.find(selected_algo) != algorithm_registry.end()) {
        algos_to_run.push_back(selected_algo);
    } else {
        std::cerr << "Error: Unknown algorithm '" << selected_algo << "'\n";
        std::cerr << "Use -h or --help for usage information.\n";
        return 1;
    }
    
    std::cout << "============================================================\n";
    std::cout << "Serial vs Parallel Sorters Performance Benchmark\n";
    std::cout << "============================================================\n";
    
    std::cout << "Hardware concurrency: " << std::thread::hardware_concurrency() << " threads\n";
    std::cout << "Warmup iterations: " << WARMUP_ITERATIONS << "\n";
    std::cout << "Benchmark iterations: " << BENCHMARK_ITERATIONS << "\n";
    std::cout << "Selected algorithms: " << (selected_algo == "all" ? "all" : selected_algo) << "\n";
    
#if defined(__AVX512F__)
    std::cout << "SIMD: AVX-512 detected\n";
#elif defined(__AVX2__)
    std::cout << "SIMD: AVX2 detected\n";
#elif defined(__AVX__)
    std::cout << "SIMD: AVX detected\n";
#else
    std::cout << "SIMD: No SIMD support detected\n";
#endif
    
    // Test sizes (>= 100K for parallel sorters, max 10M)
    std::vector<std::size_t> sizes = {100000, 500000, 1000000, 5000000, 10000000};
    
    // Test patterns
    std::vector<DataPattern> patterns = {
        DataPattern::RANDOM,
        DataPattern::SORTED,
        DataPattern::REVERSE,
        DataPattern::NEARLY_SORTED,
        DataPattern::FEW_UNIQUE
    };
    
    // Print summary header
    print_summary_header();
    
    // Run benchmarks for each algorithm pair
    for (const auto& algo_key : algos_to_run) {
        const auto& algo_pair = algorithm_registry[algo_key];
        
        for (std::size_t n : sizes) {
            // Use fewer iterations for large datasets (> 1M)
            int warmup = (n > 1000000) ? LARGE_WARMUP_ITERATIONS : WARMUP_ITERATIONS;
            int bench = (n > 1000000) ? LARGE_BENCHMARK_ITERATIONS : BENCHMARK_ITERATIONS;
            
            for (auto pattern : patterns) {
                // int
                auto results_int = run_benchmark_pair<int>(algo_pair, n, pattern, warmup, bench);
                print_summary_row(algo_key, results_int[0], results_int[1], "int", n, pattern);
                
                // float
                auto results_float = run_benchmark_pair<float>(algo_pair, n, pattern, warmup, bench);
                print_summary_row(algo_key, results_float[0], results_float[1], "float", n, pattern);
                
                // double
                auto results_double = run_benchmark_pair<double>(algo_pair, n, pattern, warmup, bench);
                print_summary_row(algo_key, results_double[0], results_double[1], "double", n, pattern);
            }
        }
    }
    
    std::cout << "\n============================================================\n";
    std::cout << "Benchmark complete\n";
    std::cout << "============================================================\n";
    
    return 0;
}
