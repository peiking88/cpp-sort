/*
 * Copyright (c) 2015-2025 Morwenn
 * SPDX-License-Identifier: MIT
 * 
 * Parallel SIMD sorter combining x86-simd-sort with libfork parallelism
 * Uses SIMD-optimized sorting for each chunk and parallel merge for large datasets
 */
#ifndef CPPSORT_SORTERS_PARALLEL_SIMD_SORTER_H_
#define CPPSORT_SORTERS_PARALLEL_SIMD_SORTER_H_

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <algorithm>
#include <functional>
#include <iterator>
#include <type_traits>
#include <utility>
#include <cpp-sort/comparators/projection_compare.h>
#include <cpp-sort/sorter_facade.h>
#include <cpp-sort/sorter_traits.h>
#include <cpp-sort/utility/as_function.h>
#include <cpp-sort/utility/functional.h>
#include "../detail/iterator_traits.h"
#include "../detail/type_traits.h"

// Include x86-simd-sort for SIMD optimization
#if defined(__AVX512F__) || defined(__AVX2__)
    #include "../detail/x86-simd-sort/x86simdsort-static-incl.h"
    #define CPPSORT_HAS_X86_SIMD_SORT_PARALLEL 1
#endif

// Check for C++20 coroutine support for parallelism
#if __cplusplus >= 202002L && __has_include(<coroutine>)
    #define CPPSORT_HAS_LIBFORK_PARALLEL_SIMD 1
    #include "libfork/core.hpp"
    #include "libfork/schedule.hpp"
#endif

namespace cppsort
{
    ////////////////////////////////////////////////////////////
    // Configuration

    // Threshold: use parallel sort for >= 1 million elements
    inline constexpr std::size_t parallel_simd_sort_threshold = 1'000'000;

    // Minimum chunk size for parallel processing
    inline constexpr std::size_t parallel_simd_chunk_size = 100'000;

    namespace detail
    {
        ////////////////////////////////////////////////////////////
        // Type traits for SIMD-sortable types

        template<typename T>
        struct is_simd_sortable_parallel : std::false_type {};

        template<> struct is_simd_sortable_parallel<uint16_t> : std::true_type {};
        template<> struct is_simd_sortable_parallel<int16_t>  : std::true_type {};
        template<> struct is_simd_sortable_parallel<uint32_t> : std::true_type {};
        template<> struct is_simd_sortable_parallel<int32_t>  : std::true_type {};
        template<> struct is_simd_sortable_parallel<uint64_t> : std::true_type {};
        template<> struct is_simd_sortable_parallel<int64_t>  : std::true_type {};
        template<> struct is_simd_sortable_parallel<float>    : std::true_type {};
        template<> struct is_simd_sortable_parallel<double>   : std::true_type {};

#if defined(CPPSORT_HAS_X86_SIMD_SORT_PARALLEL) && defined(CPPSORT_HAS_LIBFORK_PARALLEL_SIMD)

        ////////////////////////////////////////////////////////////
        // Parallel SIMD sort implementation using libfork
        //
        // Strategy: Divide array into chunks, sort each chunk with SIMD,
        // then merge chunks using parallel merge sort approach

        inline constexpr auto parallel_simd_sort_async_fn = []<typename Iterator>(
            auto self, 
            Iterator first, 
            Iterator last,
            std::size_t threshold
        ) -> lf::task<> {
            auto size = static_cast<std::size_t>(last - first);
            
            if (size <= threshold) {
                // Base case: use SIMD sort for small chunks
                if (size > 1) {
                    x86simdsortStatic::qsort(&*first, size);
                }
                co_return;
            }
            
            auto mid = first + size / 2;
            
            // Fork left half, call right half
            co_await lf::fork(self)(first, mid, threshold);
            co_await lf::call(self)(mid, last, threshold);
            
            co_await lf::join;
            
            // Merge the two sorted halves using std::inplace_merge
            std::inplace_merge(first, mid, last);
        };

        ////////////////////////////////////////////////////////////
        // Parallel SIMD sorter implementation

        struct parallel_simd_sorter_impl
        {
            template<
                typename RandomAccessIterator,
                typename Compare = std::less<>,
                typename Projection = utility::identity,
                typename = detail::enable_if_t<
                    is_projection_iterator_v<Projection, RandomAccessIterator, Compare>
                >
            >
            auto operator()(RandomAccessIterator first, RandomAccessIterator last,
                            Compare compare={}, Projection projection={}) const
                -> void
            {
                static_assert(
                    std::is_base_of_v<
                        iterator_category,
                        iterator_category_t<RandomAccessIterator>
                    >,
                    "parallel_simd_sorter requires at least random-access iterators"
                );

                using value_type = remove_cvref_t<
                    typename std::iterator_traits<RandomAccessIterator>::value_type
                >;

                auto size = static_cast<std::size_t>(last - first);
                
                if (size < 2) {
                    return;
                }

                // Check if we can use SIMD optimization
                constexpr bool can_use_simd = 
                    is_simd_sortable_parallel<value_type>::value &&
                    std::is_same_v<Projection, utility::identity> &&
                    std::is_same_v<Compare, std::less<>>;

                if constexpr (can_use_simd) {
                    // Use parallel SIMD sort for large datasets
                    if (size >= parallel_simd_sort_threshold) {
                        // Create thread pool with hardware concurrency
                        lf::lazy_pool pool;
                        
                        // Compute threshold for base case
                        auto num_threads = std::thread::hardware_concurrency();
                        if (num_threads == 0) num_threads = 4;
                        
                        std::size_t chunk_threshold = std::max(
                            parallel_simd_chunk_size,
                            size / (num_threads * 4)
                        );
                        
                        // Execute parallel SIMD sort
                        lf::sync_wait(pool, parallel_simd_sort_async_fn,
                                     first, last, chunk_threshold);
                    } else {
                        // Use SIMD sort directly for small datasets
                        x86simdsortStatic::qsort(&*first, size);
                    }
                } else {
                    // Fallback to std::sort for non-SIMD types or custom comparison
                    std::sort(std::move(first), std::move(last),
                              projection_compare(std::move(compare), std::move(projection)));
                }
            }

            ////////////////////////////////////////////////////////////
            // Sorter traits

            using iterator_category = std::random_access_iterator_tag;
            using is_always_stable = std::false_type;
        };

#else // SIMD or libfork not available

        ////////////////////////////////////////////////////////////
        // Fallback implementation

        struct parallel_simd_sorter_impl
        {
            template<
                typename RandomAccessIterator,
                typename Compare = std::less<>,
                typename Projection = utility::identity,
                typename = detail::enable_if_t<
                    is_projection_iterator_v<Projection, RandomAccessIterator, Compare>
                >
            >
            auto operator()(RandomAccessIterator first, RandomAccessIterator last,
                            Compare compare={}, Projection projection={}) const
                -> void
            {
                static_assert(
                    std::is_base_of_v<
                        iterator_category,
                        iterator_category_t<RandomAccessIterator>
                    >,
                    "parallel_simd_sorter requires at least random-access iterators"
                );

                using value_type = remove_cvref_t<
                    typename std::iterator_traits<RandomAccessIterator>::value_type
                >;

#if defined(CPPSORT_HAS_X86_SIMD_SORT_PARALLEL)
                // Fallback to SIMD sort without parallelism
                constexpr bool can_use_simd = 
                    is_simd_sortable_parallel<value_type>::value &&
                    std::is_same_v<Projection, utility::identity> &&
                    std::is_same_v<Compare, std::less<>>;

                if constexpr (can_use_simd) {
                    auto size = static_cast<std::size_t>(last - first);
                    if (size > 1) {
                        x86simdsortStatic::qsort(&*first, size);
                    }
                } else {
                    std::sort(std::move(first), std::move(last),
                              projection_compare(std::move(compare), std::move(projection)));
                }
#else
                // No SIMD support: use std::sort
                std::sort(std::move(first), std::move(last),
                          projection_compare(std::move(compare), std::move(projection)));
#endif
            }

            using iterator_category = std::random_access_iterator_tag;
            using is_always_stable = std::false_type;
        };

#endif // CPPSORT_HAS_X86_SIMD_SORT_PARALLEL && CPPSORT_HAS_LIBFORK_PARALLEL_SIMD

    } // namespace detail

    ////////////////////////////////////////////////////////////
    // Sorter facade

    struct parallel_simd_sorter:
        sorter_facade<detail::parallel_simd_sorter_impl>
    {};

    ////////////////////////////////////////////////////////////
    // Sort function

    inline constexpr parallel_simd_sorter parallel_simd_sort{};
}

#endif // CPPSORT_SORTERS_PARALLEL_SIMD_SORTER_H_
