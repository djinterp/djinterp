/*******************************************************************************
* djinterp [test]                                     view_tests_fundamental.cpp
*
* Fundamental view tests (section II): the non-owning, owning, and
* iterator-pair wrappers.
*
*   test_ref_view           -- non-owning wrap over a container.
*   test_owning_view        -- by-value capture of a (moved-in) container.
*   test_iterator_pair_view -- a view defined by a raw [first, last) range.
*
*
* path:      /tests/djinterp/cpp/functional/view_tests_fundamental.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.11.09
*******************************************************************************/

// std
#include <utility>
#include <vector>
// djinterp
#include "./view_tests.hpp"


NS_DJINTERP
NS_TESTING


bool test_ref_view()
{
    bool verdict = true;

    std::vector<int>           data = { 1, 2, 3, 4 };
    DRefView<std::vector<int>> rv(data);

    // drains in order
    record_assertion(verdict,
        (rv | to_vector()) == std::vector<int>({ 1, 2, 3, 4 }),
        "ref_view: iterates the underlying container in order");

    // non-owning: reflects later mutation of the source
    data[0] = 9;
    record_assertion(verdict,
        (rv | to_vector()) == std::vector<int>({ 9, 2, 3, 4 }),
        "ref_view: observes mutations through the stored pointer");

    // empty container -> empty view
    std::vector<int>           none;
    DRefView<std::vector<int>> empty_rv(none);
    record_assertion(verdict, (empty_rv | count()) == 0u,
        "ref_view: empty container yields an empty view");

    return verdict;
}


bool test_owning_view()
{
    bool verdict = true;

    // takes ownership of a temporary; data survives the expression
    DOwningView<std::vector<int>> ov(std::vector<int>({ 5, 6, 7 }));
    record_assertion(verdict,
        (ov | to_vector()) == std::vector<int>({ 5, 6, 7 }),
        "owning_view: iterates the owned container in order");

    record_assertion(verdict, (ov | count()) == 3u,
        "owning_view: reports the owned element count");

    return verdict;
}


bool test_iterator_pair_view()
{
    bool verdict = true;

    std::vector<int> data = { 10, 20, 30, 40, 50 };

    // full range
    DIteratorPairView<std::vector<int>::const_iterator>
        full(data.begin(), data.end());
    record_assertion(verdict,
        (full | to_vector()) == std::vector<int>({ 10, 20, 30, 40, 50 }),
        "iterator_pair_view: spans the full [first, last) range");

    // sub-range [1, 4) -> 20, 30, 40
    DIteratorPairView<std::vector<int>::const_iterator>
        sub(data.begin() + 1, data.begin() + 4);
    record_assertion(verdict,
        (sub | to_vector()) == std::vector<int>({ 20, 30, 40 }),
        "iterator_pair_view: spans an arbitrary sub-range");

    // empty range [begin, begin)
    DIteratorPairView<std::vector<int>::const_iterator>
        empty_range(data.begin(), data.begin());
    record_assertion(verdict, (empty_range | count()) == 0u,
        "iterator_pair_view: an empty range yields no elements");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
