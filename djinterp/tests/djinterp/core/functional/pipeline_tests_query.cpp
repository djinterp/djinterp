/******************************************************************************
* djinterp [test]                                     pipeline_tests_query.cpp
*
* Query / consumption tests (section iii):
*   test_query    -- any, all, none, count over populated and empty pipelines.
*   test_for_each -- visits every element for side effects and returns the
*                    same pipeline for chaining.
******************************************************************************/

#include <vector>
#include "./pipeline_tests.hpp"


NS_DJINTERP
NS_TESTING


using pi = function_pipeline<int>;


void test_query(test::test_handler& _h)
{
    pi p     = pi::from({ 2, 4, 6 });
    pi mixed = pi::from({ 1, 2, 3 });
    pi none_p = pi::from(std::vector<int>());

    // any
    test::record_assertion(_h, mixed.any(fn_is_even{}) == true,
        "any: true when at least one element matches");
    test::record_assertion(_h, none_p.any(fn_is_even{}) == false,
        "any: false for an empty pipeline");

    // all
    test::record_assertion(_h, p.all(fn_is_even{}) == true,
        "all: true when every element matches");
    test::record_assertion(_h, mixed.all(fn_is_even{}) == false,
        "all: false when some element fails");
    test::record_assertion(_h, none_p.all(fn_is_even{}) == true,
        "all: vacuously true for an empty pipeline");

    // none
    pi odds = pi::from({ 1, 3, 5 });
    test::record_assertion(_h, odds.none(fn_is_even{}) == true,
        "none: true when no element matches");
    test::record_assertion(_h, mixed.none(fn_is_even{}) == false,
        "none: false when some element matches");

    // count
    test::record_assertion(_h,
        pi::from({ 1, 2, 3, 4, 5, 6 }).count(fn_is_even{}) == 3u,
        "count: counts the elements satisfying the predicate");
    test::record_assertion(_h, none_p.count(fn_is_even{}) == 0u,
        "count: zero for an empty pipeline");

    return;
}


void test_for_each(test::test_handler& _h)
{
    pi p = pi::from({ 1, 2, 3, 4 });

    long sum = 0;
    const pi& chained = p.for_each(accumulate_into(&sum));

    test::record_assertion(_h, sum == 10,
        "for_each: visits every element for its side effect");
    test::record_assertion(_h, &chained == &p,
        "for_each: returns the same pipeline for chaining");

    // for_each over empty -> no side effects
    long untouched = 5;
    pi::from(std::vector<int>()).for_each(accumulate_into(&untouched));
    test::record_assertion(_h, untouched == 5,
        "for_each: an empty pipeline triggers no side effects");

    return;
}


NS_END  // testing
NS_END  // djinterp
