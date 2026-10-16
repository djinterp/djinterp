/*******************************************************************************
* djinterp [core]                                     heap_sort_tests_traits.cpp
*
*
* path:      /tests/djinterp/core/util/sort/heap_sort_tests_traits.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "heap_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_heap_sort_traits_values
bool tests_heap_sort_traits_values()
{
    verdict result;

    //   heap_sort_traits is one of only two per-algorithm trait blocks the
    // subsystem actually defines (merge_sort_traits is the other), even
    // though every algorithm header's banner advertises one.  Its three
    // values are pinned here.
    result.check(!heap_sort_traits::is_stable);
    result.check(heap_sort_traits::is_in_place);
    result.check(!heap_sort_traits::is_adaptive);

    //   Cross-checked against the dispatch tag, which is the subsystem's
    // real single source of truth for these properties.  Changing either
    // surface alone breaks this test rather than silently splitting the
    // answer a caller gets from the two.
    result.check(heap_sort_traits::is_stable ==
                 heap_sort_tag::is_stable);
    result.check(heap_sort_traits::is_in_place ==
                 heap_sort_tag::is_in_place);
    result.check(heap_sort_traits::is_adaptive ==
                 heap_sort_tag::is_adaptive);

    //   Heap sort and merge sort are the two algorithms that publish traits,
    // and they are meant to be opposites on the first two axes: merge sort
    // buys stability with a buffer, heap sort gives up stability to stay in
    // place.
    result.check(heap_sort_traits::is_stable !=
                 merge_sort_traits::is_stable);
    result.check(heap_sort_traits::is_in_place !=
                 merge_sort_traits::is_in_place);

    result.check(static_cast<bool>(
        std::integral_constant<bool,
                               heap_sort_traits::is_in_place>::value));

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
