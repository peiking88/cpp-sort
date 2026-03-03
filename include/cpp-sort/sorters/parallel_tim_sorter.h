/*
 * Copyright (c) 2015-2025 Morwenn
 * SPDX-License-Identifier: MIT
 * 
 * Parallel TimSort using libfork for large datasets
 * TimSort is a stable, adaptive, iterative mergesort that requires O(n log n) time
 * This parallel version uses libfork for parallel merging while maintaining stability
 */
#ifndef CPPSORT_SORTERS_PARALLEL_TIM_SORTER_H_
#define CPPSORT_SORTERS_PARALLEL_TIM_SORTER_H_

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
#include "../detail/timsort.h"
#include "../detail/type_traits.h"

// Check for C++20 coroutine support
#if __cplusplus >= 202002L && __has_include(<coroutine>)
    #define CPPSORT_HAS_LIBFORK_PARALLEL_TIM 1
    #include "libfork/core.hpp"
    #include "libfork/schedule.hpp"
#endif

namespace cppsort
{
    ////////////////////////////////////////////////////////////
    // Configuration

    // Threshold: use parallel sort for >= 1 million elements
    inline constexpr std::size_t parallel_tim_sort_threshold = 1'000'000;

    // Minimum chunk size for parallel processing
    inline constexpr std::size_t parallel_tim_chunk_size = 100'000;

#ifdef CPPSORT_HAS_LIBFORK_PARALLEL_TIM

    namespace detail
    {
        ////////////////////////////////////////////////////////////
        // Parallel stable merge sort implementation using libfork
        // This serves as the parallel core for parallel_tim_sorter
        // Using the same stable merge approach as parallel_merge_sorter

        inline constexpr auto parallel_stable_merge_for_tim_async = []<typename Iterator, typename Compare, typename Projection>(
            auto self, 
            Iterator first, 
            Iterator last,
            Compare compare, 
            Projection projection, 
            std::size_t threshold
        ) -> lf::task<> {
            auto size = static_cast<std::size_t>(last - first);
            
            if (size <= threshold) {
                // Base case: use sequential timsort for small chunks
                // TimSort is adaptive and performs well on partially sorted data
                timsort(first, last, compare, projection);
                co_return;
            }
            
            auto mid = first + size / 2;
            
            // Fork left half, call right half (both run in parallel due to continuation stealing)
            co_await lf::fork(self)(first, mid, compare, projection, threshold);
            co_await lf::call(self)(mid, last, compare, projection, threshold);
            
            co_await lf::join;
            
            // Stable merge the two sorted halves
            auto proj_func = utility::as_function(projection);
            std::inplace_merge(first, mid, last,
                [&](const auto& a, const auto& b) {
                    return compare(proj_func(a), proj_func(b));
                });
        };

        ////////////////////////////////////////////////////////////
        // Parallel TimSort implementation

        struct parallel_tim_sorter_impl
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
                    "parallel_tim_sorter requires at least random-access iterators"
                );

                auto size = static_cast<std::size_t>(last - first);
                
                if (size < 2) {
                    return;
                }

                // Use parallel sort for large datasets
                if (size >= parallel_tim_sort_threshold) {
                    // Create thread pool with hardware concurrency
                    lf::lazy_pool pool;
                    
                    // Compute threshold for base case
                    auto num_threads = std::thread::hardware_concurrency();
                    if (num_threads == 0) num_threads = 4;
                    
                    std::size_t chunk_threshold = std::max(
                        parallel_tim_chunk_size,
                        size / (num_threads * 4)
                    );
                    
                    // Execute parallel stable sort
                    lf::sync_wait(pool, parallel_stable_merge_for_tim_async,
                                 first, last, std::move(compare), std::move(projection), chunk_threshold);
                } else {
                    // Sequential timsort for small datasets
                    // TimSort is adaptive and efficient for small/partially sorted data
                    timsort(std::move(first), std::move(last),
                            std::move(compare), std::move(projection));
                }
            }

            ////////////////////////////////////////////////////////////
            // Sorter traits

            using iterator_category = std::random_access_iterator_tag;
            using is_always_stable = std::true_type;
        };
    }

#else // CPPSORT_HAS_LIBFORK_PARALLEL_TIM not defined

    namespace detail
    {
        // Fallback to sequential timsort when libfork is not available
        struct parallel_tim_sorter_impl
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
                    "parallel_tim_sorter requires at least random-access iterators"
                );

                // Fallback to sequential timsort
                timsort(std::move(first), std::move(last),
                        std::move(compare), std::move(projection));
            }

            using iterator_category = std::random_access_iterator_tag;
            using is_always_stable = std::true_type;
        };
    }

#endif // CPPSORT_HAS_LIBFORK_PARALLEL_TIM

    ////////////////////////////////////////////////////////////
    // Sorter facade

    struct parallel_tim_sorter:
        sorter_facade<detail::parallel_tim_sorter_impl>
    {};

    ////////////////////////////////////////////////////////////
    // Sort function

    inline constexpr parallel_tim_sorter parallel_tim_sort{};
}

#endif // CPPSORT_SORTERS_PARALLEL_TIM_SORTER_H_
