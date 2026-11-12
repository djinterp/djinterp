/*******************************************************************************
* djinterp [cpp]                                      heap_sort_tests_traits.cpp
*
*
* path:      /tests/djinterp/cpp/util/sort/heap_sort_tests_traits.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.12
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

    //   heap_sort_traits pins the three properties a caller can branch on.
    // Heap sort gives up stability to stay in place, and costs O(n log n) on
    // every input: heapify reorders ordered input like any other, so it is
    // not adaptive.
    result.check(!heap_sort_traits::is_stable);
    result.check(heap_sort_traits::is_in_place);
    result.check(!heap_sort_traits::is_adaptive);

    //   The members are usable in constant expressions, which is the point
    // of publishing them as static const bool.  The dispatch tags inherit
    // this struct, so the dispatch suite checks that link.
    result.check(static_cast<bool>(
        std::integral_constant<bool,
                               heap_sort_traits::is_in_place>::value));

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
