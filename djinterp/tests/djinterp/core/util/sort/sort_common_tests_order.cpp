/*******************************************************************************
* djinterp [core]                                    sort_common_tests_order.cpp
*
*
* path:      /tests/djinterp/core/util/sort/sort_common_tests_order.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
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

    //   sort_order is a scoped enum, so the enumerators are distinct values
    // that do not implicitly convert.  Their numeric values are pinned
    // because internal::order_comparator branches on them and because they
    // cross the ABI in every *_sort_ordered signature.
    result.check(static_cast<int>(sort_order::none) == 0);
    result.check(static_cast<int>(sort_order::ascending) == 1);
    result.check(static_cast<int>(sort_order::descending) == 2);

    result.check(sort_order::none != sort_order::ascending);
    result.check(sort_order::ascending != sort_order::descending);
    result.check(sort_order::none != sort_order::descending);

    result.check(sort_order::ascending == sort_order::ascending);

    return result.ok();
}


// tests_sort_common_order_underlying_type
bool tests_sort_common_order_underlying_type()
{
    verdict result;

    //   The underlying type is fixed at std::uint8_t, which is what makes
    // sort_order safe to store in a packed struct or send over a wire.  A
    // silent change to the default (int) would not break any call site, so
    // it is asserted directly.
    result.check((std::is_same<
                      std::underlying_type<sort_order>::type,
                      std::uint8_t>::value));

    result.check(sizeof(sort_order) == 1);

    //   Scoped enums do not convert to their underlying type implicitly.
    // That is the property the *_sort_ordered defect turns on: a `bool`
    // parameter cannot receive a sort_order argument.
    result.check((!std::is_convertible<sort_order, int>::value));
    result.check((!std::is_convertible<sort_order, bool>::value));
    result.check((!std::is_convertible<bool, sort_order>::value));

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
