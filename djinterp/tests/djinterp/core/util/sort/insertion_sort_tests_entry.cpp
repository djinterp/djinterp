/*******************************************************************************
* djinterp [core]                                 insertion_sort_tests_entry.cpp
*
*
* path:      /tests/djinterp/core/util/sort/insertion_sort_tests_entry.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "insertion_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_insertion_sort_entry_default
bool tests_insertion_sort_entry_default()
{
    verdict          result;
    std::vector<int> through_entry;
    std::vector<int> through_apply;

    verify_battery_reference(&insertion_sort_ints, less<int>(), result);

    through_entry = shape_random(64);
    through_apply = through_entry;

    insertion_sort_ints(through_entry.begin(), through_entry.end());
    insertion_apply_ints(through_apply.begin(), through_apply.end());

    result.check(ranges_equal(through_entry, through_apply));

    return result.ok();
}


// tests_insertion_sort_entry_comparator
bool tests_insertion_sort_entry_comparator()
{
    verdict          result;
    std::vector<int> data;
    std::size_t      comparisons;

    verify_battery_reference(&insertion_sort_ints_descending,
                             greater<int>(),
                             result);

    data        = shape_random(48);
    comparisons = 0;

    insertion_sort(data.begin(),
                   data.end(),
                   make_counting(less<int>(), &comparisons));

    result.check(comparisons > 0);
    result.check(is_sorted_range(data.begin(), data.end(), less<int>()));

    return result.ok();
}


// tests_insertion_sort_entry_ordered
bool tests_insertion_sort_entry_ordered()
{
    verdict          result;
    std::vector<int> ascending_out;
    std::vector<int> descending_out;
    std::vector<int> expected_ascending;
    std::vector<int> expected_descending;
    bool             callable;

    typedef std::vector<int>::iterator iterator_type;

    //   DEFECT: insertion_sort_ordered declares its fourth parameter as
    // `bool` yet hands it to internal::order_comparator, which takes a
    // sort_order.  The template cannot be instantiated with a sort_order
    // argument, so this check is expected to FAIL until the parameter type
    // is corrected.
    callable = insertion_ordered_probe<iterator_type, less<int> >::value;

    result.check(callable);

    ascending_out       = shape_random(64);
    descending_out      = ascending_out;
    expected_ascending  = ascending_out;
    expected_descending = ascending_out;

    std::stable_sort(expected_ascending.begin(),
                     expected_ascending.end(),
                     less<int>());
    std::stable_sort(expected_descending.begin(),
                     expected_descending.end(),
                     greater<int>());

    insertion_call_ordered(
        ascending_out.begin(),
        ascending_out.end(),
        less<int>(),
        sort_order::ascending,
        insertion_ordered_probe<iterator_type, less<int> >());

    insertion_call_ordered(
        descending_out.begin(),
        descending_out.end(),
        less<int>(),
        sort_order::descending,
        insertion_ordered_probe<iterator_type, less<int> >());

    if (callable)
    {
        result.check(ranges_equal(ascending_out, expected_ascending));
        result.check(ranges_equal(descending_out, expected_descending));
    }

    return result.ok();
}


// tests_insertion_sort_entry_stability
bool tests_insertion_sort_entry_stability()
{
    verdict                  result;
    std::vector<keyed_value> witness;

    //   insertion_sort_tag advertises is_stable == true.  The shift loop
    // stops on the first predecessor that does NOT strictly follow the key,
    // so an element never moves in front of an equivalent one.
    verify_stability_battery(&insertion_sort_keyed, true, result);

    //   The same witness that breaks selection sort must come through
    // untouched here.
    witness.push_back(make_keyed(2, 0));
    witness.push_back(make_keyed(2, 1));
    witness.push_back(make_keyed(1, 2));

    insertion_sort_keyed(witness.begin(), witness.end());

    result.check(witness[0] == make_keyed(1, 2));
    result.check(witness[1] == make_keyed(2, 0));
    result.check(witness[2] == make_keyed(2, 1));

    return result.ok();
}


// tests_insertion_sort_entry_pointer_iterators
bool tests_insertion_sort_entry_pointer_iterators()
{
    verdict result;

    verify_battery_raw(&insertion_sort_ints_raw, less<int>(), result);

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
