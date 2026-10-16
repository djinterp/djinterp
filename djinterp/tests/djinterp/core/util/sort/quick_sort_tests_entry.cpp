/*******************************************************************************
* djinterp [core]                                     quick_sort_tests_entry.cpp
*
*
* path:      /tests/djinterp/core/util/sort/quick_sort_tests_entry.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "quick_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_quick_sort_entry_default
bool tests_quick_sort_entry_default()
{
    verdict          result;
    std::vector<int> through_entry;
    std::vector<int> through_apply;

    verify_battery_reference(&quick_sort_ints, less<int>(), result);

    through_entry = shape_random(64);
    through_apply = through_entry;

    quick_sort_ints(through_entry.begin(), through_entry.end());
    quick_apply_ints(through_apply.begin(), through_apply.end());

    result.check(ranges_equal(through_entry, through_apply));

    return result.ok();
}


// tests_quick_sort_entry_comparator
bool tests_quick_sort_entry_comparator()
{
    verdict          result;
    std::vector<int> data;
    std::size_t      comparisons;

    verify_battery_reference(&quick_sort_ints_descending,
                             greater<int>(),
                             result);

    data        = shape_random(48);
    comparisons = 0;

    quick_sort(data.begin(),
               data.end(),
               make_counting(less<int>(), &comparisons));

    result.check(comparisons > 0);
    result.check(is_sorted_range(data.begin(), data.end(), less<int>()));

    return result.ok();
}


// tests_quick_sort_entry_ordered
bool tests_quick_sort_entry_ordered()
{
    verdict          result;
    std::vector<int> ascending_out;
    std::vector<int> descending_out;
    std::vector<int> none_out;
    std::vector<int> expected_ascending;
    std::vector<int> expected_descending;

    //   quick_sort_ordered takes a real sort_order, so unlike the bubble,
    // selection and insertion entry points it can simply be called.
    ascending_out       = shape_random(64);
    descending_out      = ascending_out;
    none_out            = ascending_out;
    expected_ascending  = ascending_out;
    expected_descending = ascending_out;

    std::stable_sort(expected_ascending.begin(),
                     expected_ascending.end(),
                     less<int>());
    std::stable_sort(expected_descending.begin(),
                     expected_descending.end(),
                     greater<int>());

    quick_sort_ordered(ascending_out.begin(),
                       ascending_out.end(),
                       less<int>(),
                       sort_order::ascending);

    quick_sort_ordered(descending_out.begin(),
                       descending_out.end(),
                       less<int>(),
                       sort_order::descending);

    //   sort_order::none is not "leave it alone": order_comparator special
    // cases descending only, so none behaves exactly as ascending does.
    quick_sort_ordered(none_out.begin(),
                       none_out.end(),
                       less<int>(),
                       sort_order::none);

    result.check(ranges_equal(ascending_out, expected_ascending));
    result.check(ranges_equal(descending_out, expected_descending));
    result.check(ranges_equal(none_out, expected_ascending));

    //   Ordering and permutation hold across the keyed corpus too; no
    // stability is claimed, because quicksort does not have it.
    verify_stability_battery(&quick_sort_keyed_ordered_ascending,
                             false,
                             result);

    return result.ok();
}


// tests_quick_sort_entry_short_range_guard
bool tests_quick_sort_entry_short_range_guard()
{
    verdict          result;
    std::vector<int> data;
    std::size_t      length;

    //   The partitioner cannot be handed an empty range -- it forms
    // _last - 1 before testing anything -- so the public entry point has to
    // keep short ranges away from it.  It does, via the threshold loop:
    // anything of 16 elements or fewer goes straight to insertion sort.
    // These are the calls that would fault if that guard were removed.
    for (length = 0; length <= 3; ++length)
    {
        data = shape_descending(length);

        quick_sort_ints(data.begin(), data.end());

        result.check(data.size() == length);
        result.check(is_sorted_range(data.begin(),
                                     data.end(),
                                     less<int>()));
    }

    //   An empty sub-range taken from the middle of a populated container
    // is the same guard reached a different way.
    data = shape_random(32);

    quick_sort_ints(data.begin() + 8, data.begin() + 8);

    result.check(ranges_equal(data, shape_random(32)));

    //   A single-element sub-range likewise.
    quick_sort_ints(data.begin() + 8, data.begin() + 9);

    result.check(ranges_equal(data, shape_random(32)));

    return result.ok();
}


// tests_quick_sort_entry_pointer_iterators
bool tests_quick_sort_entry_pointer_iterators()
{
    verdict result;

    verify_battery_raw(&quick_sort_ints_raw, less<int>(), result);

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
