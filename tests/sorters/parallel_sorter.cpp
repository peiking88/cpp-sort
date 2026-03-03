/*
 * Copyright (c) 2015-2025 Morwenn
 * SPDX-License-Identifier: MIT
 */
#include <algorithm>
#include <functional>
#include <iterator>
#include <vector>
#include <random>
#include <catch2/catch_test_macros.hpp>
#include <cpp-sort/sorters/parallel_sorter.h>
#include <testing-tools/distributions.h>

TEST_CASE( "parallel_sorter tests", "[parallel_sorter]" )
{
    // Test small dataset (uses sequential sort)
    SECTION( "small dataset: sort with random-access range" )
    {
        std::vector<int> vec;
        vec.reserve(100);
        auto distribution = dist::shuffled{};
        distribution(std::back_inserter(vec), 100, 0);

        cppsort::parallel_sort(vec);
        CHECK( std::is_sorted(vec.begin(), vec.end()) );
    }

    SECTION( "small dataset: sort with random-access range and compare" )
    {
        std::vector<int> vec;
        vec.reserve(100);
        auto distribution = dist::shuffled{};
        distribution(std::back_inserter(vec), 100, 0);

        cppsort::parallel_sort(vec, std::greater{});
        CHECK( std::is_sorted(vec.begin(), vec.end(), std::greater{}) );
    }

    SECTION( "small dataset: sort with random-access iterators" )
    {
        std::vector<int> vec;
        vec.reserve(100);
        auto distribution = dist::shuffled{};
        distribution(std::back_inserter(vec), 100, 0);

        cppsort::parallel_sort(vec.begin(), vec.end());
        CHECK( std::is_sorted(vec.begin(), vec.end()) );
    }

    SECTION( "small dataset: sort with random-access iterators and compare" )
    {
        std::vector<int> vec;
        vec.reserve(100);
        auto distribution = dist::shuffled{};
        distribution(std::back_inserter(vec), 100, 0);

        cppsort::parallel_sort(vec.begin(), vec.end(), std::greater{});
        CHECK( std::is_sorted(vec.begin(), vec.end(), std::greater{}) );
    }

    // Test with different distributions
    SECTION( "small dataset: ascending distribution" )
    {
        std::vector<int> vec;
        vec.reserve(100);
        auto distribution = dist::ascending{};
        distribution(std::back_inserter(vec), 100);

        cppsort::parallel_sort(vec);
        CHECK( std::is_sorted(vec.begin(), vec.end()) );
    }

    SECTION( "small dataset: descending distribution" )
    {
        std::vector<int> vec;
        vec.reserve(100);
        auto distribution = dist::descending{};
        distribution(std::back_inserter(vec), 100);

        cppsort::parallel_sort(vec);
        CHECK( std::is_sorted(vec.begin(), vec.end()) );
    }

    SECTION( "small dataset: all equal" )
    {
        std::vector<int> vec;
        vec.reserve(100);
        auto distribution = dist::all_equal{};
        distribution(std::back_inserter(vec), 100);

        cppsort::parallel_sort(vec);
        CHECK( std::is_sorted(vec.begin(), vec.end()) );
    }

    // Edge cases
    SECTION( "empty collection" )
    {
        std::vector<int> vec;
        cppsort::parallel_sort(vec);
        CHECK( vec.empty() );
    }

    SECTION( "single element" )
    {
        std::vector<int> vec = { 42 };
        cppsort::parallel_sort(vec);
        CHECK( vec.size() == 1 );
        CHECK( vec[0] == 42 );
    }

    SECTION( "two elements sorted" )
    {
        std::vector<int> vec = { 1, 2 };
        cppsort::parallel_sort(vec);
        CHECK( std::is_sorted(vec.begin(), vec.end()) );
    }

    SECTION( "two elements unsorted" )
    {
        std::vector<int> vec = { 2, 1 };
        cppsort::parallel_sort(vec);
        CHECK( std::is_sorted(vec.begin(), vec.end()) );
    }
}

TEST_CASE( "parallel_sorter large dataset tests", "[parallel_sorter][large]" )
{
    // Test large dataset (>= 1M elements, triggers parallel sort)
    SECTION( "large dataset: parallel sort with random-access range" )
    {
        std::vector<int> vec;
        vec.reserve(1'100'000);
        auto distribution = dist::shuffled{};
        distribution(std::back_inserter(vec), 1'100'000, 42);

        cppsort::parallel_sort(vec);
        CHECK( std::is_sorted(vec.begin(), vec.end()) );
    }

    SECTION( "large dataset: parallel sort with compare" )
    {
        std::vector<int> vec;
        vec.reserve(1'100'000);
        auto distribution = dist::shuffled{};
        distribution(std::back_inserter(vec), 1'100'000, 42);

        cppsort::parallel_sort(vec, std::greater{});
        CHECK( std::is_sorted(vec.begin(), vec.end(), std::greater{}) );
    }

    SECTION( "large dataset: float parallel sort" )
    {
        std::vector<float> vec;
        vec.reserve(1'100'000);
        
        std::mt19937 gen(42);
        std::uniform_real_distribution<float> dis(0.0f, 1.0f);
        for (size_t i = 0; i < 1'100'000; ++i) {
            vec.push_back(dis(gen));
        }

        cppsort::parallel_sort(vec);
        CHECK( std::is_sorted(vec.begin(), vec.end()) );
    }

    SECTION( "large dataset: double parallel sort" )
    {
        std::vector<double> vec;
        vec.reserve(1'100'000);
        
        std::mt19937 gen(42);
        std::uniform_real_distribution<double> dis(0.0, 1.0);
        for (size_t i = 0; i < 1'100'000; ++i) {
            vec.push_back(dis(gen));
        }

        cppsort::parallel_sort(vec);
        CHECK( std::is_sorted(vec.begin(), vec.end()) );
    }
}
