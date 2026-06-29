/******************************************************************************
* djinterp [test]                                event_handler_tests_traits.cpp
*
*   Sections III + IV -- HANDLER COMPATIBILITY DETECTION and HANDLER TRAITS.
* Covers the internal detection traits (handler_invoke_result: invocable vs
* not, and the extracted return type; handler_nothrow_helper: noexcept vs not)
* through the public handler_traits facade.  Exercises is_invocable (with both
* too-few and too-many argument mismatches driving the not-invocable path),
* return_type extraction, the returns_void / returns_verdict split,
* is_compatible (invocable and result void-or-verdict), is_nothrow, the
* event-derived expected_arity, and cv/reference cleaning of both the callable
* and the event operand.
*
*
* path:      /tests/djinterp/core/event/event_handler/event_handler_tests_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

// djinterp
#include "event_handler_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_handler_traits_is_invocable
bool
tests_handler_traits_is_invocable()
{
    bool ok = true;

    // a callable accepting exactly the payload's value domains is invocable.
    ok = D_EH_CHECK(
        (handler_traits<h_void_unary, ev_unary>::is_invocable)
    ) && ok;
    ok = D_EH_CHECK(
        (handler_traits<h_void_binary, ev_binary>::is_invocable)
    ) && ok;
    ok = D_EH_CHECK(
        (handler_traits<h_void_nullary, ev_empty>::is_invocable)
    ) && ok;

    // too many parameters for the payload: not invocable.
    ok = D_EH_CHECK(
        (!handler_traits<h_void_binary, ev_unary>::is_invocable)
    ) && ok;

    // too few parameters for the payload: not invocable.
    ok = D_EH_CHECK(
        (!handler_traits<h_void_nullary, ev_unary>::is_invocable)
    ) && ok;

    return ok;
}


// tests_handler_traits_return_type
bool
tests_handler_traits_return_type()
{
    bool ok = true;

    // the callable's return type is extracted when invocable...
    ok = D_EH_CHECK(
        (std::is_same<
            handler_traits<h_void_unary, ev_unary>::return_type, void
        >::value)
    ) && ok;
    ok = D_EH_CHECK(
        (std::is_same<
            handler_traits<h_pass_unary, ev_unary>::return_type, verdict
        >::value)
    ) && ok;
    ok = D_EH_CHECK(
        (std::is_same<
            handler_traits<h_int_unary, ev_unary>::return_type, int
        >::value)
    ) && ok;

    // ...and defaults to void when the callable is not invocable.
    ok = D_EH_CHECK(
        (std::is_same<
            handler_traits<h_void_binary, ev_unary>::return_type, void
        >::value)
    ) && ok;

    return ok;
}


// tests_handler_traits_returns_void_verdict
bool
tests_handler_traits_returns_void_verdict()
{
    bool ok = true;

    // a void handler reports returns_void and not returns_verdict.
    ok = D_EH_CHECK(
        (handler_traits<h_void_unary, ev_unary>::returns_void)
    ) && ok;
    ok = D_EH_CHECK(
        (!handler_traits<h_void_unary, ev_unary>::returns_verdict)
    ) && ok;

    // a verdict handler reports the opposite.
    ok = D_EH_CHECK(
        (handler_traits<h_pass_unary, ev_unary>::returns_verdict)
    ) && ok;
    ok = D_EH_CHECK(
        (!handler_traits<h_pass_unary, ev_unary>::returns_void)
    ) && ok;

    // a foreign return type reports neither.
    ok = D_EH_CHECK(
        (!handler_traits<h_int_unary, ev_unary>::returns_void)
    ) && ok;
    ok = D_EH_CHECK(
        (!handler_traits<h_int_unary, ev_unary>::returns_verdict)
    ) && ok;

    return ok;
}


// tests_handler_traits_is_compatible
bool
tests_handler_traits_is_compatible()
{
    bool ok = true;

    // compatible == invocable and result is void or verdict.
    ok = D_EH_CHECK(
        (handler_traits<h_void_unary, ev_unary>::is_compatible)
    ) && ok;
    ok = D_EH_CHECK(
        (handler_traits<h_pass_unary, ev_unary>::is_compatible)
    ) && ok;
    ok = D_EH_CHECK(
        (handler_traits<h_void_nullary, ev_empty>::is_compatible)
    ) && ok;

    // invocable but the wrong return type is not compatible.
    ok = D_EH_CHECK(
        (!handler_traits<h_int_unary, ev_unary>::is_compatible)
    ) && ok;

    // not invocable is not compatible.
    ok = D_EH_CHECK(
        (!handler_traits<h_void_binary, ev_unary>::is_compatible)
    ) && ok;

    return ok;
}


// tests_handler_traits_is_nothrow
bool
tests_handler_traits_is_nothrow()
{
    bool ok = true;

    // a noexcept invocation is detected...
    ok = D_EH_CHECK(
        (handler_traits<h_noexcept_unary, ev_unary>::is_nothrow)
    ) && ok;

    // ...a potentially-throwing invocation is not...
    ok = D_EH_CHECK(
        (!handler_traits<h_throwing_unary, ev_unary>::is_nothrow)
    ) && ok;

    // ...and a non-invocable pairing is not nothrow either.
    ok = D_EH_CHECK(
        (!handler_traits<h_void_binary, ev_unary>::is_nothrow)
    ) && ok;

    return ok;
}


// tests_handler_traits_expected_arity
bool
tests_handler_traits_expected_arity()
{
    bool ok = true;

    // expected_arity is the event's payload width, independent of the callable.
    ok = D_EH_CHECK(
        (handler_traits<h_void_nullary, ev_empty>::expected_arity == 0u)
    ) && ok;
    ok = D_EH_CHECK(
        (handler_traits<h_void_unary, ev_unary>::expected_arity == 1u)
    ) && ok;
    ok = D_EH_CHECK(
        (handler_traits<h_void_binary, ev_binary>::expected_arity == 2u)
    ) && ok;
    ok = D_EH_CHECK(
        (handler_traits<h_void_ternary, ev_ternary>::expected_arity == 3u)
    ) && ok;
    ok = D_EH_CHECK(
        (handler_traits<h_void_quaternary, ev_quaternary>::expected_arity == 4u)
    ) && ok;

    return ok;
}


// tests_handler_traits_cvref
bool
tests_handler_traits_cvref()
{
    bool ok = true;

    // cv/reference qualifiers are stripped from both the callable and the
    // event operand before the traits are computed.
    ok = D_EH_CHECK(
        (handler_traits<const h_void_unary&, const ev_unary&>::is_compatible)
    ) && ok;
    ok = D_EH_CHECK(
        (handler_traits<h_pass_unary&&, ev_unary>::returns_verdict)
    ) && ok;
    ok = D_EH_CHECK(
        (handler_traits<h_void_unary, const ev_binary&>::expected_arity == 2u)
    ) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
