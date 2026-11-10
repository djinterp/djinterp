/*******************************************************************************
* djinterp [test]                                       pipeline_tests_query.cpp
*
* Query / consumption tests (section iii):
*   test_query    -- any, all, none, count over populated and empty pipelines.
*   test_for_each -- visits every element for side effects and returns the
*                    same pipeline for chaining.
*
*
* path:      /tests/djinterp/cpp/functional/pipeline_tests_query.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.11.09
*******************************************************************************/

// std
#include <vector>
// djinterp
#include "./pipeline_tests.hpp"


NS_DJINTERP
NS_TESTING


using pi = DFunctionPipeline<int>;


bool test_query()
{
    bool verdict = true;

    pi p     = pi::from({ 2, 4, 6 });
    pi mixed = pi::from({ 1, 2, 3 });
    pi none_p = pi::from(std::vector<int>());

    // any
    record_assertion(verdict, mixed.any(fn_is_even{}) == true,
        "any: true when at least one element matches");
    record_assertion(verdict, none_p.any(fn_is_even{}) == false,
        "any: false for an empty pipeline");

    // all
    record_assertion(verdict, p.all(fn_is_even{}) == true,
        "all: true when every element matches");
    record_assertion(verdict, mixed.all(fn_is_even{}) == false,
        "all: false when some element fails");
    record_assertion(verdict, none_p.all(fn_is_even{}) == true,
        "all: vacuously true for an empty pipeline");

    // none
    pi odds = pi::from({ 1, 3, 5 });
    record_assertion(verdict, odds.none(fn_is_even{}) == true,
        "none: true when no element matches");
    record_assertion(verdict, mixed.none(fn_is_even{}) == false,
        "none: false when some element matches");

    // count
    record_assertion(verdict,
        pi::from({ 1, 2, 3, 4, 5, 6 }).count(fn_is_even{}) == 3u,
        "count: counts the elements satisfying the predicate");
    record_assertion(verdict, none_p.count(fn_is_even{}) == 0u,
        "count: zero for an empty pipeline");

    return verdict;
}


bool test_for_each()
{
    bool verdict = true;

    pi p = pi::from({ 1, 2, 3, 4 });

    long sum = 0;
    const pi& chained = p.for_each(accumulate_into(&sum));

    record_assertion(verdict, sum == 10,
        "for_each: visits every element for its side effect");
    record_assertion(verdict, &chained == &p,
        "for_each: returns the same pipeline for chaining");

    // for_each over empty -> no side effects
    long untouched = 5;
    pi::from(std::vector<int>()).for_each(accumulate_into(&untouched));
    record_assertion(verdict, untouched == 5,
        "for_each: an empty pipeline triggers no side effects");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
