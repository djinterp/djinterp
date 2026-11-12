/*******************************************************************************
* djinterp [cpp]                                   merge_sort_tests_buffered.cpp
*
*
* path:      /tests/djinterp/cpp/util/sort/merge_sort_tests_buffered.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/
// std
#include <algorithm>
// djinterp
// djinterp [test]
#include "merge_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_merge_sort_buffered_degenerate_ranges
bool tests_merge_sort_buffered_degenerate_ranges()
{
    verdict          result;
    std::vector<int> data;
    std::vector<int> scratch;

    //   Ranges of 0 and 1 elements are already sorted; the scratch is not
    // touched.
    scratch.assign(4, -1);

    merge_sort_buffered(data.begin(),
                        data.end(),
                        std::less<int>(),
                        scratch.begin());

    result.check(data.empty());
    result.check(scratch == std::vector<int>(4, -1));

    data.push_back(42);

    merge_sort_buffered(data.begin(),
                        data.end(),
                        std::less<int>(),
                        scratch.begin());

    result.check((data.size() == 1) && (data[0] == 42));
    result.check(scratch == std::vector<int>(4, -1));

    //   Two elements: one pass, whose result lands in the scratch and is
    // brought home.
    data.clear();
    data.push_back(2);
    data.push_back(1);

    merge_sort_buffered(data.begin(),
                        data.end(),
                        std::less<int>(),
                        scratch.begin());

    result.check((data[0] == 1) && (data[1] == 2));

    return result.ok();
}


// tests_merge_sort_buffered_ordering_battery
bool tests_merge_sort_buffered_ordering_battery()
{
    verdict result;

    verify_battery_reference(&merge_buffered_ints, std::less<int>(), result);

    return result.ok();
}


// tests_merge_sort_buffered_parity_with_merge_sort
bool tests_merge_sort_buffered_parity_with_merge_sort()
{
    verdict          result;
    std::vector<int> owned;
    std::vector<int> lent;
    std::vector<int> scratch;
    std::size_t      owned_comparisons;
    std::size_t      lent_comparisons;
    std::size_t      size_index;
    std::size_t      shape_index;

    //   merge_sort and merge_sort_buffered are one algorithm with two answers
    // to "who owns the scratch".  Parity is asserted on the output AND on the
    // comparison count: equal results with different counts would mean the
    // two had drifted apart even while they still agree on this corpus.
    for (size_index = 0; size_index < battery_size_count; ++size_index)
    {
        for (shape_index = 0;
             shape_index < battery_shape_count;
             ++shape_index)
        {
            owned = make_shape(shape_index, battery_size_at(size_index));
            lent  = owned;
            scratch.assign(lent.size(), 0);

            owned_comparisons = 0;
            lent_comparisons  = 0;

            merge_sort(owned.begin(),
                       owned.end(),
                       make_counting(std::less<int>(), &owned_comparisons));

            merge_sort_buffered(
                lent.begin(),
                lent.end(),
                make_counting(std::less<int>(), &lent_comparisons),
                scratch.begin());

            result.check(ranges_equal(owned, lent));
            result.check(owned_comparisons == lent_comparisons);
        }
    }

    return result.ok();
}


// tests_merge_sort_buffered_stability
bool tests_merge_sort_buffered_stability()
{
    verdict result;

    verify_stability_battery(&merge_buffered_keyed, true, result);

    return result.ok();
}


// tests_merge_sort_buffered_reuses_scratch
bool tests_merge_sort_buffered_reuses_scratch()
{
    verdict          result;
    std::vector<int> scratch;
    std::vector<int> data;
    std::vector<int> expected;
    std::size_t      length;

    //   The reason the entry point exists: one scratch, sized for the
    // largest range, serves a loop of sorts of every smaller size, whatever
    // the last sort left in it.
    scratch.assign(257, 0);

    for (length = 0; length <= 257; length += 17)
    {
        data     = shape_random(length);
        expected = data;
        std::stable_sort(expected.begin(), expected.end());

        merge_sort_buffered(data.begin(),
                            data.end(),
                            std::less<int>(),
                            scratch.begin());

        result.check(data == expected);
    }

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
