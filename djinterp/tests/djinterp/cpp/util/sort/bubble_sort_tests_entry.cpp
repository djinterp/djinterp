/*******************************************************************************
* djinterp [cpp]                                     bubble_sort_tests_entry.cpp
*
*
* path:      /tests/djinterp/cpp/util/sort/bubble_sort_tests_entry.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.12
*******************************************************************************/
// djinterp
// djinterp [test]
#include "bubble_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_bubble_sort_entry_default
bool tests_bubble_sort_entry_default()
{
    verdict          result;
    std::vector<int> through_entry;
    std::vector<int> through_apply;

    // the two-argument entry point across the whole corpus
    verify_battery_reference(&bubble_sort_ints, std::less<int>(), result);

    //   the defaulted comparator must be exactly std::less<value_type>: the
    // entry point and the internal driver have to agree element for element.
    through_entry = shape_random(64);
    through_apply = through_entry;

    bubble_sort_ints(through_entry.begin(), through_entry.end());
    bubble_apply_ints(through_apply.begin(), through_apply.end());

    result.check(ranges_equal(through_entry, through_apply));

    return result.ok();
}


// tests_bubble_sort_entry_comparator
bool tests_bubble_sort_entry_comparator()
{
    verdict          result;
    std::vector<int> data;
    std::size_t      comparisons;

    // the explicit-comparator entry point, driven descending
    verify_battery_reference(&bubble_sort_ints_descending,
                             std::greater<int>(),
                             result);

    //   the comparator is taken by value and must still be the one actually
    // consulted: a counting wrapper has to see traffic.
    data        = shape_random(48);
    comparisons = 0;

    bubble_sort(data.begin(),
                data.end(),
                make_counting(std::less<int>(), &comparisons));

    result.check(comparisons > 0);
    result.check(is_sorted_range(data.begin(), data.end(), std::less<int>()));

    return result.ok();
}


// tests_bubble_sort_entry_ordered
bool tests_bubble_sort_entry_ordered()
{
    verdict            result;
    std::vector<int>   ascending_out;
    std::vector<int>   descending_out;
    std::vector<int>   expected_ascending;
    std::vector<int>   expected_descending;
    bool               callable;

    typedef std::vector<int>::iterator iterator_type;

    //   bubble_sort_ordered takes its direction as sort_order::value, the
    // shape all six *_sort_ordered entry points share.  The probe asks
    // overload resolution rather than naming the call, so a signature that
    // drifted would fail this check instead of the suite's build.
    callable = bubble_ordered_probe<iterator_type, std::less<int> >::value;

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

    bubble_call_ordered(ascending_out.begin(),
                        ascending_out.end(),
                        std::less<int>(),
                        sort_order::ascending,
                        bubble_ordered_probe<iterator_type, std::less<int> >());

    bubble_call_ordered(descending_out.begin(),
                        descending_out.end(),
                        std::less<int>(),
                        sort_order::descending,
                        bubble_ordered_probe<iterator_type, std::less<int> >());

    //   the behavioural half of the test is only meaningful once the entry
    // point is callable at all
    if (callable)
    {
        result.check(ranges_equal(ascending_out, expected_ascending));
        result.check(ranges_equal(descending_out, expected_descending));
    }

    return result.ok();
}


// tests_bubble_sort_entry_stability
bool tests_bubble_sort_entry_stability()
{
    verdict result;

    //   DBubbleSortTag advertises is_stable == true, and only ever swaps a
    // pair that is STRICTLY out of order, so equivalents can never cross.
    verify_stability_battery(&bubble_sort_keyed, true, result);

    return result.ok();
}


// tests_bubble_sort_entry_pointer_iterators
bool tests_bubble_sort_entry_pointer_iterators()
{
    verdict result;

    // raw pointers are the minimal random-access iterator model
    verify_battery_raw(&bubble_sort_ints_raw, std::less<int>(), result);

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
