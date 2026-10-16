/*******************************************************************************
* djinterp [core]                                    merge_sort_tests_traits.cpp
*
*
* path:      /tests/djinterp/core/util/sort/merge_sort_tests_traits.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "merge_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_merge_sort_traits_values
bool tests_merge_sort_traits_values()
{
    verdict result;

    //   merge_sort_traits is one of only two per-algorithm trait blocks the
    // subsystem actually defines (heap_sort_traits is the other), even though
    // every algorithm header's banner advertises one.  Its three values are
    // pinned here.
    result.check(merge_sort_traits::is_stable);
    result.check(!merge_sort_traits::is_in_place);
    result.check(!merge_sort_traits::is_adaptive);

    //   The dispatch tags are the subsystem's real single source of truth for
    // these properties, so the two surfaces are cross-checked: a change to
    // either one alone breaks this test rather than silently splitting the
    // answer a caller gets from sorter<merge_sort_tag>::is_stable and the one
    // it gets from merge_sort_traits::is_stable.
    result.check(merge_sort_traits::is_stable ==
                 merge_sort_tag::is_stable);
    result.check(merge_sort_traits::is_in_place ==
                 merge_sort_tag::is_in_place);
    result.check(merge_sort_traits::is_adaptive ==
                 merge_sort_tag::is_adaptive);

    //   The members are usable in constant expressions, which is the point of
    // publishing them as static const bool.
    result.check(static_cast<bool>(
        std::integral_constant<bool, merge_sort_traits::is_stable>::value));

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
