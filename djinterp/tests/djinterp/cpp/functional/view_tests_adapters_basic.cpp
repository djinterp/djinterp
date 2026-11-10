/*******************************************************************************
* djinterp [test]                                  view_tests_adapters_basic.cpp
*
* Basic adapter view tests (section IV, part 1): the element-wise and prefix
* adapters.
*
*   test_transform  -- maps each element, including a type-changing map.
*   test_filter     -- keeps only elements satisfying a predicate.
*   test_take       -- prefix of n, with n == 0 and n > size edge cases.
*   test_drop       -- skips n, with n == 0 and n > size edge cases.
*   test_take_while -- prefix while predicate holds (all / none / partial).
*   test_drop_while -- drops the leading run satisfying the predicate.
*
*
* path:      /tests/djinterp/cpp/functional/view_tests_adapters_basic.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.11.09
*******************************************************************************/

// std
#include <vector>
// djinterp
#include "./view_tests.hpp"


NS_DJINTERP
NS_TESTING


bool test_transform()
{
    bool verdict = true;

    std::vector<int> data = { 1, 2, 3 };

    // value-preserving-type map
    record_assertion(verdict,
        (data | djinterp::transform(DFnDouble{}) | to_vector())
            == std::vector<int>({ 2, 4, 6 }),
        "transform: applies the function to each element");

    // type-changing map (int -> double)
    record_assertion(verdict,
        (data | djinterp::transform(DFnToDouble{}) | to_vector())
            == std::vector<double>({ 1.5, 2.5, 3.5 }),
        "transform: can change the element type");

    // over an empty input -> empty
    std::vector<int> none;
    record_assertion(verdict,
        (none | djinterp::transform(DFnDouble{}) | count()) == 0u,
        "transform: empty input yields empty output");

    return verdict;
}


bool test_filter()
{
    bool verdict = true;

    std::vector<int> data = { 1, 2, 3, 4, 5, 6 };

    // keeps matching elements
    record_assertion(verdict,
        (data | djinterp::filter(fn_is_even{}) | to_vector())
            == std::vector<int>({ 2, 4, 6 }),
        "filter: keeps only elements satisfying the predicate");

    // none match -> empty
    std::vector<int> odds = { 1, 3, 5 };
    record_assertion(verdict,
        (odds | djinterp::filter(fn_is_even{}) | count()) == 0u,
        "filter: yields nothing when no element matches");

    // all match -> all retained
    std::vector<int> evens = { 2, 4 };
    record_assertion(verdict,
        (evens | djinterp::filter(fn_is_even{}) | to_vector())
            == std::vector<int>({ 2, 4 }),
        "filter: retains everything when all match");

    return verdict;
}


bool test_take()
{
    bool verdict = true;

    std::vector<int> data = { 1, 2, 3, 4, 5 };

    // take fewer than size
    record_assertion(verdict,
        (data | djinterp::take(3) | to_vector())
            == std::vector<int>({ 1, 2, 3 }),
        "take: yields the first n elements");

    // take(0) -> empty
    record_assertion(verdict, (data | djinterp::take(0) | count()) == 0u,
        "take: take(0) yields nothing");

    // take more than size -> all
    record_assertion(verdict,
        (data | djinterp::take(99) | to_vector())
            == std::vector<int>({ 1, 2, 3, 4, 5 }),
        "take: n beyond size yields the whole input");

    return verdict;
}


bool test_drop()
{
    bool verdict = true;

    std::vector<int> data = { 1, 2, 3, 4, 5 };

    // drop some
    record_assertion(verdict,
        (data | djinterp::drop(2) | to_vector())
            == std::vector<int>({ 3, 4, 5 }),
        "drop: skips the first n elements");

    // drop(0) -> all
    record_assertion(verdict,
        (data | djinterp::drop(0) | to_vector())
            == std::vector<int>({ 1, 2, 3, 4, 5 }),
        "drop: drop(0) keeps everything");

    // drop more than size -> empty
    record_assertion(verdict, (data | djinterp::drop(99) | count()) == 0u,
        "drop: n beyond size yields nothing");

    return verdict;
}


bool test_take_while()
{
    bool verdict = true;

    std::vector<int> data = { 1, 2, 3, 7, 4 };

    // stops at the first element failing the predicate (7)
    record_assertion(verdict,
        (data | djinterp::take_while(DFnLessThan5{}) | to_vector())
            == std::vector<int>({ 1, 2, 3 }),
        "take_while: yields the leading run satisfying the predicate");

    // first element fails -> empty
    record_assertion(verdict,
        (data | djinterp::take_while(DFnAlwaysFalse{}) | count()) == 0u,
        "take_while: empty when the first element fails");

    // all satisfy -> all
    record_assertion(verdict,
        (data | djinterp::take_while(DFnAlwaysTrue{}) | to_vector())
            == std::vector<int>({ 1, 2, 3, 7, 4 }),
        "take_while: yields everything when all satisfy");

    return verdict;
}


bool test_drop_while()
{
    bool verdict = true;

    std::vector<int> data = { 1, 2, 3, 7, 4 };

    // drops the leading run < 5, keeps from the first failure (7) onward
    record_assertion(verdict,
        (data | djinterp::drop_while(DFnLessThan5{}) | to_vector())
            == std::vector<int>({ 7, 4 }),
        "drop_while: drops the leading satisfying run");

    // all satisfy -> empty
    record_assertion(verdict,
        (data | djinterp::drop_while(DFnAlwaysTrue{}) | count()) == 0u,
        "drop_while: empty when every element satisfies");

    // first fails -> nothing dropped
    record_assertion(verdict,
        (data | djinterp::drop_while(DFnAlwaysFalse{}) | to_vector())
            == std::vector<int>({ 1, 2, 3, 7, 4 }),
        "drop_while: keeps everything when the first element fails");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
