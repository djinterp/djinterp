/*******************************************************************************
* djinterp [cpp]                                    sort_common_tests_header.cpp
*
*
* path:      /tests/djinterp/cpp/util/sort/sort_common_tests_header.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/
// djinterp
// djinterp [test]
#include "sort_common_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_sort_common_include_guard
bool tests_sort_common_include_guard()
{
    verdict result;

#ifdef DJINTERP_UTIL_SORT_COMMON_HPP
    //   The header's guard follows the tree's path rule: util/sort/
    // sort_common.hpp guards as DJINTERP_UTIL_SORT_COMMON_HPP.
    result.check(DJINTERP_UTIL_SORT_COMMON_HPP == 1);
#else
    result.check(false);
#endif

#ifdef DJINTERP_UTIL_SORT_HPP
    //   Reaching sort_common.hpp -- here through insertion_sort.hpp too --
    // must not drag in the facade.  The two guards differ by one word, and a
    // collision would silently blank one of the headers.
    result.check(false);
#else
    result.check(true);
#endif

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
