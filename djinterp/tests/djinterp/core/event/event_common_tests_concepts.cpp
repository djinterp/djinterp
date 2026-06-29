/******************************************************************************
* djinterp [test]                               event_common_tests_concepts.cpp
*
*   Section VI -- CONCEPT CONSTRAINTS (C++20+).  Covers the concept layer that
* classifies event tags: is_event, event_type / non_event_type, the
* empty/argument split, the named/unnamed split, event_of_arity and its
* nullary..ternary aliases, and variadic_event_type.  Each concept is checked
* as a boolean value over positive, negative, and cv/reference-qualified
* operands, and -- crucially -- over a non-event operand to confirm the
* derived concepts short-circuit on event_type and never instantiate
* event_traits for a non-event (which would fire its static_assert).
*
*   The whole translation unit is gated on D_ENV_CPP_FEATURE_LANG_CONCEPTS.
* Where concepts are unavailable (pre-C++20) the concept layer of the header
* is omitted, so the tests compile to a vacuous pass and the suite stays
* portable across C++11..C++23.
*
*
* path:      /tests/djinterp/core/event/event_common/event_common_tests_concepts.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

// djinterp
#include "event_common_tests.hpp"


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP
NS_TESTING


// tests_concept_is_event
bool
tests_concept_is_event()
{
    bool ok = true;

    // is_event holds for a tag with a tuple payload under either spelling...
    ok = D_EC_CHECK(is_event<ev_unary>) && ok;
    ok = D_EC_CHECK(is_event<ev_empty>) && ok;
    ok = D_EC_CHECK(is_event<ev_legacy>) && ok;
    ok = D_EC_CHECK(is_event<ev_both>) && ok;

    // ...and fails for a non-event, without tripping any static_assert.
    ok = D_EC_CHECK(!is_event<ev_plain>) && ok;

    return ok;
}


// tests_concept_event_type
bool
tests_concept_event_type()
{
    bool ok = true;

    // event_type is is_event composed with clean_t, so it strips qualifiers.
    ok = D_EC_CHECK(event_type<ev_unary>) && ok;
    ok = D_EC_CHECK(event_type<const ev_unary&>) && ok;
    ok = D_EC_CHECK(event_type<ev_unary&&>) && ok;

    // a non-event remains a non-event after cleaning.
    ok = D_EC_CHECK(!event_type<ev_plain>) && ok;

    return ok;
}


// tests_concept_non_event_type
bool
tests_concept_non_event_type()
{
    bool ok = true;

    // non_event_type is the negation of event_type, qualifiers and all.
    ok = D_EC_CHECK(non_event_type<ev_plain>) && ok;
    ok = D_EC_CHECK(non_event_type<const ev_plain>) && ok;
    ok = D_EC_CHECK(non_event_type<ev_plain&>) && ok;
    ok = D_EC_CHECK(!non_event_type<ev_unary>) && ok;

    return ok;
}


// tests_concept_empty_event_type
bool
tests_concept_empty_event_type()
{
    bool ok = true;

    // empty_event_type holds only for events whose payload is empty.
    ok = D_EC_CHECK(empty_event_type<ev_empty>) && ok;
    ok = D_EC_CHECK(!empty_event_type<ev_unary>) && ok;

    // on a non-event it must short-circuit to false (no event_traits
    // instantiation).
    ok = D_EC_CHECK(!empty_event_type<ev_plain>) && ok;

    return ok;
}


// tests_concept_argument_event_type
bool
tests_concept_argument_event_type()
{
    bool ok = true;

    // argument_event_type is the complementary split: events with a payload.
    ok = D_EC_CHECK(argument_event_type<ev_unary>) && ok;
    ok = D_EC_CHECK(argument_event_type<ev_quaternary>) && ok;
    ok = D_EC_CHECK(!argument_event_type<ev_empty>) && ok;

    // short-circuit safe on a non-event.
    ok = D_EC_CHECK(!argument_event_type<ev_plain>) && ok;

    return ok;
}


// tests_concept_named_event_type
bool
tests_concept_named_event_type()
{
    bool ok = true;

    // named_event_type holds for events exposing a conforming name().
    ok = D_EC_CHECK(named_event_type<ev_unary>) && ok;
    ok = D_EC_CHECK(!named_event_type<ev_unnamed>) && ok;

    // short-circuit safe on a non-event.
    ok = D_EC_CHECK(!named_event_type<ev_plain>) && ok;

    return ok;
}


// tests_concept_unnamed_event_type
bool
tests_concept_unnamed_event_type()
{
    bool ok = true;

    // unnamed_event_type is the complementary split: events without a name().
    ok = D_EC_CHECK(unnamed_event_type<ev_unnamed>) && ok;
    ok = D_EC_CHECK(!unnamed_event_type<ev_unary>) && ok;

    // short-circuit safe on a non-event.
    ok = D_EC_CHECK(!unnamed_event_type<ev_plain>) && ok;

    return ok;
}


// tests_concept_event_of_arity
bool
tests_concept_event_of_arity()
{
    bool ok = true;

    // event_of_arity<T, N> holds when the event's payload width is exactly N.
    ok = D_EC_CHECK((event_of_arity<ev_binary, 2>)) && ok;
    ok = D_EC_CHECK((!event_of_arity<ev_binary, 3>)) && ok;
    ok = D_EC_CHECK((event_of_arity<ev_empty, 0>)) && ok;

    // qualifiers are cleaned before the arity is read.
    ok = D_EC_CHECK((event_of_arity<const ev_binary&, 2>)) && ok;

    // short-circuit safe on a non-event.
    ok = D_EC_CHECK((!event_of_arity<ev_plain, 0>)) && ok;

    return ok;
}


// tests_concept_nullary_unary
bool
tests_concept_nullary_unary()
{
    bool ok = true;

    // the arity aliases pin a single width and exclude their neighbours.
    ok = D_EC_CHECK(nullary_event_type<ev_empty>) && ok;
    ok = D_EC_CHECK(!nullary_event_type<ev_unary>) && ok;

    ok = D_EC_CHECK(unary_event_type<ev_unary>) && ok;
    ok = D_EC_CHECK(!unary_event_type<ev_empty>) && ok;
    ok = D_EC_CHECK(!unary_event_type<ev_binary>) && ok;

    // short-circuit safe on a non-event.
    ok = D_EC_CHECK(!nullary_event_type<ev_plain>) && ok;
    ok = D_EC_CHECK(!unary_event_type<ev_plain>) && ok;

    return ok;
}


// tests_concept_binary_ternary
bool
tests_concept_binary_ternary()
{
    bool ok = true;

    // continuing the arity ladder for widths two and three.
    ok = D_EC_CHECK(binary_event_type<ev_binary>) && ok;
    ok = D_EC_CHECK(!binary_event_type<ev_unary>) && ok;
    ok = D_EC_CHECK(!binary_event_type<ev_ternary>) && ok;

    ok = D_EC_CHECK(ternary_event_type<ev_ternary>) && ok;
    ok = D_EC_CHECK(!ternary_event_type<ev_binary>) && ok;
    ok = D_EC_CHECK(!ternary_event_type<ev_quaternary>) && ok;

    // short-circuit safe on a non-event.
    ok = D_EC_CHECK(!binary_event_type<ev_plain>) && ok;
    ok = D_EC_CHECK(!ternary_event_type<ev_plain>) && ok;

    return ok;
}


// tests_concept_variadic_event_type
bool
tests_concept_variadic_event_type()
{
    bool ok = true;

    // variadic_event_type holds for events of arity four or more, and so
    // excludes the ternary boundary just below it.
    ok = D_EC_CHECK(variadic_event_type<ev_quaternary>) && ok;
    ok = D_EC_CHECK(!variadic_event_type<ev_ternary>) && ok;
    ok = D_EC_CHECK(!variadic_event_type<ev_empty>) && ok;

    // short-circuit safe on a non-event.
    ok = D_EC_CHECK(!variadic_event_type<ev_plain>) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp


#else   // !D_ENV_CPP_FEATURE_LANG_CONCEPTS -- concept layer absent


NS_DJINTERP
NS_TESTING


// On standards without concepts the header omits the concept layer entirely,
// so these tests have nothing to bind to and pass vacuously.  The declarations
// are kept (matching the header) so the runner links uniformly on every
// standard.

bool tests_concept_is_event()             { return true; }
bool tests_concept_event_type()           { return true; }
bool tests_concept_non_event_type()       { return true; }
bool tests_concept_empty_event_type()     { return true; }
bool tests_concept_argument_event_type()  { return true; }
bool tests_concept_named_event_type()     { return true; }
bool tests_concept_unnamed_event_type()   { return true; }
bool tests_concept_event_of_arity()       { return true; }
bool tests_concept_nullary_unary()        { return true; }
bool tests_concept_binary_ternary()       { return true; }
bool tests_concept_variadic_event_type()  { return true; }


NS_END  // testing
NS_END  // djinterp


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS
