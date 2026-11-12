/*******************************************************************************
* djinterp [cpp]                                   sort_dispatch_tests_entry.cpp
*
*
* path:      /tests/djinterp/cpp/util/sort/sort_dispatch_tests_entry.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.12
*******************************************************************************/
// djinterp
// djinterp [test]
#include "sort_dispatch_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_sort_dispatch_entry_compile_time_forms
bool tests_sort_dispatch_entry_compile_time_forms()
{
    verdict          result;
    std::vector<int> data;
    std::vector<int> expected_ascending;
    std::vector<int> expected_descending;

    expected_ascending = shape_random(64);
    std::stable_sort(expected_ascending.begin(),
                     expected_ascending.end(),
                     std::less<int>());

    expected_descending = shape_random(64);
    std::stable_sort(expected_descending.begin(),
                     expected_descending.end(),
                     std::greater<int>());

    //   Form A1: sort(tag(), first, last, comp).
    data = shape_random(64);
    sort(DMergeSortTag(), data.begin(), data.end(), std::greater<int>());
    result.check(data == expected_descending);

    //   Form A2: sort(tag(), first, last) -- default comparator.
    data = shape_random(64);
    sort(DHeapSortTag(), data.begin(), data.end());
    result.check(data == expected_ascending);

    //   Form A3: sort(tag(), first, last, comp, order).
    data = shape_random(64);
    sort(DInsertionSortTag(),
         data.begin(),
         data.end(),
         std::less<int>(),
         sort_order::descending);
    result.check(data == expected_descending);

    data = shape_random(64);
    sort(DInsertionSortTag(),
         data.begin(),
         data.end(),
         std::less<int>(),
         sort_order::ascending);
    result.check(data == expected_ascending);

    data = shape_random(64);
    sort(DInsertionSortTag(),
         data.begin(),
         data.end(),
         std::less<int>(),
         sort_order::none);
    result.check(data == expected_ascending);

    //   All six tags reach an algorithm through the free compile-time form.
    data = shape_random(64);
    sort(DBubbleSortTag(), data.begin(), data.end());
    result.check(data == expected_ascending);

    data = shape_random(64);
    sort(DSelectionSortTag(), data.begin(), data.end());
    result.check(data == expected_ascending);

    data = shape_random(64);
    sort(DInsertionSortTag(), data.begin(), data.end());
    result.check(data == expected_ascending);

    data = shape_random(64);
    sort(DMergeSortTag(), data.begin(), data.end());
    result.check(data == expected_ascending);

    data = shape_random(64);
    sort(DQuickSortTag(), data.begin(), data.end());
    result.check(data == expected_ascending);

    return result.ok();
}


// tests_sort_dispatch_entry_runtime_forms
bool tests_sort_dispatch_entry_runtime_forms()
{
    verdict          result;
    std::vector<int> data;
    std::vector<int> expected_ascending;
    std::vector<int> expected_descending;
    std::size_t      comparisons;
    std::size_t      i;

    DSortAlgorithm every[6];

    every[0] = DSortAlgorithm::bubble;
    every[1] = DSortAlgorithm::selection;
    every[2] = DSortAlgorithm::insertion;
    every[3] = DSortAlgorithm::merge;
    every[4] = DSortAlgorithm::quick;
    every[5] = DSortAlgorithm::heap;

    expected_ascending = shape_random(64);
    std::stable_sort(expected_ascending.begin(),
                     expected_ascending.end(),
                     std::less<int>());

    expected_descending = shape_random(64);
    std::stable_sort(expected_descending.begin(),
                     expected_descending.end(),
                     std::greater<int>());

    //   Form B2: sort(algorithm, first, last).  The leading DSortAlgorithm
    // is what tells the overload set to take the run-time family; because
    // the enumeration is scoped there is no conversion by which this could
    // be mistaken for the compile-time sort(first, last, comp).
    for (i = 0; i < 6; ++i)
    {
        data = shape_random(64);
        sort(every[i], data.begin(), data.end());
        result.check(data == expected_ascending);
    }

    //   Form B1: sort(algorithm, first, last, comp).
    for (i = 0; i < 6; ++i)
    {
        data = shape_random(64);
        sort(every[i], data.begin(), data.end(), std::greater<int>());
        result.check(data == expected_descending);
    }

    //   Form B3: sort(algorithm, first, last, comp, order).
    for (i = 0; i < 6; ++i)
    {
        data = shape_random(64);
        sort(every[i],
             data.begin(),
             data.end(),
             std::less<int>(),
             sort_order::descending);
        result.check(data == expected_descending);

        data = shape_random(64);
        sort(every[i],
             data.begin(),
             data.end(),
             std::less<int>(),
             sort_order::ascending);
        result.check(data == expected_ascending);
    }

    //   An unrecognised enumerator is a no-op here too, through all three
    // run-time forms.
    data        = shape_random(64);
    comparisons = 0;

    sort(unrecognised_algorithm(),
         data.begin(),
         data.end(),
         make_counting(std::less<int>(), &comparisons));

    result.check(ranges_equal(data, shape_random(64)));
    result.check(comparisons == 0);

    data = shape_random(64);
    sort(unrecognised_algorithm(), data.begin(), data.end());
    result.check(ranges_equal(data, shape_random(64)));

    data = shape_random(64);
    sort(unrecognised_algorithm(),
         data.begin(),
         data.end(),
         std::less<int>(),
         sort_order::descending);
    result.check(ranges_equal(data, shape_random(64)));

    return result.ok();
}


// tests_sort_dispatch_entry_default_tag
bool tests_sort_dispatch_entry_default_tag()
{
    verdict          result;
    std::vector<int> defaulted;
    std::vector<int> named;
    std::size_t      defaulted_comparisons;
    std::size_t      named_comparisons;
    std::size_t      foreign_comparisons;

    //   sort(first, last, comp) with no explicit tag must be exactly
    // sort(DDefaultSortTag(), first, last, comp).  Identical output is not
    // enough to show that -- every correct sort of ints produces identical
    // output -- so the comparison COUNT is checked too.  Two different
    // algorithms would essentially never agree on it.
    defaulted             = shape_random(256);
    named                 = defaulted;
    defaulted_comparisons = 0;
    named_comparisons     = 0;
    foreign_comparisons   = 0;

    //   Qualified deliberately.  An unqualified sort(first, last, comp) on a
    // standard-library iterator is AMBIGUOUS: ordinary lookup finds
    // djinterp::sort, and ADL on __normal_iterator<int*, std::vector<int> >
    // adds std::sort, which matches equally well.  The default-tag entry
    // point is still what is under test here -- the qualification only says
    // WHICH sort, it does not supply a tag.
    ::djinterp::sort(defaulted.begin(),
                     defaulted.end(),
                     make_counting(std::less<int>(), &defaulted_comparisons));

    sort(DDefaultSortTag(),
         named.begin(),
         named.end(),
         make_counting(std::less<int>(), &named_comparisons));

    result.check(ranges_equal(defaulted, named));
    result.check(defaulted_comparisons == named_comparisons);
    result.check(defaulted_comparisons > 0);

    //   And the count really does discriminate: a different algorithm on the
    // same input disagrees.
    named = shape_random(256);

    sort(DBubbleSortTag(),
         named.begin(),
         named.end(),
         make_counting(std::less<int>(), &foreign_comparisons));

    result.check(foreign_comparisons != defaulted_comparisons);

    //   The two-argument defaulted form, and the ordered defaulted form.
    defaulted = shape_random(64);
    named     = shape_random(64);

    ::djinterp::sort(defaulted.begin(), defaulted.end());
    sort(DDefaultSortTag(), named.begin(), named.end());

    result.check(ranges_equal(defaulted, named));

    defaulted = shape_random(64);
    named     = shape_random(64);

    ::djinterp::sort(defaulted.begin(),
                     defaulted.end(),
                     std::less<int>(),
                     sort_order::descending);
    sort(DDefaultSortTag(),
         named.begin(),
         named.end(),
         std::less<int>(),
         sort_order::descending);

    result.check(ranges_equal(defaulted, named));
    result.check(is_sorted_range(defaulted.begin(),
                                 defaulted.end(),
                                 std::greater<int>()));

    return result.ok();
}


// tests_sort_dispatch_entry_battery
bool tests_sort_dispatch_entry_battery()
{
    verdict result;

    //   The compile-time free form, every tag, over the whole corpus.
    verify_battery_reference(&free_sort_ints<DBubbleSortTag>,
                             std::less<int>(),
                             result);
    verify_battery_reference(&free_sort_ints<DSelectionSortTag>,
                             std::less<int>(),
                             result);
    verify_battery_reference(&free_sort_ints<DInsertionSortTag>,
                             std::less<int>(),
                             result);
    verify_battery_reference(&free_sort_ints<DMergeSortTag>,
                             std::less<int>(),
                             result);
    verify_battery_reference(&free_sort_ints<DQuickSortTag>,
                             std::less<int>(),
                             result);
    verify_battery_reference(&free_sort_ints<DHeapSortTag>,
                             std::less<int>(),
                             result);

    //   The run-time free form and the dynamic DSorter, likewise.  These go
    // through the functor adapters, because the selector is a value rather
    // than a template argument.
    verify_battery_reference(
        free_runtime_sort_adapter(DSortAlgorithm::bubble),
        std::less<int>(),
        result);
    verify_battery_reference(
        free_runtime_sort_adapter(DSortAlgorithm::merge),
        std::less<int>(),
        result);
    verify_battery_reference(
        free_runtime_sort_adapter(DSortAlgorithm::heap),
        std::less<int>(),
        result);

    verify_battery_reference(
        dynamic_sorter_adapter(DSortAlgorithm::selection),
        std::less<int>(),
        result);
    verify_battery_reference(
        dynamic_sorter_adapter(DSortAlgorithm::insertion),
        std::less<int>(),
        result);
    verify_battery_reference(
        dynamic_sorter_adapter(DSortAlgorithm::quick),
        std::less<int>(),
        result);

    return result.ok();
}


// tests_sort_dispatch_entry_pointer_iterators
bool tests_sort_dispatch_entry_pointer_iterators()
{
    verdict result;

    //   The whole dispatch layer is iterator-generic, so it has to work over
    // raw pointers as well as over vector iterators.  This is the check that
    // would catch a stray dependence on a class-type iterator anywhere in
    // the chain from sort() down to the algorithm.
    verify_battery_raw(&free_sort_ints_raw<DBubbleSortTag>,
                       std::less<int>(),
                       result);
    verify_battery_raw(&free_sort_ints_raw<DSelectionSortTag>,
                       std::less<int>(),
                       result);
    verify_battery_raw(&free_sort_ints_raw<DInsertionSortTag>,
                       std::less<int>(),
                       result);
    verify_battery_raw(&free_sort_ints_raw<DMergeSortTag>,
                       std::less<int>(),
                       result);
    verify_battery_raw(&free_sort_ints_raw<DQuickSortTag>,
                       std::less<int>(),
                       result);
    verify_battery_raw(&free_sort_ints_raw<DHeapSortTag>,
                       std::less<int>(),
                       result);

    verify_battery_raw(free_runtime_sort_adapter(DSortAlgorithm::quick),
                       std::less<int>(),
                       result);
    verify_battery_raw(dynamic_sorter_adapter(DSortAlgorithm::merge),
                       std::less<int>(),
                       result);

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
