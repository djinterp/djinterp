/******************************************************************************
* djinterp [test]                                bifunctor_tests_protocol.cpp
*
*   Section 0 + I of the bifunctor.hpp suite: the protocol and its detection
* vocabulary.  Covers is_bifunctor (and its _v shorthand), bifunctor_first_type
* / bifunctor_second_type (and the _t aliases, well-formed and SFINAE-safe
* absent arms), and the C++20 Bifunctor concept -- across the two shipped
* instances (std::pair, kv_pair), the custom two<A,B> fixture, and several
* non-bifunctors (a scalar, a standard class type, and a one-parameter
* container, which is not a bifunctor).
*
* path:      /tests/djinterp/core/functional/bifunctor_tests_protocol.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// std
#include <vector>
// djinterp
#include "bifunctor_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///                DETECTION:  is_bifunctor                                  ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_is_bifunctor_pair
  Tests the following:
  - is_bifunctor is true for the shipped std::pair instance.
*/
static bool
tests_is_bifunctor_pair()
{
    return ( ::djinterp::is_bifunctor< std::pair<int, int> >::value &&
             ::djinterp::is_bifunctor< std::pair<int, std::string> >::value );
}

/*
tests_is_bifunctor_kv
  Tests the following:
  - is_bifunctor is true for the shipped kv_pair instance.
*/
static bool
tests_is_bifunctor_kv()
{
    return ::djinterp::is_bifunctor< ::djinterp::kv_pair<int, int> >::value;
}

/*
tests_is_bifunctor_custom
  Tests the following:
  - is_bifunctor is true for the user-defined two<A,B> fixture, proving
    detection keys on any bifunctor_traits specialization.
*/
static bool
tests_is_bifunctor_custom()
{
    return ::djinterp::is_bifunctor< two<int, int> >::value;
}

/*
tests_is_bifunctor_int_false
  Tests the following:
  - is_bifunctor is false for a scalar.
*/
static bool
tests_is_bifunctor_int_false()
{
    return (!::djinterp::is_bifunctor<int>::value);
}

/*
tests_is_bifunctor_string_false
  Tests the following:
  - is_bifunctor is false for an unrelated standard class type.
*/
static bool
tests_is_bifunctor_string_false()
{
    return (!::djinterp::is_bifunctor<std::string>::value);
}

/*
tests_is_bifunctor_container_false
  Tests the following:
  - a one-parameter container is not a bifunctor: is_bifunctor<vector<int>> is
    false (being a functor does not make a type a bifunctor here).
*/
static bool
tests_is_bifunctor_container_false()
{
    return (!::djinterp::is_bifunctor< std::vector<int> >::value);
}

/*
tests_is_bifunctor_cvref_decays
  Tests the following:
  - is_bifunctor strips cv-qualifiers and references before deciding, for both
    a shipped instance (std::pair) and a custom one (two).
*/
static bool
tests_is_bifunctor_cvref_decays()
{
    const bool pair_const_ref = ::djinterp::is_bifunctor< const std::pair<int, int>& >::value;
    const bool pair_rvalue    = ::djinterp::is_bifunctor< std::pair<int, int>&& >::value;
    const bool kv_volatile    = ::djinterp::is_bifunctor< volatile ::djinterp::kv_pair<int, int> >::value;
    const bool two_const_ref  = ::djinterp::is_bifunctor< const two<int, int>& >::value;

    return ( pair_const_ref &&
             pair_rvalue    &&
             kv_volatile    &&
             two_const_ref );
}


///////////////////////////////////////////////////////////////////////////////
///                PARAMETER TYPES:  first_type / second_type                ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_first_second_type_pair
  Tests the following:
  - the two parameter types of std::pair are recovered.
*/
static bool
tests_first_second_type_pair()
{
    const bool first_ok =
        std::is_same< ::djinterp::bifunctor_first_type_t< std::pair<int, std::string> >,
                      int >::value;

    const bool second_ok =
        std::is_same< ::djinterp::bifunctor_second_type_t< std::pair<int, std::string> >,
                      std::string >::value;

    return (first_ok && second_ok);
}

/*
tests_first_second_type_kv
  Tests the following:
  - the two parameter types of kv_pair are recovered.
*/
static bool
tests_first_second_type_kv()
{
    const bool first_ok =
        std::is_same< ::djinterp::bifunctor_first_type_t< ::djinterp::kv_pair<char, double> >,
                      char >::value;

    const bool second_ok =
        std::is_same< ::djinterp::bifunctor_second_type_t< ::djinterp::kv_pair<char, double> >,
                      double >::value;

    return (first_ok && second_ok);
}

/*
tests_first_second_type_custom
  Tests the following:
  - the two parameter types of the custom two<A,B> fixture are recovered.
*/
static bool
tests_first_second_type_custom()
{
    const bool first_ok =
        std::is_same< ::djinterp::bifunctor_first_type_t< two<int, std::string> >,
                      int >::value;

    const bool second_ok =
        std::is_same< ::djinterp::bifunctor_second_type_t< two<int, std::string> >,
                      std::string >::value;

    return (first_ok && second_ok);
}

/*
tests_first_second_type_decays
  Tests the following:
  - the parameter-type traits decay their argument.
*/
static bool
tests_first_second_type_decays()
{
    const bool first_ok =
        std::is_same< ::djinterp::bifunctor_first_type_t< const std::pair<double, int>& >,
                      double >::value;

    const bool second_ok =
        std::is_same< ::djinterp::bifunctor_second_type_t< const std::pair<double, int>& >,
                      int >::value;

    return (first_ok && second_ok);
}

/*
tests_bi_types_present_absent
  Tests the following:
  - the bifunctor_traits parameter-type surface is present for bifunctors
    (shipped and custom) and absent for non-bifunctors, detected the SFINAE-safe
    way (probing the undefined primary trait).
*/
static bool
tests_bi_types_present_absent()
{
    const bool present_pair = has_bi_types< std::pair<int, int> >::value;
    const bool present_kv   = has_bi_types< ::djinterp::kv_pair<int, int> >::value;
    const bool present_two  = has_bi_types< two<int, int> >::value;
    const bool absent_int   = has_bi_types< int >::value;
    const bool absent_vec   = has_bi_types< std::vector<int> >::value;

    return ( present_pair   &&
             present_kv     &&
             present_two    &&
             (!absent_int)  &&
             (!absent_vec) );
}


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

/*
tests_is_bifunctor_v
  Tests the following:
  - the is_bifunctor_v shorthand agrees with is_bifunctor<>::value across a
    shipped instance, a custom instance, and a non-bifunctor.  (C++14+.)
*/
static bool
tests_is_bifunctor_v()
{
    return ( ::djinterp::is_bifunctor_v< std::pair<int, int> > &&
             ::djinterp::is_bifunctor_v< two<int, int> >       &&
             (!::djinterp::is_bifunctor_v<int>) );
}

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

/*
tests_concept_bifunctor
  Tests the following:
  - the Bifunctor concept is satisfied by the shipped instances and the custom
    fixture, and not by a scalar.  (C++20.)
*/
static bool
tests_concept_bifunctor()
{
    const bool pair_ok = ::djinterp::Bifunctor< std::pair<int, int> >;
    const bool kv_ok   = ::djinterp::Bifunctor< ::djinterp::kv_pair<int, int> >;
    const bool two_ok  = ::djinterp::Bifunctor< two<int, int> >;
    const bool int_no  = ::djinterp::Bifunctor< int >;

    return ( pair_ok && kv_ok && two_ok && (!int_no) );
}

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
bifunctor_protocol_block()
{
    dt::block_spec block;

    block.name       = "0+I. protocol & traits";
    block.descriptor =
        "is_bifunctor / first_type / second_type / concept across instances";

    block.tests.push_back(dt::test_spec{
        "is_bifunctor: std::pair",
        "shipped pair instance is detected",
        &tests_is_bifunctor_pair });

    block.tests.push_back(dt::test_spec{
        "is_bifunctor: kv_pair",
        "shipped kv_pair instance is detected",
        &tests_is_bifunctor_kv });

    block.tests.push_back(dt::test_spec{
        "is_bifunctor: custom two",
        "user-defined bifunctor is detected",
        &tests_is_bifunctor_custom });

    block.tests.push_back(dt::test_spec{
        "is_bifunctor: int",
        "is_bifunctor<int> is false",
        &tests_is_bifunctor_int_false });

    block.tests.push_back(dt::test_spec{
        "is_bifunctor: std::string",
        "is_bifunctor<string> is false",
        &tests_is_bifunctor_string_false });

    block.tests.push_back(dt::test_spec{
        "is_bifunctor: container",
        "a one-parameter container is not a bifunctor",
        &tests_is_bifunctor_container_false });

    block.tests.push_back(dt::test_spec{
        "is_bifunctor: cv/ref decay",
        "const/ref/rvalue/volatile bifunctors still detected",
        &tests_is_bifunctor_cvref_decays });

    block.tests.push_back(dt::test_spec{
        "first/second_type: std::pair",
        "both parameter types recovered",
        &tests_first_second_type_pair });

    block.tests.push_back(dt::test_spec{
        "first/second_type: kv_pair",
        "both parameter types recovered",
        &tests_first_second_type_kv });

    block.tests.push_back(dt::test_spec{
        "first/second_type: custom two",
        "both parameter types recovered",
        &tests_first_second_type_custom });

    block.tests.push_back(dt::test_spec{
        "first/second_type: cv/ref decay",
        "parameter-type traits decay their argument",
        &tests_first_second_type_decays });

    block.tests.push_back(dt::test_spec{
        "parameter types: present vs absent",
        "trait parameter types exist for bifunctors, absent otherwise",
        &tests_bi_types_present_absent });

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    block.tests.push_back(dt::test_spec{
        "is_bifunctor_v shorthand",
        "is_bifunctor_v agrees with is_bifunctor<>::value (C++14+)",
        &tests_is_bifunctor_v });
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    block.tests.push_back(dt::test_spec{
        "Bifunctor concept",
        "Bifunctor satisfied by instances + custom, not by int (C++20)",
        &tests_concept_bifunctor });
#endif

    return block;
}


NS_END  // testing
NS_END  // djinterp
