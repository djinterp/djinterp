/******************************************************************************
* djinterp [test]                                 comonad_tests_protocol.cpp
*
*   Section 0 + I of the comonad.hpp suite: the protocol and its detection
* vocabulary.  Covers is_comonad (and its _v shorthand), comonad_value_type
* (and the _t alias, well-formed and SFINAE-safe absent arms), and the C++20
* Comonad concept -- across the two shipped Env instances (std::pair, kv_pair),
* the custom Identity ident<A> fixture, and non-comonads (a scalar, a class
* type, a one-parameter container).  A focus test verifies the semantic that
* matters most: the comonad's value_type is the SECOND component (the focus),
* not the first (the environment).
*
* path:      /tests/djinterp/core/functional/comonad_tests_protocol.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// std
#include <vector>
// djinterp
#include "comonad_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///                DETECTION:  is_comonad                                    ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_is_comonad_pair
  Tests the following:
  - is_comonad is true for the shipped std::pair Env instance.
*/
static bool
tests_is_comonad_pair()
{
    return ( ::djinterp::is_comonad< std::pair<std::string, int> >::value &&
             ::djinterp::is_comonad< std::pair<int, int> >::value );
}

/*
tests_is_comonad_kv
  Tests the following:
  - is_comonad is true for the shipped kv_pair Env instance.
*/
static bool
tests_is_comonad_kv()
{
    return ::djinterp::is_comonad< ::djinterp::kv_pair<int, int> >::value;
}

/*
tests_is_comonad_ident
  Tests the following:
  - is_comonad is true for the custom Identity comonad, proving detection keys
    on any comonad_traits specialization.
*/
static bool
tests_is_comonad_ident()
{
    return ::djinterp::is_comonad< ident<int> >::value;
}

/*
tests_is_comonad_int_false
  Tests the following:
  - is_comonad is false for a scalar.
*/
static bool
tests_is_comonad_int_false()
{
    return (!::djinterp::is_comonad<int>::value);
}

/*
tests_is_comonad_string_false
  Tests the following:
  - is_comonad is false for an unrelated class type.
*/
static bool
tests_is_comonad_string_false()
{
    return (!::djinterp::is_comonad<std::string>::value);
}

/*
tests_is_comonad_container_false
  Tests the following:
  - a one-parameter container is not a comonad (it has no always-readable
    focus): is_comonad<vector<int>> is false.
*/
static bool
tests_is_comonad_container_false()
{
    return (!::djinterp::is_comonad< std::vector<int> >::value);
}

/*
tests_is_comonad_cvref_decays
  Tests the following:
  - is_comonad strips cv-qualifiers and references before deciding, for a
    shipped instance and the custom one.
*/
static bool
tests_is_comonad_cvref_decays()
{
    const bool pair_const_ref = ::djinterp::is_comonad< const std::pair<std::string, int>& >::value;
    const bool pair_rvalue    = ::djinterp::is_comonad< std::pair<std::string, int>&& >::value;
    const bool kv_volatile    = ::djinterp::is_comonad< volatile ::djinterp::kv_pair<int, int> >::value;
    const bool ident_cref     = ::djinterp::is_comonad< const ident<int>& >::value;

    return ( pair_const_ref &&
             pair_rvalue    &&
             kv_volatile    &&
             ident_cref );
}


///////////////////////////////////////////////////////////////////////////////
///                FOCUS TYPE:  comonad_value_type                           ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_value_type_pair_is_second
  Tests the following:
  - the comonad focus is the SECOND component: comonad_value_type_t of
    pair<E, A> is A (not E).  Checked with distinct first/second types both
    ways so position, not type, decides.
*/
static bool
tests_value_type_pair_is_second()
{
    const bool a =
        std::is_same< ::djinterp::comonad_value_type_t< std::pair<std::string, int> >,
                      int >::value;
    const bool b =
        std::is_same< ::djinterp::comonad_value_type_t< std::pair<int, std::string> >,
                      std::string >::value;

    return (a && b);
}

/*
tests_value_type_kv_is_value
  Tests the following:
  - the kv_pair comonad focus is the value: comonad_value_type_t of
    kv_pair<K, V> is V.
*/
static bool
tests_value_type_kv_is_value()
{
    return std::is_same< ::djinterp::comonad_value_type_t< ::djinterp::kv_pair<int, double> >,
                         double >::value;
}

/*
tests_value_type_ident
  Tests the following:
  - the Identity comonad focus is its sole type.
*/
static bool
tests_value_type_ident()
{
    return std::is_same< ::djinterp::comonad_value_type_t< ident<char> >,
                         char >::value;
}

/*
tests_value_type_decays
  Tests the following:
  - comonad_value_type_t decays its argument.
*/
static bool
tests_value_type_decays()
{
    return std::is_same<
        ::djinterp::comonad_value_type_t< const std::pair<std::string, int>& >,
        int >::value;
}

/*
tests_co_value_type_present_absent
  Tests the following:
  - the comonad_traits value_type surface is present for comonads (shipped and
    custom) and absent for non-comonads, detected the SFINAE-safe way.
*/
static bool
tests_co_value_type_present_absent()
{
    const bool present_pair  = has_co_value_type< std::pair<std::string, int> >::value;
    const bool present_kv    = has_co_value_type< ::djinterp::kv_pair<int, int> >::value;
    const bool present_ident = has_co_value_type< ident<int> >::value;
    const bool absent_int    = has_co_value_type< int >::value;
    const bool absent_vec    = has_co_value_type< std::vector<int> >::value;

    return ( present_pair   &&
             present_kv     &&
             present_ident  &&
             (!absent_int)  &&
             (!absent_vec) );
}


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

/*
tests_is_comonad_v
  Tests the following:
  - the is_comonad_v shorthand agrees with is_comonad<>::value across a shipped
    instance, the custom one, and a non-comonad.  (C++14+.)
*/
static bool
tests_is_comonad_v()
{
    return ( ::djinterp::is_comonad_v< std::pair<std::string, int> > &&
             ::djinterp::is_comonad_v< ident<int> >                  &&
             (!::djinterp::is_comonad_v<int>) );
}

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

/*
tests_concept_comonad
  Tests the following:
  - the Comonad concept is satisfied by the shipped instances and the custom
    fixture, and not by a scalar.  (C++20.)
*/
static bool
tests_concept_comonad()
{
    const bool pair_ok  = ::djinterp::Comonad< std::pair<std::string, int> >;
    const bool kv_ok    = ::djinterp::Comonad< ::djinterp::kv_pair<int, int> >;
    const bool ident_ok = ::djinterp::Comonad< ident<int> >;
    const bool int_no   = ::djinterp::Comonad< int >;

    return ( pair_ok && kv_ok && ident_ok && (!int_no) );
}

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
comonad_protocol_block()
{
    dt::block_spec block;

    block.name       = "0+I. protocol & traits";
    block.descriptor =
        "is_comonad / value_type (focus = second) / concept across instances";

    block.tests.push_back(dt::test_spec{
        "is_comonad: std::pair",
        "shipped pair Env instance is detected",
        &tests_is_comonad_pair });

    block.tests.push_back(dt::test_spec{
        "is_comonad: kv_pair",
        "shipped kv_pair Env instance is detected",
        &tests_is_comonad_kv });

    block.tests.push_back(dt::test_spec{
        "is_comonad: custom ident",
        "user-defined comonad is detected",
        &tests_is_comonad_ident });

    block.tests.push_back(dt::test_spec{
        "is_comonad: int",
        "is_comonad<int> is false",
        &tests_is_comonad_int_false });

    block.tests.push_back(dt::test_spec{
        "is_comonad: std::string",
        "is_comonad<string> is false",
        &tests_is_comonad_string_false });

    block.tests.push_back(dt::test_spec{
        "is_comonad: container",
        "a one-parameter container is not a comonad",
        &tests_is_comonad_container_false });

    block.tests.push_back(dt::test_spec{
        "is_comonad: cv/ref decay",
        "const/ref/rvalue/volatile comonads still detected",
        &tests_is_comonad_cvref_decays });

    block.tests.push_back(dt::test_spec{
        "value_type: pair focus is second",
        "comonad_value_type_t<pair<E,A>> is A",
        &tests_value_type_pair_is_second });

    block.tests.push_back(dt::test_spec{
        "value_type: kv focus is value",
        "comonad_value_type_t<kv<K,V>> is V",
        &tests_value_type_kv_is_value });

    block.tests.push_back(dt::test_spec{
        "value_type: ident",
        "focus type of the Identity comonad",
        &tests_value_type_ident });

    block.tests.push_back(dt::test_spec{
        "value_type: cv/ref decay",
        "comonad_value_type_t decays its argument",
        &tests_value_type_decays });

    block.tests.push_back(dt::test_spec{
        "value_type: present vs absent",
        "trait value_type exists for comonads, absent otherwise",
        &tests_co_value_type_present_absent });

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    block.tests.push_back(dt::test_spec{
        "is_comonad_v shorthand",
        "is_comonad_v agrees with is_comonad<>::value (C++14+)",
        &tests_is_comonad_v });
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    block.tests.push_back(dt::test_spec{
        "Comonad concept",
        "Comonad satisfied by instances + custom, not by int (C++20)",
        &tests_concept_comonad });
#endif

    return block;
}


NS_END  // testing
NS_END  // djinterp
