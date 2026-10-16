/*******************************************************************************
* djinterp [core]                                 insertion_sort_tests_apply.cpp
*
*
* path:      /tests/djinterp/core/util/sort/insertion_sort_tests_apply.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "insertion_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_insertion_sort_apply_degenerate_ranges
bool tests_insertion_sort_apply_degenerate_ranges()
{
    verdict          result;
    std::vector<int> empty;
    std::vector<int> single;
    std::vector<int> ordered_pair;
    std::vector<int> reversed_pair;
    std::vector<int> populated;

    // an empty range takes the _first == _last early return
    insertion_apply_ints(empty.begin(), empty.end());
    result.check(empty.empty());

    //   a single element sets current to _last immediately, so the outer
    // loop body never runs
    single.push_back(42);
    insertion_apply_ints(single.begin(), single.end());
    result.check(single.size() == 1);
    result.check(single[0] == 42);

    ordered_pair.push_back(1);
    ordered_pair.push_back(2);
    insertion_apply_ints(ordered_pair.begin(), ordered_pair.end());
    result.check(ordered_pair[0] == 1);
    result.check(ordered_pair[1] == 2);

    reversed_pair.push_back(2);
    reversed_pair.push_back(1);
    insertion_apply_ints(reversed_pair.begin(), reversed_pair.end());
    result.check(reversed_pair[0] == 1);
    result.check(reversed_pair[1] == 2);

    populated = shape_descending(8);
    insertion_apply_ints(populated.begin(), populated.begin());
    result.check(ranges_equal(populated, shape_descending(8)));

    return result.ok();
}


// tests_insertion_sort_apply_ordering_battery
bool tests_insertion_sort_apply_ordering_battery()
{
    verdict result;

    verify_battery_reference(&insertion_apply_ints, less<int>(), result);

    return result.ok();
}


// tests_insertion_sort_apply_fast_path
bool tests_insertion_sort_apply_fast_path()
{
    verdict          result;
    std::vector<int> data;
    std::size_t      comparisons;
    std::size_t      size_index;
    std::size_t      length;

    for (size_index = 0; size_index < battery_size_count; ++size_index)
    {
        length = battery_size_at(size_index);

        if (length < 2)
        {
            continue;
        }

        //   already ascending: every outer step takes the `continue` on its
        // single fast-path comparison and the shift loop never runs, so the
        // total is exactly n-1.  This is the O(n) best case the header
        // claims.
        data        = shape_ascending(length);
        comparisons = 0;

        ::djinterp::internal::insertion_sort_apply(
            data.begin(),
            data.end(),
            make_counting(less<int>(), &comparisons));

        result.check(comparisons == (length - 1));

        //   all-equal input reaches the same fast path: no element strictly
        // precedes its predecessor.
        data        = shape_constant(length);
        comparisons = 0;

        ::djinterp::internal::insertion_sort_apply(
            data.begin(),
            data.end(),
            make_counting(less<int>(), &comparisons));

        result.check(comparisons == (length - 1));
    }

    return result.ok();
}


// tests_insertion_sort_apply_full_shift
bool tests_insertion_sort_apply_full_shift()
{
    verdict                  result;
    std::vector<int>         data;
    std::vector<tracked_int> tracked;
    std::vector<int>         values;
    std::size_t              comparisons;
    std::size_t              swaps;
    std::size_t              length;
    std::size_t              i;

    length = 32;

    //   The minimum parked at the very end has to travel the entire range,
    // so the shift loop runs to hole == _first -- the only input that exits
    // that loop by exhausting the prefix rather than by the break.
    //
    //   Cost: (n-2) fast-path comparisons for the ordered prefix, then 1
    // fast-path plus (n-1) shift comparisons for the traveller, i.e. 2n-2.
    data        = shape_single_outlier(length);
    comparisons = 0;

    ::djinterp::internal::insertion_sort_apply(
        data.begin(),
        data.end(),
        make_counting(less<int>(), &comparisons));

    result.check(comparisons == ((2 * length) - 2));
    result.check(data[0] == -1);
    result.check(is_sorted_range(data.begin(), data.end(), less<int>()));

    //   Insertion sort is shift-based, not exchange-based: it assigns over
    // the hole and never calls swap.  A tracked element type therefore sees
    // no swap traffic at all, even on the maximal-shift input.
    swaps  = 0;
    values = shape_single_outlier(length);

    tracked.reserve(length);

    for (i = 0; i < length; ++i)
    {
        tracked.push_back(make_tracked(values[i], &swaps));
    }

    ::djinterp::internal::insertion_sort_apply(tracked.begin(),
                                               tracked.end(),
                                               tracked_int_less());

    result.check(swaps == 0);

    values = tracked_values(tracked);
    result.check(is_sorted_range(values.begin(),
                                 values.end(),
                                 less<int>()));

    return result.ok();
}


// tests_insertion_sort_apply_comparison_counts
bool tests_insertion_sort_apply_comparison_counts()
{
    verdict          result;
    std::vector<int> data;
    std::size_t      comparisons;
    std::size_t      expected;
    std::size_t      size_index;
    std::size_t      length;

    for (size_index = 0; size_index < battery_size_count; ++size_index)
    {
        length = battery_size_at(size_index);

        if (length < 2)
        {
            continue;
        }

        //   Strictly descending is the worst case.  Element k costs one
        // fast-path comparison plus k shift comparisons, so the total is
        //     sum(k = 1 .. n-1) of (k + 1)  =  (n-1)(n+2)/2
        // -- quadratic, and exactly so.
        data        = shape_descending(length);
        comparisons = 0;
        expected    = (((length - 1) * (length + 2)) / 2);

        ::djinterp::internal::insertion_sort_apply(
            data.begin(),
            data.end(),
            make_counting(less<int>(), &comparisons));

        result.check(comparisons == expected);
        result.check(is_sorted_range(data.begin(),
                                     data.end(),
                                     less<int>()));

        //   Every other shape must fall between the two extremes: never
        // cheaper than the sorted best case, never dearer than the reversed
        // worst case.
        data        = shape_random(length);
        comparisons = 0;

        ::djinterp::internal::insertion_sort_apply(
            data.begin(),
            data.end(),
            make_counting(less<int>(), &comparisons));

        result.check(comparisons >= (length - 1));
        result.check(comparisons <= expected);
    }

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
