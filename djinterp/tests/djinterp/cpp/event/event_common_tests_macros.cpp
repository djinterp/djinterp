/*******************************************************************************
* djinterp [cpp][event]                            event_common_tests_macros.cpp
*
*   Section V - DECLARATION MACROS.  Covers D_EVENT and D_EVENT_EMPTY: the
* payload_type tuple they synthesize, the static name() member they emit (the
* declared name, checked with std::strcmp), the empty-payload variant, the
* arity range produced across zero through four payload domains, and the fact
* that a macro-declared tag is a well-formed event as seen by the detection
* traits and event_traits. Also that C's D_EVENT_DECLARE and C++'s D_EVENT,
* given one name, agree on the name and the key, whatever the tag is called
* (the event core's decision 15).
*
*   The fixtures exercised here are declared fresh, in a file-local anonymous
* namespace, so the macro expansion is tested at its own call site rather than
* relying on the shared header fixtures.
*
*
* path:      /tests/djinterp/cpp/event/event_common_tests_macros.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.29
*                                                            revised: 2026.11.17
*******************************************************************************/

// djinterp
#include "event_common_tests.hpp"  // the suite's fixtures and checks
#include "../../../../inc/djinterp/c/event/event_common.h"  // C's macros


NS_DJINTERP
NS_TESTING


namespace
{
    // DMacEmpty
    //   fixture: macro-declared event tag with an empty payload.
    D_EVENT_EMPTY(DMacEmpty, "t.event.macros.empty");

    // DMacUnary
    //   fixture: macro-declared event tag with a one-element payload.
    D_EVENT(DMacUnary, "t.event.macros.unary", int);

    // DMacPair
    //   fixture: macro-declared event tag with a two-element payload.
    D_EVENT(DMacPair, "t.event.macros.pair", int, double);

    // DMacTriple
    //   fixture: macro-declared event tag with a three-element payload.
    D_EVENT(DMacTriple, "t.event.macros.triple", int, double, char);

    // DMacQuad
    //   fixture: macro-declared event tag with a four-element payload.
    D_EVENT(DMacQuad, "t.event.macros.quad", int, int, int, int);

    // DMacPairRenamed
    //   fixture: another spelling of DMacPair's event, as after a rename.
    D_EVENT(DMacPairRenamed, "t.event.macros.pair", int, double);

#if D_ENV_PP_HAS_VARIADIC_MACROS
    // mac_pair
    //   fixture: DMacPair's event as C declares it.
    D_EVENT_DECLARE(mac_pair, "t.event.macros.pair", int, double);
#endif

    // mac_empty
    //   fixture: DMacEmpty's event as C declares it.
    D_EVENT_DECLARE_EMPTY(mac_empty, "t.event.macros.empty");

    // a header's own prefix, set after its includes and restored after
    // its events, as cfg_event_common.h shows
#pragma push_macro("D_EVENT_NAMESPACE")
#undef  D_EVENT_NAMESPACE
#define D_EVENT_NAMESPACE "t.event.ns."

    // DMacSpaced
    //   fixture: an event declared under D_EVENT_NAMESPACE.
    D_EVENT(DMacSpaced, "save", int);
    // mac_spaced
    //   fixture: DMacSpaced's event as C declares it, under the same prefix.
    D_EVENT_DECLARE_EMPTY(mac_spaced, "save");
#if D_CFG_EVENT_NAME_FROM_TYPE
    // DMacByType
    //   fixture: an event named by its type's spelling, under the prefix.
    D_EVENT_BY_TYPE(DMacByType, int);
    // DMacByTypeEmpty
    //   fixture: an empty event named by its type's spelling.
    D_EVENT_EMPTY_BY_TYPE(DMacByTypeEmpty);
    // DMacByTypeEmpty
    //   fixture: its C twin, by its handle's spelling.
    D_EVENT_DECLARE_EMPTY_BY_ID(DMacByTypeEmpty);
#endif

#pragma pop_macro("D_EVENT_NAMESPACE")

    // DMacAfter
    //   fixture: an event declared after the prefix was restored.
    D_EVENT_EMPTY(DMacAfter, "t.event.macros.after");
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

    // D_EVENT emits a static name() returning the declared name, not the
    // tag's spelling (the event core's decision 15).
    ok = D_EC_CHECK(
        (std::is_same<decltype(DMacPair::name()), const char*>::value)
    ) && ok;
    ok = D_EC_CHECK(
        std::strcmp(DMacPair::name(), "t.event.macros.pair") == 0
    ) && ok;
    ok = D_EC_CHECK(
        std::strcmp(DMacUnary::name(), "t.event.macros.unary") == 0
    ) && ok;
    ok = D_EC_CHECK(
        std::strcmp(DMacQuad::name(), "t.event.macros.quad") == 0
    ) && ok;

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

    // ...while still emitting its declared name.
    ok = D_EC_CHECK(
        std::strcmp(DMacEmpty::name(), "t.event.macros.empty") == 0
    ) && ok;

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


// tests_d_event_name_agrees
bool tests_d_event_name_agrees()
{
    bool ok = true;

    // a renamed tag keeps its event: the same name, so the same key.
    ok = D_EC_CHECK(
        std::strcmp(DMacPairRenamed::name(), DMacPair::name()) == 0
    ) && ok;
    ok = D_EC_CHECK(
        d_event_key_of_name(DMacPairRenamed::name()) ==
        d_event_key_of_name(DMacPair::name())
    ) && ok;

    // C's declaration of the same name gives C++'s name and key.
#if D_ENV_PP_HAS_VARIADIC_MACROS
    ok = D_EC_CHECK(
        std::strcmp(D_EVENT_NAME(mac_pair), DMacPair::name()) == 0
    ) && ok;
    ok = D_EC_CHECK(
        D_EVENT_KEY(mac_pair) == d_event_key_of_name(DMacPair::name())
    ) && ok;
    ok = D_EC_CHECK(D_EVENT_ARITY(mac_pair) ==
                    event_traits<DMacPair>::arity) && ok;
#endif
    ok = D_EC_CHECK(
        std::strcmp(D_EVENT_NAME(mac_empty), DMacEmpty::name()) == 0
    ) && ok;
    ok = D_EC_CHECK(
        D_EVENT_KEY(mac_empty) == d_event_key_of_name(DMacEmpty::name())
    ) && ok;

    // and two names give two keys.
    ok = D_EC_CHECK(
        d_event_key_of_name(DMacPair::name()) !=
        d_event_key_of_name(DMacEmpty::name())
    ) && ok;

    return ok;
}


// tests_d_event_namespace
bool tests_d_event_namespace()
{
    bool ok = true;

    // the prefix where it stood, in both languages, key and all
    ok = D_EC_CHECK(
        std::strcmp(DMacSpaced::name(), "t.event.ns.save") == 0
    ) && ok;
    ok = D_EC_CHECK(
        std::strcmp(D_EVENT_NAME(mac_spaced), "t.event.ns.save") == 0
    ) && ok;
    ok = D_EC_CHECK(
        D_EVENT_KEY(mac_spaced) == d_event_key_of_name("t.event.ns.save")
    ) && ok;

    // restored after: none, as by default
    ok = D_EC_CHECK(
        std::strcmp(DMacAfter::name(), "t.event.macros.after") == 0
    ) && ok;
    ok = D_EC_CHECK(std::strcmp(D_EVENT_NAMESPACE, "") == 0) && ok;

#if D_CFG_EVENT_NAME_FROM_TYPE
    // a type's spelling, under the prefix it was declared beneath
    ok = D_EC_CHECK(
        std::strcmp(DMacByType::name(), "t.event.ns.DMacByType") == 0
    ) && ok;
    ok = D_EC_CHECK(
        std::strcmp(D_EVENT_NAME(DMacByTypeEmpty),
                    DMacByTypeEmpty::name()) == 0
    ) && ok;
    ok = D_EC_CHECK(event_traits<DMacByType>::arity == 1u) && ok;
#endif

    return ok;
}


NS_END  // testing
NS_END  // djinterp
