/*******************************************************************************
* djinterp [test]                               event_common_tests_detection.cpp
*
*   Section III -- EVENT TAG DETECTION.  Covers the internal detection traits
* that classify a candidate event tag: has_payload_type / has_args_type (the
* canonical and legacy payload spellings), has_event_payload (either), the
* event_payload picker (canonical preferred, legacy fallback), has_event_name
* (a static name() returning const char*), and is_tuple.  Each trait is probed
* with a positive case, a negative case, and -- where it matters -- a
* cv/reference-qualified case to confirm clean_t strips qualifiers.
*
*
* path:      /tests/djinterp/cpp/event/event_common_tests_detection.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.29
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "event_common_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_has_payload_type
bool tests_has_payload_type()
{
    bool ok = true;

    // positive: the canonical `payload_type` spelling is present.
    ok = D_EC_CHECK(internal::has_payload_type<DEvUnary>::value) && ok;
    ok = D_EC_CHECK(internal::has_payload_type<DEvEmpty>::value) && ok;
    ok = D_EC_CHECK(internal::has_payload_type<DEvBoth>::value) && ok;

    // negative: legacy-only and non-event tags have no `payload_type`.
    ok = D_EC_CHECK(!internal::has_payload_type<DEvLegacy>::value) && ok;
    ok = D_EC_CHECK(!internal::has_payload_type<DEvPlain>::value) && ok;

    // cv/reference qualifiers are stripped before detection.
    ok = D_EC_CHECK(
        internal::has_payload_type<const DEvUnary&>::value
    ) && ok;

    return ok;
}


// tests_has_args_type
bool tests_has_args_type()
{
    bool ok = true;

    // positive: the legacy `args_type` spelling is present.
    ok = D_EC_CHECK(internal::has_args_type<DEvLegacy>::value) && ok;
    ok = D_EC_CHECK(internal::has_args_type<DEvBoth>::value) && ok;

    // negative: canonical-only and non-event tags have no `args_type`.
    ok = D_EC_CHECK(!internal::has_args_type<DEvUnary>::value) && ok;
    ok = D_EC_CHECK(!internal::has_args_type<DEvPlain>::value) && ok;

    // cv/reference qualifiers are stripped before detection.
    ok = D_EC_CHECK(
        internal::has_args_type<volatile DEvLegacy>::value
    ) && ok;

    return ok;
}


// tests_has_event_payload
bool tests_has_event_payload()
{
    bool ok = true;

    // a payload under either spelling satisfies has_event_payload.
    ok = D_EC_CHECK(internal::has_event_payload<DEvUnary>::value) && ok;
    ok = D_EC_CHECK(internal::has_event_payload<DEvLegacy>::value) && ok;
    ok = D_EC_CHECK(internal::has_event_payload<DEvBoth>::value) && ok;
    ok = D_EC_CHECK(internal::has_event_payload<DEvEmpty>::value) && ok;

    // a tag with neither spelling does not.
    ok = D_EC_CHECK(!internal::has_event_payload<DEvPlain>::value) && ok;

    return ok;
}


// tests_event_payload_select
bool tests_event_payload_select()
{
    bool ok = true;

    // canonical spelling selected when present.
    ok = D_EC_CHECK(
        (std::is_same<
            internal::event_payload<DEvUnary>::type,
            std::tuple<int>
        >::value)
    ) && ok;

    // legacy spelling selected when canonical is absent.
    ok = D_EC_CHECK(
        (std::is_same<
            internal::event_payload<DEvLegacy>::type,
            std::tuple<int, char>
        >::value)
    ) && ok;

    // canonical preferred over legacy when both are present.
    ok = D_EC_CHECK(
        (std::is_same<
            internal::event_payload<DEvBoth>::type,
            std::tuple<int>
        >::value)
    ) && ok;

    // qualifiers stripped before selection.
    ok = D_EC_CHECK(
        (std::is_same<
            internal::event_payload<const DEvUnary&>::type,
            std::tuple<int>
        >::value)
    ) && ok;

    return ok;
}


// tests_has_event_name
bool tests_has_event_name()
{
    bool ok = true;

    // positive: a static name() returning const char*.
    ok = D_EC_CHECK(internal::has_event_name<DEvUnary>::value) && ok;

    // negative: no name() member at all.
    ok = D_EC_CHECK(!internal::has_event_name<DEvUnnamed>::value) && ok;

    // negative: name() exists but returns the wrong type (not const char*).
    ok = D_EC_CHECK(!internal::has_event_name<DEvBadname>::value) && ok;

    // qualifiers stripped before detection.
    ok = D_EC_CHECK(internal::has_event_name<const DEvUnary&>::value) && ok;

    return ok;
}


// tests_is_tuple
bool tests_is_tuple()
{
    bool ok = true;

    // positive: any std::tuple specialization, including the empty tuple.
    ok = D_EC_CHECK(internal::is_tuple<std::tuple<> >::value) && ok;
    ok = D_EC_CHECK(internal::is_tuple<std::tuple<int, char> >::value) && ok;

    // negative: scalars and other tuple-like aggregates are not std::tuple.
    ok = D_EC_CHECK(!internal::is_tuple<int>::value) && ok;
    ok = D_EC_CHECK(!internal::is_tuple<std::pair<int, int> >::value) && ok;
    ok = D_EC_CHECK(!internal::is_tuple<std::array<int, 3> >::value) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
