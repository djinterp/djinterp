/*******************************************************************************
* djinterp [core]                                 quick_sort_tests_partition.cpp
*
*
* path:      /tests/djinterp/core/util/sort/quick_sort_tests_partition.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "quick_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_quick_sort_partition_postcondition
bool tests_quick_sort_partition_postcondition()
{
    verdict                    result;
    std::vector<int>           data;
    std::vector<int>           original;
    std::vector<int>::iterator pivot;
    std::size_t                index;
    std::size_t                shape_index;
    std::size_t                size_index;
    std::size_t                length;
    std::size_t                i;
    int                        pivot_value;

    //   The partitioner's contract, asserted over the whole corpus: the
    // returned iterator is inside the range, the element it names is the
    // pivot that was sitting at _first, nothing to its left follows the
    // pivot, nothing to its right precedes it, and the multiset is
    // unchanged.
    //
    //   Ranges of length zero are excluded deliberately: the routine forms
    // _last - 1 before it tests anything, so an empty range would build an
    // iterator before the start of the sequence.  That is a genuine
    // precondition of the function, not an oversight of this test -- see
    // tests_quick_sort_partition_minimal_ranges.
    for (size_index = 0; size_index < battery_size_count; ++size_index)
    {
        length = battery_size_at(size_index);

        if (length < 1)
        {
            continue;
        }

        for (shape_index = 0;
             shape_index < battery_shape_count;
             ++shape_index)
        {
            data        = make_shape(shape_index, length);
            original    = data;
            pivot_value = data[0];

            pivot = quick_partition_ints(data.begin(), data.end());

            index = static_cast<std::size_t>(pivot - data.begin());

            result.check(index < length);
            result.check(*pivot == pivot_value);

            for (i = 0; i < index; ++i)
            {
                // left side: a[i] <= pivot
                result.check(!(pivot_value < data[i]));
            }

            for (i = (index + 1); i < length; ++i)
            {
                // right side: a[i] >= pivot
                result.check(!(data[i] < pivot_value));
            }

            result.check(is_permutation_of(data, original));
        }
    }

    return result.ok();
}


// tests_quick_sort_partition_all_equal
bool tests_quick_sort_partition_all_equal()
{
    verdict                    result;
    std::vector<int>           data;
    std::vector<int>::iterator pivot;
    std::size_t                index;
    std::size_t                size_index;
    std::size_t                length;

    //   All-equal input is the classic quicksort trap: a partitioner that
    // sends every equivalent element to one side degrades to O(n^2).  This
    // one does not -- neither inner scan advances at all (no element is
    // STRICTLY on the wrong side), so the two cursors close on each other
    // one step at a time from both ends and meet in the middle.
    //
    //   The meeting point is exactly n - 1 - (n / 2), i.e. dead centre, so
    // the recursion stays balanced.
    for (size_index = 0; size_index < battery_size_count; ++size_index)
    {
        length = battery_size_at(size_index);

        if (length < 1)
        {
            continue;
        }

        data  = shape_constant(length);
        pivot = quick_partition_ints(data.begin(), data.end());
        index = static_cast<std::size_t>(pivot - data.begin());

        result.check(index == ((length - 1) - (length / 2)));
        result.check(ranges_equal(data, shape_constant(length)));
    }

    return result.ok();
}


// tests_quick_sort_partition_extreme_pivots
bool tests_quick_sort_partition_extreme_pivots()
{
    verdict                    result;
    std::vector<int>           data;
    std::vector<int>::iterator pivot;
    std::size_t                index;
    std::size_t                i;

    //   Pivot is the strict MINIMUM: the left-hand scan stops immediately
    // and the right-hand scan retreats the whole way down, so the pivot
    // stays at _first and the left partition comes back empty.  This is the
    // path that walks `hi` from _last - 1 to _first in one go.
    data.clear();
    data.push_back(0);
    data.push_back(5);
    data.push_back(3);
    data.push_back(4);
    data.push_back(1);
    data.push_back(2);

    pivot = quick_partition_ints(data.begin(), data.end());
    index = static_cast<std::size_t>(pivot - data.begin());

    result.check(index == 0);
    result.check(data[0] == 0);

    //   Pivot is the strict MAXIMUM: now it is the left-hand scan that runs
    // the length of the range -- and it stops on the `lo <= hi` guard, not
    // on the comparator, which is the check that keeps it from stepping off
    // the end.
    data.clear();
    data.push_back(9);
    data.push_back(5);
    data.push_back(3);
    data.push_back(4);
    data.push_back(1);
    data.push_back(2);

    pivot = quick_partition_ints(data.begin(), data.end());
    index = static_cast<std::size_t>(pivot - data.begin());

    result.check(index == 5);
    result.check(data[5] == 9);

    for (i = 0; i < 5; ++i)
    {
        result.check(data[i] < 9);
    }

    //   A pivot equivalent to the minimum but with duplicates on both
    // sides: the scans stop on equivalence rather than on strict order, so
    // the swap-and-step branch runs.
    data.clear();
    data.push_back(2);
    data.push_back(2);
    data.push_back(1);
    data.push_back(3);
    data.push_back(2);

    pivot = quick_partition_ints(data.begin(), data.end());
    index = static_cast<std::size_t>(pivot - data.begin());

    result.check(*pivot == 2);

    for (i = 0; i < index; ++i)
    {
        result.check(!(2 < data[i]));
    }

    for (i = (index + 1); i < data.size(); ++i)
    {
        result.check(!(data[i] < 2));
    }

    return result.ok();
}


// tests_quick_sort_partition_minimal_ranges
bool tests_quick_sort_partition_minimal_ranges()
{
    verdict                    result;
    std::vector<int>           data;
    std::vector<int>::iterator pivot;

    //   Length 1 is the shortest range the partitioner accepts: lo starts
    // one past hi, both scans fail their guard, the crossing test fires on
    // the first pass and the closing swap is a self-swap.
    data.clear();
    data.push_back(7);

    pivot = quick_partition_ints(data.begin(), data.end());

    result.check(pivot == data.begin());
    result.check(data[0] == 7);

    //   Length 2, already ordered: hi retreats onto _first and the pivot
    // stays put.
    data.clear();
    data.push_back(1);
    data.push_back(2);

    pivot = quick_partition_ints(data.begin(), data.end());

    result.check(pivot == data.begin());
    result.check(data[0] == 1);
    result.check(data[1] == 2);

    //   Length 2, inverted: lo advances past hi and the pivot is swapped to
    // the back.
    data.clear();
    data.push_back(2);
    data.push_back(1);

    pivot = quick_partition_ints(data.begin(), data.end());

    result.check(pivot == (data.begin() + 1));
    result.check(data[0] == 1);
    result.check(data[1] == 2);

    //   Length 2, equivalent elements: the crossing happens through the
    // swap branch instead.
    data.clear();
    data.push_back(4);
    data.push_back(4);

    pivot = quick_partition_ints(data.begin(), data.end());

    result.check(pivot == data.begin());
    result.check(data[0] == 4);
    result.check(data[1] == 4);

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
