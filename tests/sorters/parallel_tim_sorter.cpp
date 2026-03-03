/*
 * Copyright (c) 2015-2025 Morwenn
 * SPDX-License-Identifier: MIT
 */
#include <algorithm>
#include <functional>
#include <iterator>
#include <random>
#include <vector>
#include <map>
#include <catch2/catch_test_macros.hpp>
#include <cpp-sort/sorters/parallel_tim_sorter.h>
#include <testing-tools/distributions.h>

TEST_CASE( "parallel_tim_sorter tests", "[parallel_tim_sorter]" )
{
    SECTION( "empty collection" )
    {
        std::vector<int> vec;
        cppsort::parallel_tim_sort(vec);
        CHECK( vec.empty() );
    }

    SECTION( "single element" )
    {
        std::vector<int> vec = { 42 };
        cppsort::parallel_tim_sort(vec);
        REQUIRE( vec.size() == 1 );
        CHECK( vec[0] == 42 );
    }

    SECTION( "two elements sorted" )
    {
        std::vector<int> vec = { 1, 2 };
        cppsort::parallel_tim_sort(vec);
        REQUIRE( vec.size() == 2 );
        CHECK( vec[0] == 1 );
        CHECK( vec[1] == 2 );
    }

    SECTION( "two elements unsorted" )
    {
        std::vector<int> vec = { 2, 1 };
        cppsort::parallel_tim_sort(vec);
        REQUIRE( vec.size() == 2 );
        CHECK( vec[0] == 1 );
        CHECK( vec[1] == 2 );
    }

    SECTION( "three elements" )
    {
        std::vector<int> vec = { 3, 1, 2 };
        cppsort::parallel_tim_sort(vec);
        REQUIRE( vec.size() == 3 );
        CHECK( vec[0] == 1 );
        CHECK( vec[1] == 2 );
        CHECK( vec[2] == 3 );
    }

    SECTION( "small collection" )
    {
        std::vector<int> vec = { 5, 3, 8, 1, 9, 4, 2, 7, 6, 0 };
        cppsort::parallel_tim_sort(vec);
        
        std::vector<int> expected = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        CHECK( vec == expected );
    }

    SECTION( "with duplicates" )
    {
        std::vector<int> vec = { 5, 3, 8, 3, 9, 5, 2, 7, 2, 0 };
        cppsort::parallel_tim_sort(vec);
        
        std::vector<int> expected = { 0, 2, 2, 3, 3, 5, 5, 7, 8, 9 };
        CHECK( vec == expected );
    }

    SECTION( "already sorted" )
    {
        std::vector<int> vec = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        cppsort::parallel_tim_sort(vec);
        
        std::vector<int> expected = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        CHECK( vec == expected );
    }

    SECTION( "reverse sorted" )
    {
        std::vector<int> vec = { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
        cppsort::parallel_tim_sort(vec);
        
        std::vector<int> expected = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        CHECK( vec == expected );
    }

    SECTION( "all equal elements" )
    {
        std::vector<int> vec(100, 42);
        cppsort::parallel_tim_sort(vec);
        
        std::vector<int> expected(100, 42);
        CHECK( vec == expected );
    }

    SECTION( "random data" )
    {
        std::vector<int> vec;
        vec.reserve(1000);
        std::mt19937 gen(42);
        std::uniform_int_distribution<> dist(-10000, 10000);
        
        for (int i = 0; i < 1000; ++i) {
            vec.push_back(dist(gen));
        }
        
        std::vector<int> expected = vec;
        std::sort(expected.begin(), expected.end());
        
        cppsort::parallel_tim_sort(vec);
        CHECK( vec == expected );
    }

    SECTION( "large dataset (triggers parallel)" )
    {
        std::vector<int> vec;
        vec.reserve(1'100'000);
        std::mt19937 gen(12345);
        std::uniform_int_distribution<> dist(-1000000, 1000000);
        
        for (int i = 0; i < 1'100'000; ++i) {
            vec.push_back(dist(gen));
        }
        
        std::vector<int> expected = vec;
        std::sort(expected.begin(), expected.end());
        
        cppsort::parallel_tim_sort(vec);
        CHECK( vec == expected );
    }

    SECTION( "custom compare (descending)" )
    {
        std::vector<int> vec = { 5, 3, 8, 1, 9, 4, 2, 7, 6, 0 };
        cppsort::parallel_tim_sort(vec, std::greater<>{});
        
        std::vector<int> expected = { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
        CHECK( vec == expected );
    }

    SECTION( "double values" )
    {
        std::vector<double> vec = { 3.14, 1.41, 2.71, 0.577, 1.732 };
        cppsort::parallel_tim_sort(vec);
        
        std::vector<double> expected = { 0.577, 1.41, 1.732, 2.71, 3.14 };
        CHECK( vec == expected );
    }

    SECTION( "large dataset with custom compare" )
    {
        std::vector<int> vec;
        vec.reserve(1'100'000);
        std::mt19937 gen(54321);
        std::uniform_int_distribution<> dist(-1000000, 1000000);
        
        for (int i = 0; i < 1'100'000; ++i) {
            vec.push_back(dist(gen));
        }
        
        std::vector<int> expected = vec;
        std::sort(expected.begin(), expected.end(), std::greater<>{});
        
        cppsort::parallel_tim_sort(vec, std::greater<>{});
        CHECK( vec == expected );
    }
}

TEST_CASE( "parallel_tim_sorter stability test", "[parallel_tim_sorter][stability]" )
{
    // Test that parallel_tim_sorter is stable
    struct Element {
        int value;
        int id;  // Unique identifier
        
        bool operator<(const Element& other) const {
            return value < other.value;
        }
    };
    
    SECTION( "small dataset: stability check" )
    {
        std::vector<Element> vec;
        vec.reserve(100);
        for (int i = 0; i < 100; ++i) {
            vec.push_back({i % 10, i});  // Equal values with different IDs
        }
        
        // Shuffle
        std::mt19937 gen(42);
        std::shuffle(vec.begin(), vec.end(), gen);
        
        // Record the order of elements with same value after shuffle
        std::map<int, std::vector<int>> expected_order;
        for (const auto& e : vec) {
            expected_order[e.value].push_back(e.id);
        }
        
        cppsort::parallel_tim_sort(vec);
        
        // Check stability: elements with equal values should preserve shuffle order
        for (size_t i = 1; i < vec.size(); ++i) {
            if (vec[i-1].value == vec[i].value) {
                // Find position in expected order
                auto& ids = expected_order[vec[i].value];
                auto it_prev = std::find(ids.begin(), ids.end(), vec[i-1].id);
                auto it_curr = std::find(ids.begin(), ids.end(), vec[i].id);
                CHECK( it_prev < it_curr );
            }
        }
    }
    
    SECTION( "large dataset: stability check" )
    {
        std::vector<Element> vec;
        vec.reserve(1'100'000);
        for (int i = 0; i < 1'100'000; ++i) {
            vec.push_back({i % 100, i});  // Equal values with different IDs
        }
        
        // Shuffle
        std::mt19937 gen(42);
        std::shuffle(vec.begin(), vec.end(), gen);
        
        // Record the order of elements with same value after shuffle
        std::map<int, std::vector<int>> expected_order;
        for (const auto& e : vec) {
            expected_order[e.value].push_back(e.id);
        }
        
        cppsort::parallel_tim_sort(vec);
        
        // Check stability: elements with equal values should preserve shuffle order
        for (size_t i = 1; i < vec.size(); ++i) {
            if (vec[i-1].value == vec[i].value) {
                // Find position in expected order
                auto& ids = expected_order[vec[i].value];
                auto it_prev = std::find(ids.begin(), ids.end(), vec[i-1].id);
                auto it_curr = std::find(ids.begin(), ids.end(), vec[i].id);
                CHECK( it_prev < it_curr );
            }
        }
    }
}
