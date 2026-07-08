/******************************************************************************
* djinterp [test]                                view_tests_adapters_basic.cpp
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
******************************************************************************/

#include <vector>
#include "./view_tests.hpp"


NS_DJINTERP
NS_TESTING


void test_transform(test::test_handler& _h)
{
    std::vector<int> data = { 1, 2, 3 };

    // value-preserving-type map
    test::record_assertion(_h,
        (data | views::transform(fn_double{}) | to_vector())
            == std::vector<int>({ 2, 4, 6 }),
        "transform: applies the function to each element");

    // type-changing map (int -> double)
    test::record_assertion(_h,
        (data | views::transform(fn_to_double{}) | to_vector())
            == std::vector<double>({ 1.5, 2.5, 3.5 }),
        "transform: can change the element type");

    // over an empty input -> empty
    std::vector<int> none;
    test::record_assertion(_h,
        (none | views::transform(fn_double{}) | count()) == 0u,
        "transform: empty input yields empty output");

    return;
}


void test_filter(test::test_handler& _h)
{
    std::vector<int> data = { 1, 2, 3, 4, 5, 6 };

    // keeps matching elements
    test::record_assertion(_h,
        (data | views::filter(fn_is_even{}) | to_vector())
            == std::vector<int>({ 2, 4, 6 }),
        "filter: keeps only elements satisfying the predicate");

    // none match -> empty
    std::vector<int> odds = { 1, 3, 5 };
    test::record_assertion(_h,
        (odds | views::filter(fn_is_even{}) | count()) == 0u,
        "filter: yields nothing when no element matches");

    // all match -> all retained
    std::vector<int> evens = { 2, 4 };
    test::record_assertion(_h,
        (evens | views::filter(fn_is_even{}) | to_vector())
            == std::vector<int>({ 2, 4 }),
        "filter: retains everything when all match");

    return;
}


void test_take(test::test_handler& _h)
{
    std::vector<int> data = { 1, 2, 3, 4, 5 };

    // take fewer than size
    test::record_assertion(_h,
        (data | views::take(3) | to_vector())
            == std::vector<int>({ 1, 2, 3 }),
        "take: yields the first n elements");

    // take(0) -> empty
    test::record_assertion(_h, (data | views::take(0) | count()) == 0u,
        "take: take(0) yields nothing");

    // take more than size -> all
    test::record_assertion(_h,
        (data | views::take(99) | to_vector())
            == std::vector<int>({ 1, 2, 3, 4, 5 }),
        "take: n beyond size yields the whole input");

    return;
}


void test_drop(test::test_handler& _h)
{
    std::vector<int> data = { 1, 2, 3, 4, 5 };

    // drop some
    test::record_assertion(_h,
        (data | views::drop(2) | to_vector())
            == std::vector<int>({ 3, 4, 5 }),
        "drop: skips the first n elements");

    // drop(0) -> all
    test::record_assertion(_h,
        (data | views::drop(0) | to_vector())
            == std::vector<int>({ 1, 2, 3, 4, 5 }),
        "drop: drop(0) keeps everything");

    // drop more than size -> empty
    test::record_assertion(_h, (data | views::drop(99) | count()) == 0u,
        "drop: n beyond size yields nothing");

    return;
}


void test_take_while(test::test_handler& _h)
{
    std::vector<int> data = { 1, 2, 3, 7, 4 };

    // stops at the first element failing the predicate (7)
    test::record_assertion(_h,
        (data | views::take_while(fn_less_than_5{}) | to_vector())
            == std::vector<int>({ 1, 2, 3 }),
        "take_while: yields the leading run satisfying the predicate");

    // first element fails -> empty
    test::record_assertion(_h,
        (data | views::take_while(fn_always_false{}) | count()) == 0u,
        "take_while: empty when the first element fails");

    // all satisfy -> all
    test::record_assertion(_h,
        (data | views::take_while(fn_always_true{}) | to_vector())
            == std::vector<int>({ 1, 2, 3, 7, 4 }),
        "take_while: yields everything when all satisfy");

    return;
}


void test_drop_while(test::test_handler& _h)
{
    std::vector<int> data = { 1, 2, 3, 7, 4 };

    // drops the leading run < 5, keeps from the first failure (7) onward
    test::record_assertion(_h,
        (data | views::drop_while(fn_less_than_5{}) | to_vector())
            == std::vector<int>({ 7, 4 }),
        "drop_while: drops the leading satisfying run");

    // all satisfy -> empty
    test::record_assertion(_h,
        (data | views::drop_while(fn_always_true{}) | count()) == 0u,
        "drop_while: empty when every element satisfies");

    // first fails -> nothing dropped
    test::record_assertion(_h,
        (data | views::drop_while(fn_always_false{}) | to_vector())
            == std::vector<int>({ 1, 2, 3, 7, 4 }),
        "drop_while: keeps everything when the first element fails");

    return;
}


NS_END  // testing
NS_END  // djinterp
