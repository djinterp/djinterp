/******************************************************************************
* djinterp [test]                                event_common_tests_verdict.cpp
*
*   Section II -- VERDICT (the set P).  Covers the `verdict` enum class and the
* `consumed` query: the enumerator values, the scoped-enum type properties,
* and the agreement between `consumed` and a direct comparison against
* verdict::consume.
*
*
* path:      /tests/djinterp/core/event/event_common/event_common_tests_verdict.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

// djinterp
#include "event_common_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_verdict_enumerators
bool
tests_verdict_enumerators()
{
    bool ok = true;

    // the verdict set P is a two-point set ordered {pass, consume}; the
    // underlying values are the default 0 and 1 respectively.
    ok = D_EC_CHECK(static_cast<int>(verdict::pass) == 0) && ok;
    ok = D_EC_CHECK(static_cast<int>(verdict::consume) == 1) && ok;
    ok = D_EC_CHECK(verdict::pass != verdict::consume) && ok;

    return ok;
}


// tests_verdict_type_properties
bool
tests_verdict_type_properties()
{
    bool ok = true;

    // verdict is a scoped enumeration: an enum that does NOT implicitly
    // convert to its underlying integer type.
    ok = D_EC_CHECK(std::is_enum<verdict>::value) && ok;
    ok = D_EC_CHECK(!std::is_convertible<verdict, int>::value) && ok;

    // an unspecified fixed underlying type defaults to int.
    ok = D_EC_CHECK(
        (std::is_same<std::underlying_type<verdict>::type, int>::value)
    ) && ok;

    return ok;
}


// tests_consumed_values
bool
tests_consumed_values()
{
    bool ok = true;

    // consumed is true exactly when the verdict halts propagation.
    ok = D_EC_CHECK(consumed(verdict::consume)) && ok;
    ok = D_EC_CHECK(!consumed(verdict::pass)) && ok;

    return ok;
}


// tests_consumed_consistency
bool
tests_consumed_consistency()
{
    bool ok = true;

    // consumed(_v) must agree with the direct comparison (_v == consume)
    // for every point of the verdict set.
    const verdict pass_v    = verdict::pass;
    const verdict consume_v = verdict::consume;

    ok = D_EC_CHECK(consumed(pass_v) == (pass_v == verdict::consume)) && ok;
    ok = D_EC_CHECK(
        consumed(consume_v) == (consume_v == verdict::consume)
    ) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
