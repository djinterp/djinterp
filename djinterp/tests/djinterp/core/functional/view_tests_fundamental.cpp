/******************************************************************************
* djinterp [test]                                   view_tests_fundamental.cpp
*
* Fundamental view tests (section II): the non-owning, owning, and
* iterator-pair wrappers.
*
*   test_ref_view           -- non-owning wrap over a container.
*   test_owning_view        -- by-value capture of a (moved-in) container.
*   test_iterator_pair_view -- a view defined by a raw [first, last) range.
******************************************************************************/

#include <vector>
#include <utility>
#include "./view_tests.hpp"


NS_DJINTERP
NS_TESTING


void test_ref_view(test::test_handler& _h)
{
    std::vector<int>           data = { 1, 2, 3, 4 };
    ref_view<std::vector<int>> rv(data);

    // drains in order
    test::record_assertion(_h,
        (rv | to_vector()) == std::vector<int>({ 1, 2, 3, 4 }),
        "ref_view: iterates the underlying container in order");

    // non-owning: reflects later mutation of the source
    data[0] = 9;
    test::record_assertion(_h,
        (rv | to_vector()) == std::vector<int>({ 9, 2, 3, 4 }),
        "ref_view: observes mutations through the stored pointer");

    // empty container -> empty view
    std::vector<int>           none;
    ref_view<std::vector<int>> empty_rv(none);
    test::record_assertion(_h, (empty_rv | count()) == 0u,
        "ref_view: empty container yields an empty view");

    return;
}


void test_owning_view(test::test_handler& _h)
{
    // takes ownership of a temporary; data survives the expression
    owning_view<std::vector<int>> ov(std::vector<int>({ 5, 6, 7 }));
    test::record_assertion(_h,
        (ov | to_vector()) == std::vector<int>({ 5, 6, 7 }),
        "owning_view: iterates the owned container in order");

    test::record_assertion(_h, (ov | count()) == 3u,
        "owning_view: reports the owned element count");

    return;
}


void test_iterator_pair_view(test::test_handler& _h)
{
    std::vector<int> data = { 10, 20, 30, 40, 50 };

    // full range
    iterator_pair_view<std::vector<int>::const_iterator>
        full(data.begin(), data.end());
    test::record_assertion(_h,
        (full | to_vector()) == std::vector<int>({ 10, 20, 30, 40, 50 }),
        "iterator_pair_view: spans the full [first, last) range");

    // sub-range [1, 4) -> 20, 30, 40
    iterator_pair_view<std::vector<int>::const_iterator>
        sub(data.begin() + 1, data.begin() + 4);
    test::record_assertion(_h,
        (sub | to_vector()) == std::vector<int>({ 20, 30, 40 }),
        "iterator_pair_view: spans an arbitrary sub-range");

    // empty range [begin, begin)
    iterator_pair_view<std::vector<int>::const_iterator>
        empty_range(data.begin(), data.begin());
    test::record_assertion(_h, (empty_range | count()) == 0u,
        "iterator_pair_view: an empty range yields no elements");

    return;
}


NS_END  // testing
NS_END  // djinterp
