# Benchmark: cpp-sort sorting algorithms comparison
cmake_minimum_required(VERSION 3.14)
project(sort-comparison CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable optimizations
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# cpp-sort include (project root is 3 levels up)
set(CPPSORT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../include")
set(LIBFORK_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../external/libfork/include")

# Benchmark executable
add_executable(bench-sort-comparison
    bench-sort-comparison.cpp
)

target_include_directories(bench-sort-comparison PRIVATE
    ${CPPSORT_INCLUDE_DIR}
    ${LIBFORK_INCLUDE_DIR}
)

target_compile_options(bench-sort-comparison PRIVATE
    -O3
    -march=native
    -Wall
    -Wextra
)

# Link against libm for math functions and pthread for libfork
target_link_libraries(bench-sort-comparison PRIVATE m pthread)

# Optional: Enable OpenMP for x86-simd-sort
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(bench-sort-comparison PRIVATE OpenMP::OpenMP_CXX)
    target_compile_definitions(bench-sort-comparison PRIVATE XSS_USE_OPENMP)
endif()

# Test executable for simd_sorter
add_executable(test-simd-sorter
    test-simd-sorter.cpp
)

target_include_directories(test-simd-sorter PRIVATE
    ${CPPSORT_INCLUDE_DIR}
    ${LIBFORK_INCLUDE_DIR}
)

target_compile_options(test-simd-sorter PRIVATE
    -O3
    -march=native
    -Wall
    -Wextra
)

target_link_libraries(test-simd-sorter PRIVATE m pthread)

if(OpenMP_CXX_FOUND)
    target_link_libraries(test-simd-sorter PRIVATE OpenMP::OpenMP_CXX)
    target_compile_definitions(test-simd-sorter PRIVATE XSS_USE_OPENMP)
endif()
