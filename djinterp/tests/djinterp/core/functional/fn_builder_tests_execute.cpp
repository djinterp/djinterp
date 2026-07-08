/******************************************************************************
* djinterp [test]                                  fn_builder_tests_execute.cpp
*
* Execution-overload tests (section iii):
*   test_execute -- execute(vector), execute(generic container),
*                   execute(pointer, count), and operator() shorthand.
******************************************************************************/

#include <vector>
#include <list>
#include "./fn_builder_tests.hpp"


NS_DJINTERP
NS_TESTING


void test_execute(test::test_handler& _h)
{
    auto doubler = fn_builder<int>::create().map(fn_double{});

    // execute(vector)
    std::vector<int> vin = { 1, 2, 3 };
    test::record_assertion(_h,
        doubler.execute(vin) == std::vector<int>({ 2, 4, 6 }),
        "execute: drains a std::vector input");

    // execute(generic container) -- std::list is converted to the input vector
    std::list<int> lin = { 4, 5 };
    test::record_assertion(_h,
        doubler.execute(lin) == std::vector<int>({ 8, 10 }),
        "execute: accepts a generic container and converts it");

    // execute(pointer, count)
    const int arr[] = { 6, 7 };
    test::record_assertion(_h,
        doubler.execute(arr, 2) == std::vector<int>({ 12, 14 }),
        "execute: accepts a raw array and count");

    // operator() is a shorthand for execute
    test::record_assertion(_h,
        doubler(vin) == std::vector<int>({ 2, 4, 6 }),
        "execute: operator() forwards to execute");

    // operator() over a generic container
    test::record_assertion(_h,
        doubler(lin) == std::vector<int>({ 8, 10 }),
        "execute: operator() accepts a generic container");

    return;
}


NS_END  // testing
NS_END  // djinterp
