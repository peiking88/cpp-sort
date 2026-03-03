/*
 * Copyright (c) 2015-2025 Morwenn
 * SPDX-License-Identifier: MIT
 * 
 * Parallel stable merge sorter using libfork for large datasets
 * Uses parallel stable merge sort for datasets >= 1 million elements
 */
#ifndef CPPSORT_SORTERS_PARALLEL_MERGE_SORTER_H_
#define CPPSORT_SORTERS_PARALLEL_MERGE_SORTER_H_

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

// Check for C++20 coroutine support
#if __cplusplus >= 202002L && __has_include(<coroutine>)
    #define CPPSORT_HAS_LIBFORK_PARALLEL_MERGE 1
    #include "libfork/core.hpp"
    #include "libfork/schedule.hpp"
#endif

namespace cppsort
{
    ////////////////////////////////////////////////////////////
    // Configuration

    // Threshold: use parallel sort for >= 1 million elements
    inline constexpr std::size_t parallel_merge_sort_threshold = 1'000'000;

    // Minimum chunk size for parallel processing
    inline constexpr std::size_t parallel_merge_chunk_size = 100'000;

#ifdef CPPSORT_HAS_LIBFORK_PARALLEL_MERGE

    namespace detail
    {
        ////////////////////////////////////////////////////////////
        // Parallel stable merge sort implementation using libfork
        //
        // This is a stable parallel merge sort that preserves element order
        // for equal elements.

        inline constexpr auto parallel_stable_merge_sort_async = []<typename Iterator, typename Compare, typename Projection>(
            auto self, 
            Iterator first, 
            Iterator last,
            Compare compare, 
            Projection projection, 
            std::size_t threshold
        ) -> lf::task<> {
            auto size = static_cast<std::size_t>(last - first);
            
            if (size <= threshold) {
                // Base case: use sequential stable sort for small chunks
                std::stable_sort(first, last, projection_compare(compare, projection));
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
        // Parallel merge sorter implementation

        struct parallel_merge_sorter_impl
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
                    "parallel_merge_sorter requires at least random-access iterators"
                );

                auto size = static_cast<std::size_t>(last - first);
                
                if (size < 2) {
                    return;
                }

                // Use parallel sort for large datasets
                if (size >= parallel_merge_sort_threshold) {
                    // Create thread pool with hardware concurrency
                    lf::lazy_pool pool;
                    
                    // Compute threshold for base case
                    auto num_threads = std::thread::hardware_concurrency();
                    if (num_threads == 0) num_threads = 4;
                    
                    std::size_t chunk_threshold = std::max(
                        parallel_merge_chunk_size,
                        size / (num_threads * 4)
                    );
                    
                    // Execute parallel stable sort
                    lf::sync_wait(pool, parallel_stable_merge_sort_async,
                                 first, last, std::move(compare), std::move(projection), chunk_threshold);
                } else {
                    // Sequential stable sort for small datasets
                    std::stable_sort(std::move(first), std::move(last),
                              projection_compare(std::move(compare), std::move(projection)));
                }
            }

            ////////////////////////////////////////////////////////////
            // Sorter traits

            using iterator_category = std::random_access_iterator_tag;
            using is_always_stable = std::true_type;
        };
    }

#else // CPPSORT_HAS_LIBFORK_PARALLEL_MERGE not defined

    namespace detail
    {
        // Fallback to sequential std::stable_sort when libfork is not available
        struct parallel_merge_sorter_impl
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
                    "parallel_merge_sorter requires at least random-access iterators"
                );

                // Fallback to sequential stable sort
                std::stable_sort(std::move(first), std::move(last),
                          projection_compare(std::move(compare), std::move(projection)));
            }

            using iterator_category = std::random_access_iterator_tag;
            using is_always_stable = std::true_type;
        };
    }

#endif // CPPSORT_HAS_LIBFORK_PARALLEL_MERGE

    ////////////////////////////////////////////////////////////
    // Sorter facade

    struct parallel_merge_sorter:
        sorter_facade<detail::parallel_merge_sorter_impl>
    {};

    ////////////////////////////////////////////////////////////
    // Sort function

    inline constexpr parallel_merge_sorter parallel_merge_sort{};
}

#endif // CPPSORT_SORTERS_PARALLEL_MERGE_SORTER_H_
