/*******************************************************************************
* djinterp [cpp]                                  sort_dispatch_tests_sorter.cpp
*
*
* path:      /tests/djinterp/cpp/util/sort/sort_dispatch_tests_sorter.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.12
*******************************************************************************/
// djinterp
// djinterp [test]
#include "sort_dispatch_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_sort_dispatch_sorter_static_surface
bool tests_sort_dispatch_sorter_static_surface()
{
    verdict result;

    //   DSorter<tag> republishes its tag's compile-time surface, so a caller
    // can query the algorithm without naming the tag twice.  Every member
    // must agree with the tag it was built from.
    result.check(DSorter<DBubbleSortTag>::algorithm ==
                 DSortAlgorithm::bubble);
    result.check(DSorter<DMergeSortTag>::algorithm ==
                 DSortAlgorithm::merge);
    result.check(DSorter<DHeapSortTag>::algorithm == DSortAlgorithm::heap);

    result.check(DSorter<DInsertionSortTag>::is_stable);
    result.check(DSorter<DInsertionSortTag>::is_in_place);
    result.check(DSorter<DInsertionSortTag>::is_adaptive);

    result.check(DSorter<DMergeSortTag>::is_stable);
    result.check(!DSorter<DMergeSortTag>::is_in_place);
    result.check(DSorter<DMergeSortTag>::is_adaptive);

    result.check(!DSorter<DQuickSortTag>::is_stable);
    result.check(DSorter<DQuickSortTag>::is_in_place);

    result.check(names_equal(DSorter<DSelectionSortTag>::name(),
                             "selection_sort"));
    result.check(names_equal(DSorter<DHeapSortTag>::name(), "heap_sort"));

    //   DSorter<DDefaultSortTag> is the compile-time form of the default.
    // Note that DSorter<> is NOT this type -- the template parameter defaults
    // to the DDynamicAlgorithm sentinel, so DSorter<> is the run-time
    // specialization and has no static `algorithm` member at all.
    result.check(DSorter<DDefaultSortTag>::algorithm ==
                 DSortAlgorithm::quick);
    result.check(DSorter<DQuickSortTag>::algorithm ==
                 DDefaultSortTag::value);

    //   The compile-time form is stateless: that is what makes it
    // zero-overhead next to calling the algorithm directly.
    result.check(std::is_empty<DSorter<DQuickSortTag> >::value);

    return result.ok();
}


// tests_sort_dispatch_sorter_static_calls
bool tests_sort_dispatch_sorter_static_calls()
{
    verdict          result;
    std::vector<int> data;
    std::vector<int> expected;

    //   The two-argument call operator, over the whole corpus, for every
    // tag.
    verify_battery_reference(&static_sorter_ints<DBubbleSortTag>,
                             std::less<int>(),
                             result);
    verify_battery_reference(&static_sorter_ints<DSelectionSortTag>,
                             std::less<int>(),
                             result);
    verify_battery_reference(&static_sorter_ints<DInsertionSortTag>,
                             std::less<int>(),
                             result);
    verify_battery_reference(&static_sorter_ints<DMergeSortTag>,
                             std::less<int>(),
                             result);
    verify_battery_reference(&static_sorter_ints<DQuickSortTag>,
                             std::less<int>(),
                             result);
    verify_battery_reference(&static_sorter_ints<DHeapSortTag>,
                             std::less<int>(),
                             result);

    //   The three-argument call operator forwards an explicit comparator.
    data     = shape_random(64);
    expected = data;

    std::stable_sort(expected.begin(), expected.end(), std::greater<int>());

    DSorter<DMergeSortTag>()(data.begin(), data.end(), std::greater<int>());

    result.check(data == expected);

    return result.ok();
}


// tests_sort_dispatch_sorter_dynamic_construction
bool tests_sort_dispatch_sorter_dynamic_construction()
{
    verdict result;

    DSorter<DDynamicAlgorithm> defaulted;
    DSorter<DDynamicAlgorithm> explicit_bubble(DSortAlgorithm::bubble);
    DSorter<>                  shorthand;

    //   The default constructor binds to DDefaultSortTag -- quicksort --
    // matching the compile-time default.
    result.check(defaulted.algorithm() == DDefaultSortTag::value);
    result.check(defaulted.algorithm() == DSortAlgorithm::quick);
    result.check(names_equal(defaulted.name(), "quick_sort"));

    //   DSorter<> and DSorter<DDynamicAlgorithm> are the same type: the
    // template parameter defaults to the sentinel.
    result.check((std::is_same<DSorter<>,
                               DSorter<DDynamicAlgorithm> >::value));
    result.check(shorthand.algorithm() == defaulted.algorithm());

    //   The converting constructor is explicit, so a bare DSortAlgorithm
    // never silently becomes a DSorter.
    result.check(explicit_bubble.algorithm() == DSortAlgorithm::bubble);
    result.check(names_equal(explicit_bubble.name(), "bubble_sort"));
    result.check((!std::is_convertible<
                      DSortAlgorithm,
                      DSorter<DDynamicAlgorithm> >::value));

    return result.ok();
}


// tests_sort_dispatch_sorter_dynamic_rebinding
bool tests_sort_dispatch_sorter_dynamic_rebinding()
{
    verdict          result;
    std::vector<int> data;
    std::vector<int> expected;
    std::size_t      i;

    DSorter<DDynamicAlgorithm> dispatcher;

    DSortAlgorithm every[6];

    every[0] = DSortAlgorithm::bubble;
    every[1] = DSortAlgorithm::selection;
    every[2] = DSortAlgorithm::insertion;
    every[3] = DSortAlgorithm::merge;
    every[4] = DSortAlgorithm::quick;
    every[5] = DSortAlgorithm::heap;

    expected = shape_random(64);
    std::stable_sort(expected.begin(), expected.end(), std::less<int>());

    //   One instance, rebound six times: the selector is state, not a
    // compile-time property, and every observable follows it.
    for (i = 0; i < 6; ++i)
    {
        dispatcher.set_algorithm(every[i]);

        result.check(dispatcher.algorithm() == every[i]);
        result.check(names_equal(dispatcher.name(), name_of(every[i])));

        data = shape_random(64);

        dispatcher(data.begin(), data.end());

        result.check(data == expected);
    }

    //   Rebinding to an unrecognised enumerator makes the DSorter a no-op
    // rather than a fault -- it inherits dispatch_runtime's fall-through.
    dispatcher.set_algorithm(unrecognised_algorithm());

    data     = shape_random(64);
    expected = data;

    dispatcher(data.begin(), data.end());

    result.check(ranges_equal(data, expected));
    result.check(names_equal(dispatcher.name(), "unknown"));

    return result.ok();
}


// tests_sort_dispatch_sorter_dynamic_properties
bool tests_sort_dispatch_sorter_dynamic_properties()
{
    verdict     result;
    std::size_t i;

    DSorter<DDynamicAlgorithm> dispatcher;

    DSortAlgorithm every[6];

    every[0] = DSortAlgorithm::bubble;
    every[1] = DSortAlgorithm::selection;
    every[2] = DSortAlgorithm::insertion;
    every[3] = DSortAlgorithm::merge;
    every[4] = DSortAlgorithm::quick;
    every[5] = DSortAlgorithm::heap;

    //   The dynamic DSorter's property accessors are functions where the
    // static DSorter's are constants, but they must answer identically for
    // the same algorithm.  Checked against properties_of, which is where
    // they source their answers.
    for (i = 0; i < 6; ++i)
    {
        dispatcher.set_algorithm(every[i]);

        result.check(dispatcher.is_stable() ==
                     properties_of(every[i]).is_stable);
        result.check(dispatcher.is_in_place() ==
                     properties_of(every[i]).is_in_place);
        result.check(dispatcher.is_adaptive() ==
                     properties_of(every[i]).is_adaptive);
    }

    //   Spot-checked against the compile-time surface, so a divergence
    // between the two faces of the subsystem shows up here.
    dispatcher.set_algorithm(DSortAlgorithm::merge);
    result.check(dispatcher.is_stable() == DSorter<DMergeSortTag>::is_stable);
    result.check(dispatcher.is_in_place() ==
                 DSorter<DMergeSortTag>::is_in_place);

    dispatcher.set_algorithm(DSortAlgorithm::heap);
    result.check(dispatcher.is_stable() == DSorter<DHeapSortTag>::is_stable);
    result.check(dispatcher.is_in_place() ==
                 DSorter<DHeapSortTag>::is_in_place);

    //   An unrecognised enumerator reports all-false, inherited from
    // properties_of.
    dispatcher.set_algorithm(unrecognised_algorithm());

    result.check(!dispatcher.is_stable());
    result.check(!dispatcher.is_in_place());
    result.check(!dispatcher.is_adaptive());

    return result.ok();
}


// tests_sort_dispatch_sorter_ordered_overloads
bool tests_sort_dispatch_sorter_ordered_overloads()
{
    verdict          result;
    std::vector<int> data;
    std::vector<int> expected_ascending;
    std::vector<int> expected_descending;

    DSorter<DDynamicAlgorithm> dispatcher(DSortAlgorithm::merge);

    expected_ascending = shape_random(64);
    std::stable_sort(expected_ascending.begin(),
                     expected_ascending.end(),
                     std::less<int>());

    expected_descending = shape_random(64);
    std::stable_sort(expected_descending.begin(),
                     expected_descending.end(),
                     std::greater<int>());

    //   Both DSorter forms carry a four-argument call operator that wraps the
    // comparator in an DOrderComparator.  Unlike the bubble, selection and
    // insertion *_sort_ordered free functions, these take a real sort_order
    // and so are callable as documented.
    data = shape_random(64);
    DSorter<DMergeSortTag>()(data.begin(),
                             data.end(),
                             std::less<int>(),
                             sort_order::ascending);
    result.check(data == expected_ascending);

    data = shape_random(64);
    DSorter<DMergeSortTag>()(data.begin(),
                             data.end(),
                             std::less<int>(),
                             sort_order::descending);
    result.check(data == expected_descending);

    data = shape_random(64);
    DSorter<DMergeSortTag>()(data.begin(),
                             data.end(),
                             std::less<int>(),
                             sort_order::none);
    result.check(data == expected_ascending);

    data = shape_random(64);
    dispatcher(data.begin(),
               data.end(),
               std::less<int>(),
               sort_order::descending);
    result.check(data == expected_descending);

    data = shape_random(64);
    dispatcher(data.begin(),
               data.end(),
               std::less<int>(),
               sort_order::ascending);
    result.check(data == expected_ascending);

    //   Handing the ordered overload an already-descending comparator flips
    // it back, which is the composition a caller would expect.
    data = shape_random(64);
    dispatcher(data.begin(),
               data.end(),
               std::greater<int>(),
               sort_order::descending);
    result.check(data == expected_ascending);

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
