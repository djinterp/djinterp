/*******************************************************************************
* djinterp [cpp]                             insertion_sort_tests_primitives.cpp
*
*
* path:      /tests/djinterp/cpp/util/sort/insertion_sort_tests_primitives.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/
// djinterp
// djinterp [test]
#include "insertion_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// insertion_place_ints
//   helper: insertion_place over vector<int>, ascending.
static
std::ptrdiff_t insertion_place_ints(
    std::vector<int>& _data,
    std::ptrdiff_t    _lower,
    std::ptrdiff_t    _index
)
{
    return ::djinterp::internal::insertion_place(_data.begin(),
                                                 _lower,
                                                 _index,
                                                 std::less<int>());
}


// tests_insertion_sort_place_landing_index
bool tests_insertion_sort_place_landing_index()
{
    verdict          result;
    std::vector<int> data;
    std::vector<int> original;
    std::size_t      comparisons;

    //   [1, 3, 5, 2]: the candidate at 3 travels back over 5 and 3 and lands
    // at 1, and the call reports where.
    data.push_back(1);
    data.push_back(3);
    data.push_back(5);
    data.push_back(2);

    result.check(insertion_place_ints(data, 0, 3) == 1);
    result.check((data[0] == 1) && (data[1] == 2) &&
                 (data[2] == 3) && (data[3] == 5));

    //   Already in order relative to its predecessor: one comparison, no
    // move, and the candidate reports its own index -- the O(n) best case.
    original    = data;
    comparisons = 0;

    result.check(::djinterp::internal::insertion_place(
                     data.begin(),
                     static_cast<std::ptrdiff_t>(0),
                     static_cast<std::ptrdiff_t>(3),
                     make_counting(std::less<int>(), &comparisons)) == 3);
    result.check(comparisons == 1);
    result.check(ranges_equal(data, original));

    //   The run's first element has nothing behind it to be placed among.
    result.check(insertion_place_ints(data, 2, 2) == 2);
    result.check(insertion_place_ints(data, 3, 1) == 1);
    result.check(ranges_equal(data, original));

    //   The lower bound is honoured: [9, 4, 6, 1] placed within [2, 4) lands
    // the 1 at 2, not at 0.
    data.clear();
    data.push_back(9);
    data.push_back(4);
    data.push_back(6);
    data.push_back(1);

    result.check(insertion_place_ints(data, 2, 3) == 2);
    result.check((data[0] == 9) && (data[1] == 4) &&
                 (data[2] == 1) && (data[3] == 6));

    return result.ok();
}


// tests_insertion_sort_place_stops_at_equivalent
bool tests_insertion_sort_place_stops_at_equivalent()
{
    verdict                  result;
    std::vector<DKeyedValue> keyed;

    //   [(1,0), (3,1), (5,2)] and a candidate (3,3): it passes the 5, which
    // it strictly precedes, and stops at the equivalent 3 rather than passing
    // it.  Equal elements keep their input order -- the stability that
    // lives in the scan.
    keyed.push_back(make_keyed(1, 0));
    keyed.push_back(make_keyed(3, 1));
    keyed.push_back(make_keyed(5, 2));
    keyed.push_back(make_keyed(3, 3));

    result.check(::djinterp::internal::insertion_place(
                     keyed.begin(),
                     static_cast<std::ptrdiff_t>(0),
                     static_cast<std::ptrdiff_t>(3),
                     keyed_value_less()) == 2);

    result.check(keyed[0] == make_keyed(1, 0));
    result.check(keyed[1] == make_keyed(3, 1));
    result.check(keyed[2] == make_keyed(3, 3));
    result.check(keyed[3] == make_keyed(5, 2));

    return result.ok();
}


// tests_insertion_sort_pass_sub_run
bool tests_insertion_sort_pass_sub_run()
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
    ::djinterp::internal::insertion_pass(data.begin(),
                                         static_cast<std::ptrdiff_t>(4),
                                         static_cast<std::ptrdiff_t>(5),
                                         std::less<int>());

    result.check(ranges_equal(data, original));

    //   The pass sorts [1, 4) and touches nothing outside it -- the property
    // quick_sort_apply relies on when it hands insertion sort a sub-range,
    // and a block-partitioned driver when it hands one to each worker.
    ::djinterp::internal::insertion_pass(data.begin(),
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
