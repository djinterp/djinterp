/*******************************************************************************
* djinterp [core]                                      heap_sort_tests_entry.cpp
*
*
* path:      /tests/djinterp/core/util/sort/heap_sort_tests_entry.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "heap_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_heap_sort_entry_default
bool tests_heap_sort_entry_default()
{
    verdict          result;
    std::vector<int> through_entry;
    std::vector<int> through_apply;

    verify_battery_reference(&heap_sort_ints, less<int>(), result);

    through_entry = shape_random(64);
    through_apply = through_entry;

    heap_sort_ints(through_entry.begin(), through_entry.end());
    heap_apply_ints(through_apply.begin(), through_apply.end());

    result.check(ranges_equal(through_entry, through_apply));

    return result.ok();
}


// tests_heap_sort_entry_comparator
bool tests_heap_sort_entry_comparator()
{
    verdict          result;
    std::vector<int> data;
    std::size_t      comparisons;

    //   Under greater<int> the "max-heap" is a min-heap on the values, so
    // the descending battery also proves the heap logic is expressed purely
    // through the comparator rather than through a baked-in ordering.
    verify_battery_reference(&heap_sort_ints_descending,
                             greater<int>(),
                             result);

    data        = shape_random(48);
    comparisons = 0;

    heap_sort(data.begin(),
              data.end(),
              make_counting(less<int>(), &comparisons));

    result.check(comparisons > 0);
    result.check(is_sorted_range(data.begin(), data.end(), less<int>()));

    return result.ok();
}


// tests_heap_sort_entry_ordered
bool tests_heap_sort_entry_ordered()
{
    verdict          result;
    std::vector<int> ascending_out;
    std::vector<int> descending_out;
    std::vector<int> none_out;
    std::vector<int> expected_ascending;
    std::vector<int> expected_descending;

    //   heap_sort_ordered takes a real sort_order, so unlike the bubble,
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

    heap_sort_ordered(ascending_out.begin(),
                      ascending_out.end(),
                      less<int>(),
                      sort_order::ascending);

    heap_sort_ordered(descending_out.begin(),
                      descending_out.end(),
                      less<int>(),
                      sort_order::descending);

    //   sort_order::none is not "leave it alone": order_comparator special
    // cases descending only, so none behaves exactly as ascending does.
    heap_sort_ordered(none_out.begin(),
                      none_out.end(),
                      less<int>(),
                      sort_order::none);

    result.check(ranges_equal(ascending_out, expected_ascending));
    result.check(ranges_equal(descending_out, expected_descending));
    result.check(ranges_equal(none_out, expected_ascending));

    verify_stability_battery(&heap_sort_keyed_ordered_ascending,
                             false,
                             result);

    return result.ok();
}


// tests_heap_sort_entry_instability
bool tests_heap_sort_entry_instability()
{
    verdict                  result;
    std::vector<keyed_value> witness;

    //   heap_sort_traits and heap_sort_tag both claim is_stable == false.
    // The claim has a concrete witness:
    //
    //     input   (2,0) (2,1) (1,2)
    //     heapify leaves the range untouched -- the root already dominates
    //     extract swaps the root to the back, which throws (2,0) BEHIND
    //             (2,1)
    //     result  (1,2) (2,1) (2,0)
    //
    //   Documenting instability by demonstration rather than by assertion
    // keeps the trait honest.
    witness.push_back(make_keyed(2, 0));
    witness.push_back(make_keyed(2, 1));
    witness.push_back(make_keyed(1, 2));

    heap_sort_keyed(witness.begin(), witness.end());

    result.check(witness.size() == 3);
    result.check(witness[0] == make_keyed(1, 2));
    result.check(witness[1] == make_keyed(2, 1));
    result.check(witness[2] == make_keyed(2, 0));
    result.check(!is_stable_result(witness));

    //   Ordering and permutation still hold across the keyed corpus; only
    // the stability claim is withheld.
    verify_stability_battery(&heap_sort_keyed, false, result);

    return result.ok();
}


// tests_heap_sort_entry_pointer_iterators
bool tests_heap_sort_entry_pointer_iterators()
{
    verdict result;

    verify_battery_raw(&heap_sort_ints_raw, less<int>(), result);

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
