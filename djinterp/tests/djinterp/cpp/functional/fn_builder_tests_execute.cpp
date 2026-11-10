/*******************************************************************************
* djinterp [test]                                   fn_builder_tests_execute.cpp
*
* Execution-overload tests (section iii):
*   test_execute -- execute(vector), execute(generic container),
*                   execute(pointer, count), and operator() shorthand.
*
*
* path:      /tests/djinterp/cpp/functional/fn_builder_tests_execute.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.11.09
*******************************************************************************/

// std
#include <list>
#include <vector>
// djinterp
#include "./fn_builder_tests.hpp"


NS_DJINTERP
NS_TESTING


bool test_execute()
{
    bool verdict = true;

    auto doubler = DFnBuilder<int>::create().map(DFnDouble{});

    // execute(vector)
    std::vector<int> vin = { 1, 2, 3 };
    record_assertion(verdict,
        doubler.execute(vin) == std::vector<int>({ 2, 4, 6 }),
        "execute: drains a std::vector input");

    // execute(generic container) -- std::list is converted to the input vector
    std::list<int> lin = { 4, 5 };
    record_assertion(verdict,
        doubler.execute(lin) == std::vector<int>({ 8, 10 }),
        "execute: accepts a generic container and converts it");

    // execute(pointer, count)
    const int arr[] = { 6, 7 };
    record_assertion(verdict,
        doubler.execute(arr, 2) == std::vector<int>({ 12, 14 }),
        "execute: accepts a raw array and count");

    // operator() is a shorthand for execute
    record_assertion(verdict,
        doubler(vin) == std::vector<int>({ 2, 4, 6 }),
        "execute: operator() forwards to execute");

    // operator() over a generic container
    record_assertion(verdict,
        doubler(lin) == std::vector<int>({ 8, 10 }),
        "execute: operator() accepts a generic container");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
