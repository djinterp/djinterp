/*******************************************************************************
* djinterp [core]                                     quick_sort_tests_apply.cpp
*
*
* path:      /tests/djinterp/core/util/sort/quick_sort_tests_apply.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "quick_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_quick_sort_apply_threshold_boundary
bool tests_quick_sort_apply_threshold_boundary()
{
    verdict          result;
    std::vector<int> quick_path;
    std::vector<int> insertion_path;
    std::size_t      quick_comparisons;
    std::size_t      insertion_comparisons;
    std::size_t      threshold;
    std::size_t      shape_index;
    std::size_t      length;

    threshold = static_cast<std::size_t>(D_QUICK_SORT_INSERTION_THRESHOLD);

    //   At or below the threshold the while loop is never entered and the
    // whole range is handed to insertion sort.  "Never entered" is checked
    // the only way it can be from outside: the two drivers must agree on the
    // output AND on the exact comparison count.  A single partition pass
    // would change the count immediately.
    for (length = 0; length <= threshold; ++length)
    {
        for (shape_index = 0;
             shape_index < battery_shape_count;
             ++shape_index)
        {
            quick_path            = make_shape(shape_index, length);
            insertion_path        = quick_path;
            quick_comparisons     = 0;
            insertion_comparisons = 0;

            ::djinterp::internal::quick_sort_apply(
                quick_path.begin(),
                quick_path.end(),
                make_counting(less<int>(), &quick_comparisons));

            ::djinterp::internal::insertion_sort_apply(
                insertion_path.begin(),
                insertion_path.end(),
                make_counting(less<int>(), &insertion_comparisons));

            result.check(ranges_equal(quick_path, insertion_path));
            result.check(quick_comparisons == insertion_comparisons);
        }
    }

    //   One element past the threshold the loop runs at least once.  A
    // strictly descending range is the cleanest witness: plain insertion
    // sort costs (n-1)(n+2)/2 comparisons on it, and partitioning first
    // must come in well under that.
    length                = (threshold + 1);
    quick_path            = shape_descending(length);
    insertion_path        = quick_path;
    quick_comparisons     = 0;
    insertion_comparisons = 0;

    ::djinterp::internal::quick_sort_apply(
        quick_path.begin(),
        quick_path.end(),
        make_counting(less<int>(), &quick_comparisons));

    ::djinterp::internal::insertion_sort_apply(
        insertion_path.begin(),
        insertion_path.end(),
        make_counting(less<int>(), &insertion_comparisons));

    result.check(insertion_comparisons ==
                 (((length - 1) * (length + 2)) / 2));
    result.check(quick_comparisons < insertion_comparisons);
    result.check(ranges_equal(quick_path, insertion_path));

    return result.ok();
}


// tests_quick_sort_apply_ordering_battery
bool tests_quick_sort_apply_ordering_battery()
{
    verdict result;

    verify_battery_reference(&quick_apply_ints, less<int>(), result);

    return result.ok();
}


// tests_quick_sort_apply_adversarial_shapes
bool tests_quick_sort_apply_adversarial_shapes()
{
    verdict          result;
    std::vector<int> data;
    std::vector<int> expected;
    std::size_t      comparisons;
    std::size_t      shape_index;
    std::size_t      length;

    //   The shapes that break naive quicksorts, at sizes large enough for a
    // quadratic blow-up to be unmistakable.  Median-of-three plus the
    // "swap-and-step on equivalence" partitioner handles all of them; the
    // check here is that the results stay correct and the work stays
    // sub-quadratic.
    length = 2048;

    for (shape_index = 0; shape_index < battery_shape_count; ++shape_index)
    {
        data     = make_shape(shape_index, length);
        expected = data;

        std::stable_sort(expected.begin(), expected.end(), less<int>());

        comparisons = 0;

        ::djinterp::internal::quick_sort_apply(
            data.begin(),
            data.end(),
            make_counting(less<int>(), &comparisons));

        result.check(data == expected);

        //   A generous ceiling: every shape in the corpus lands between 7n
        // and 27n comparisons at this size, whereas a degenerate partition
        // would cost on the order of n^2 / 2 -- about a thousand times the
        // limit below.
        result.check(comparisons < (60 * length));
    }

    //   Random input is the case that should be comfortably n log n, so it
    // gets the tighter bound.
    length      = 4096;
    data        = shape_random(length);
    comparisons = 0;

    ::djinterp::internal::quick_sort_apply(
        data.begin(),
        data.end(),
        make_counting(less<int>(), &comparisons));

    result.check(is_sorted_range(data.begin(), data.end(), less<int>()));
    result.check(comparisons < (25 * length));

    //   The driver recurses into the SMALLER partition and loops on the
    // larger, which caps the recursion depth at log2(n) however unbalanced
    // the splits are.  A large all-equal range is the input that would
    // exhaust the stack if that were the other way round.
    length = 8192;
    data   = shape_constant(length);

    ::djinterp::internal::quick_sort_apply(data.begin(),
                                           data.end(),
                                           less<int>());

    result.check(is_sorted_range(data.begin(), data.end(), less<int>()));

    data = shape_ascending(length);

    ::djinterp::internal::quick_sort_apply(data.begin(),
                                           data.end(),
                                           less<int>());

    result.check(is_sorted_range(data.begin(), data.end(), less<int>()));

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
