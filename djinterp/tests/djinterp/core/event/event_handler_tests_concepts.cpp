/******************************************************************************
* djinterp [test]                              event_handler_tests_concepts.cpp
*
*   Section VI -- CONCEPT CONSTRAINTS (C++20+).  Covers the handler concept
* layer: is_handler / handler_for, the void vs verdict return split, the
* nothrow / throwing split, handler_for_event_of_arity and its
* nullary..ternary aliases plus variadic_handler_for.  Each concept is checked
* as a boolean over positive and negative operands, and -- crucially -- over a
* non-event operand to confirm every derived concept short-circuits on event
* identity and never instantiates handler_traits for a non-event (which would
* fire event_traits' static_assert at compile time).
*
*   The whole translation unit is gated on D_ENV_CPP_FEATURE_LANG_CONCEPTS.
* Where concepts are unavailable (pre-C++20) the concept layer of the header
* is omitted, so the tests compile to a vacuous pass and the suite stays
* portable across C++11..C++23.
*
*
* path:      /tests/djinterp/core/event/event_handler/event_handler_tests_concepts.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

// djinterp
#include "event_handler_tests.hpp"


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP
NS_TESTING


// tests_concept_is_handler
bool
tests_concept_is_handler()
{
    bool ok = true;

    // a compatible callable for an event satisfies is_handler...
    ok = D_EH_CHECK(is_handler<h_void_unary, ev_unary>) && ok;
    ok = D_EH_CHECK(is_handler<h_pass_unary, ev_unary>) && ok;

    // ...a foreign return type does not...
    ok = D_EH_CHECK(!is_handler<h_int_unary, ev_unary>) && ok;

    // ...nor does an arity mismatch...
    ok = D_EH_CHECK(!is_handler<h_void_binary, ev_unary>) && ok;

    // ...and a non-event short-circuits to false (no handler_traits
    // instantiation, hence no static_assert).
    ok = D_EH_CHECK(!is_handler<h_void_unary, ev_plain>) && ok;

    // cv/reference qualifiers on the event operand are cleaned.
    ok = D_EH_CHECK(is_handler<h_void_unary, const ev_unary&>) && ok;

    return ok;
}


// tests_concept_handler_for
bool
tests_concept_handler_for()
{
    bool ok = true;

    // handler_for is the readable spelling of is_handler.
    ok = D_EH_CHECK(handler_for<h_void_unary, ev_unary>) && ok;
    ok = D_EH_CHECK(handler_for<h_pass_binary, ev_binary>) && ok;
    ok = D_EH_CHECK(!handler_for<h_int_unary, ev_unary>) && ok;

    return ok;
}


// tests_concept_void_verdict_handler_for
bool
tests_concept_void_verdict_handler_for()
{
    bool ok = true;

    // void_handler_for selects always-pass (void-returning) handlers.
    ok = D_EH_CHECK(void_handler_for<h_void_unary, ev_unary>) && ok;
    ok = D_EH_CHECK(!void_handler_for<h_pass_unary, ev_unary>) && ok;

    // verdict_handler_for selects explicit-verdict handlers.
    ok = D_EH_CHECK(verdict_handler_for<h_pass_unary, ev_unary>) && ok;
    ok = D_EH_CHECK(!verdict_handler_for<h_void_unary, ev_unary>) && ok;

    return ok;
}


// tests_concept_nothrow_handlers
bool
tests_concept_nothrow_handlers()
{
    bool ok = true;

    // the nothrow / throwing split over compatible handlers.
    ok = D_EH_CHECK(is_nothrow_handler<h_noexcept_unary, ev_unary>) && ok;
    ok = D_EH_CHECK(!is_nothrow_handler<h_throwing_unary, ev_unary>) && ok;

    // nothrow_handler_for is the readable spelling.
    ok = D_EH_CHECK(nothrow_handler_for<h_noexcept_unary, ev_unary>) && ok;

    // throwing_handler_for is the complementary split.
    ok = D_EH_CHECK(throwing_handler_for<h_throwing_unary, ev_unary>) && ok;
    ok = D_EH_CHECK(!throwing_handler_for<h_noexcept_unary, ev_unary>) && ok;

    return ok;
}


// tests_concept_handler_for_event_of_arity
bool
tests_concept_handler_for_event_of_arity()
{
    bool ok = true;

    // exact-arity match against the event's payload width.
    ok = D_EH_CHECK((handler_for_event_of_arity<h_void_binary, ev_binary, 2>))
         && ok;
    ok = D_EH_CHECK((!handler_for_event_of_arity<h_void_binary, ev_binary, 3>))
         && ok;
    ok = D_EH_CHECK((handler_for_event_of_arity<h_void_nullary, ev_empty, 0>))
         && ok;

    return ok;
}


// tests_concept_arity_aliases
bool
tests_concept_arity_aliases()
{
    bool ok = true;

    // each alias pins one arity and excludes its neighbours.
    ok = D_EH_CHECK(nullary_handler_for<h_void_nullary, ev_empty>) && ok;
    ok = D_EH_CHECK(!nullary_handler_for<h_void_unary, ev_unary>) && ok;

    ok = D_EH_CHECK(unary_handler_for<h_void_unary, ev_unary>) && ok;
    ok = D_EH_CHECK(!unary_handler_for<h_void_nullary, ev_empty>) && ok;

    ok = D_EH_CHECK(binary_handler_for<h_void_binary, ev_binary>) && ok;
    ok = D_EH_CHECK(ternary_handler_for<h_void_ternary, ev_ternary>) && ok;

    // variadic covers arity four or more, excluding the ternary boundary.
    ok = D_EH_CHECK(variadic_handler_for<h_void_quaternary, ev_quaternary>)
         && ok;
    ok = D_EH_CHECK(!variadic_handler_for<h_void_ternary, ev_ternary>) && ok;

    return ok;
}


// tests_concept_non_event_safety
bool
tests_concept_non_event_safety()
{
    bool ok = true;

    // every derived handler concept must short-circuit to false on a
    // non-event operand without instantiating handler_traits (this whole
    // function failing to compile would itself be the failure mode).
    ok = D_EH_CHECK(!is_handler<h_void_unary, ev_plain>) && ok;
    ok = D_EH_CHECK(!handler_for<h_void_unary, ev_plain>) && ok;
    ok = D_EH_CHECK(!void_handler_for<h_void_unary, ev_plain>) && ok;
    ok = D_EH_CHECK(!verdict_handler_for<h_pass_unary, ev_plain>) && ok;
    ok = D_EH_CHECK(!is_nothrow_handler<h_noexcept_unary, ev_plain>) && ok;
    ok = D_EH_CHECK(!nothrow_handler_for<h_noexcept_unary, ev_plain>) && ok;
    ok = D_EH_CHECK(!throwing_handler_for<h_throwing_unary, ev_plain>) && ok;
    ok = D_EH_CHECK(!nullary_handler_for<h_void_nullary, ev_plain>) && ok;
    ok = D_EH_CHECK(!unary_handler_for<h_void_unary, ev_plain>) && ok;
    ok = D_EH_CHECK(!binary_handler_for<h_void_binary, ev_plain>) && ok;
    ok = D_EH_CHECK(!ternary_handler_for<h_void_ternary, ev_plain>) && ok;
    ok = D_EH_CHECK(!variadic_handler_for<h_void_quaternary, ev_plain>) && ok;
    ok = D_EH_CHECK((!handler_for_event_of_arity<h_void_unary, ev_plain, 1>))
         && ok;

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

bool tests_concept_is_handler()                  { return true; }
bool tests_concept_handler_for()                 { return true; }
bool tests_concept_void_verdict_handler_for()    { return true; }
bool tests_concept_nothrow_handlers()            { return true; }
bool tests_concept_handler_for_event_of_arity()  { return true; }
bool tests_concept_arity_aliases()               { return true; }
bool tests_concept_non_event_safety()            { return true; }


NS_END  // testing
NS_END  // djinterp


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS
