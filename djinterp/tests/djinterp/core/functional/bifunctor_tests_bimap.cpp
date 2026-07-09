/******************************************************************************
* djinterp [test]                                   bifunctor_tests_bimap.cpp
*
*   Section II.1 of the bifunctor.hpp suite: bimap, the one obligation, which
* maps f : A -> C over the first parameter and g : B -> D over the second,
* yielding F<C, D>.  Covers the both-sides mapping, the f-to-first / g-to-second
* routing (shown with asymmetric functions), independent type changes on each
* side, the identity edge case, argument forwarding (lvalue / rvalue), and the
* result types -- over std::pair, kv_pair, and the custom two<A,B> fixture.
*
*   kv_pair equality is key-only, so kv checks read the m_key / m_value fields.
*
* path:      /tests/djinterp/core/functional/bifunctor_tests_bimap.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "bifunctor_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///                bimap OVER std::pair                                      ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_bimap_pair_both
  Tests the following:
  - both components are mapped: bimap((3,4), +1, *2) == (4, 8).
*/
static bool
tests_bimap_pair_both()
{
    const std::pair<int, int> r =
        ::djinterp::bimap(std::pair<int, int>(3, 4), inc_int(), dbl_int());

    return (r == std::make_pair(4, 8));
}

/*
tests_bimap_pair_routing
  Tests the following:
  - f maps the FIRST and g maps the SECOND (not swapped): with asymmetric
    functions, bimap((10,10), *2, +1) == (20, 11).
*/
static bool
tests_bimap_pair_routing()
{
    const std::pair<int, int> r =
        ::djinterp::bimap(std::pair<int, int>(10, 10), dbl_int(), inc_int());

    return (r == std::make_pair(20, 11));
}

/*
tests_bimap_pair_type_change_both
  Tests the following:
  - each side may change type independently: bimap((2,3), int->string,
    int->double) == ("2", 3.0).
*/
static bool
tests_bimap_pair_type_change_both()
{
    const std::pair<std::string, double> r =
        ::djinterp::bimap(std::pair<int, int>(2, 3), show_int(), real_int());

    return ( (r.first == "2") &&
             (r.second == 3.0) );
}

/*
tests_bimap_pair_identity
  Tests the following:
  - mapping both sides with identity leaves the pair unchanged.
*/
static bool
tests_bimap_pair_identity()
{
    const std::pair<int, int> r =
        ::djinterp::bimap(std::pair<int, int>(3, 4), idf(), idf());

    return (r == std::make_pair(3, 4));
}

/*
tests_bimap_pair_forwarding
  Tests the following:
  - bimap accepts a named lvalue bifunctor and an rvalue (temporary) one.
*/
static bool
tests_bimap_pair_forwarding()
{
    std::pair<int, int> p(3, 4);

    const std::pair<int, int> r_lvalue =
        ::djinterp::bimap(p, inc_int(), dbl_int());
    const std::pair<int, int> r_rvalue =
        ::djinterp::bimap(std::pair<int, int>(3, 4), inc_int(), dbl_int());

    return ( (r_lvalue == std::make_pair(4, 8)) &&
             (r_rvalue == std::make_pair(4, 8)) );
}

/*
tests_bimap_pair_result_type
  Tests the following:
  - the result type of bimap over std::pair is std::pair<C, D> with each side's
    mapped (decayed) type.
*/
static bool
tests_bimap_pair_result_type()
{
    return std::is_same<
        decltype(::djinterp::bimap(std::pair<int, int>(1, 2), show_int(), real_int())),
        std::pair<std::string, double> >::value;
}


///////////////////////////////////////////////////////////////////////////////
///                bimap OVER kv_pair                                        ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_bimap_kv_both
  Tests the following:
  - both key and value are mapped (checked at the field level, since kv_pair
    equality is key-only): bimap(kv(3,4), *2, +1) has m_key 6, m_value 5.
*/
static bool
tests_bimap_kv_both()
{
    const ::djinterp::kv_pair<int, int> r =
        ::djinterp::bimap(::djinterp::kv_pair<int, int>(3, 4), dbl_int(), inc_int());

    return ( (r.m_key == 6) &&
             (r.m_value == 5) );
}

/*
tests_bimap_kv_type_change
  Tests the following:
  - key and value may change type: bimap(kv(1,2), int->string, int->double) has
    m_key "1", m_value 2.0.
*/
static bool
tests_bimap_kv_type_change()
{
    const ::djinterp::kv_pair<std::string, double> r =
        ::djinterp::bimap(::djinterp::kv_pair<int, int>(1, 2), show_int(), real_int());

    return ( (r.m_key == "1") &&
             (r.m_value == 2.0) );
}

/*
tests_bimap_kv_result_type
  Tests the following:
  - the result type of bimap over kv_pair is kv_pair<mapped_key, mapped_value>.
*/
static bool
tests_bimap_kv_result_type()
{
    return std::is_same<
        decltype(::djinterp::bimap(
            ::djinterp::kv_pair<int, int>(1, 2), show_int(), real_int())),
        ::djinterp::kv_pair<std::string, double> >::value;
}


///////////////////////////////////////////////////////////////////////////////
///                bimap OVER THE CUSTOM two<A,B>                            ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_bimap_custom_both
  Tests the following:
  - bimap is generic over a user-defined bifunctor: bimap(two(3,4), +1, *2) ==
    two(4, 8).
*/
static bool
tests_bimap_custom_both()
{
    const two<int, int> r =
        ::djinterp::bimap(two<int, int>(3, 4), inc_int(), dbl_int());

    return (r == two<int, int>(4, 8));
}

/*
tests_bimap_custom_type_change
  Tests the following:
  - each side of the custom bifunctor may change type independently.
*/
static bool
tests_bimap_custom_type_change()
{
    const two<std::string, double> r =
        ::djinterp::bimap(two<int, int>(2, 3), show_int(), real_int());

    return ( (r.a == "2") &&
             (r.b == 3.0) );
}


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
bifunctor_bimap_block()
{
    dt::block_spec block;

    block.name       = "II.1 bimap";
    block.descriptor =
        "map both parameters: routing, type changes, identity, forwarding";

    block.tests.push_back(dt::test_spec{
        "pair: both sides",
        "bimap maps first and second",
        &tests_bimap_pair_both });

    block.tests.push_back(dt::test_spec{
        "pair: f->first, g->second",
        "asymmetric functions confirm routing",
        &tests_bimap_pair_routing });

    block.tests.push_back(dt::test_spec{
        "pair: type change both sides",
        "each side may change type independently",
        &tests_bimap_pair_type_change_both });

    block.tests.push_back(dt::test_spec{
        "pair: identity both sides",
        "bimap(id, id) leaves the pair unchanged",
        &tests_bimap_pair_identity });

    block.tests.push_back(dt::test_spec{
        "pair: forwarding",
        "lvalue and rvalue bifunctor operands",
        &tests_bimap_pair_forwarding });

    block.tests.push_back(dt::test_spec{
        "pair: result type",
        "std::pair<C, D> with mapped decayed types",
        &tests_bimap_pair_result_type });

    block.tests.push_back(dt::test_spec{
        "kv_pair: both sides",
        "key and value mapped (field-level check)",
        &tests_bimap_kv_both });

    block.tests.push_back(dt::test_spec{
        "kv_pair: type change",
        "key and value may change type",
        &tests_bimap_kv_type_change });

    block.tests.push_back(dt::test_spec{
        "kv_pair: result type",
        "kv_pair<mapped_key, mapped_value>",
        &tests_bimap_kv_result_type });

    block.tests.push_back(dt::test_spec{
        "custom two: both sides",
        "bimap is generic over a user bifunctor",
        &tests_bimap_custom_both });

    block.tests.push_back(dt::test_spec{
        "custom two: type change",
        "each side may change type",
        &tests_bimap_custom_type_change });

    return block;
}


NS_END  // testing
NS_END  // djinterp
