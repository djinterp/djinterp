/*******************************************************************************
* djinterp [test]                                          view_tests_source.cpp
*
* Source view tests (section III): views that produce elements with no input
* view.
*
*   test_iota     -- bounded [start, end), empty when start >= end, and the
*                    infinite form bounded by take.
*   test_repeat   -- repeat_n (incl. zero count) and the infinite form bounded
*                    by take.
*   test_generate -- DGenerateView driven directly. NOTE: DGenerateView is the
*                    one view whose begin() is non-const (it mutates the stored
*                    function on advance), so unlike every other view it cannot
*                    flow through the const-by-reference terminal/adapter
*                    machinery; it is exercised here by direct iteration over a
*                    non-const instance.
*   test_empty    -- the zero-element source.
*   test_single   -- the one-element source.
*
*
* path:      /tests/djinterp/cpp/functional/view_tests_source.cpp
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


bool test_iota()
{
    bool verdict = true;

    // bounded [0, 5)
    record_assertion(verdict,
        (djinterp::iota(0, 5) | to_vector())
            == std::vector<int>({ 0, 1, 2, 3, 4 }),
        "iota: bounded range yields [start, end)");

    // empty when start == end
    record_assertion(verdict, (djinterp::iota(5, 5) | count()) == 0u,
        "iota: start == end yields an empty range");

    // infinite, bounded by take
    record_assertion(verdict,
        (djinterp::iota(0) | djinterp::take(4) | to_vector())
            == std::vector<int>({ 0, 1, 2, 3 }),
        "iota: infinite source bounded by take");

    return verdict;
}


bool test_repeat()
{
    bool verdict = true;

    // repeat_n with a positive count
    record_assertion(verdict,
        (djinterp::repeat_n(7, 3) | to_vector())
            == std::vector<int>({ 7, 7, 7 }),
        "repeat_n: yields the value exactly n times");

    // repeat_n with zero count -> empty
    record_assertion(verdict, (djinterp::repeat_n(7, 0) | count()) == 0u,
        "repeat_n: zero count yields nothing");

    // infinite repeat bounded by take
    record_assertion(verdict,
        (djinterp::repeat(9) | djinterp::take(3) | to_vector())
            == std::vector<int>({ 9, 9, 9 }),
        "repeat: infinite source bounded by take");

    return verdict;
}


bool test_generate()
{
    bool verdict = true;

    // direct iteration over a non-const DGenerateView (see header note).
    // counter yields 1, 2, 3, ...; the live iterator caches the first call,
    // and each advance produces the next value.
    auto g = djinterp::generate(counter{});

    record_assertion(verdict, is_view<decltype(g)>::value,
        "generate: produces a view");

    std::vector<int> got;
    auto             it = g.begin();
    for (int i = 0; i < 3; ++i)
    {
        got.push_back(*it);
        ++it;
    }
    record_assertion(verdict, got == std::vector<int>({ 1, 2, 3 }),
        "generate: successive advances invoke the function in order");

    return verdict;
}


bool test_empty()
{
    bool verdict = true;

    record_assertion(verdict, (djinterp::empty<int>() | count()) == 0u,
        "empty: reports zero elements");

    record_assertion(verdict,
        (djinterp::empty<int>() | to_vector()).empty(),
        "empty: drains to an empty vector");

    return verdict;
}


bool test_single()
{
    bool verdict = true;

    record_assertion(verdict,
        (djinterp::single(42) | to_vector()) == std::vector<int>({ 42 }),
        "single: yields exactly its one element");

    record_assertion(verdict, (djinterp::single(42) | count()) == 1u,
        "single: reports a count of one");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
