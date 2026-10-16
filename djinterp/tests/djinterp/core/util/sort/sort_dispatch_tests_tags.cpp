/*******************************************************************************
* djinterp [core]                                   sort_dispatch_tests_tags.cpp
*
*
* path:      /tests/djinterp/core/util/sort/sort_dispatch_tests_tags.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
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
    result.check(bubble_sort_tag::value == sort_algorithm::bubble);
    result.check(selection_sort_tag::value == sort_algorithm::selection);
    result.check(insertion_sort_tag::value == sort_algorithm::insertion);
    result.check(merge_sort_tag::value == sort_algorithm::merge);
    result.check(quick_sort_tag::value == sort_algorithm::quick);
    result.check(heap_sort_tag::value == sort_algorithm::heap);

    return result.ok();
}


// tests_sort_dispatch_tag_properties
bool tests_sort_dispatch_tag_properties()
{
    verdict result;

    //   The property matrix.  These are the subsystem's public promises
    // about each algorithm and they drive real caller decisions, so all
    // eighteen values are pinned individually rather than spot-checked.
    //
    //             stable  in-place  adaptive
    //   bubble      yes      yes       yes
    //   selection   no       yes       no
    //   insertion   yes      yes       yes
    //   merge       yes      no        no
    //   quick       no       yes       no
    //   heap        no       yes       no
    result.check(bubble_sort_tag::is_stable);
    result.check(bubble_sort_tag::is_in_place);
    result.check(bubble_sort_tag::is_adaptive);

    result.check(!selection_sort_tag::is_stable);
    result.check(selection_sort_tag::is_in_place);
    result.check(!selection_sort_tag::is_adaptive);

    result.check(insertion_sort_tag::is_stable);
    result.check(insertion_sort_tag::is_in_place);
    result.check(insertion_sort_tag::is_adaptive);

    result.check(merge_sort_tag::is_stable);
    result.check(!merge_sort_tag::is_in_place);
    result.check(!merge_sort_tag::is_adaptive);

    result.check(!quick_sort_tag::is_stable);
    result.check(quick_sort_tag::is_in_place);
    result.check(!quick_sort_tag::is_adaptive);

    result.check(!heap_sort_tag::is_stable);
    result.check(heap_sort_tag::is_in_place);
    result.check(!heap_sort_tag::is_adaptive);

    //   Merge sort is the only algorithm in the set that is not in place --
    // it is the one that allocates a buffer.
    result.check(!merge_sort_tag::is_in_place);
    result.check(bubble_sort_tag::is_in_place &&
                 selection_sort_tag::is_in_place &&
                 insertion_sort_tag::is_in_place &&
                 quick_sort_tag::is_in_place &&
                 heap_sort_tag::is_in_place);

    return result.ok();
}


// tests_sort_dispatch_tag_names
bool tests_sort_dispatch_tag_names()
{
    verdict result;

    result.check(names_equal(bubble_sort_tag::name(), "bubble_sort"));
    result.check(names_equal(selection_sort_tag::name(), "selection_sort"));
    result.check(names_equal(insertion_sort_tag::name(), "insertion_sort"));
    result.check(names_equal(merge_sort_tag::name(), "merge_sort"));
    result.check(names_equal(quick_sort_tag::name(), "quick_sort"));
    result.check(names_equal(heap_sort_tag::name(), "heap_sort"));

    //   Never null, and never accidentally shared between two tags.
    result.check(bubble_sort_tag::name() != nullptr);
    result.check(!names_equal(quick_sort_tag::name(),
                              heap_sort_tag::name()));
    result.check(!names_equal(merge_sort_tag::name(),
                              insertion_sort_tag::name()));

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
    verify_battery_reference(&tag_apply_ints<bubble_sort_tag>,
                             less<int>(),
                             result);
    verify_battery_reference(&tag_apply_ints<selection_sort_tag>,
                             less<int>(),
                             result);
    verify_battery_reference(&tag_apply_ints<insertion_sort_tag>,
                             less<int>(),
                             result);
    verify_battery_reference(&tag_apply_ints<merge_sort_tag>,
                             less<int>(),
                             result);
    verify_battery_reference(&tag_apply_ints<quick_sort_tag>,
                             less<int>(),
                             result);
    verify_battery_reference(&tag_apply_ints<heap_sort_tag>,
                             less<int>(),
                             result);

    return result.ok();
}


// tests_sort_dispatch_is_sort_tag
bool tests_sort_dispatch_is_sort_tag()
{
    verdict result;

    //   is_sort_tag is is_base_of<sort_tag_base, T>, so it accepts exactly
    // the six tags.
    result.check(is_sort_tag<bubble_sort_tag>::value);
    result.check(is_sort_tag<selection_sort_tag>::value);
    result.check(is_sort_tag<insertion_sort_tag>::value);
    result.check(is_sort_tag<merge_sort_tag>::value);
    result.check(is_sort_tag<quick_sort_tag>::value);
    result.check(is_sort_tag<heap_sort_tag>::value);

    result.check(!is_sort_tag<int>::value);
    result.check(!is_sort_tag<sort_algorithm>::value);
    result.check(!is_sort_tag<less<int> >::value);

    //   NOTE: dynamic_algorithm is NOT a sort tag -- it does not derive from
    // sort_tag_base.  sorter's static_assert message says the parameter must
    // be "a sort tag type ... or the dynamic_algorithm sentinel", which
    // reads as though this were true.  It is harmless, because
    // sorter<dynamic_algorithm> is an explicit specialization and so never
    // instantiates the primary template's assertion -- but the message is
    // misleading and is pinned here so the discrepancy stays visible.
    result.check(!is_sort_tag<dynamic_algorithm>::value);

    return result.ok();
}


// tests_sort_dispatch_default_tag
bool tests_sort_dispatch_default_tag()
{
    verdict result;

    //   The documented default is quicksort.  sorter<>, the defaulted
    // sort<>() overloads and the default-constructed dynamic sorter all
    // resolve through this typedef, so it is asserted once here and relied
    // on elsewhere.
    result.check((std::is_same<default_sort_tag, quick_sort_tag>::value));
    result.check(default_sort_tag::value == sort_algorithm::quick);
    result.check(names_equal(default_sort_tag::name(), "quick_sort"));
    result.check(is_sort_tag<default_sort_tag>::value);

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
