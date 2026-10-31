/*******************************************************************************
* djinterp [test]                                  event_common_tests_traits.cpp
*
*   Section IV -- EVENT TRAITS.  Covers event_traits<_Event>: the payload_type
* it exposes, the args_type alias that mirrors it, the arity drawn from the
* payload tuple (0 through 4), the has_name flag, the has_args flag, and the
* behaviour on legacy-spelled and dual-spelled tags (legacy resolves, both
* prefers the canonical payload).
*
*
* path:      /tests/djinterp/cpp/event/event_common_tests_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.29
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "event_common_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_event_traits_payload_type
bool tests_event_traits_payload_type()
{
    bool ok = true;

    // payload_type is the event's declared payload tuple A_e.
    ok = D_EC_CHECK(
        (std::is_same<
            event_traits<DEvBinary>::payload_type,
            std::tuple<int, double>
        >::value)
    ) && ok;

    // an empty payload remains the empty tuple.
    ok = D_EC_CHECK(
        (std::is_same<
            event_traits<DEvEmpty>::payload_type,
            std::tuple<>
        >::value)
    ) && ok;

    return ok;
}


// tests_event_traits_args_alias
bool tests_event_traits_args_alias()
{
    bool ok = true;

    // args_type is a legacy alias that always equals payload_type.
    ok = D_EC_CHECK(
        (std::is_same<
            event_traits<DEvBinary>::args_type,
            event_traits<DEvBinary>::payload_type
        >::value)
    ) && ok;
    ok = D_EC_CHECK(
        (std::is_same<
            event_traits<DEvBinary>::args_type,
            std::tuple<int, double>
        >::value)
    ) && ok;

    return ok;
}


// tests_event_traits_arity
bool tests_event_traits_arity()
{
    bool ok = true;

    // arity is the width of the payload tuple, spanning the empty payload
    // through the four-element (variadic) payload.
    ok = D_EC_CHECK(event_traits<DEvEmpty>::arity == 0u)      && ok;
    ok = D_EC_CHECK(event_traits<DEvUnary>::arity == 1u)      && ok;
    ok = D_EC_CHECK(event_traits<DEvBinary>::arity == 2u)     && ok;
    ok = D_EC_CHECK(event_traits<DEvTernary>::arity == 3u)    && ok;
    ok = D_EC_CHECK(event_traits<DEvQuaternary>::arity == 4u) && ok;

    return ok;
}


// tests_event_traits_has_name
bool tests_event_traits_has_name()
{
    bool ok = true;

    // has_name reflects the presence of a conforming static name() member.
    ok = D_EC_CHECK(event_traits<DEvUnary>::has_name)    && ok;
    ok = D_EC_CHECK(!event_traits<DEvUnnamed>::has_name) && ok;

    // a name() of the wrong return type does not count as a name.
    ok = D_EC_CHECK(!event_traits<DEvBadname>::has_name) && ok;

    return ok;
}


// tests_event_traits_has_args
bool tests_event_traits_has_args()
{
    bool ok = true;

    // has_args is true exactly when the payload is non-empty (arity > 0).
    ok = D_EC_CHECK(!event_traits<DEvEmpty>::has_args)   && ok;
    ok = D_EC_CHECK(event_traits<DEvUnary>::has_args)    && ok;
    ok = D_EC_CHECK(event_traits<DEvQuaternary>::has_args) && ok;

    return ok;
}


// tests_event_traits_legacy_and_both
bool tests_event_traits_legacy_and_both()
{
    bool ok = true;

    // a legacy-spelled tag resolves its payload through args_type.
    ok = D_EC_CHECK(
        (std::is_same<
            event_traits<DEvLegacy>::payload_type,
            std::tuple<int, char>
        >::value)
    ) && ok;
    ok = D_EC_CHECK(event_traits<DEvLegacy>::arity == 2u) && ok;

    // a legacy-only tag carries no name() member.
    ok = D_EC_CHECK(!event_traits<DEvLegacy>::has_name) && ok;

    // a dual-spelled tag prefers the canonical payload over the legacy one.
    ok = D_EC_CHECK(
        (std::is_same<
            event_traits<DEvBoth>::payload_type,
            std::tuple<int>
        >::value)
    ) && ok;
    ok = D_EC_CHECK(event_traits<DEvBoth>::arity == 1u) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
