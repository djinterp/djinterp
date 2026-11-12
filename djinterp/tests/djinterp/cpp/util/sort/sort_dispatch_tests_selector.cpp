/*******************************************************************************
* djinterp [cpp]                                sort_dispatch_tests_selector.cpp
*
*
* path:      /tests/djinterp/cpp/util/sort/sort_dispatch_tests_selector.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.12
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
    // in every run-time sort() call and every DSorter<DDynamicAlgorithm>, so
    // reordering them would silently redirect existing callers to a
    // different algorithm -- an error no compiler would report.
    result.check(static_cast<int>(DSortAlgorithm::bubble) == 0);
    result.check(static_cast<int>(DSortAlgorithm::selection) == 1);
    result.check(static_cast<int>(DSortAlgorithm::insertion) == 2);
    result.check(static_cast<int>(DSortAlgorithm::merge) == 3);
    result.check(static_cast<int>(DSortAlgorithm::quick) == 4);
    result.check(static_cast<int>(DSortAlgorithm::heap) == 5);

    //   All six are distinct.
    result.check(DSortAlgorithm::bubble != DSortAlgorithm::selection);
    result.check(DSortAlgorithm::selection != DSortAlgorithm::insertion);
    result.check(DSortAlgorithm::insertion != DSortAlgorithm::merge);
    result.check(DSortAlgorithm::merge != DSortAlgorithm::quick);
    result.check(DSortAlgorithm::quick != DSortAlgorithm::heap);
    result.check(DSortAlgorithm::heap != DSortAlgorithm::bubble);

    return result.ok();
}


// tests_sort_dispatch_algorithm_underlying_type
bool tests_sort_dispatch_algorithm_underlying_type()
{
    verdict result;

    //   Fixed at re_std::uint8_t, so a stored choice is one byte.  A silent
    // widening to int would break no call site, so it is asserted directly.
    result.check((std::is_same<
                      std::underlying_type<DSortAlgorithm>::type,
                      re_std::uint8_t>::value));

    result.check(sizeof(DSortAlgorithm) == 1);

    //   Scoped, so it never converts implicitly.  That is what keeps the
    // run-time sort(algorithm, first, last) overloads from colliding with
    // the compile-time sort(first, last, comp) ones: an int would be
    // ambiguous, a DSortAlgorithm is not.
    result.check((!std::is_convertible<DSortAlgorithm, int>::value));
    result.check((!std::is_convertible<int, DSortAlgorithm>::value));

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
