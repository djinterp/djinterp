/******************************************************************************
* djinterp [test]                                        view_tests_source.cpp
*
* Source view tests (section III): views that produce elements with no input
* view.
*
*   test_iota     -- bounded [start, end), empty when start >= end, and the
*                    infinite form bounded by take.
*   test_repeat   -- repeat_n (incl. zero count) and the infinite form bounded
*                    by take.
*   test_generate -- generate_view driven directly. NOTE: generate_view is the
*                    one view whose begin() is non-const (it mutates the stored
*                    function on advance), so unlike every other view it cannot
*                    flow through the const-by-reference terminal/adapter
*                    machinery; it is exercised here by direct iteration over a
*                    non-const instance.
*   test_empty    -- the zero-element source.
*   test_single   -- the one-element source.
******************************************************************************/

#include <vector>
#include "./view_tests.hpp"


NS_DJINTERP
NS_TESTING


void test_iota(test::test_handler& _h)
{
    // bounded [0, 5)
    test::record_assertion(_h,
        (views::iota(0, 5) | to_vector())
            == std::vector<int>({ 0, 1, 2, 3, 4 }),
        "iota: bounded range yields [start, end)");

    // empty when start == end
    test::record_assertion(_h, (views::iota(5, 5) | count()) == 0u,
        "iota: start == end yields an empty range");

    // infinite, bounded by take
    test::record_assertion(_h,
        (views::iota(0) | views::take(4) | to_vector())
            == std::vector<int>({ 0, 1, 2, 3 }),
        "iota: infinite source bounded by take");

    return;
}


void test_repeat(test::test_handler& _h)
{
    // repeat_n with a positive count
    test::record_assertion(_h,
        (views::repeat_n(7, 3) | to_vector())
            == std::vector<int>({ 7, 7, 7 }),
        "repeat_n: yields the value exactly n times");

    // repeat_n with zero count -> empty
    test::record_assertion(_h, (views::repeat_n(7, 0) | count()) == 0u,
        "repeat_n: zero count yields nothing");

    // infinite repeat bounded by take
    test::record_assertion(_h,
        (views::repeat(9) | views::take(3) | to_vector())
            == std::vector<int>({ 9, 9, 9 }),
        "repeat: infinite source bounded by take");

    return;
}


void test_generate(test::test_handler& _h)
{
    // direct iteration over a non-const generate_view (see header note).
    // counter yields 1, 2, 3, ...; the live iterator caches the first call,
    // and each advance produces the next value.
    auto g = views::generate(counter{});

    test::record_assertion(_h, is_view<decltype(g)>::value,
        "generate: produces a view");

    std::vector<int> got;
    auto             it = g.begin();
    for (int i = 0; i < 3; ++i)
    {
        got.push_back(*it);
        ++it;
    }
    test::record_assertion(_h, got == std::vector<int>({ 1, 2, 3 }),
        "generate: successive advances invoke the function in order");

    return;
}


void test_empty(test::test_handler& _h)
{
    test::record_assertion(_h, (views::empty<int>() | count()) == 0u,
        "empty: reports zero elements");

    test::record_assertion(_h,
        (views::empty<int>() | to_vector()).empty(),
        "empty: drains to an empty vector");

    return;
}


void test_single(test::test_handler& _h)
{
    test::record_assertion(_h,
        (views::single(42) | to_vector()) == std::vector<int>({ 42 }),
        "single: yields exactly its one element");

    test::record_assertion(_h, (views::single(42) | count()) == 1u,
        "single: reports a count of one");

    return;
}


NS_END  // testing
NS_END  // djinterp
