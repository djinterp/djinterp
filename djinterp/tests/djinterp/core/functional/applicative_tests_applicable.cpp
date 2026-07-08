/******************************************************************************
* djinterp [test]                             applicative_tests_applicable.cpp
*
*   Section 0 (applicability) of the applicative.hpp suite: is_applicable and
* its _v shorthand / applicable_with concept, which answer "is ap(Ff, Fa) a
* well-formed expression?".  Covers the positive arms (a wrapped function over a
* matching argument, for both the bridge and the direct applicative) and the
* false primary via several distinct ill-formed cases: a non-applicative on
* either side, a wrapped value that is not callable, and a cross-kind pairing (a
* box-wrapped function with an ident argument -- ap requires the SAME
* applicative on both sides).
*
* path:      /tests/djinterp/core/functional/applicative_tests_applicable.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "applicative_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


/*
tests_applicable_box_positive
  Tests the following:
  - ap is well-formed for a box-wrapped function over a matching box argument.
*/
static bool
tests_applicable_box_positive()
{
    return ::djinterp::is_applicable< box<times2>, box<int> >::value;
}

/*
tests_applicable_ident_positive
  Tests the following:
  - ap is well-formed for the direct ident applicative too.
*/
static bool
tests_applicable_ident_positive()
{
    return ::djinterp::is_applicable< ident<times2>, ident<int> >::value;
}

/*
tests_applicable_int_int_false
  Tests the following:
  - ap(int, int) is ill-formed: neither operand is an applicative.
*/
static bool
tests_applicable_int_int_false()
{
    return (!::djinterp::is_applicable< int, int >::value);
}

/*
tests_applicable_wrapped_nonfunc
  Tests the following:
  - a wrapped value that is not callable is not applicable: ap(box<int>,
    box<int>) is ill-formed (the "function" int cannot be applied to the
    argument).
*/
static bool
tests_applicable_wrapped_nonfunc()
{
    return (!::djinterp::is_applicable< box<int>, box<int> >::value);
}

/*
tests_applicable_arg_nonapplicative
  Tests the following:
  - a non-applicative ARGUMENT is not applicable: ap(box<times2>, int) is
    ill-formed.
*/
static bool
tests_applicable_arg_nonapplicative()
{
    return (!::djinterp::is_applicable< box<times2>, int >::value);
}

/*
tests_applicable_first_nonapplicative
  Tests the following:
  - a non-applicative FUNCTION position is not applicable: ap(int, box<int>) is
    ill-formed (no applicative_traits<int>).
*/
static bool
tests_applicable_first_nonapplicative()
{
    return (!::djinterp::is_applicable< int, box<int> >::value);
}

/*
tests_applicable_mismatched_kinds
  Tests the following:
  - ap requires the SAME applicative on both sides: a box-wrapped function with
    an ident argument is ill-formed (box's ap tries to monad_map the argument,
    which fails because ident is not a monad).
*/
static bool
tests_applicable_mismatched_kinds()
{
    return (!::djinterp::is_applicable< box<times2>, ident<int> >::value);
}


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

/*
tests_is_applicable_v
  Tests the following:
  - the is_applicable_v shorthand agrees with is_applicable<>::value for a valid
    and an invalid pairing.  (C++14+.)
*/
static bool
tests_is_applicable_v()
{
    return ( ::djinterp::is_applicable_v< box<times2>, box<int> > &&
             (!::djinterp::is_applicable_v< int, int >) );
}

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

/*
tests_concept_applicable_with
  Tests the following:
  - the applicable_with concept is satisfied by a valid pairing and not by an
    invalid one.  (C++20.)
*/
static bool
tests_concept_applicable_with()
{
    const bool ok = ::djinterp::applicable_with< box<times2>, box<int> >;
    const bool no = ::djinterp::applicable_with< int, int >;

    return ( ok && (!no) );
}

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
applicative_applicable_block()
{
    dt::block_spec block;

    block.name       = "0. is_applicable";
    block.descriptor =
        "ap well-formedness: positives, non-callable, cross-kind negatives";

    block.tests.push_back(dt::test_spec{
        "applicable: box positive",
        "ap(box<fn>, box<arg>) is well-formed",
        &tests_applicable_box_positive });

    block.tests.push_back(dt::test_spec{
        "applicable: ident positive",
        "ap(ident<fn>, ident<arg>) is well-formed",
        &tests_applicable_ident_positive });

    block.tests.push_back(dt::test_spec{
        "not applicable: int/int",
        "neither operand an applicative",
        &tests_applicable_int_int_false });

    block.tests.push_back(dt::test_spec{
        "not applicable: wrapped non-function",
        "ap(box<int>, box<int>) -- value not callable",
        &tests_applicable_wrapped_nonfunc });

    block.tests.push_back(dt::test_spec{
        "not applicable: non-applicative arg",
        "ap(box<fn>, int)",
        &tests_applicable_arg_nonapplicative });

    block.tests.push_back(dt::test_spec{
        "not applicable: non-applicative fn pos",
        "ap(int, box<arg>)",
        &tests_applicable_first_nonapplicative });

    block.tests.push_back(dt::test_spec{
        "not applicable: mismatched kinds",
        "ap(box<fn>, ident<arg>) -- must be same applicative",
        &tests_applicable_mismatched_kinds });

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    block.tests.push_back(dt::test_spec{
        "is_applicable_v shorthand",
        "is_applicable_v agrees with is_applicable<>::value (C++14+)",
        &tests_is_applicable_v });
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    block.tests.push_back(dt::test_spec{
        "applicable_with concept",
        "applicable_with satisfied by valid pairing only (C++20)",
        &tests_concept_applicable_with });
#endif

    return block;
}


NS_END  // testing
NS_END  // djinterp
