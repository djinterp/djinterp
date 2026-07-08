/******************************************************************************
* djinterp [test]                                 fn_builder_tests_terminal.cpp
*
* Terminal-operation tests (section iii):
*   test_fold      -- materialize the chain, then left-fold from an init.
*   test_count_any -- count() (size after the chain) and any() (non-empty
*                     after the chain).
******************************************************************************/

#include <vector>
#include "./fn_builder_tests.hpp"


NS_DJINTERP
NS_TESTING


void test_fold(test::test_handler& _h)
{
    std::vector<int> in = { 1, 2, 3 };

    // fold over a mapping chain: double then sum from 0 -> 2+4+6
    int sum = fn_builder<int>::create()
                  .map(fn_double{})
                  .fold(in, 0, fn_add{});
    test::record_assertion(_h, sum == 12,
        "fold: materializes the chain then accumulates from the init");

    // the init participates
    int seeded = fn_builder<int>::create().fold(in, 100, fn_add{});
    test::record_assertion(_h, seeded == 106,
        "fold: the initial value participates");

    // fold over an empty result returns the init unchanged
    int empty_fold = fn_builder<int>::create()
                         .filter(fn_is_even{})
                         .fold(std::vector<int>({ 1, 3, 5 }), 7, fn_add{});
    test::record_assertion(_h, empty_fold == 7,
        "fold: an empty chain result returns the initial value");

    return;
}


void test_count_any(test::test_handler& _h)
{
    std::vector<int> in = { 1, 2, 3, 4, 5, 6 };

    // count is the size after the chain
    test::record_assertion(_h,
        fn_builder<int>::create().filter(fn_is_even{}).count(in) == 3u,
        "count: reports the element count after the chain");

    // count over an empty result
    test::record_assertion(_h,
        fn_builder<int>::create()
            .filter(fn_is_even{})
            .count(std::vector<int>({ 1, 3 })) == 0u,
        "count: zero when the chain produces nothing");

    // any is true iff the chain yields at least one element
    test::record_assertion(_h,
        fn_builder<int>::create().filter(fn_is_even{}).any(in) == true,
        "any: true when the chain yields at least one element");
    test::record_assertion(_h,
        fn_builder<int>::create()
            .filter(fn_is_even{})
            .any(std::vector<int>({ 1, 3, 5 })) == false,
        "any: false when the chain yields nothing");

    return;
}


NS_END  // testing
NS_END  // djinterp
