/*******************************************************************************
* djinterp [cpp]                             selection_sort_tests_primitives.cpp
*
*
* path:      /tests/djinterp/cpp/util/sort/selection_sort_tests_primitives.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/
// djinterp
// djinterp [test]
#include "selection_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_selection_sort_min_index_earliest_wins
bool tests_selection_sort_min_index_earliest_wins()
{
    verdict                  result;
    std::vector<int>         data;
    std::vector<DKeyedValue> keyed;
    std::ptrdiff_t           zero;
    std::ptrdiff_t           five;

    zero = 0;
    five = 5;

    //   [3, 1, 4, 1, 5]: two minima.  A candidate must STRICTLY precede the
    // incumbent to displace it, so the earlier 1 wins -- one of the two
    // choices that pin the arrangement this unstable family produces.
    data.push_back(3);
    data.push_back(1);
    data.push_back(4);
    data.push_back(1);
    data.push_back(5);

    result.check(::djinterp::internal::selection_min_index(
        data.begin(), zero, five, std::less<int>()) == 1);

    //   The run is honoured: from index 2 the later 1 is the minimum.
    result.check(::djinterp::internal::selection_min_index(
        data.begin(), static_cast<std::ptrdiff_t>(2), five,
        std::less<int>()) == 3);

    //   "Minimum" is under the comparator, so a reversed one finds the
    // largest without a second code path.
    result.check(::djinterp::internal::selection_min_index(
        data.begin(), zero, five, std::greater<int>()) == 4);

    //   An empty run reports its own start.
    result.check(::djinterp::internal::selection_min_index(
        data.begin(), static_cast<std::ptrdiff_t>(2),
        static_cast<std::ptrdiff_t>(2), std::less<int>()) == 2);

    //   All equivalent: the first one wins.
    keyed.push_back(make_keyed(7, 0));
    keyed.push_back(make_keyed(7, 1));
    keyed.push_back(make_keyed(7, 2));

    result.check(::djinterp::internal::selection_min_index(
        keyed.begin(), zero, static_cast<std::ptrdiff_t>(3),
        keyed_value_less()) == 0);

    return result.ok();
}


// tests_selection_sort_pass_sub_run
bool tests_selection_sort_pass_sub_run()
{
    verdict          result;
    std::vector<int> data;
    std::vector<int> original;

    data.push_back(9);
    data.push_back(3);
    data.push_back(2);
    data.push_back(1);
    data.push_back(0);
    original = data;

    //   A run of fewer than two elements is already sorted.
    ::djinterp::internal::selection_pass(data.begin(),
                                         static_cast<std::ptrdiff_t>(2),
                                         static_cast<std::ptrdiff_t>(3),
                                         std::less<int>());

    result.check(ranges_equal(data, original));

    //   The pass sorts [1, 4) and touches nothing outside it -- the property
    // a driver handing out sub-runs relies on.
    ::djinterp::internal::selection_pass(data.begin(),
                                         static_cast<std::ptrdiff_t>(1),
                                         static_cast<std::ptrdiff_t>(4),
                                         std::less<int>());

    result.check(data[0] == 9);
    result.check((data[1] == 1) && (data[2] == 2) && (data[3] == 3));
    result.check(data[4] == 0);

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
