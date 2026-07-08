/******************************************************************************
* djinterp [test]                                   applicative_tests_pure.cpp
*
*   Section II.1 of the applicative.hpp suite: pure, which lifts a bare value
* into an explicitly-named applicative (the dual of monad_unit).  Covers pure
* over the monad-bridge applicative (box, where it is the monad's unit -- an
* engaged/just value) and the direct applicative (ident), a class element type,
* the delegation of the bridge's pure to monad_unit, and the result type
* identity.
*
* path:      /tests/djinterp/core/functional/applicative_tests_pure.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "applicative_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


/*
tests_pure_box_bridge
  Tests the following:
  - pure into the bridge applicative yields an engaged value (just), equal to
    the wrapped input.
*/
static bool
tests_pure_box_bridge()
{
    const box<int> p = ::djinterp::pure< box<int> >(7);

    return ( p.has() &&
             (p == box<int>(7)) );
}

/*
tests_pure_ident_direct
  Tests the following:
  - pure into the direct applicative wraps the value.
*/
static bool
tests_pure_ident_direct()
{
    const ident<int> p = ::djinterp::pure< ident<int> >(9);

    return (p == ident<int>(9));
}

/*
tests_pure_box_string
  Tests the following:
  - pure lifts a class-typed value at a different element type.
*/
static bool
tests_pure_box_string()
{
    const box<std::string> p =
        ::djinterp::pure< box<std::string> >(std::string("hi"));

    return (p == box<std::string>("hi"));
}

/*
tests_pure_matches_unit_box
  Tests the following:
  - the bridge's pure delegates to the monad's unit: pure<box<int>>(v) equals
    monad_unit<box<int>>(v).
*/
static bool
tests_pure_matches_unit_box()
{
    const box<int> viaPure = ::djinterp::pure< box<int> >(5);
    const box<int> viaUnit = ::djinterp::monad_unit< box<int> >(5);

    return (viaPure == viaUnit);
}

/*
tests_pure_type_identity
  Tests the following:
  - the result type of pure<F>(v) is exactly F, for both a bridge and a direct
    applicative.
*/
static bool
tests_pure_type_identity()
{
    const bool box_type =
        std::is_same< decltype(::djinterp::pure< box<int> >(1)),
                      box<int> >::value;

    const bool ident_type =
        std::is_same< decltype(::djinterp::pure< ident<int> >(1)),
                      ident<int> >::value;

    return (box_type && ident_type);
}


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
applicative_pure_block()
{
    dt::block_spec block;

    block.name       = "II.1 pure";
    block.descriptor =
        "lift a bare value into an explicit applicative (bridge + direct)";

    block.tests.push_back(dt::test_spec{
        "box (bridge) is engaged",
        "pure into box yields just(value)",
        &tests_pure_box_bridge });

    block.tests.push_back(dt::test_spec{
        "ident (direct)",
        "pure into ident wraps the value",
        &tests_pure_ident_direct });

    block.tests.push_back(dt::test_spec{
        "class element type",
        "pure lifts a std::string value",
        &tests_pure_box_string });

    block.tests.push_back(dt::test_spec{
        "delegates to unit",
        "pure<box>(v) equals monad_unit<box>(v)",
        &tests_pure_matches_unit_box });

    block.tests.push_back(dt::test_spec{
        "result type identity",
        "decltype(pure<F>(v)) is F",
        &tests_pure_type_identity });

    return block;
}


NS_END  // testing
NS_END  // djinterp
