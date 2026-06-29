/******************************************************************************
* djinterp [test]                                 event_common_tests_macros.cpp
*
*   Section V -- DECLARATION MACROS.  Covers D_EVENT and D_EVENT_EMPTY: the
* payload_type tuple they synthesize, the static name() member they emit (its
* exact string is checked with std::strcmp), the empty-payload variant, the
* arity range produced across zero through four payload domains, and the fact
* that a macro-declared tag is a well-formed event as seen by the detection
* traits and event_traits.
*
*   The fixtures exercised here are declared fresh, in a file-local anonymous
* namespace, so the macro expansion is tested at its own call site rather than
* relying on the shared header fixtures.
*
*
* path:      /tests/djinterp/core/event/event_common/event_common_tests_macros.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

// djinterp
#include "event_common_tests.hpp"


NS_DJINTERP
NS_TESTING


namespace
{
    // mac_empty
    //   fixture: macro-declared event tag with an empty payload.
    D_EVENT_EMPTY(mac_empty);

    // mac_unary
    //   fixture: macro-declared event tag with a one-element payload.
    D_EVENT(mac_unary, int);

    // mac_pair
    //   fixture: macro-declared event tag with a two-element payload.
    D_EVENT(mac_pair, int, double);

    // mac_triple
    //   fixture: macro-declared event tag with a three-element payload.
    D_EVENT(mac_triple, int, double, char);

    // mac_quad
    //   fixture: macro-declared event tag with a four-element payload.
    D_EVENT(mac_quad, int, int, int, int);
}


// tests_d_event_payload
bool
tests_d_event_payload()
{
    bool ok = true;

    // D_EVENT synthesizes payload_type as a std::tuple of the given domains,
    // in declaration order.
    ok = D_EC_CHECK(
        (std::is_same<
            mac_pair::payload_type,
            std::tuple<int, double>
        >::value)
    ) && ok;
    ok = D_EC_CHECK(
        (std::is_same<
            mac_triple::payload_type,
            std::tuple<int, double, char>
        >::value)
    ) && ok;

    return ok;
}


// tests_d_event_name
bool
tests_d_event_name()
{
    bool ok = true;

    // D_EVENT emits a static name() returning the stringized tag name.
    ok = D_EC_CHECK(
        (std::is_same<decltype(mac_pair::name()), const char*>::value)
    ) && ok;
    ok = D_EC_CHECK(std::strcmp(mac_pair::name(), "mac_pair") == 0) && ok;
    ok = D_EC_CHECK(std::strcmp(mac_unary::name(), "mac_unary") == 0) && ok;

    return ok;
}


// tests_d_event_empty
bool
tests_d_event_empty()
{
    bool ok = true;

    // D_EVENT_EMPTY synthesizes an empty payload tuple...
    ok = D_EC_CHECK(
        (std::is_same<mac_empty::payload_type, std::tuple<> >::value)
    ) && ok;

    // ...while still emitting the named-tag name() member.
    ok = D_EC_CHECK(std::strcmp(mac_empty::name(), "mac_empty") == 0) && ok;

    return ok;
}


// tests_d_event_arity_range
bool
tests_d_event_arity_range()
{
    bool ok = true;

    // the payload width seen through event_traits matches the number of
    // domains passed to the macro, from zero through four.
    ok = D_EC_CHECK(event_traits<mac_empty>::arity == 0u)  && ok;
    ok = D_EC_CHECK(event_traits<mac_unary>::arity == 1u)  && ok;
    ok = D_EC_CHECK(event_traits<mac_pair>::arity == 2u)   && ok;
    ok = D_EC_CHECK(event_traits<mac_triple>::arity == 3u) && ok;
    ok = D_EC_CHECK(event_traits<mac_quad>::arity == 4u)   && ok;

    return ok;
}


// tests_d_event_is_event
bool
tests_d_event_is_event()
{
    bool ok = true;

    // a macro-declared tag is a well-formed event: it carries a payload, that
    // payload is a std::tuple, and (for D_EVENT) it is named.
    ok = D_EC_CHECK(internal::has_event_payload<mac_pair>::value) && ok;
    ok = D_EC_CHECK(internal::has_payload_type<mac_pair>::value)  && ok;
    ok = D_EC_CHECK(
        internal::is_tuple<event_traits<mac_pair>::payload_type>::value
    ) && ok;
    ok = D_EC_CHECK(event_traits<mac_pair>::has_name) && ok;

    // the empty variant is equally well-formed, just with no arguments.
    ok = D_EC_CHECK(internal::has_event_payload<mac_empty>::value) && ok;
    ok = D_EC_CHECK(!event_traits<mac_empty>::has_args)            && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
