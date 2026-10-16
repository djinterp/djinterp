/*******************************************************************************
* djinterp [core]                                  sort_common_tests_aliases.cpp
*
*
* path:      /tests/djinterp/core/util/sort/sort_common_tests_aliases.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "sort_common_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_sort_common_default_comparator
bool tests_sort_common_default_comparator()
{
    verdict result;

    //   default_comparator<T>::type is the C++98-compatible spelling, and it
    // resolves to less<T> -- ascending.  Every algorithm's two-argument
    // entry point defaults through this, so the identity is asserted rather
    // than assumed.
    result.check((std::is_same<default_comparator<int>::type,
                               less<int> >::value));

    result.check((std::is_same<default_comparator<boxed_int>::type,
                               less<boxed_int> >::value));

    //   It is a real, usable comparator, not just a typedef.
    result.check(default_comparator<int>::type()(1, 2));
    result.check(!default_comparator<int>::type()(2, 1));
    result.check(!default_comparator<int>::type()(2, 2));

    return result.ok();
}


// tests_sort_common_default_comparator_alias
bool tests_sort_common_default_comparator_alias()
{
    verdict result;

    //   default_comparator_t<T> is the C++11 alias-template shorthand, and
    // it must name exactly the same type as the ::type spelling.  The two
    // are used interchangeably across the subsystem -- sort_monoid.hpp
    // defaults its _Compare parameter to the alias while the algorithm
    // headers reach for less<T> directly -- so a divergence would show up as
    // a baffling template mismatch far from here.
    result.check((std::is_same<default_comparator_t<int>,
                               default_comparator<int>::type>::value));

    result.check((std::is_same<default_comparator_t<int>,
                               less<int> >::value));

    result.check((std::is_same<default_comparator_t<boxed_int>,
                               less<boxed_int> >::value));

    result.check(default_comparator_t<int>()(1, 2));
    result.check(!default_comparator_t<int>()(2, 1));

    return result.ok();
}


// tests_sort_common_include_guard
bool tests_sort_common_include_guard()
{
    verdict result;

#ifdef DJINTERP_UTILITY_SORT_
    //   The core header's guard is DJINTERP_UTILITY_SORT_ -- deliberately
    // NOT the facade's DJINTERP_UTILITY_SORT_FACADE_.  The two are easy to
    // confuse, and a collision would silently blank one of the headers.
    result.check(DJINTERP_UTILITY_SORT_ == 1);
#else
    result.check(false);
#endif

#ifdef DJINTERP_UTILITY_SORT_FACADE_
    //   Reaching sort_common.hpp on its own must not drag in the facade.
    result.check(false);
#else
    result.check(true);
#endif

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
