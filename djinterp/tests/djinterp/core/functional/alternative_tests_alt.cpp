/******************************************************************************
* djinterp [test]                                    alternative_tests_alt.cpp
*
*   Section II.2 of the alternative.hpp suite: alt, the associative choice
* (Haskell's <|>) that delegates to the instance's choice().  Covers the basic
* selection (first engaged operand wins), the empty-choice case, the monoid
* laws (left / right identity with aempty, and associativity), the lvalue /
* rvalue / mixed argument-forwarding paths, the result type identity, and a
* non-int element type.
*
* path:      /tests/djinterp/core/functional/alternative_tests_alt.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "alternative_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


/*
tests_alt_none_just
  Tests the following:
  - when the first operand is empty, alt selects the second.
*/
static bool
tests_alt_none_just()
{
    const opt<int> r = ::djinterp::alt(opt<int>(), opt<int>(7));

    return (r == opt<int>(7));
}

/*
tests_alt_just_none
  Tests the following:
  - when the first operand is engaged, alt selects it (ignoring an empty
    second).
*/
static bool
tests_alt_just_none()
{
    const opt<int> r = ::djinterp::alt(opt<int>(4), opt<int>());

    return (r == opt<int>(4));
}

/*
tests_alt_just_just_first
  Tests the following:
  - with both operands engaged, alt keeps the FIRST.
*/
static bool
tests_alt_just_just_first()
{
    const opt<int> r = ::djinterp::alt(opt<int>(1), opt<int>(2));

    return (r == opt<int>(1));
}

/*
tests_alt_keeps_first_distinct
  Tests the following:
  - the selection is specifically the first engaged operand, not a merge / max:
    swapping the operands swaps the result.
*/
static bool
tests_alt_keeps_first_distinct()
{
    const opt<int> a = ::djinterp::alt(opt<int>(2), opt<int>(1));
    const opt<int> b = ::djinterp::alt(opt<int>(1), opt<int>(2));

    return ( (a == opt<int>(2)) &&
             (b == opt<int>(1)) );
}

/*
tests_alt_none_none
  Tests the following:
  - choosing between two empties yields empty.
*/
static bool
tests_alt_none_none()
{
    const opt<int> r = ::djinterp::alt(opt<int>(), opt<int>());

    return (r == opt<int>());
}

/*
tests_alt_left_identity
  Tests the following:
  - aempty is a LEFT identity for alt: alt(aempty, x) == x, for engaged and
    empty x.
*/
static bool
tests_alt_left_identity()
{
    const opt<int> some = opt<int>(5);
    const opt<int> none = opt<int>();

    const opt<int> l_some = ::djinterp::alt(::djinterp::aempty< opt<int> >(), some);
    const opt<int> l_none = ::djinterp::alt(::djinterp::aempty< opt<int> >(), none);

    return ( (l_some == some) &&
             (l_none == none) );
}

/*
tests_alt_right_identity
  Tests the following:
  - aempty is a RIGHT identity for alt: alt(x, aempty) == x, for engaged and
    empty x.
*/
static bool
tests_alt_right_identity()
{
    const opt<int> some = opt<int>(5);
    const opt<int> none = opt<int>();

    const opt<int> r_some = ::djinterp::alt(some, ::djinterp::aempty< opt<int> >());
    const opt<int> r_none = ::djinterp::alt(none, ::djinterp::aempty< opt<int> >());

    return ( (r_some == some) &&
             (r_none == none) );
}

/*
tests_alt_associative
  Tests the following:
  - alt is associative: alt(alt(a,b),c) == alt(a,alt(b,c)) across a spread of
    engaged/empty combinations.
*/
static bool
tests_alt_associative()
{
    bool ok = true;

    // (none, some, some)
    ok = ok &&
         ( ::djinterp::alt(::djinterp::alt(opt<int>(), opt<int>(2)), opt<int>(3)) ==
           ::djinterp::alt(opt<int>(), ::djinterp::alt(opt<int>(2), opt<int>(3))) );

    // (some, some, some)
    ok = ok &&
         ( ::djinterp::alt(::djinterp::alt(opt<int>(1), opt<int>(2)), opt<int>(3)) ==
           ::djinterp::alt(opt<int>(1), ::djinterp::alt(opt<int>(2), opt<int>(3))) );

    // (none, none, some)
    ok = ok &&
         ( ::djinterp::alt(::djinterp::alt(opt<int>(), opt<int>()), opt<int>(9)) ==
           ::djinterp::alt(opt<int>(), ::djinterp::alt(opt<int>(), opt<int>(9))) );

    // (none, none, none)
    ok = ok &&
         ( ::djinterp::alt(::djinterp::alt(opt<int>(), opt<int>()), opt<int>()) ==
           ::djinterp::alt(opt<int>(), ::djinterp::alt(opt<int>(), opt<int>())) );

    return ok;
}

/*
tests_alt_lvalue_args
  Tests the following:
  - alt accepts named lvalue operands (the lvalue-forwarding path).
*/
static bool
tests_alt_lvalue_args()
{
    opt<int> a = opt<int>();
    opt<int> b = opt<int>(7);

    const opt<int> r = ::djinterp::alt(a, b);

    return (r == opt<int>(7));
}

/*
tests_alt_rvalue_args
  Tests the following:
  - alt accepts rvalue operands (temporaries; the rvalue-forwarding path).
*/
static bool
tests_alt_rvalue_args()
{
    const opt<int> r = ::djinterp::alt(opt<int>(), opt<int>(8));

    return (r == opt<int>(8));
}

/*
tests_alt_mixed_value_ref
  Tests the following:
  - alt accepts a mix of lvalue and rvalue operands in either position.
*/
static bool
tests_alt_mixed_value_ref()
{
    opt<int> a = opt<int>(3);
    opt<int> b = opt<int>(4);

    const opt<int> r1 = ::djinterp::alt(a, opt<int>(4));   // lvalue, rvalue
    const opt<int> r2 = ::djinterp::alt(opt<int>(3), b);   // rvalue, lvalue

    return ( (r1 == opt<int>(3)) &&
             (r2 == opt<int>(3)) );
}

/*
tests_alt_type_identity
  Tests the following:
  - for a uniform alternative the result type of alt is exactly opt<T>.
*/
static bool
tests_alt_type_identity()
{
    opt<int> a = opt<int>(1);
    opt<int> b = opt<int>(2);

    return std::is_same< decltype(::djinterp::alt(a, b)), opt<int> >::value;
}

/*
tests_alt_string_element
  Tests the following:
  - alt selects correctly at a non-int element type.
*/
static bool
tests_alt_string_element()
{
    const opt<std::string> r =
        ::djinterp::alt(opt<std::string>(), opt<std::string>("hi"));

    return (r == opt<std::string>("hi"));
}


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
alternative_alt_block()
{
    dt::block_spec block;

    block.name       = "II.2 alt";
    block.descriptor =
        "associative choice: selection, identity laws, associativity, forwarding";

    block.tests.push_back(dt::test_spec{
        "empty | engaged",
        "first empty -> select second",
        &tests_alt_none_just });

    block.tests.push_back(dt::test_spec{
        "engaged | empty",
        "first engaged -> select first",
        &tests_alt_just_none });

    block.tests.push_back(dt::test_spec{
        "engaged | engaged",
        "both engaged -> keep the first",
        &tests_alt_just_just_first });

    block.tests.push_back(dt::test_spec{
        "keeps first (distinct)",
        "swapping operands swaps the result",
        &tests_alt_keeps_first_distinct });

    block.tests.push_back(dt::test_spec{
        "empty | empty",
        "two empties -> empty",
        &tests_alt_none_none });

    block.tests.push_back(dt::test_spec{
        "left identity",
        "alt(aempty, x) == x",
        &tests_alt_left_identity });

    block.tests.push_back(dt::test_spec{
        "right identity",
        "alt(x, aempty) == x",
        &tests_alt_right_identity });

    block.tests.push_back(dt::test_spec{
        "associativity",
        "alt(alt(a,b),c) == alt(a,alt(b,c))",
        &tests_alt_associative });

    block.tests.push_back(dt::test_spec{
        "lvalue operands",
        "named lvalue arguments forwarded",
        &tests_alt_lvalue_args });

    block.tests.push_back(dt::test_spec{
        "rvalue operands",
        "temporary arguments forwarded",
        &tests_alt_rvalue_args });

    block.tests.push_back(dt::test_spec{
        "mixed lvalue/rvalue",
        "mixed operand value categories in either position",
        &tests_alt_mixed_value_ref });

    block.tests.push_back(dt::test_spec{
        "result type identity",
        "decltype(alt(a,b)) is opt<T>",
        &tests_alt_type_identity });

    block.tests.push_back(dt::test_spec{
        "string element",
        "selection correct at a non-int element type",
        &tests_alt_string_element });

    return block;
}


NS_END  // testing
NS_END  // djinterp
