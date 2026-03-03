/*
 * Copyright (c) 2015-2025 Morwenn
 * SPDX-License-Identifier: MIT
 */
#include <algorithm>
#include <functional>
#include <iterator>
#include <vector>
#include <random>
#include <string>
#include <catch2/catch_test_macros.hpp>
#include <cpp-sort/sorters/parallel_sorter.h>
#include <testing-tools/algorithm.h>
#include <testing-tools/distributions.h>
#include <testing-tools/wrapper.h>

TEST_CASE( "parallel_sorter tests with projections",
           "[parallel_sorter][projection]" )
{
    // Wrapper to hide the integer
    using wrapper = generic_wrapper<int>;

    // Small dataset tests with projection
    SECTION( "small dataset: sort with random-access range and projection" )
    {
        std::vector<wrapper> vec;
        auto distribution = dist::shuffled{};
        distribution(std::back_inserter(vec), 100);

        cppsort::parallel_sort(vec, &wrapper::value);
        CHECK( helpers::is_sorted(vec.begin(), vec.end(), std::less{}, &wrapper::value) );
    }

    SECTION( "small dataset: sort with iterators and projection" )
    {
        std::vector<wrapper> vec;
        auto distribution = dist::shuffled{};
        distribution(std::back_inserter(vec), 100);

        cppsort::parallel_sort(vec.begin(), vec.end(), &wrapper::value);
        CHECK( helpers::is_sorted(vec.begin(), vec.end(), std::less{}, &wrapper::value) );
    }

    SECTION( "small dataset: sort with compare and projection" )
    {
        std::vector<wrapper> vec;
        auto distribution = dist::shuffled{};
        distribution(std::back_inserter(vec), 100);

        cppsort::parallel_sort(vec, std::greater{}, &wrapper::value);
        CHECK( helpers::is_sorted(vec.begin(), vec.end(), std::greater{}, &wrapper::value) );
    }

    SECTION( "small dataset: sort with iterators, compare and projection" )
    {
        std::vector<wrapper> vec;
        auto distribution = dist::shuffled{};
        distribution(std::back_inserter(vec), 100);

        cppsort::parallel_sort(vec.begin(), vec.end(), std::greater{}, &wrapper::value);
        CHECK( helpers::is_sorted(vec.begin(), vec.end(), std::greater{}, &wrapper::value) );
    }
}

TEST_CASE( "parallel_sorter large dataset tests with projections",
           "[parallel_sorter][projection][large]" )
{
    using wrapper = generic_wrapper<int>;

    // Large dataset tests with projection
    SECTION( "large dataset: sort with projection" )
    {
        std::vector<wrapper> vec;
        vec.reserve(1'100'000);
        auto distribution = dist::shuffled{};
        distribution(std::back_inserter(vec), 1'100'000);

        cppsort::parallel_sort(vec, &wrapper::value);
        CHECK( helpers::is_sorted(vec.begin(), vec.end(), std::less{}, &wrapper::value) );
    }

    SECTION( "large dataset: sort with compare and projection" )
    {
        std::vector<wrapper> vec;
        vec.reserve(1'100'000);
        auto distribution = dist::shuffled{};
        distribution(std::back_inserter(vec), 1'100'000);

        cppsort::parallel_sort(vec, std::greater{}, &wrapper::value);
        CHECK( helpers::is_sorted(vec.begin(), vec.end(), std::greater{}, &wrapper::value) );
    }
}

TEST_CASE( "parallel_sorter tests with string projection",
           "[parallel_sorter][projection][string]" )
{
    struct Person {
        std::string name;
        int age;
    };

    SECTION( "small dataset: sort by name" )
    {
        std::vector<Person> people = {
            {"Alice", 30},
            {"Bob", 25},
            {"Charlie", 35},
            {"David", 28}
        };

        cppsort::parallel_sort(people, &Person::name);
        CHECK( std::is_sorted(people.begin(), people.end(), 
            [](const Person& a, const Person& b) { return a.name < b.name; }) );
    }

    SECTION( "small dataset: sort by age" )
    {
        std::vector<Person> people = {
            {"Alice", 30},
            {"Bob", 25},
            {"Charlie", 35},
            {"David", 28}
        };

        cppsort::parallel_sort(people, &Person::age);
        CHECK( std::is_sorted(people.begin(), people.end(),
            [](const Person& a, const Person& b) { return a.age < b.age; }) );
    }

    SECTION( "small dataset: sort by age descending" )
    {
        std::vector<Person> people = {
            {"Alice", 30},
            {"Bob", 25},
            {"Charlie", 35},
            {"David", 28}
        };

        cppsort::parallel_sort(people, std::greater{}, &Person::age);
        CHECK( std::is_sorted(people.begin(), people.end(),
            [](const Person& a, const Person& b) { return a.age > b.age; }) );
    }
}
