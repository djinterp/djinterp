/******************************************************************************
* djinterp [test]                                applicative_tests_lift_a2.cpp
*
*   Section II.3 of the applicative.hpp suite: lift_a2, the applicative binary
* lift derived generically as ap(functor_map(fa, curry2(f)), fb).  Covers the
* basic two-context combination, the maybe-style short-circuit for the bridge
* applicative (either empty side yields empty), the fixed left-biased argument
* order (the first context supplies the first argument, shown with a
* non-commutative reducer), a lift whose two inputs have DIFFERENT types and a
* result type different from both, the direct (total) applicative, and the
* result type identity.  Exercises the internal applicative_a2_curry /
* applicative_a2_binder currying helpers.
*
* path:      /tests/djinterp/core/functional/applicative_tests_lift_a2.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "applicative_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///                lift_a2 OVER THE BRIDGE APPLICATIVE  (box)                ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_lift_a2_box_basic
  Tests the following:
  - two engaged contexts are combined by the binary function: lift_a2(just 2,
    just 3, +) == just 5.
*/
static bool
tests_lift_a2_box_basic()
{
    const box<int> r = ::djinterp::lift_a2(
        box<int>(2),
        box<int>(3),
        [](int _x, int _y) -> int
        {
            return _x + _y;
        });

    return (r == box<int>(5));
}

/*
tests_lift_a2_box_nothing_first
  Tests the following:
  - short-circuit when the first context is empty: lift_a2(nothing, just 3, +)
    == nothing.
*/
static bool
tests_lift_a2_box_nothing_first()
{
    const box<int> r = ::djinterp::lift_a2(
        box<int>(),
        box<int>(3),
        [](int _x, int _y) -> int
        {
            return _x + _y;
        });

    return (r == box<int>());
}

/*
tests_lift_a2_box_nothing_second
  Tests the following:
  - short-circuit when the second context is empty: lift_a2(just 2, nothing, +)
    == nothing.
*/
static bool
tests_lift_a2_box_nothing_second()
{
    const box<int> r = ::djinterp::lift_a2(
        box<int>(2),
        box<int>(),
        [](int _x, int _y) -> int
        {
            return _x + _y;
        });

    return (r == box<int>());
}

/*
tests_lift_a2_box_both_nothing
  Tests the following:
  - short-circuit when both contexts are empty.
*/
static bool
tests_lift_a2_box_both_nothing()
{
    const box<int> r = ::djinterp::lift_a2(
        box<int>(),
        box<int>(),
        [](int _x, int _y) -> int
        {
            return _x + _y;
        });

    return (r == box<int>());
}

/*
tests_lift_a2_box_arg_order
  Tests the following:
  - the lift is left-biased: the FIRST context supplies the FIRST argument.  A
    non-commutative reducer makes this observable: lift_a2(just 10, just 3, -)
    == just 7 (10 - 3), not just(-7).
*/
static bool
tests_lift_a2_box_arg_order()
{
    const box<int> r = ::djinterp::lift_a2(
        box<int>(10),
        box<int>(3),
        [](int _x, int _y) -> int
        {
            return _x - _y;
        });

    return (r == box<int>(7));
}

/*
tests_lift_a2_box_type_mixing
  Tests the following:
  - the result type may differ from the input types: combining two ints with a
    string-producing reducer yields a box<std::string>.
*/
static bool
tests_lift_a2_box_type_mixing()
{
    const box<std::string> r = ::djinterp::lift_a2(
        box<int>(1),
        box<int>(2),
        [](int _x, int _y) -> std::string
        {
            return std::to_string(_x) + std::to_string(_y);
        });

    return (r == box<std::string>("12"));
}

/*
tests_lift_a2_box_different_input_types
  Tests the following:
  - the two contexts may hold DIFFERENT types: F<int> and F<std::string>
    combined by f : (int, string) -> string, with the first context first.
*/
static bool
tests_lift_a2_box_different_input_types()
{
    const box<std::string> r = ::djinterp::lift_a2(
        box<int>(2),
        box<std::string>("ab"),
        [](int _n, const std::string& _s) -> std::string
        {
            return _s + std::to_string(_n);
        });

    return (r == box<std::string>("ab2"));
}

/*
tests_lift_a2_box_result_type
  Tests the following:
  - the result type of lift_a2 over box is exactly box<C>.
*/
static bool
tests_lift_a2_box_result_type()
{
    // Bind the result first so the lambda lives in an evaluated context; the
    // type check then reads the variable's type (a lambda inside decltype would
    // require C++20).
    auto r = ::djinterp::lift_a2(
        box<int>(1),
        box<int>(2),
        [](int _x, int _y) -> int { return _x + _y; });

    return std::is_same< decltype(r), box<int> >::value;
}


///////////////////////////////////////////////////////////////////////////////
///                lift_a2 OVER THE DIRECT APPLICATIVE  (ident)              ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_lift_a2_ident_basic
  Tests the following:
  - lift_a2 works over the direct (total) applicative: lift_a2(ident 4, ident 6,
    +) == ident 10.
*/
static bool
tests_lift_a2_ident_basic()
{
    const ident<int> r = ::djinterp::lift_a2(
        ident<int>(4),
        ident<int>(6),
        [](int _x, int _y) -> int
        {
            return _x + _y;
        });

    return (r == ident<int>(10));
}

/*
tests_lift_a2_ident_arg_order
  Tests the following:
  - the left-biased argument order holds for the direct applicative too:
    lift_a2(ident 10, ident 3, -) == ident 7.
*/
static bool
tests_lift_a2_ident_arg_order()
{
    const ident<int> r = ::djinterp::lift_a2(
        ident<int>(10),
        ident<int>(3),
        [](int _x, int _y) -> int
        {
            return _x - _y;
        });

    return (r == ident<int>(7));
}


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
applicative_lift_a2_block()
{
    dt::block_spec block;

    block.name       = "II.3 lift_a2";
    block.descriptor =
        "binary applicative lift: combination, short-circuit, order, type mixing";

    block.tests.push_back(dt::test_spec{
        "box: basic combine",
        "lift_a2(just 2, just 3, +) == just 5",
        &tests_lift_a2_box_basic });

    block.tests.push_back(dt::test_spec{
        "box: short-circuit first",
        "lift_a2(nothing, just 3, +) == nothing",
        &tests_lift_a2_box_nothing_first });

    block.tests.push_back(dt::test_spec{
        "box: short-circuit second",
        "lift_a2(just 2, nothing, +) == nothing",
        &tests_lift_a2_box_nothing_second });

    block.tests.push_back(dt::test_spec{
        "box: both empty",
        "lift_a2(nothing, nothing, +) == nothing",
        &tests_lift_a2_box_both_nothing });

    block.tests.push_back(dt::test_spec{
        "box: left-biased arg order",
        "first context is first argument (10 - 3 == 7)",
        &tests_lift_a2_box_arg_order });

    block.tests.push_back(dt::test_spec{
        "box: result type mixing",
        "two ints -> box<std::string>",
        &tests_lift_a2_box_type_mixing });

    block.tests.push_back(dt::test_spec{
        "box: different input types",
        "F<int> and F<string> combined, first first",
        &tests_lift_a2_box_different_input_types });

    block.tests.push_back(dt::test_spec{
        "box: result type identity",
        "decltype(lift_a2(...)) is box<C>",
        &tests_lift_a2_box_result_type });

    block.tests.push_back(dt::test_spec{
        "ident: basic combine",
        "direct applicative combines two contexts",
        &tests_lift_a2_ident_basic });

    block.tests.push_back(dt::test_spec{
        "ident: left-biased arg order",
        "first context is first argument (direct)",
        &tests_lift_a2_ident_arg_order });

    return block;
}


NS_END  // testing
NS_END  // djinterp
