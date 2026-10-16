/*******************************************************************************
* djinterp [core]                               sort_dispatch_tests_selector.cpp
*
*
* path:      /tests/djinterp/core/util/sort/sort_dispatch_tests_selector.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "sort_dispatch_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_sort_dispatch_algorithm_enumerators
bool tests_sort_dispatch_algorithm_enumerators()
{
    verdict result;

    //   The six enumerators and their numeric values.  These cross the ABI
    // in every run-time sort() call and every sorter<dynamic_algorithm>, so
    // reordering them would silently redirect existing callers to a
    // different algorithm -- an error no compiler would report.
    result.check(static_cast<int>(sort_algorithm::bubble) == 0);
    result.check(static_cast<int>(sort_algorithm::selection) == 1);
    result.check(static_cast<int>(sort_algorithm::insertion) == 2);
    result.check(static_cast<int>(sort_algorithm::merge) == 3);
    result.check(static_cast<int>(sort_algorithm::quick) == 4);
    result.check(static_cast<int>(sort_algorithm::heap) == 5);

    //   All six are distinct.
    result.check(sort_algorithm::bubble != sort_algorithm::selection);
    result.check(sort_algorithm::selection != sort_algorithm::insertion);
    result.check(sort_algorithm::insertion != sort_algorithm::merge);
    result.check(sort_algorithm::merge != sort_algorithm::quick);
    result.check(sort_algorithm::quick != sort_algorithm::heap);
    result.check(sort_algorithm::heap != sort_algorithm::bubble);

    return result.ok();
}


// tests_sort_dispatch_algorithm_underlying_type
bool tests_sort_dispatch_algorithm_underlying_type()
{
    verdict result;

    //   Fixed at std::uint8_t, like sort_order.  A silent widening to int
    // would break no call site, so it is asserted directly.
    result.check((std::is_same<
                      std::underlying_type<sort_algorithm>::type,
                      std::uint8_t>::value));

    result.check(sizeof(sort_algorithm) == 1);

    //   Scoped, so it never converts implicitly.  That is what keeps the
    // run-time sort(algorithm, first, last) overloads from colliding with
    // the compile-time sort(first, last, comp) ones: an int would be
    // ambiguous, a sort_algorithm is not.
    result.check((!std::is_convertible<sort_algorithm, int>::value));
    result.check((!std::is_convertible<int, sort_algorithm>::value));

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
