/******************************************************************************
* djinterp [test]                                 alternative_tests_aempty.cpp
*
*   Section II.1 of the alternative.hpp suite: aempty, the empty / failure
* element of an alternative (the type supplied explicitly, as with mempty).
* Covers the disengaged value it returns for more than one element type, its
* delegation to the instance's empty(), and the identity of its result type.
* (Its role as the identity for choice is exercised in the alt section.)
*
* path:      /tests/djinterp/core/functional/alternative_tests_aempty.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "alternative_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


/*
tests_aempty_opt_int_empty
  Tests the following:
  - aempty<opt<int>>() yields the disengaged (failure) value.
*/
static bool
tests_aempty_opt_int_empty()
{
    const opt<int> e = ::djinterp::aempty< opt<int> >();

    return (!e.engaged);
}

/*
tests_aempty_opt_string_empty
  Tests the following:
  - aempty yields the disengaged value at a different element type.
*/
static bool
tests_aempty_opt_string_empty()
{
    const opt<std::string> e = ::djinterp::aempty< opt<std::string> >();

    return (!e.engaged);
}

/*
tests_aempty_matches_trait_empty
  Tests the following:
  - aempty<F>() delegates to the instance's empty(): the two are equal.
*/
static bool
tests_aempty_matches_trait_empty()
{
    const opt<int> viaFree  = ::djinterp::aempty< opt<int> >();
    const opt<int> viaTrait = ::djinterp::alternative_traits< opt<int> >::empty();

    return (viaFree == viaTrait);
}

/*
tests_aempty_type_identity
  Tests the following:
  - the result type of aempty<opt<T>>() is exactly opt<T>.
*/
static bool
tests_aempty_type_identity()
{
    return std::is_same<
        decltype(::djinterp::aempty< opt<int> >()),
        opt<int> >::value;
}

/*
tests_aempty_distinct_types
  Tests the following:
  - aempty is independent per element type: opt<int> and opt<double> each yield
    their own disengaged value.
*/
static bool
tests_aempty_distinct_types()
{
    const opt<int>    ei = ::djinterp::aempty< opt<int> >();
    const opt<double> ed = ::djinterp::aempty< opt<double> >();

    return ( (!ei.engaged) &&
             (!ed.engaged) );
}


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
alternative_aempty_block()
{
    dt::block_spec block;

    block.name       = "II.1 aempty";
    block.descriptor =
        "the empty / failure element: value, delegation, type identity";

    block.tests.push_back(dt::test_spec{
        "opt<int> is disengaged",
        "aempty<opt<int>>() is the failure value",
        &tests_aempty_opt_int_empty });

    block.tests.push_back(dt::test_spec{
        "opt<string> is disengaged",
        "aempty at another element type is the failure value",
        &tests_aempty_opt_string_empty });

    block.tests.push_back(dt::test_spec{
        "delegates to empty()",
        "aempty<F>() equals the instance's empty()",
        &tests_aempty_matches_trait_empty });

    block.tests.push_back(dt::test_spec{
        "result type identity",
        "decltype(aempty<opt<T>>()) is opt<T>",
        &tests_aempty_type_identity });

    block.tests.push_back(dt::test_spec{
        "independent per element type",
        "opt<int> and opt<double> each yield their own empty",
        &tests_aempty_distinct_types });

    return block;
}


NS_END  // testing
NS_END  // djinterp
