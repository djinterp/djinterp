/*******************************************************************************
* djinterp [cpp]                                     sort_common_tests_order.cpp
*
*
* path:      /tests/djinterp/cpp/util/sort/sort_common_tests_order.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.12
*******************************************************************************/
// djinterp
// djinterp [test]
#include "sort_common_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_sort_common_order_enumerators
bool tests_sort_common_order_enumerators()
{
    verdict result;

    //   The numeric values are pinned: internal::DOrderComparator branches on
    // them, and they cross into every *_sort_ordered call.
    result.check(static_cast<int>(sort_order::none) == 0);
    result.check(static_cast<int>(sort_order::ascending) == 1);
    result.check(static_cast<int>(sort_order::descending) == 2);

    result.check(sort_order::none != sort_order::ascending);
    result.check(sort_order::ascending != sort_order::descending);
    result.check(sort_order::none != sort_order::descending);

    result.check(sort_order::ascending == sort_order::ascending);

    return result.ok();
}


// tests_sort_common_order_matches_c_module
bool tests_sort_common_order_matches_c_module()
{
    verdict result;

    //   sort_common.hpp promises "the values of the C module's dj_sort_order",
    // so a direction can cross between the two languages as a plain integer.
    // The promise is a claim about another header, so it is checked against
    // that header rather than against literals.
    result.check(static_cast<int>(sort_order::none) ==
                 static_cast<int>(D_SORT_ORDER_NONE));
    result.check(static_cast<int>(sort_order::ascending) ==
                 static_cast<int>(D_SORT_ORDER_ASCENDING));
    result.check(static_cast<int>(sort_order::descending) ==
                 static_cast<int>(D_SORT_ORDER_DESCENDING));

    return result.ok();
}


// tests_sort_common_order_shape
bool tests_sort_common_order_shape()
{
    verdict result;

    //   sort_order is a struct wrapping an unscoped enum named value -- the
    // C++98 spelling of a scoped enumeration, which keeps sort_common.hpp and
    // every *_sort_ordered entry point valid from C++98.  The enumerators are
    // named through the struct, as a scoped enum's would be.
    result.check(std::is_class<sort_order>::value);
    result.check(std::is_enum<sort_order::value>::value);

    //   The property the *_sort_ordered entry points rely on: no implicit
    // conversion reaches sort_order::value, so a bool -- or an int -- handed
    // to the fourth parameter is a compile error rather than a direction.
    result.check((!std::is_convertible<bool, sort_order::value>::value));
    result.check((!std::is_convertible<int, sort_order::value>::value));

    //   The other direction does convert, as an unscoped enum does; that is
    // what lets a direction be compared with the C module's values above.
    result.check((std::is_convertible<sort_order::value, int>::value));

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
