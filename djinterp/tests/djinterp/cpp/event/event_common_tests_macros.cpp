/*******************************************************************************
* djinterp [test]                                  event_common_tests_macros.cpp
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
* path:      /tests/djinterp/cpp/event/event_common_tests_macros.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.29
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "event_common_tests.hpp"


NS_DJINTERP
NS_TESTING


namespace
{
    // DMacEmpty
    //   fixture: macro-declared event tag with an empty payload.
    D_EVENT_EMPTY(DMacEmpty);

    // DMacUnary
    //   fixture: macro-declared event tag with a one-element payload.
    D_EVENT(DMacUnary, int);

    // DMacPair
    //   fixture: macro-declared event tag with a two-element payload.
    D_EVENT(DMacPair, int, double);

    // DMacTriple
    //   fixture: macro-declared event tag with a three-element payload.
    D_EVENT(DMacTriple, int, double, char);

    // DMacQuad
    //   fixture: macro-declared event tag with a four-element payload.
    D_EVENT(DMacQuad, int, int, int, int);
}


// tests_d_event_payload
bool tests_d_event_payload()
{
    bool ok = true;

    // D_EVENT synthesizes payload_type as a std::tuple of the given domains,
    // in declaration order.
    ok = D_EC_CHECK(
        (std::is_same<
            DMacPair::payload_type,
            std::tuple<int, double>
        >::value)
    ) && ok;
    ok = D_EC_CHECK(
        (std::is_same<
            DMacTriple::payload_type,
            std::tuple<int, double, char>
        >::value)
    ) && ok;

    return ok;
}


// tests_d_event_name
bool tests_d_event_name()
{
    bool ok = true;

    // D_EVENT emits a static name() returning the stringized tag name.
    ok = D_EC_CHECK(
        (std::is_same<decltype(DMacPair::name()), const char*>::value)
    ) && ok;
    ok = D_EC_CHECK(std::strcmp(DMacPair::name(), "mac_pair") == 0) && ok;
    ok = D_EC_CHECK(std::strcmp(DMacUnary::name(), "mac_unary") == 0) && ok;

    return ok;
}


// tests_d_event_empty
bool tests_d_event_empty()
{
    bool ok = true;

    // D_EVENT_EMPTY synthesizes an empty payload tuple...
    ok = D_EC_CHECK(
        (std::is_same<DMacEmpty::payload_type, std::tuple<> >::value)
    ) && ok;

    // ...while still emitting the named-tag name() member.
    ok = D_EC_CHECK(std::strcmp(DMacEmpty::name(), "mac_empty") == 0) && ok;

    return ok;
}


// tests_d_event_arity_range
bool tests_d_event_arity_range()
{
    bool ok = true;

    // the payload width seen through event_traits matches the number of
    // domains passed to the macro, from zero through four.
    ok = D_EC_CHECK(event_traits<DMacEmpty>::arity == 0u)  && ok;
    ok = D_EC_CHECK(event_traits<DMacUnary>::arity == 1u)  && ok;
    ok = D_EC_CHECK(event_traits<DMacPair>::arity == 2u)   && ok;
    ok = D_EC_CHECK(event_traits<DMacTriple>::arity == 3u) && ok;
    ok = D_EC_CHECK(event_traits<DMacQuad>::arity == 4u)   && ok;

    return ok;
}


// tests_d_event_is_event
bool tests_d_event_is_event()
{
    bool ok = true;

    // a macro-declared tag is a well-formed event: it carries a payload, that
    // payload is a std::tuple, and (for D_EVENT) it is named.
    ok = D_EC_CHECK(internal::has_event_payload<DMacPair>::value) && ok;
    ok = D_EC_CHECK(internal::has_payload_type<DMacPair>::value)  && ok;
    ok = D_EC_CHECK(
        internal::is_tuple<event_traits<DMacPair>::payload_type>::value
    ) && ok;
    ok = D_EC_CHECK(event_traits<DMacPair>::has_name) && ok;

    // the empty variant is equally well-formed, just with no arguments.
    ok = D_EC_CHECK(internal::has_event_payload<DMacEmpty>::value) && ok;
    ok = D_EC_CHECK(!event_traits<DMacEmpty>::has_args)            && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
