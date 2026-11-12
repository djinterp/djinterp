/*******************************************************************************
* djinterp [cpp]                                     quick_sort_tests_traits.cpp
*
*
* path:      /tests/djinterp/cpp/util/sort/quick_sort_tests_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/
// std
#include <type_traits>
// djinterp
// djinterp [test]
#include "quick_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_quick_sort_traits_values
bool tests_quick_sort_traits_values()
{
    verdict result;

    //   The three properties every algorithm publishes.  Quicksort gives up
    // stability to stay in place, and partitions ordered input like any other
    // -- the median-of-three pivot keeps that O(n log n), not O(n).
    result.check(!quick_sort_traits::is_stable);
    result.check(quick_sort_traits::is_in_place);
    result.check(!quick_sort_traits::is_adaptive);

    //   The threshold is the build's: cfg_sort.h's knob, validated there to
    // 0 .. 32767, with 16 when the build says nothing.
    result.check(quick_sort_traits::insertion_threshold ==
                 D_INTERNAL_SORT_QUICK_INSERTION_THRESHOLD);
    result.check(quick_sort_traits::insertion_threshold >= 0);
    result.check(quick_sort_traits::insertion_threshold <= 32767);

    //   Usable in a constant expression, which is the point of publishing it
    // as a static const member rather than a function.
    result.check((std::integral_constant<
                      int,
                      quick_sort_traits::insertion_threshold>::value ==
                  quick_sort_traits::insertion_threshold));

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
