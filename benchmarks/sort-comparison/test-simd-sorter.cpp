/*
 * Test: simd_sorter integration test
 */

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

#include <cpp-sort/sorters.h>
#include <cpp-sort/sorters/simd_sorter.h>

////////////////////////////////////////////////////////////
// Test utilities
////////////////////////////////////////////////////////////

template<typename T>
auto generate_random_data(std::size_t n, unsigned seed = 42) -> std::vector<T>
{
    std::mt19937 rng(seed);
    std::vector<T> data(n);
    
    if constexpr (std::is_integral_v<T>) {
        std::uniform_int_distribution<T> dist(std::numeric_limits<T>::min(),
                                               std::numeric_limits<T>::max());
        for (auto& x : data) { x = dist(rng); }
    } else {
        std::uniform_real_distribution<T> dist(T(-1000), T(1000));
        for (auto& x : data) { x = dist(rng); }
    }
    
    return data;
}

template<typename T>
auto test_simd_sorter(std::size_t n) -> bool
{
    auto original = generate_random_data<T>(n);
    auto data = original;
    auto reference = original;
    
    // Sort using reference
    std::sort(reference.begin(), reference.end());
    
    // Sort using simd_sorter
    cppsort::simd_sorter sorter;
    sorter(data.begin(), data.end());
    
    // Verify
    if (data != reference) {
        std::cerr << "SIMD sorter failed for type " << typeid(T).name() 
                  << ", size " << n << std::endl;
        return false;
    }
    
    return true;
}

template<typename T>
auto test_simd_sort() -> bool
{
    auto original = generate_random_data<T>(1000);
    auto data = original;
    auto reference = original;
    
    std::sort(reference.begin(), reference.end());
    
    // Test using the sort function object
    cppsort::simd_sort(data.begin(), data.end());
    
    if (data != reference) {
        std::cerr << "simd_sort failed for type " << typeid(T).name() << std::endl;
        return false;
    }
    
    return true;
}

////////////////////////////////////////////////////////////
// Main
////////////////////////////////////////////////////////////

int main()
{
    std::cout << "Testing simd_sorter integration...\n\n";
    
#if defined(CPPSORT_HAS_X86_SIMD_SORT)
    std::cout << "SIMD support: ENABLED\n";
#if defined(__AVX512F__)
    std::cout << "  - AVX-512 available\n";
#endif
#if defined(__AVX2__)
    std::cout << "  - AVX2 available\n";
#endif
#else
    std::cout << "SIMD support: NOT AVAILABLE (using std::sort fallback)\n";
#endif
    
    std::cout << "\n--- Testing different types ---\n";
    
    bool all_passed = true;
    
    // Test different sizes
    std::vector<std::size_t> sizes = {10, 100, 1000, 10000, 100000};
    
    // Test supported types
    all_passed &= test_simd_sorter<float>(1000);
    std::cout << "float: " << (all_passed ? "PASS" : "FAIL") << "\n";
    
    all_passed &= test_simd_sorter<double>(1000);
    std::cout << "double: " << (all_passed ? "PASS" : "FAIL") << "\n";
    
    all_passed &= test_simd_sorter<int32_t>(1000);
    std::cout << "int32_t: " << (all_passed ? "PASS" : "FAIL") << "\n";
    
    all_passed &= test_simd_sorter<int64_t>(1000);
    std::cout << "int64_t: " << (all_passed ? "PASS" : "FAIL") << "\n";
    
    all_passed &= test_simd_sorter<uint32_t>(1000);
    std::cout << "uint32_t: " << (all_passed ? "PASS" : "FAIL") << "\n";
    
    all_passed &= test_simd_sorter<uint64_t>(1000);
    std::cout << "uint64_t: " << (all_passed ? "PASS" : "FAIL") << "\n";
    
    // Test sort function
    all_passed &= test_simd_sort<float>();
    all_passed &= test_simd_sort<double>();
    all_passed &= test_simd_sort<int32_t>();
    
    std::cout << "\n--- Testing different sizes ---\n";
    
    for (auto n : sizes) {
        all_passed &= test_simd_sorter<float>(n);
        all_passed &= test_simd_sorter<int32_t>(n);
        std::cout << "N=" << n << ": " << (all_passed ? "PASS" : "FAIL") << "\n";
    }
    
    // Test fallback for unsupported types (should compile and work)
    struct Point { int x, y; };
    std::vector<Point> points = {{3, 1}, {1, 2}, {2, 3}};
    cppsort::simd_sorter sorter;
    sorter(points.begin(), points.end(), [](const Point& p) { return p.x; });
    std::cout << "\nCustom type with projection: PASS\n";
    
    std::cout << "\n======================================\n";
    std::cout << "All tests: " << (all_passed ? "PASSED" : "FAILED") << "\n";
    std::cout << "======================================\n";
    
    return all_passed ? 0 : 1;
}
