/*
 * Copyright (c) 2015-2025 Morwenn
 * SPDX-License-Identifier: MIT
 * 
 * SIMD sorter wrapper for x86-simd-sort
 * Integrates x86-simd-sort into cpp-sort framework
 */
#ifndef CPPSORT_SORTERS_SIMD_SORTER_H_
#define CPPSORT_SORTERS_SIMD_SORTER_H_

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

// Include x86-simd-sort static implementation
// The files are now integrated into cpp-sort at detail/x86-simd-sort/
#if defined(__AVX512F__) || defined(__AVX2__)
    #include "../detail/x86-simd-sort/x86simdsort-static-incl.h"
    #define CPPSORT_HAS_X86_SIMD_SORT 1
#endif

namespace cppsort
{
    ////////////////////////////////////////////////////////////
    // Type traits for SIMD-sortable types

    namespace detail
    {
        // Types supported by x86-simd-sort
        template<typename T>
        struct is_simd_sortable : std::false_type {};

        template<> struct is_simd_sortable<uint16_t> : std::true_type {};
        template<> struct is_simd_sortable<int16_t>  : std::true_type {};
        template<> struct is_simd_sortable<uint32_t> : std::true_type {};
        template<> struct is_simd_sortable<int32_t>  : std::true_type {};
        template<> struct is_simd_sortable<uint64_t> : std::true_type {};
        template<> struct is_simd_sortable<int64_t>  : std::true_type {};
        template<> struct is_simd_sortable<float>    : std::true_type {};
        template<> struct is_simd_sortable<double>   : std::true_type {};

        ////////////////////////////////////////////////////////////
        // SIMD sorter implementation

        struct simd_sorter_impl
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
                    "simd_sorter requires at least random-access iterators"
                );

                using value_type = remove_cvref_t<
                    typename std::iterator_traits<RandomAccessIterator>::value_type
                >;
                
                constexpr bool can_use_simd = 
                    is_simd_sortable<value_type>::value &&
                    std::is_same_v<Projection, utility::identity> &&
                    std::is_same_v<Compare, std::less<>>;

#if defined(CPPSORT_HAS_X86_SIMD_SORT)
                if constexpr (can_use_simd) {
                    // Fast path: direct SIMD sort
                    auto size = static_cast<std::size_t>(last - first);
                    if (size > 1) {
                        x86simdsortStatic::qsort(&*first, size);
                    }
                } else {
                    // Fallback to std::sort for custom comparison/projection
                    std::sort(std::move(first), std::move(last),
                              projection_compare(std::move(compare), std::move(projection)));
                }
#else
                // No SIMD support: use std::sort
                std::sort(std::move(first), std::move(last),
                          projection_compare(std::move(compare), std::move(projection)));
#endif
            }

            ////////////////////////////////////////////////////////////
            // Sorter traits

            using iterator_category = std::random_access_iterator_tag;
            using is_always_stable = std::false_type;
        };
    }

    ////////////////////////////////////////////////////////////
    // Sorter facade

    struct simd_sorter:
        sorter_facade<detail::simd_sorter_impl>
    {};

    ////////////////////////////////////////////////////////////
    // Sort function

    inline constexpr simd_sorter simd_sort{};
}

#endif // CPPSORT_SORTERS_SIMD_SORTER_H_
