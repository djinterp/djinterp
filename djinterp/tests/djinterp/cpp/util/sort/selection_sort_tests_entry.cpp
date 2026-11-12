/*******************************************************************************
* djinterp [cpp]                                  selection_sort_tests_entry.cpp
*
*
* path:      /tests/djinterp/cpp/util/sort/selection_sort_tests_entry.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.12
*******************************************************************************/
// djinterp
// djinterp [test]
#include "selection_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_selection_sort_entry_default
bool tests_selection_sort_entry_default()
{
    verdict          result;
    std::vector<int> through_entry;
    std::vector<int> through_apply;

    verify_battery_reference(&selection_sort_ints, std::less<int>(), result);

    through_entry = shape_random(64);
    through_apply = through_entry;

    selection_sort_ints(through_entry.begin(), through_entry.end());
    selection_apply_ints(through_apply.begin(), through_apply.end());

    result.check(ranges_equal(through_entry, through_apply));

    return result.ok();
}


// tests_selection_sort_entry_comparator
bool tests_selection_sort_entry_comparator()
{
    verdict          result;
    std::vector<int> data;
    std::size_t      comparisons;

    verify_battery_reference(&selection_sort_ints_descending,
                             std::greater<int>(),
                             result);

    data        = shape_random(48);
    comparisons = 0;

    selection_sort(data.begin(),
                   data.end(),
                   make_counting(std::less<int>(), &comparisons));

    result.check(comparisons == ((48 * 47) / 2));
    result.check(is_sorted_range(data.begin(), data.end(), std::less<int>()));

    return result.ok();
}


// tests_selection_sort_entry_ordered
bool tests_selection_sort_entry_ordered()
{
    verdict          result;
    std::vector<int> ascending_out;
    std::vector<int> descending_out;
    std::vector<int> expected_ascending;
    std::vector<int> expected_descending;
    bool             callable;

    typedef std::vector<int>::iterator iterator_type;

    //   selection_sort_ordered takes its direction as sort_order::value, the
    // shape all six *_sort_ordered entry points share.  The probe asks
    // overload resolution rather than naming the call, so a signature that
    // drifted would fail this check instead of the suite's build.
    callable = selection_ordered_probe<iterator_type, std::less<int> >::value;

    result.check(callable);

    ascending_out       = shape_random(64);
    descending_out      = ascending_out;
    expected_ascending  = ascending_out;
    expected_descending = ascending_out;

    std::stable_sort(expected_ascending.begin(),
                     expected_ascending.end(),
                     std::less<int>());
    std::stable_sort(expected_descending.begin(),
                     expected_descending.end(),
                     std::greater<int>());

    selection_call_ordered(
        ascending_out.begin(),
        ascending_out.end(),
        std::less<int>(),
        sort_order::ascending,
        selection_ordered_probe<iterator_type, std::less<int> >());

    selection_call_ordered(
        descending_out.begin(),
        descending_out.end(),
        std::less<int>(),
        sort_order::descending,
        selection_ordered_probe<iterator_type, std::less<int> >());

    if (callable)
    {
        result.check(ranges_equal(ascending_out, expected_ascending));
        result.check(ranges_equal(descending_out, expected_descending));
    }

    return result.ok();
}


// tests_selection_sort_entry_instability
bool tests_selection_sort_entry_instability()
{
    verdict                  result;
    std::vector<DKeyedValue> witness;

    //   DSelectionSortTag advertises is_stable == false.  That is not a
    // vague caveat: the long-range exchange has a concrete witness.
    //
    //     input   (2,0) (2,1) (1,2)
    //     pass 0  minimum is (1,2); it is swapped with slot 0, which throws
    //             (2,0) to the back, BEHIND (2,1)
    //     result  (1,2) (2,1) (2,0)
    //
    //   The two key-2 elements have exchanged their input order, so the
    // documented instability is demonstrated rather than assumed.
    witness.push_back(make_keyed(2, 0));
    witness.push_back(make_keyed(2, 1));
    witness.push_back(make_keyed(1, 2));

    selection_sort_keyed(witness.begin(), witness.end());

    result.check(witness.size() == 3);
    result.check(witness[0] == make_keyed(1, 2));
    result.check(witness[1] == make_keyed(2, 1));
    result.check(witness[2] == make_keyed(2, 0));
    result.check(!is_stable_result(witness));

    //   Ordering and permutation still hold everywhere; only the stability
    // claim is withheld.
    verify_stability_battery(&selection_sort_keyed, false, result);

    return result.ok();
}


// tests_selection_sort_entry_pointer_iterators
bool tests_selection_sort_entry_pointer_iterators()
{
    verdict result;

    verify_battery_raw(&selection_sort_ints_raw, std::less<int>(), result);

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
