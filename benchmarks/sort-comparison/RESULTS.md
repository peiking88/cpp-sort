# cpp-sort Sorting Algorithms Performance Comparison

## Test Environment
- **CPU**: AVX-512 supported
- **Compiler**: GCC 15.2.0
- **Build**: Release mode with `-O3 -march=native`

## SIMD Support Integrated

`simd_sorter` is now fully integrated into cpp-sort with all required x86-simd-sort files included in the project structure. No external dependencies required.

### Directory Structure
```
cpp-sort/
├── include/cpp-sort/
│   ├── sorters/
│   │   └── simd_sorter.h          # SIMD sorter wrapper
│   └── detail/
│       └── x86-simd-sort/         # x86-simd-sort implementation
│           ├── x86simdsort-static-incl.h
│           ├── avx2-*.hpp
│           ├── avx512-*.hpp
│           └── xss-*.h
└── benchmarks/sort-comparison/
    ├── bench-sort-comparison.cpp  # Performance benchmark
    └── test-simd-sorter.cpp       # Unit tests
```

### Usage

```cpp
#include <cpp-sort/sorters/simd_sorter.h>

// Method 1: Using the sorter directly
cppsort::simd_sorter sorter;
sorter(arr.begin(), arr.end());

// Method 2: Using the function object
cppsort::simd_sort(arr.begin(), arr.end());
```

## Summary of Results

### x86-simd-sort Performance Advantage

| Array Size | Data Type | Pattern | x86-simd-sort Speedup vs std::sort | x86-simd-sort Speedup vs Best cpp-sort |
|------------|-----------|---------|-----------------------------------|----------------------------------------|
| 100 | float | Random | 4.5x | 4.5x |
| 100 | double | Random | 3.5x | 3.5x |
| 100 | int32_t | Random | 4.8x | 4.8x |
| 1000 | float | Random | 3.8x | 2.9x |
| 1000 | double | Random | 2.9x | 2.9x |
| 1000 | int32_t | Random | 4.3x | 3.3x |
| 10000 | float | Random | 4.8x | 2.6x |
| 10000 | double | Random | 3.5x | 2.3x |
| 10000 | int32_t | Random | 5.1x | 3.3x |
| 100000 | float | Random | 7.5x | 3.2x |
| 100000 | double | Random | 5.5x | 2.9x |
| 100000 | int32_t | Random | 8.2x | 4.6x |
| 1000000 | float | Random | 10.0x | 3.8x |
| 1000000 | double | Random | 7.5x | 3.1x |
| 1000000 | int32_t | Random | 10.0x | 4.5x |

### Key Findings

#### 1. x86-simd-sort Performance
- **Small arrays (N=100)**: 3-5x faster than std::sort
- **Medium arrays (N=1000-10000)**: 3-5x faster than std::sort
- **Large arrays (N=100000-1000000)**: 7-10x faster than std::sort
- **Few Unique pattern**: Up to 35x faster than std::sort

#### 2. Best cpp-sort Algorithms by Scenario

| Scenario | Best cpp-sort Algorithm |
|----------|------------------------|
| Random data, small arrays | pdq_sorter, std_sorter |
| Random data, large arrays | pdq_sorter, spread_sorter |
| Sorted data | tim_sorter (extremely fast) |
| Reverse sorted | tim_sorter (extremely fast) |
| Nearly sorted | tim_sorter, pdq_sorter |
| Few unique values | spread_sorter, ska_sorter |

#### 3. Performance Patterns

**Random Data**:
- x86-simd-sort is consistently fastest
- pdq_sorter is the best cpp-sort algorithm (closest to std::sort performance)
- cpp-sort algorithms are 2-4x slower than x86-simd-sort

**Sorted/Reverse Sorted**:
- tim_sorter excels (adaptive sorting)
- x86-simd-sort still fast but not dominant
- pdq_sorter also performs well on nearly sorted data

**Few Unique Values**:
- x86-simd-sort has massive advantage (up to 35x vs std::sort)
- spread_sorter is best cpp-sort option for this pattern
- ska_sorter also good for few unique values

### Algorithm Recommendations

#### Use x86-simd-sort when:
1. Sorting primitive types (int32_t, int64_t, float, double)
2. Large arrays (>1000 elements)
3. Performance is critical
4. AVX2 or AVX-512 available

#### Use cpp-sort algorithms when:
1. Sorting custom types/objects
2. Need stable sorting
3. Special patterns (nearly sorted -> tim_sorter)
4. No SIMD available
5. Need specific algorithm properties (in-place, stable, etc.)

### Specific cpp-sort Algorithm Recommendations

| Algorithm | Best For |
|-----------|----------|
| pdq_sorter | General purpose, competitive with std::sort |
| tim_sorter | Nearly sorted data, adaptive cases |
| spread_sorter | Integer types with limited range |
| ska_sorter | Radix-sortable types, few unique values |
| spin_sorter | Memory-constrained environments |
| std_sorter | When std::sort behavior needed |

### Conclusion

x86-simd-sort demonstrates significant performance advantages for sorting primitive types across all tested scenarios:

1. **Consistent 3-10x speedup** over std::sort for random data
2. **Up to 35x speedup** for data with few unique values
3. **Scalable performance** that improves with larger arrays

cpp-sort provides a rich collection of sorting algorithms with different trade-offs. For primitive types where SIMD is available, x86-simd-sort is clearly superior. For custom types or when specific algorithm properties are needed, cpp-sort offers valuable alternatives.
