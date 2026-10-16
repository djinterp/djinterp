/*******************************************************************************
* djinterp [core]                                 selection_sort_tests_apply.cpp
*
*
* path:      /tests/djinterp/core/util/sort/selection_sort_tests_apply.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "selection_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_selection_sort_apply_degenerate_ranges
bool tests_selection_sort_apply_degenerate_ranges()
{
    verdict          result;
    std::vector<int> empty;
    std::vector<int> single;
    std::vector<int> ordered_pair;
    std::vector<int> reversed_pair;
    std::vector<int> populated;

    // an empty range takes the _first == _last early return
    selection_apply_ints(empty.begin(), empty.end());
    result.check(empty.empty());

    //   a single element still runs one outer pass, whose inner scan is
    // empty and whose swap is skipped
    single.push_back(42);
    selection_apply_ints(single.begin(), single.end());
    result.check(single.size() == 1);
    result.check(single[0] == 42);

    ordered_pair.push_back(1);
    ordered_pair.push_back(2);
    selection_apply_ints(ordered_pair.begin(), ordered_pair.end());
    result.check(ordered_pair[0] == 1);
    result.check(ordered_pair[1] == 2);

    reversed_pair.push_back(2);
    reversed_pair.push_back(1);
    selection_apply_ints(reversed_pair.begin(), reversed_pair.end());
    result.check(reversed_pair[0] == 1);
    result.check(reversed_pair[1] == 2);

    populated = shape_descending(8);
    selection_apply_ints(populated.begin(), populated.begin());
    result.check(ranges_equal(populated, shape_descending(8)));

    return result.ok();
}


// tests_selection_sort_apply_ordering_battery
bool tests_selection_sort_apply_ordering_battery()
{
    verdict result;

    verify_battery_reference(&selection_apply_ints, less<int>(), result);

    return result.ok();
}


// tests_selection_sort_apply_comparison_count
bool tests_selection_sort_apply_comparison_count()
{
    verdict          result;
    std::vector<int> data;
    std::size_t      comparisons;
    std::size_t      expected;
    std::size_t      size_index;
    std::size_t      shape_index;
    std::size_t      length;

    //   Selection sort is NOT adaptive: the inner scan always runs to the
    // end of the suffix, so the comparison count is exactly n(n-1)/2 for
    // EVERY input of length n.  Checking that across the whole corpus also
    // pins the outer loop's extra final pass -- which contributes an empty
    // scan and therefore no comparisons.
    for (size_index = 0; size_index < battery_size_count; ++size_index)
    {
        length   = battery_size_at(size_index);
        expected = 0;

        if (length > 1)
        {
            expected = ((length * (length - 1)) / 2);
        }

        for (shape_index = 0;
             shape_index < battery_shape_count;
             ++shape_index)
        {
            data        = make_shape(shape_index, length);
            comparisons = 0;

            ::djinterp::internal::selection_sort_apply(
                data.begin(),
                data.end(),
                make_counting(less<int>(), &comparisons));

            result.check(comparisons == expected);
        }
    }

    return result.ok();
}


// tests_selection_sort_apply_swap_bound
bool tests_selection_sort_apply_swap_bound()
{
    verdict                  result;
    std::vector<tracked_int> data;
    std::vector<int>         values;
    std::size_t              swaps;
    std::size_t              length;
    std::size_t              size_index;
    std::size_t              shape_index;
    std::size_t              i;

    //   The defining property of selection sort: at most (n - 1) exchanges,
    // whatever the input.  Checked over the whole corpus.
    for (size_index = 0; size_index < battery_size_count; ++size_index)
    {
        length = battery_size_at(size_index);

        if (length < 2)
        {
            continue;
        }

        for (shape_index = 0;
             shape_index < battery_shape_count;
             ++shape_index)
        {
            values = make_shape(shape_index, length);
            swaps  = 0;

            data.clear();
            data.reserve(length);

            for (i = 0; i < length; ++i)
            {
                data.push_back(make_tracked(values[i], &swaps));
            }

            ::djinterp::internal::selection_sort_apply(data.begin(),
                                                       data.end(),
                                                       tracked_int_less());

            result.check(swaps <= (length - 1));

            values = tracked_values(data);
            result.check(is_sorted_range(values.begin(),
                                         values.end(),
                                         less<int>()));
        }
    }

    //   A strictly descending run of length n is resolved by exactly
    // floor(n / 2) exchanges: each swap places two elements at once, and the
    // middle element of an odd-length run is already home.  A non-zero count
    // here is also what proves the ADL swap hook is being reached at all.
    length = 32;
    swaps  = 0;

    data.clear();
    data.reserve(length);

    for (i = 0; i < length; ++i)
    {
        data.push_back(make_tracked(static_cast<int>(length - i), &swaps));
    }

    ::djinterp::internal::selection_sort_apply(data.begin(),
                                               data.end(),
                                               tracked_int_less());

    result.check(swaps == (length / 2));
    result.check(swaps >= 1);

    return result.ok();
}


// tests_selection_sort_apply_self_swap_skipped
bool tests_selection_sort_apply_self_swap_skipped()
{
    verdict                  result;
    std::vector<tracked_int> data;
    std::size_t              swaps;
    std::size_t              length;
    std::size_t              i;

    length = 32;

    //   Already-ordered input: the minimum of every suffix is its first
    // element, so min_pos == current on every pass and the guarded swap is
    // skipped every time.  Any exchange at all means the guard is gone.
    swaps = 0;

    data.reserve(length);

    for (i = 0; i < length; ++i)
    {
        data.push_back(make_tracked(static_cast<int>(i), &swaps));
    }

    ::djinterp::internal::selection_sort_apply(data.begin(),
                                               data.end(),
                                               tracked_int_less());

    result.check(swaps == 0);

    //   All-equal input takes the same path for a different reason: no
    // element is ever STRICTLY less than the running minimum, so min_pos is
    // never moved off current.
    swaps = 0;
    data.clear();
    data.reserve(length);

    for (i = 0; i < length; ++i)
    {
        data.push_back(make_tracked(7, &swaps));
    }

    ::djinterp::internal::selection_sort_apply(data.begin(),
                                               data.end(),
                                               tracked_int_less());

    result.check(swaps == 0);

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
