/******************************************************************************
* djinterp [test]                               bifunctor_tests_instances.cpp
*
*   Section III of the bifunctor.hpp suite: the two shipped instances, std::pair
* and kv_pair, exercised specifically.  Covers each instance's bimap called
* directly on its trait (in isolation from the generic wrapper), the instance
* markers (is_specialized / first_type / second_type), the result-type
* construction (std::pair via make_pair; kv_pair rebuilt over the mapped types),
* and -- for kv_pair -- the key-only equality subtlety the header calls out:
* mapping the value preserves == (keys unchanged), while mapping the key breaks
* it.
*
* path:      /tests/djinterp/core/functional/bifunctor_tests_instances.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "bifunctor_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///                std::pair INSTANCE                                        ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_pair_direct_bimap
  Tests the following:
  - the std::pair instance's bimap, called directly on the trait, maps both
    components.
*/
static bool
tests_pair_direct_bimap()
{
    const std::pair<int, int> r =
        ::djinterp::bifunctor_traits< std::pair<int, int> >::bimap(
            std::pair<int, int>(3, 4), dbl_int(), inc_int());

    return (r == std::make_pair(6, 5));
}

/*
tests_pair_direct_result_type
  Tests the following:
  - the std::pair instance builds a std::pair over the mapped (decayed) types.
*/
static bool
tests_pair_direct_result_type()
{
    return std::is_same<
        decltype(::djinterp::bifunctor_traits< std::pair<int, int> >::bimap(
            std::pair<int, int>(1, 2), show_int(), real_int())),
        std::pair<std::string, double> >::value;
}

/*
tests_pair_markers
  Tests the following:
  - the std::pair instance publishes is_specialized == true_type and the two
    parameter types.
*/
static bool
tests_pair_markers()
{
    const bool specialized =
        ::djinterp::bifunctor_traits< std::pair<int, char> >::is_specialized::value;

    const bool first_ok =
        std::is_same<
            ::djinterp::bifunctor_traits< std::pair<int, char> >::first_type,
            int >::value;

    const bool second_ok =
        std::is_same<
            ::djinterp::bifunctor_traits< std::pair<int, char> >::second_type,
            char >::value;

    return (specialized && first_ok && second_ok);
}


///////////////////////////////////////////////////////////////////////////////
///                kv_pair INSTANCE                                          ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_kv_direct_bimap
  Tests the following:
  - the kv_pair instance's bimap, called directly on the trait, maps both key
    and value (field-level check).
*/
static bool
tests_kv_direct_bimap()
{
    const ::djinterp::kv_pair<int, int> r =
        ::djinterp::bifunctor_traits< ::djinterp::kv_pair<int, int> >::bimap(
            ::djinterp::kv_pair<int, int>(3, 4), dbl_int(), inc_int());

    return ( (r.m_key == 6) &&
             (r.m_value == 5) );
}

/*
tests_kv_direct_result_type
  Tests the following:
  - the kv_pair instance rebuilds a kv_pair over the mapped (decayed) types.
*/
static bool
tests_kv_direct_result_type()
{
    return std::is_same<
        decltype(::djinterp::bifunctor_traits< ::djinterp::kv_pair<int, int> >::bimap(
            ::djinterp::kv_pair<int, int>(1, 2), show_int(), real_int())),
        ::djinterp::kv_pair<std::string, double> >::value;
}

/*
tests_kv_markers
  Tests the following:
  - the kv_pair instance publishes is_specialized == true_type and the two
    parameter types.
*/
static bool
tests_kv_markers()
{
    const bool specialized =
        ::djinterp::bifunctor_traits< ::djinterp::kv_pair<int, double> >::is_specialized::value;

    const bool first_ok =
        std::is_same<
            ::djinterp::bifunctor_traits< ::djinterp::kv_pair<int, double> >::first_type,
            int >::value;

    const bool second_ok =
        std::is_same<
            ::djinterp::bifunctor_traits< ::djinterp::kv_pair<int, double> >::second_type,
            double >::value;

    return (specialized && first_ok && second_ok);
}

/*
tests_kv_key_only_eq_value_map
  Tests the following:
  - kv_pair equality is key-only: mapping the VALUE (map_second) leaves the key
    unchanged, so the result compares EQUAL to the original even though the
    value differs (verified at the field level).
*/
static bool
tests_kv_key_only_eq_value_map()
{
    const ::djinterp::kv_pair<int, int> base(5, 1);

    const ::djinterp::kv_pair<int, int> mapped =
        ::djinterp::map_second(base, [](int _v) -> int { return _v + 999; });

    return ( (mapped == base)          &&   // key-only equality: keys unchanged
             (base.m_value == 1)       &&
             (mapped.m_value == 1000)  &&   // value really did change
             (mapped.m_key == 5) );
}

/*
tests_kv_key_only_eq_key_map
  Tests the following:
  - conversely, mapping the KEY (map_first) changes the key, so the result
    compares UNEQUAL to the original under kv_pair's key-only equality.
*/
static bool
tests_kv_key_only_eq_key_map()
{
    const ::djinterp::kv_pair<int, int> base(5, 1);

    const ::djinterp::kv_pair<int, int> mapped =
        ::djinterp::map_first(base, inc_int());

    return ( (mapped != base)     &&   // key changed -> not equal
             (mapped.m_key == 6)  &&
             (mapped.m_value == 1) );   // value untouched
}


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
bifunctor_instances_block()
{
    dt::block_spec block;

    block.name       = "III. instances (std::pair, kv_pair)";
    block.descriptor =
        "direct trait bimap, markers, result types, kv key-only equality";

    block.tests.push_back(dt::test_spec{
        "pair: direct bimap",
        "instance bimap maps both components",
        &tests_pair_direct_bimap });

    block.tests.push_back(dt::test_spec{
        "pair: result via make_pair",
        "std::pair over mapped decayed types",
        &tests_pair_direct_result_type });

    block.tests.push_back(dt::test_spec{
        "pair: markers",
        "is_specialized / first_type / second_type",
        &tests_pair_markers });

    block.tests.push_back(dt::test_spec{
        "kv_pair: direct bimap",
        "instance bimap maps key and value (field-level)",
        &tests_kv_direct_bimap });

    block.tests.push_back(dt::test_spec{
        "kv_pair: result rebuilt",
        "kv_pair over mapped decayed types",
        &tests_kv_direct_result_type });

    block.tests.push_back(dt::test_spec{
        "kv_pair: markers",
        "is_specialized / first_type / second_type",
        &tests_kv_markers });

    block.tests.push_back(dt::test_spec{
        "kv_pair: value-map preserves ==",
        "key-only equality -- mapped value still equal",
        &tests_kv_key_only_eq_value_map });

    block.tests.push_back(dt::test_spec{
        "kv_pair: key-map breaks ==",
        "key-only equality -- mapped key not equal",
        &tests_kv_key_only_eq_key_map });

    return block;
}


NS_END  // testing
NS_END  // djinterp
