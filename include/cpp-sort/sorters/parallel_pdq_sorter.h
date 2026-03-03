/*
 * Copyright (c) 2015-2025 Morwenn
 * SPDX-License-Identifier: MIT
 * 
 * Parallel pdqsort (Pattern-Defeating Quicksort) using libfork for large datasets
 * Uses parallel pdqsort for datasets >= 1 million elements
 */
#ifndef CPPSORT_SORTERS_PARALLEL_PDQ_SORTER_H_
#define CPPSORT_SORTERS_PARALLEL_PDQ_SORTER_H_

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
#include "../detail/pdqsort.h"
#include "../detail/type_traits.h"

// Check for C++20 coroutine support
#if __cplusplus >= 202002L && __has_include(<coroutine>)
    #define CPPSORT_HAS_LIBFORK_PARALLEL_PDQ 1
    #include "libfork/core.hpp"
    #include "libfork/schedule.hpp"
#endif

namespace cppsort
{
    ////////////////////////////////////////////////////////////
    // Configuration

    // Threshold: use parallel sort for >= 1 million elements
    inline constexpr std::size_t parallel_pdq_sort_threshold = 1'000'000;

    // Minimum chunk size for parallel processing
    inline constexpr std::size_t parallel_pdq_chunk_size = 100'000;

#ifdef CPPSORT_HAS_LIBFORK_PARALLEL_PDQ

    namespace detail
    {
        ////////////////////////////////////////////////////////////
        // Parallel pdqsort implementation using libfork
        //
        // Uses a simplified partition with median-of-three pivot selection.
        // For simplicity, we use std::sort for base cases and pdqsort
        // is used as the underlying sequential algorithm.

        template<typename Iterator, typename Compare, typename Projection>
        auto partition_pivot_pdq(Iterator first, Iterator last, Compare compare, Projection projection)
            -> Iterator
        {
            auto size = static_cast<std::size_t>(last - first);
            auto proj_func = utility::as_function(projection);
            
            // Median-of-three pivot selection
            Iterator mid = first + size / 2;
            
            // Sort first, mid, last
            if (compare(proj_func(*mid), proj_func(*first))) {
                std::iter_swap(mid, first);
            }
            if (compare(proj_func(*(last - 1)), proj_func(*first))) {
                std::iter_swap(last - 1, first);
            }
            if (compare(proj_func(*(last - 1)), proj_func(*mid))) {
                std::iter_swap(last - 1, mid);
            }
            
            // Move pivot to second-to-last position
            std::iter_swap(mid, last - 2);
            
            // Partition
            auto pivot = last - 2;
            auto i = first;
            auto j = pivot;
            
            while (true) {
                while (compare(proj_func(*++i), proj_func(*pivot)));
                while (compare(proj_func(*pivot), proj_func(*--j)));
                
                if (i < j) {
                    std::iter_swap(i, j);
                } else {
                    break;
                }
            }
            
            // Move pivot to final position
            std::iter_swap(i, pivot);
            return i;
        }

        inline constexpr auto parallel_pdqsort_async = []<typename Iterator, typename Compare, typename Projection>(
            auto self, 
            Iterator first, 
            Iterator last,
            Compare compare, 
            Projection projection, 
            std::size_t threshold
        ) -> lf::task<> {
            auto size = static_cast<std::size_t>(last - first);
            
            if (size <= threshold) {
                // Base case: use pdqsort for small chunks
                pdqsort(first, last, compare, projection);
                co_return;
            }
            
            // Partition around pivot
            auto pivot = partition_pivot_pdq(first, last, compare, projection);
            
            // Fork left partition, call right partition
            co_await lf::fork(self)(first, pivot, compare, projection, threshold);
            co_await lf::call(self)(pivot + 1, last, compare, projection, threshold);
            
            co_await lf::join;
        };

        ////////////////////////////////////////////////////////////
        // Parallel pdq sorter implementation

        struct parallel_pdq_sorter_impl
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
                    "parallel_pdq_sorter requires at least random-access iterators"
                );

                auto size = static_cast<std::size_t>(last - first);
                
                if (size < 2) {
                    return;
                }

                // Use parallel sort for large datasets
                if (size >= parallel_pdq_sort_threshold) {
                    // Create thread pool with hardware concurrency
                    lf::lazy_pool pool;
                    
                    // Compute threshold for base case
                    auto num_threads = std::thread::hardware_concurrency();
                    if (num_threads == 0) num_threads = 4;
                    
                    std::size_t chunk_threshold = std::max(
                        parallel_pdq_chunk_size,
                        size / (num_threads * 4)
                    );
                    
                    // Execute parallel pdqsort
                    lf::sync_wait(pool, parallel_pdqsort_async,
                                 first, last, std::move(compare), std::move(projection), chunk_threshold);
                } else {
                    // Sequential pdqsort for small datasets
                    pdqsort(std::move(first), std::move(last),
                            std::move(compare), std::move(projection));
                }
            }

            ////////////////////////////////////////////////////////////
            // Sorter traits

            using iterator_category = std::random_access_iterator_tag;
            using is_always_stable = std::false_type;
        };
    }

#else // CPPSORT_HAS_LIBFORK_PARALLEL_PDQ not defined

    namespace detail
    {
        // Fallback to sequential pdqsort when libfork is not available
        struct parallel_pdq_sorter_impl
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
                    "parallel_pdq_sorter requires at least random-access iterators"
                );

                // Fallback to sequential pdqsort
                pdqsort(std::move(first), std::move(last),
                        std::move(compare), std::move(projection));
            }

            using iterator_category = std::random_access_iterator_tag;
            using is_always_stable = std::false_type;
        };
    }

#endif // CPPSORT_HAS_LIBFORK_PARALLEL_PDQ

    ////////////////////////////////////////////////////////////
    // Sorter facade

    struct parallel_pdq_sorter:
        sorter_facade<detail::parallel_pdq_sorter_impl>
    {};

    ////////////////////////////////////////////////////////////
    // Sort function

    inline constexpr parallel_pdq_sorter parallel_pdq_sort{};
}

#endif // CPPSORT_SORTERS_PARALLEL_PDQ_SORTER_H_
