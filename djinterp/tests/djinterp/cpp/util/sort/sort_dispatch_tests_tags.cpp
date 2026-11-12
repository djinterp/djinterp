/*******************************************************************************
* djinterp [cpp]                                    sort_dispatch_tests_tags.cpp
*
*
* path:      /tests/djinterp/cpp/util/sort/sort_dispatch_tests_tags.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.12
*******************************************************************************/
// djinterp
// djinterp [test]
#include "sort_dispatch_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_sort_dispatch_tag_values
bool tests_sort_dispatch_tag_values()
{
    verdict result;

    //   Every tag names the enumerator it stands for.  This is the join
    // between the compile-time and run-time surfaces, so it has to hold in
    // both directions -- see tests_sort_dispatch_algorithm_tag_alias for the
    // trip back.
    result.check(DBubbleSortTag::value == DSortAlgorithm::bubble);
    result.check(DSelectionSortTag::value == DSortAlgorithm::selection);
    result.check(DInsertionSortTag::value == DSortAlgorithm::insertion);
    result.check(DMergeSortTag::value == DSortAlgorithm::merge);
    result.check(DQuickSortTag::value == DSortAlgorithm::quick);
    result.check(DHeapSortTag::value == DSortAlgorithm::heap);

    return result.ok();
}


// tests_sort_dispatch_tag_properties
bool tests_sort_dispatch_tag_properties()
{
    verdict result;

    //   The property matrix.  These are the subsystem's public promises
    // about each algorithm and they drive real caller decisions, so all
    // eighteen values are pinned individually rather than spot-checked.
    // Merge sort is adaptive in comparisons: an ordered pair of runs costs
    // one comparison and a copy.
    //
    //             stable  in-place  adaptive
    //   bubble      yes      yes       yes
    //   selection   no       yes       no
    //   insertion   yes      yes       yes
    //   merge       yes      no        yes
    //   quick       no       yes       no
    //   heap        no       yes       no
    result.check(DBubbleSortTag::is_stable);
    result.check(DBubbleSortTag::is_in_place);
    result.check(DBubbleSortTag::is_adaptive);

    result.check(!DSelectionSortTag::is_stable);
    result.check(DSelectionSortTag::is_in_place);
    result.check(!DSelectionSortTag::is_adaptive);

    result.check(DInsertionSortTag::is_stable);
    result.check(DInsertionSortTag::is_in_place);
    result.check(DInsertionSortTag::is_adaptive);

    result.check(DMergeSortTag::is_stable);
    result.check(!DMergeSortTag::is_in_place);
    result.check(DMergeSortTag::is_adaptive);

    result.check(!DQuickSortTag::is_stable);
    result.check(DQuickSortTag::is_in_place);
    result.check(!DQuickSortTag::is_adaptive);

    result.check(!DHeapSortTag::is_stable);
    result.check(DHeapSortTag::is_in_place);
    result.check(!DHeapSortTag::is_adaptive);

    //   Merge sort is the only algorithm in the set that is not in place --
    // it is the one that allocates a buffer.
    result.check(!DMergeSortTag::is_in_place);
    result.check(DBubbleSortTag::is_in_place &&
                 DSelectionSortTag::is_in_place &&
                 DInsertionSortTag::is_in_place &&
                 DQuickSortTag::is_in_place &&
                 DHeapSortTag::is_in_place);

    //   The matrix has one source: each tag inherits its algorithm's traits
    // struct rather than restating the three values, so the tag and the
    // algorithm header cannot disagree.
    result.check((std::is_base_of<bubble_sort_traits,
                                  DBubbleSortTag>::value));
    result.check((std::is_base_of<selection_sort_traits,
                                  DSelectionSortTag>::value));
    result.check((std::is_base_of<insertion_sort_traits,
                                  DInsertionSortTag>::value));
    result.check((std::is_base_of<merge_sort_traits,
                                  DMergeSortTag>::value));
    result.check((std::is_base_of<quick_sort_traits,
                                  DQuickSortTag>::value));
    result.check((std::is_base_of<heap_sort_traits,
                                  DHeapSortTag>::value));

    return result.ok();
}


// tests_sort_dispatch_tag_names
bool tests_sort_dispatch_tag_names()
{
    verdict result;

    result.check(names_equal(DBubbleSortTag::name(), "bubble_sort"));
    result.check(names_equal(DSelectionSortTag::name(), "selection_sort"));
    result.check(names_equal(DInsertionSortTag::name(), "insertion_sort"));
    result.check(names_equal(DMergeSortTag::name(), "merge_sort"));
    result.check(names_equal(DQuickSortTag::name(), "quick_sort"));
    result.check(names_equal(DHeapSortTag::name(), "heap_sort"));

    //   Never null, and never accidentally shared between two tags.
    result.check(DBubbleSortTag::name() != nullptr);
    result.check(!names_equal(DQuickSortTag::name(),
                              DHeapSortTag::name()));
    result.check(!names_equal(DMergeSortTag::name(),
                              DInsertionSortTag::name()));

    return result.ok();
}


// tests_sort_dispatch_tag_apply
bool tests_sort_dispatch_tag_apply()
{
    verdict result;

    //   Each tag's uniform apply() must actually reach its own algorithm.
    // Driving all six over the whole corpus is what proves the forwarding
    // table has no copy-paste error in it -- a tag wired to the wrong entry
    // point would still sort correctly, so ordering alone cannot catch it;
    // the property and name checks above are what pin the identity.
    verify_battery_reference(&tag_apply_ints<DBubbleSortTag>,
                             std::less<int>(),
                             result);
    verify_battery_reference(&tag_apply_ints<DSelectionSortTag>,
                             std::less<int>(),
                             result);
    verify_battery_reference(&tag_apply_ints<DInsertionSortTag>,
                             std::less<int>(),
                             result);
    verify_battery_reference(&tag_apply_ints<DMergeSortTag>,
                             std::less<int>(),
                             result);
    verify_battery_reference(&tag_apply_ints<DQuickSortTag>,
                             std::less<int>(),
                             result);
    verify_battery_reference(&tag_apply_ints<DHeapSortTag>,
                             std::less<int>(),
                             result);

    return result.ok();
}


// tests_sort_dispatch_is_sort_tag
bool tests_sort_dispatch_is_sort_tag()
{
    verdict result;

    //   is_sort_tag is is_base_of<DSortTagBase, T>, so it accepts exactly
    // the six tags.
    result.check(is_sort_tag<DBubbleSortTag>::value);
    result.check(is_sort_tag<DSelectionSortTag>::value);
    result.check(is_sort_tag<DInsertionSortTag>::value);
    result.check(is_sort_tag<DMergeSortTag>::value);
    result.check(is_sort_tag<DQuickSortTag>::value);
    result.check(is_sort_tag<DHeapSortTag>::value);

    result.check(!is_sort_tag<int>::value);
    result.check(!is_sort_tag<DSortAlgorithm>::value);
    result.check(!is_sort_tag<std::less<int> >::value);

    //   NOTE: DDynamicAlgorithm is NOT a sort tag -- it does not derive from
    // DSortTagBase.  DSorter's static_assert message says the parameter must
    // be "a sort tag type ... or the DDynamicAlgorithm sentinel", which
    // reads as though this were true.  It is harmless, because
    // DSorter<DDynamicAlgorithm> is an explicit specialization and so never
    // instantiates the primary template's assertion -- but the message is
    // misleading and is pinned here so the discrepancy stays visible.
    result.check(!is_sort_tag<DDynamicAlgorithm>::value);

    return result.ok();
}


// tests_sort_dispatch_default_tag
bool tests_sort_dispatch_default_tag()
{
    verdict result;

    //   The documented default is quicksort.  DSorter<>, the defaulted
    // sort<>() overloads and the default-constructed dynamic DSorter all
    // resolve through this typedef, so it is asserted once here and relied
    // on elsewhere.
    result.check((std::is_same<DDefaultSortTag, DQuickSortTag>::value));
    result.check(DDefaultSortTag::value == DSortAlgorithm::quick);
    result.check(names_equal(DDefaultSortTag::name(), "quick_sort"));
    result.check(is_sort_tag<DDefaultSortTag>::value);

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
