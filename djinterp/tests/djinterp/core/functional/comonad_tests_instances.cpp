/******************************************************************************
* djinterp [test]                                 comonad_tests_instances.cpp
*
*   Section III of the comonad.hpp suite: the two shipped Env (co-reader)
* instances, std::pair and kv_pair, exercised specifically.  Covers each
* instance's extract / extend called directly on its trait (in isolation from
* the generic wrappers), the instance markers (is_specialized / value_type =
* focus), the Env positions (the environment is the FIRST component / the key;
* the focus is the SECOND / the value), the direct-extend result type, and --
* for kv_pair -- the interaction of the Env comonad with kv_pair's key-only
* equality (extend preserves the key / environment, so == survives a recomputed
* value).
*
* path:      /tests/djinterp/core/functional/comonad_tests_instances.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "comonad_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///                std::pair Env INSTANCE                                    ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_pair_direct_extract
  Tests the following:
  - the std::pair instance's extract, called directly on the trait, reads the
    second component.
*/
static bool
tests_pair_direct_extract()
{
    return ( ::djinterp::comonad_traits< std::pair<std::string, int> >::extract(
                 std::pair<std::string, int>("c", 10)) == 10 );
}

/*
tests_pair_direct_extend
  Tests the following:
  - the std::pair instance's extend, called directly, recomputes the focus and
    keeps the environment.
*/
static bool
tests_pair_direct_extend()
{
    const std::pair<std::string, int> r =
        ::djinterp::comonad_traits< std::pair<std::string, int> >::extend(
            std::pair<std::string, int>("c", 10), co_focus_x2());

    return (r == std::make_pair(std::string("c"), 20));
}

/*
tests_pair_markers
  Tests the following:
  - the std::pair instance publishes is_specialized == true_type and value_type
    == the FOCUS (the second component's type).
*/
static bool
tests_pair_markers()
{
    const bool specialized =
        ::djinterp::comonad_traits< std::pair<std::string, int> >::is_specialized::value;

    const bool focus_is_second =
        std::is_same<
            ::djinterp::comonad_traits< std::pair<std::string, int> >::value_type,
            int >::value;

    return (specialized && focus_is_second);
}

/*
tests_pair_env_focus_positions
  Tests the following:
  - the Env positions for std::pair: the environment is the FIRST component and
    the focus is the SECOND.  extract reads the second; extend keeps the first
    and recomputes the second.
*/
static bool
tests_pair_env_focus_positions()
{
    const std::pair<std::string, int> w("env", 10);

    const int                         focus = ::djinterp::extract(w);          // second
    const std::pair<std::string, int> r     = ::djinterp::extend(w, co_focus_x2());

    return ( (focus == 10)        &&   // focus is the second component
             (r.first == "env")   &&   // environment (first) preserved
             (r.second == 20) );       // focus (second) recomputed
}

/*
tests_pair_direct_extend_result_type
  Tests the following:
  - the std::pair instance builds a std::pair<Env, mapped_focus>.
*/
static bool
tests_pair_direct_extend_result_type()
{
    return std::is_same<
        decltype(::djinterp::comonad_traits< std::pair<std::string, int> >::extend(
            std::declval< const std::pair<std::string, int>& >(),
            std::declval< co_focus_show >())),
        std::pair<std::string, std::string> >::value;
}


///////////////////////////////////////////////////////////////////////////////
///                kv_pair Env INSTANCE                                      ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_kv_direct_extract
  Tests the following:
  - the kv_pair instance's extract, called directly, reads the value.
*/
static bool
tests_kv_direct_extract()
{
    return ( ::djinterp::comonad_traits< ::djinterp::kv_pair<int, int> >::extract(
                 ::djinterp::kv_pair<int, int>(3, 10)) == 10 );
}

/*
tests_kv_direct_extend
  Tests the following:
  - the kv_pair instance's extend, called directly, keeps the key and recomputes
    the value (field-level).
*/
static bool
tests_kv_direct_extend()
{
    const ::djinterp::kv_pair<int, int> r =
        ::djinterp::comonad_traits< ::djinterp::kv_pair<int, int> >::extend(
            ::djinterp::kv_pair<int, int>(3, 10), co_focus_x2());

    return ( (r.m_key == 3) &&
             (r.m_value == 20) );
}

/*
tests_kv_markers
  Tests the following:
  - the kv_pair instance publishes is_specialized == true_type and value_type ==
    the value (the focus).
*/
static bool
tests_kv_markers()
{
    const bool specialized =
        ::djinterp::comonad_traits< ::djinterp::kv_pair<int, double> >::is_specialized::value;

    const bool focus_is_value =
        std::is_same<
            ::djinterp::comonad_traits< ::djinterp::kv_pair<int, double> >::value_type,
            double >::value;

    return (specialized && focus_is_value);
}

/*
tests_kv_env_focus_positions
  Tests the following:
  - the Env positions for kv_pair: the environment is the KEY and the focus is
    the VALUE.  Shown with distinct key/value types: extract reads the value;
    extend keeps the key.
*/
static bool
tests_kv_env_focus_positions()
{
    const ::djinterp::kv_pair<std::string, int> w(std::string("k"), 10);

    const int focus = ::djinterp::extract(w);                       // the value
    const ::djinterp::kv_pair<std::string, int> r =
        ::djinterp::extend(w, co_focus_x2());

    return ( (focus == 10)       &&   // focus is the value
             (r.m_key == "k")     &&   // environment (key) preserved
             (r.m_value == 20) );      // focus (value) recomputed
}

/*
tests_kv_extend_key_only_equality
  Tests the following:
  - the Env comonad meets kv_pair's key-only equality: extend preserves the key
    (the environment), so the result compares EQUAL to the original even though
    the value (focus) was recomputed (verified at the field level).
*/
static bool
tests_kv_extend_key_only_equality()
{
    const ::djinterp::kv_pair<int, int> base(5, 1);

    const ::djinterp::kv_pair<int, int> mapped =
        ::djinterp::extend(base, co_focus_x2());

    return ( (mapped == base)        &&   // key-only equality: key unchanged
             (base.m_value == 1)     &&
             (mapped.m_value == 2)   &&   // value really did change
             (mapped.m_key == 5) );
}


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
comonad_instances_block()
{
    dt::block_spec block;

    block.name       = "III. instances (Env comonad)";
    block.descriptor =
        "std::pair / kv_pair: direct extract/extend, markers, env/focus, kv equality";

    block.tests.push_back(dt::test_spec{
        "pair: direct extract",
        "instance extract reads the second component",
        &tests_pair_direct_extract });

    block.tests.push_back(dt::test_spec{
        "pair: direct extend",
        "instance extend recomputes focus, keeps env",
        &tests_pair_direct_extend });

    block.tests.push_back(dt::test_spec{
        "pair: markers",
        "is_specialized / value_type == focus (second)",
        &tests_pair_markers });

    block.tests.push_back(dt::test_spec{
        "pair: env/focus positions",
        "env is first, focus is second",
        &tests_pair_env_focus_positions });

    block.tests.push_back(dt::test_spec{
        "pair: direct extend result type",
        "std::pair<Env, mapped focus>",
        &tests_pair_direct_extend_result_type });

    block.tests.push_back(dt::test_spec{
        "kv: direct extract",
        "instance extract reads the value",
        &tests_kv_direct_extract });

    block.tests.push_back(dt::test_spec{
        "kv: direct extend",
        "instance extend keeps key, recomputes value (field-level)",
        &tests_kv_direct_extend });

    block.tests.push_back(dt::test_spec{
        "kv: markers",
        "is_specialized / value_type == value (focus)",
        &tests_kv_markers });

    block.tests.push_back(dt::test_spec{
        "kv: env/focus positions",
        "env is key, focus is value",
        &tests_kv_env_focus_positions });

    block.tests.push_back(dt::test_spec{
        "kv: extend + key-only equality",
        "recomputed value still equal (key preserved)",
        &tests_kv_extend_key_only_equality });

    return block;
}


NS_END  // testing
NS_END  // djinterp
