/*
 * Copyright (c) 2015-2025 Morwenn
 * SPDX-License-Identifier: MIT
 */
#include <algorithm>
#include <functional>
#include <iterator>
#include <map>
#include <random>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <catch2/catch_test_macros.hpp>
#include <cpp-sort/sorters/parallel_tim_sorter.h>
#include <testing-tools/distributions.h>

TEST_CASE( "parallel_tim_sorter with projection", "[parallel_tim_sorter][projection]" )
{
    SECTION( "projection with integers (negate)" )
    {
        std::vector<int> vec = { 5, 3, 8, 1, 9, 4, 2, 7, 6, 0 };
        // Sort by negative value (descending order)
        cppsort::parallel_tim_sort(vec, std::less<>{}, [](int n) { return -n; });
        
        std::vector<int> expected = { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
        CHECK( vec == expected );
    }

    SECTION( "projection with pairs (first element)" )
    {
        std::vector<std::pair<int, std::string>> vec = {
            {5, "five"}, {3, "three"}, {8, "eight"}, {1, "one"}, {9, "nine"}
        };
        
        cppsort::parallel_tim_sort(vec, std::less<>{}, &std::pair<int, std::string>::first);
        
        std::vector<std::pair<int, std::string>> expected = {
            {1, "one"}, {3, "three"}, {5, "five"}, {8, "eight"}, {9, "nine"}
        };
        CHECK( vec == expected );
    }

    SECTION( "projection with pairs (second element)" )
    {
        std::vector<std::pair<int, std::string>> vec = {
            {5, "five"}, {3, "three"}, {8, "eight"}, {1, "one"}, {9, "nine"}
        };
        
        cppsort::parallel_tim_sort(vec, std::less<>{}, [](const auto& p) { return p.second; });
        
        std::vector<std::pair<int, std::string>> expected = {
            {8, "eight"}, {5, "five"}, {9, "nine"}, {1, "one"}, {3, "three"}
        };
        CHECK( vec == expected );
    }

    SECTION( "projection with tuples" )
    {
        std::vector<std::tuple<int, double, std::string>> vec = {
            {5, 3.14, "five"}, {3, 2.71, "three"}, {8, 1.41, "eight"}, {1, 0.577, "one"}
        };
        
        // Sort by first element of tuple
        cppsort::parallel_tim_sort(vec, std::less<>{}, [](const auto& t) { return std::get<0>(t); });
        
        std::vector<std::tuple<int, double, std::string>> expected = {
            {1, 0.577, "one"}, {3, 2.71, "three"}, {5, 3.14, "five"}, {8, 1.41, "eight"}
        };
        CHECK( vec == expected );
    }

    SECTION( "projection with struct" )
    {
        struct Person {
            std::string name;
            int age;
        };
        
        std::vector<Person> vec = {
            {"Alice", 30}, {"Bob", 25}, {"Charlie", 35}, {"David", 28}
        };
        
        // Sort by age
        cppsort::parallel_tim_sort(vec, std::less<>{}, &Person::age);
        
        std::vector<Person> expected = {
            {"Bob", 25}, {"David", 28}, {"Alice", 30}, {"Charlie", 35}
        };
        
        REQUIRE( vec.size() == expected.size() );
        for (size_t i = 0; i < vec.size(); ++i) {
            CHECK( vec[i].name == expected[i].name );
            CHECK( vec[i].age == expected[i].age );
        }
    }

    SECTION( "large dataset with projection" )
    {
        struct Data {
            int key;
            int value;
        };
        
        std::vector<Data> vec;
        vec.reserve(1'100'000);
        std::mt19937 gen(99999);
        std::uniform_int_distribution<> dist(-1000000, 1000000);
        
        for (int i = 0; i < 1'100'000; ++i) {
            vec.push_back({dist(gen), i});
        }
        
        std::vector<Data> expected = vec;
        std::sort(expected.begin(), expected.end(), 
                  [](const Data& a, const Data& b) { return a.key < b.key; });
        
        cppsort::parallel_tim_sort(vec, std::less<>{}, &Data::key);
        
        REQUIRE( vec.size() == expected.size() );
        for (size_t i = 0; i < vec.size(); ++i) {
            CHECK( vec[i].key == expected[i].key );
        }
    }
}

TEST_CASE( "parallel_tim_sorter projection stability", "[parallel_tim_sorter][projection][stability]" )
{
    struct Element {
        int value;
        int id;
    };
    
    SECTION( "projection preserves stability" )
    {
        std::vector<Element> vec;
        vec.reserve(200);
        for (int i = 0; i < 200; ++i) {
            vec.push_back({i % 10, i});
        }
        
        // Shuffle
        std::mt19937 gen(42);
        std::shuffle(vec.begin(), vec.end(), gen);
        
        // Record the order of elements with same value after shuffle
        std::map<int, std::vector<int>> expected_order;
        for (const auto& e : vec) {
            expected_order[e.value].push_back(e.id);
        }
        
        cppsort::parallel_tim_sort(vec, std::less<>{}, &Element::value);
        
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
