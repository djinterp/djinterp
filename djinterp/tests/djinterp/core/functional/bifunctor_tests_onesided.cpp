/******************************************************************************
* djinterp [test]                                bifunctor_tests_onesided.cpp
*
*   Sections II.2 - II.3 of the bifunctor.hpp suite: map_first and map_second,
* derived as bimap with the identity helper on the untouched side.  Covers
* mapping exactly one parameter while the other is preserved (exercising
* bifunctor_identity_helper), a one-sided type change, the identity edge case,
* the result types, and the consistency law map_second(map_first(x, f), g) ==
* bimap(x, f, g) -- over std::pair, kv_pair, and the custom two<A,B> fixture.
*
*   kv_pair equality is key-only, so kv checks read the m_key / m_value fields.
*
* path:      /tests/djinterp/core/functional/bifunctor_tests_onesided.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "bifunctor_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///                map_first                                                 ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_map_first_pair
  Tests the following:
  - map_first maps the first component and leaves the second untouched (via the
    identity helper): map_first((3,4), +1) == (4, 4).
*/
static bool
tests_map_first_pair()
{
    const std::pair<int, int> r =
        ::djinterp::map_first(std::pair<int, int>(3, 4), inc_int());

    return (r == std::make_pair(4, 4));
}

/*
tests_map_first_pair_type_change
  Tests the following:
  - map_first may change the first type while the second is preserved exactly:
    map_first((2,"x"), int->string) == ("2", "x").
*/
static bool
tests_map_first_pair_type_change()
{
    const std::pair<std::string, std::string> r =
        ::djinterp::map_first(
            std::pair<int, std::string>(2, std::string("x")), show_int());

    return (r == std::make_pair(std::string("2"), std::string("x")));
}

/*
tests_map_first_identity
  Tests the following:
  - map_first with identity leaves the bifunctor unchanged.
*/
static bool
tests_map_first_identity()
{
    const std::pair<int, int> r =
        ::djinterp::map_first(std::pair<int, int>(3, 4), idf());

    return (r == std::make_pair(3, 4));
}

/*
tests_map_first_kv
  Tests the following:
  - map_first maps only the key; the value is preserved (field-level check).
*/
static bool
tests_map_first_kv()
{
    const ::djinterp::kv_pair<int, int> r =
        ::djinterp::map_first(::djinterp::kv_pair<int, int>(3, 4), dbl_int());

    return ( (r.m_key == 6) &&
             (r.m_value == 4) );
}

/*
tests_map_first_custom
  Tests the following:
  - map_first maps only the first component of the custom bifunctor.
*/
static bool
tests_map_first_custom()
{
    const two<int, int> r =
        ::djinterp::map_first(two<int, int>(3, 4), inc_int());

    return (r == two<int, int>(4, 4));
}


///////////////////////////////////////////////////////////////////////////////
///                map_second                                                ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_map_second_pair
  Tests the following:
  - map_second maps the second component and leaves the first untouched:
    map_second((3,4), +1) == (3, 5).
*/
static bool
tests_map_second_pair()
{
    const std::pair<int, int> r =
        ::djinterp::map_second(std::pair<int, int>(3, 4), inc_int());

    return (r == std::make_pair(3, 5));
}

/*
tests_map_second_pair_type_change
  Tests the following:
  - map_second may change the second type while the first is preserved exactly:
    map_second(("x",5), int->double) == ("x", 5.0).
*/
static bool
tests_map_second_pair_type_change()
{
    const std::pair<std::string, double> r =
        ::djinterp::map_second(
            std::pair<std::string, int>(std::string("x"), 5), real_int());

    return ( (r.first == "x") &&
             (r.second == 5.0) );
}

/*
tests_map_second_identity
  Tests the following:
  - map_second with identity leaves the bifunctor unchanged.
*/
static bool
tests_map_second_identity()
{
    const std::pair<int, int> r =
        ::djinterp::map_second(std::pair<int, int>(3, 4), idf());

    return (r == std::make_pair(3, 4));
}

/*
tests_map_second_kv
  Tests the following:
  - map_second maps only the value; the key is preserved (field-level check).
*/
static bool
tests_map_second_kv()
{
    const ::djinterp::kv_pair<int, int> r =
        ::djinterp::map_second(::djinterp::kv_pair<int, int>(3, 4), inc_int());

    return ( (r.m_key == 3) &&
             (r.m_value == 5) );
}

/*
tests_map_second_custom
  Tests the following:
  - map_second maps only the second component of the custom bifunctor.
*/
static bool
tests_map_second_custom()
{
    const two<int, int> r =
        ::djinterp::map_second(two<int, int>(3, 4), dbl_int());

    return (r == two<int, int>(3, 8));
}


///////////////////////////////////////////////////////////////////////////////
///                CONSISTENCY + RESULT TYPES                                ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_onesided_compose_equals_bimap
  Tests the following:
  - the one-sided maps compose to bimap: map_second(map_first(x, f), g) ==
    bimap(x, f, g), including under type changes on both sides.
*/
static bool
tests_onesided_compose_equals_bimap()
{
    const std::pair<int, int> base(2, 3);

    const std::pair<std::string, double> composed =
        ::djinterp::map_second(
            ::djinterp::map_first(base, show_int()), real_int());

    const std::pair<std::string, double> at_once =
        ::djinterp::bimap(base, show_int(), real_int());

    return (composed == at_once);
}

/*
tests_onesided_result_types
  Tests the following:
  - map_first over pair<int,int> yields pair<C, int> (second type preserved);
    map_second yields pair<int, D> (first type preserved).
*/
static bool
tests_onesided_result_types()
{
    const bool first_type =
        std::is_same<
            decltype(::djinterp::map_first(std::pair<int, int>(1, 2), show_int())),
            std::pair<std::string, int> >::value;

    const bool second_type =
        std::is_same<
            decltype(::djinterp::map_second(std::pair<int, int>(1, 2), real_int())),
            std::pair<int, double> >::value;

    return (first_type && second_type);
}


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
bifunctor_onesided_block()
{
    dt::block_spec block;

    block.name       = "II.2-3 map_first / map_second";
    block.descriptor =
        "one-sided maps (identity on the other side), composition, result types";

    block.tests.push_back(dt::test_spec{
        "map_first: pair",
        "maps first, preserves second",
        &tests_map_first_pair });

    block.tests.push_back(dt::test_spec{
        "map_first: pair type change",
        "first type changes, second preserved exactly",
        &tests_map_first_pair_type_change });

    block.tests.push_back(dt::test_spec{
        "map_first: identity",
        "map_first(id) leaves the value unchanged",
        &tests_map_first_identity });

    block.tests.push_back(dt::test_spec{
        "map_first: kv_pair",
        "maps key, preserves value (field-level)",
        &tests_map_first_kv });

    block.tests.push_back(dt::test_spec{
        "map_first: custom two",
        "maps first component only",
        &tests_map_first_custom });

    block.tests.push_back(dt::test_spec{
        "map_second: pair",
        "maps second, preserves first",
        &tests_map_second_pair });

    block.tests.push_back(dt::test_spec{
        "map_second: pair type change",
        "second type changes, first preserved exactly",
        &tests_map_second_pair_type_change });

    block.tests.push_back(dt::test_spec{
        "map_second: identity",
        "map_second(id) leaves the value unchanged",
        &tests_map_second_identity });

    block.tests.push_back(dt::test_spec{
        "map_second: kv_pair",
        "maps value, preserves key (field-level)",
        &tests_map_second_kv });

    block.tests.push_back(dt::test_spec{
        "map_second: custom two",
        "maps second component only",
        &tests_map_second_custom });

    block.tests.push_back(dt::test_spec{
        "compose == bimap",
        "map_second(map_first(x,f),g) == bimap(x,f,g)",
        &tests_onesided_compose_equals_bimap });

    block.tests.push_back(dt::test_spec{
        "result types",
        "map_first/map_second preserve the untouched type",
        &tests_onesided_result_types });

    return block;
}


NS_END  // testing
NS_END  // djinterp
