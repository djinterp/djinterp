/******************************************************************************
* djinterp [test]                                     applicative_tests_ap.cpp
*
*   Section II.2 of the applicative.hpp suite: ap, wrapped application
*   F<a -> b> -> F<a> -> F<b> (Haskell <*>), delegating to
* applicative_traits<F>::ap keyed on the wrapped-function context.  Covers the
* basic application, the maybe-style short-circuit for the bridge applicative (a
* nothing on either or both sides propagates), a wrapped function that changes
* the value type, the direct applicative (total, no short-circuit), the
* lvalue/rvalue forwarding paths, and the result type identity.
*
* path:      /tests/djinterp/core/functional/applicative_tests_ap.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "applicative_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///                ap OVER THE BRIDGE APPLICATIVE  (box)                     ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_ap_box_apply
  Tests the following:
  - a wrapped function applied to a wrapped argument yields the wrapped result:
    just(times2) <*> just(21) == just(42).
*/
static bool
tests_ap_box_apply()
{
    const box<int> r = ::djinterp::ap(box<times2>(times2()), box<int>(21));

    return (r == box<int>(42));
}

/*
tests_ap_box_nothing_fn
  Tests the following:
  - short-circuit on the function side: nothing <*> just(21) == nothing.
*/
static bool
tests_ap_box_nothing_fn()
{
    const box<int> r = ::djinterp::ap(box<times2>(), box<int>(21));

    return (r == box<int>());
}

/*
tests_ap_box_nothing_arg
  Tests the following:
  - short-circuit on the argument side: just(times2) <*> nothing == nothing.
*/
static bool
tests_ap_box_nothing_arg()
{
    const box<int> r = ::djinterp::ap(box<times2>(times2()), box<int>());

    return (r == box<int>());
}

/*
tests_ap_box_both_nothing
  Tests the following:
  - short-circuit with both sides empty: nothing <*> nothing == nothing.
*/
static bool
tests_ap_box_both_nothing()
{
    const box<int> r = ::djinterp::ap(box<times2>(), box<int>());

    return (r == box<int>());
}

/*
tests_ap_box_type_changing
  Tests the following:
  - the wrapped function may change the value type: just(to_text) <*> just(7)
    yields a box<std::string> holding "7".
*/
static bool
tests_ap_box_type_changing()
{
    const box<std::string> r =
        ::djinterp::ap(box<to_text>(to_text()), box<int>(7));

    return (r == box<std::string>("7"));
}

/*
tests_ap_box_second_fn
  Tests the following:
  - a different wrapped function: just(square) <*> just(4) == just(16).
*/
static bool
tests_ap_box_second_fn()
{
    const box<int> r = ::djinterp::ap(box<square>(square()), box<int>(4));

    return (r == box<int>(16));
}

/*
tests_ap_box_forwarding
  Tests the following:
  - ap accepts both named lvalue operands and rvalue (temporary) operands.
*/
static bool
tests_ap_box_forwarding()
{
    box<times2> ff = box<times2>(times2());
    box<int>    fa = box<int>(21);

    const box<int> r_lvalue = ::djinterp::ap(ff, fa);
    const box<int> r_rvalue = ::djinterp::ap(box<times2>(times2()), box<int>(21));

    return ( (r_lvalue == box<int>(42)) &&
             (r_rvalue == box<int>(42)) );
}

/*
tests_ap_box_result_type
  Tests the following:
  - the result type of ap over box is exactly box<U>.
*/
static bool
tests_ap_box_result_type()
{
    return std::is_same<
        decltype(::djinterp::ap(box<times2>(times2()), box<int>(1))),
        box<int> >::value;
}


///////////////////////////////////////////////////////////////////////////////
///                ap OVER THE DIRECT APPLICATIVE  (ident)                   ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_ap_ident_apply
  Tests the following:
  - ap over the direct (total) applicative applies the wrapped function:
    ident(times2) <*> ident(5) == ident(10).
*/
static bool
tests_ap_ident_apply()
{
    const ident<int> r =
        ::djinterp::ap(ident<times2>(times2()), ident<int>(5));

    return (r == ident<int>(10));
}

/*
tests_ap_ident_type_changing
  Tests the following:
  - the wrapped function may change the value type over the direct applicative:
    ident(to_text) <*> ident(9) == ident("9").
*/
static bool
tests_ap_ident_type_changing()
{
    const ident<std::string> r =
        ::djinterp::ap(ident<to_text>(to_text()), ident<int>(9));

    return (r == ident<std::string>("9"));
}


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
applicative_ap_block()
{
    dt::block_spec block;

    block.name       = "II.2 ap";
    block.descriptor =
        "wrapped application, short-circuit, type change, forwarding";

    block.tests.push_back(dt::test_spec{
        "box: apply",
        "just(f) <*> just(a) == just(f a)",
        &tests_ap_box_apply });

    block.tests.push_back(dt::test_spec{
        "box: short-circuit function",
        "nothing <*> just(a) == nothing",
        &tests_ap_box_nothing_fn });

    block.tests.push_back(dt::test_spec{
        "box: short-circuit argument",
        "just(f) <*> nothing == nothing",
        &tests_ap_box_nothing_arg });

    block.tests.push_back(dt::test_spec{
        "box: both empty",
        "nothing <*> nothing == nothing",
        &tests_ap_box_both_nothing });

    block.tests.push_back(dt::test_spec{
        "box: type-changing function",
        "just(int->string) <*> just(7) == just(\"7\")",
        &tests_ap_box_type_changing });

    block.tests.push_back(dt::test_spec{
        "box: second function",
        "just(square) <*> just(4) == just(16)",
        &tests_ap_box_second_fn });

    block.tests.push_back(dt::test_spec{
        "box: forwarding",
        "lvalue and rvalue operands",
        &tests_ap_box_forwarding });

    block.tests.push_back(dt::test_spec{
        "box: result type identity",
        "decltype(ap(...)) is box<U>",
        &tests_ap_box_result_type });

    block.tests.push_back(dt::test_spec{
        "ident: apply",
        "direct (total) applicative applies the function",
        &tests_ap_ident_apply });

    block.tests.push_back(dt::test_spec{
        "ident: type-changing function",
        "direct applicative may change the value type",
        &tests_ap_ident_type_changing });

    return block;
}


NS_END  // testing
NS_END  // djinterp
