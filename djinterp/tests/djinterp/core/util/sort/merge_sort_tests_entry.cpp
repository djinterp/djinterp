/*******************************************************************************
* djinterp [core]                                     merge_sort_tests_entry.cpp
*
*
* path:      /tests/djinterp/core/util/sort/merge_sort_tests_entry.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "merge_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_merge_sort_entry_default
bool tests_merge_sort_entry_default()
{
    verdict          result;
    std::vector<int> through_entry;
    std::vector<int> through_apply;

    verify_battery_reference(&merge_sort_ints, less<int>(), result);

    through_entry = shape_random(64);
    through_apply = through_entry;

    merge_sort_ints(through_entry.begin(), through_entry.end());
    merge_apply_ints(through_apply.begin(), through_apply.end());

    result.check(ranges_equal(through_entry, through_apply));

    return result.ok();
}


// tests_merge_sort_entry_comparator
bool tests_merge_sort_entry_comparator()
{
    verdict          result;
    std::vector<int> data;
    std::size_t      comparisons;

    verify_battery_reference(&merge_sort_ints_descending,
                             greater<int>(),
                             result);

    data        = shape_random(48);
    comparisons = 0;

    merge_sort(data.begin(),
               data.end(),
               make_counting(less<int>(), &comparisons));

    result.check(comparisons > 0);
    result.check(is_sorted_range(data.begin(), data.end(), less<int>()));

    return result.ok();
}


// tests_merge_sort_entry_ordered
bool tests_merge_sort_entry_ordered()
{
    verdict                  result;
    std::vector<int>         ascending_out;
    std::vector<int>         descending_out;
    std::vector<int>         none_out;
    std::vector<int>         expected_ascending;
    std::vector<int>         expected_descending;
    std::vector<keyed_value> keyed;

    //   merge_sort_ordered takes a real sort_order, so unlike the bubble,
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

    merge_sort_ordered(ascending_out.begin(),
                       ascending_out.end(),
                       less<int>(),
                       sort_order::ascending);

    merge_sort_ordered(descending_out.begin(),
                       descending_out.end(),
                       less<int>(),
                       sort_order::descending);

    //   sort_order::none is not "do nothing": order_comparator only special
    // cases descending, so none behaves exactly as ascending does.
    merge_sort_ordered(none_out.begin(),
                       none_out.end(),
                       less<int>(),
                       sort_order::none);

    result.check(ranges_equal(ascending_out, expected_ascending));
    result.check(ranges_equal(descending_out, expected_descending));
    result.check(ranges_equal(none_out, expected_ascending));

    //   Wrapping the comparator must not cost stability: order_comparator
    // forwards an equivalence unchanged in the ascending direction.
    verify_stability_battery(&merge_sort_keyed_ordered_ascending,
                             true,
                             result);

    //   Descending is a genuine operand swap rather than a negation, so
    // equivalents keep their input order there too.
    keyed.push_back(make_keyed(1, 0));
    keyed.push_back(make_keyed(2, 1));
    keyed.push_back(make_keyed(1, 2));
    keyed.push_back(make_keyed(2, 3));

    merge_sort_ordered(keyed.begin(),
                       keyed.end(),
                       keyed_value_less(),
                       sort_order::descending);

    result.check(keyed[0] == make_keyed(2, 1));
    result.check(keyed[1] == make_keyed(2, 3));
    result.check(keyed[2] == make_keyed(1, 0));
    result.check(keyed[3] == make_keyed(1, 2));

    return result.ok();
}


// tests_merge_sort_entry_stability
bool tests_merge_sort_entry_stability()
{
    verdict result;

    //   merge_sort_traits and merge_sort_tag both claim stability; the claim
    // is checked across the whole keyed corpus.
    verify_stability_battery(&merge_sort_keyed, true, result);

    return result.ok();
}


// tests_merge_sort_entry_pointer_iterators
bool tests_merge_sort_entry_pointer_iterators()
{
    verdict result;

    verify_battery_raw(&merge_sort_ints_raw, less<int>(), result);

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
