/*******************************************************************************
* djinterp [core]                                    bubble_sort_tests_apply.cpp
*
*
* path:      /tests/djinterp/core/util/sort/bubble_sort_tests_apply.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "bubble_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_bubble_sort_apply_degenerate_ranges
bool tests_bubble_sort_apply_degenerate_ranges()
{
    verdict          result;
    std::vector<int> empty;
    std::vector<int> single;
    std::vector<int> ordered_pair;
    std::vector<int> reversed_pair;
    std::vector<int> populated;

    // an empty range takes the _first == _last early return
    bubble_apply_ints(empty.begin(), empty.end());
    result.check(empty.empty());

    // a single element enters the outer loop, finds an empty inner range,
    // sees no swap and exits on the first pass
    single.push_back(42);
    bubble_apply_ints(single.begin(), single.end());
    result.check(single.size() == 1);
    result.check(single[0] == 42);

    ordered_pair.push_back(1);
    ordered_pair.push_back(2);
    bubble_apply_ints(ordered_pair.begin(), ordered_pair.end());
    result.check(ordered_pair[0] == 1);
    result.check(ordered_pair[1] == 2);

    reversed_pair.push_back(2);
    reversed_pair.push_back(1);
    bubble_apply_ints(reversed_pair.begin(), reversed_pair.end());
    result.check(reversed_pair[0] == 1);
    result.check(reversed_pair[1] == 2);

    // an empty sub-range of a populated container must leave it untouched
    populated = shape_descending(8);
    bubble_apply_ints(populated.begin(), populated.begin());
    result.check(ranges_equal(populated, shape_descending(8)));

    return result.ok();
}


// tests_bubble_sort_apply_ordering_battery
bool tests_bubble_sort_apply_ordering_battery()
{
    verdict result;

    // every length and shape, checked against a std::stable_sort reference
    verify_battery_reference(&bubble_apply_ints, less<int>(), result);

    return result.ok();
}


// tests_bubble_sort_apply_early_exit
bool tests_bubble_sort_apply_early_exit()
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

        //   already ascending: the first pass makes n-1 comparisons, swaps
        // nothing, and the !swapped break fires.  Any larger count means the
        // adaptive early exit did not happen.
        data        = shape_ascending(length);
        comparisons = 0;

        ::djinterp::internal::bubble_sort_apply(
            data.begin(),
            data.end(),
            make_counting(less<int>(), &comparisons));

        result.check(comparisons == (length - 1));

        //   all-equal input is the same story through a different route: no
        // pair is ever strictly out of order.
        data        = shape_constant(length);
        comparisons = 0;

        ::djinterp::internal::bubble_sort_apply(
            data.begin(),
            data.end(),
            make_counting(less<int>(), &comparisons));

        result.check(comparisons == (length - 1));
    }

    return result.ok();
}


// tests_bubble_sort_apply_shrinking_boundary
bool tests_bubble_sort_apply_shrinking_boundary()
{
    verdict          result;
    std::vector<int> leading_inversion;
    std::vector<int> trailing_inversions;
    std::size_t      comparisons;
    std::size_t      i;

    //   [2, 1, 3, 4, 5, 6]: the only swap happens at the very front, so the
    // last-swap position collapses the unsorted boundary to index 1 and the
    // second pass compares nothing at all.  Five comparisons total; a bubble
    // sort without the boundary optimisation would make nine.
    leading_inversion.push_back(2);
    leading_inversion.push_back(1);

    for (i = 3; i <= 6; ++i)
    {
        leading_inversion.push_back(static_cast<int>(i));
    }

    comparisons = 0;

    ::djinterp::internal::bubble_sort_apply(
        leading_inversion.begin(),
        leading_inversion.end(),
        make_counting(less<int>(), &comparisons));

    result.check(comparisons == 5);
    result.check(is_sorted_range(leading_inversion.begin(),
                                 leading_inversion.end(),
                                 less<int>()));

    //   [1, 2, 3, 6, 5, 4]: the inversions sit at the back, so the boundary
    // retreats one slot per pass -- 5 + 4 + 3 comparisons over three passes.
    trailing_inversions.push_back(1);
    trailing_inversions.push_back(2);
    trailing_inversions.push_back(3);
    trailing_inversions.push_back(6);
    trailing_inversions.push_back(5);
    trailing_inversions.push_back(4);

    comparisons = 0;

    ::djinterp::internal::bubble_sort_apply(
        trailing_inversions.begin(),
        trailing_inversions.end(),
        make_counting(less<int>(), &comparisons));

    result.check(comparisons == 12);
    result.check(is_sorted_range(trailing_inversions.begin(),
                                 trailing_inversions.end(),
                                 less<int>()));

    return result.ok();
}


// tests_bubble_sort_apply_swap_traffic
bool tests_bubble_sort_apply_swap_traffic()
{
    verdict                  result;
    std::vector<tracked_int> data;
    std::vector<int>         values;
    std::size_t              swaps;
    std::size_t              expected;
    std::size_t              length;
    std::size_t              i;

    length = 32;

    //   strictly descending input is the worst case: every one of the
    // n(n-1)/2 adjacent pairs is an inversion and each is swapped exactly
    // once.
    swaps = 0;
    data.clear();
    data.reserve(length);

    for (i = 0; i < length; ++i)
    {
        data.push_back(make_tracked(static_cast<int>(length - i), &swaps));
    }

    ::djinterp::internal::bubble_sort_apply(data.begin(),
                                            data.end(),
                                            tracked_int_less());

    expected = ((length * (length - 1)) / 2);

    result.check(swaps == expected);

    values = tracked_values(data);
    result.check(is_sorted_range(values.begin(),
                                 values.end(),
                                 less<int>()));

    //   already-ordered input must not swap at all: the early exit fires
    // before any exchange.
    swaps = 0;
    data.clear();
    data.reserve(length);

    for (i = 0; i < length; ++i)
    {
        data.push_back(make_tracked(static_cast<int>(i), &swaps));
    }

    ::djinterp::internal::bubble_sort_apply(data.begin(),
                                            data.end(),
                                            tracked_int_less());

    result.check(swaps == 0);

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
