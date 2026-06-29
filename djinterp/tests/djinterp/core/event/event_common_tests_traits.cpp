/******************************************************************************
* djinterp [test]                                 event_common_tests_traits.cpp
*
*   Section IV -- EVENT TRAITS.  Covers event_traits<_Event>: the payload_type
* it exposes, the args_type alias that mirrors it, the arity drawn from the
* payload tuple (0 through 4), the has_name flag, the has_args flag, and the
* behaviour on legacy-spelled and dual-spelled tags (legacy resolves, both
* prefers the canonical payload).
*
*
* path:      /tests/djinterp/core/event/event_common/event_common_tests_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

// djinterp
#include "event_common_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_event_traits_payload_type
bool
tests_event_traits_payload_type()
{
    bool ok = true;

    // payload_type is the event's declared payload tuple A_e.
    ok = D_EC_CHECK(
        (std::is_same<
            event_traits<ev_binary>::payload_type,
            std::tuple<int, double>
        >::value)
    ) && ok;

    // an empty payload remains the empty tuple.
    ok = D_EC_CHECK(
        (std::is_same<
            event_traits<ev_empty>::payload_type,
            std::tuple<>
        >::value)
    ) && ok;

    return ok;
}


// tests_event_traits_args_alias
bool
tests_event_traits_args_alias()
{
    bool ok = true;

    // args_type is a legacy alias that always equals payload_type.
    ok = D_EC_CHECK(
        (std::is_same<
            event_traits<ev_binary>::args_type,
            event_traits<ev_binary>::payload_type
        >::value)
    ) && ok;
    ok = D_EC_CHECK(
        (std::is_same<
            event_traits<ev_binary>::args_type,
            std::tuple<int, double>
        >::value)
    ) && ok;

    return ok;
}


// tests_event_traits_arity
bool
tests_event_traits_arity()
{
    bool ok = true;

    // arity is the width of the payload tuple, spanning the empty payload
    // through the four-element (variadic) payload.
    ok = D_EC_CHECK(event_traits<ev_empty>::arity == 0u)      && ok;
    ok = D_EC_CHECK(event_traits<ev_unary>::arity == 1u)      && ok;
    ok = D_EC_CHECK(event_traits<ev_binary>::arity == 2u)     && ok;
    ok = D_EC_CHECK(event_traits<ev_ternary>::arity == 3u)    && ok;
    ok = D_EC_CHECK(event_traits<ev_quaternary>::arity == 4u) && ok;

    return ok;
}


// tests_event_traits_has_name
bool
tests_event_traits_has_name()
{
    bool ok = true;

    // has_name reflects the presence of a conforming static name() member.
    ok = D_EC_CHECK(event_traits<ev_unary>::has_name)    && ok;
    ok = D_EC_CHECK(!event_traits<ev_unnamed>::has_name) && ok;

    // a name() of the wrong return type does not count as a name.
    ok = D_EC_CHECK(!event_traits<ev_badname>::has_name) && ok;

    return ok;
}


// tests_event_traits_has_args
bool
tests_event_traits_has_args()
{
    bool ok = true;

    // has_args is true exactly when the payload is non-empty (arity > 0).
    ok = D_EC_CHECK(!event_traits<ev_empty>::has_args)   && ok;
    ok = D_EC_CHECK(event_traits<ev_unary>::has_args)    && ok;
    ok = D_EC_CHECK(event_traits<ev_quaternary>::has_args) && ok;

    return ok;
}


// tests_event_traits_legacy_and_both
bool
tests_event_traits_legacy_and_both()
{
    bool ok = true;

    // a legacy-spelled tag resolves its payload through args_type.
    ok = D_EC_CHECK(
        (std::is_same<
            event_traits<ev_legacy>::payload_type,
            std::tuple<int, char>
        >::value)
    ) && ok;
    ok = D_EC_CHECK(event_traits<ev_legacy>::arity == 2u) && ok;

    // a legacy-only tag carries no name() member.
    ok = D_EC_CHECK(!event_traits<ev_legacy>::has_name) && ok;

    // a dual-spelled tag prefers the canonical payload over the legacy one.
    ok = D_EC_CHECK(
        (std::is_same<
            event_traits<ev_both>::payload_type,
            std::tuple<int>
        >::value)
    ) && ok;
    ok = D_EC_CHECK(event_traits<ev_both>::arity == 1u) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
