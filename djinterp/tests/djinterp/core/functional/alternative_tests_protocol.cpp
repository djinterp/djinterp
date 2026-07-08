/******************************************************************************
* djinterp [test]                                alternative_tests_protocol.cpp
*
*   Section 0 + I of the alternative.hpp suite: the protocol and its detection
* vocabulary.  Covers is_alternative (and its _v shorthand), the opt<T> fixture
* instance's markers and direct empty()/choice(), alternative_value_type / _t
* (well-formed and SFINAE-safe absent arms), the C++20 Alternative concept, and
* the orthogonality of the Alternative and Foldable protocols (opt<T> is an
* Alternative but not a Foldable; the bag<T> Foldable fixture is the reverse).
*
* path:      /tests/djinterp/core/functional/alternative_tests_protocol.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "alternative_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///                DETECTION:  is_alternative                                ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_is_alternative_opt_true
  Tests the following:
  - is_alternative is true for the opt<T> fixture at more than one element type.
*/
static bool
tests_is_alternative_opt_true()
{
    return ( ::djinterp::is_alternative< opt<int> >::value &&
             ::djinterp::is_alternative< opt<std::string> >::value );
}

/*
tests_is_alternative_int_false
  Tests the following:
  - is_alternative is false for a scalar with no alternative_traits.
*/
static bool
tests_is_alternative_int_false()
{
    return (!::djinterp::is_alternative<int>::value);
}

/*
tests_is_alternative_string_false
  Tests the following:
  - is_alternative is false for an unrelated standard type.
*/
static bool
tests_is_alternative_string_false()
{
    return (!::djinterp::is_alternative<std::string>::value);
}

/*
tests_is_alternative_cvref_decays
  Tests the following:
  - is_alternative strips cv-qualifiers and references before deciding, so
    const / reference / rvalue / volatile forms of an alternative still qualify.
*/
static bool
tests_is_alternative_cvref_decays()
{
    const bool as_const_ref  = ::djinterp::is_alternative< const opt<int>& >::value;
    const bool as_rvalue_ref = ::djinterp::is_alternative< opt<int>&& >::value;
    const bool as_volatile   = ::djinterp::is_alternative< volatile opt<int> >::value;
    const bool str_const_ref = ::djinterp::is_alternative< const opt<std::string>& >::value;

    return ( as_const_ref  &&
             as_rvalue_ref &&
             as_volatile   &&
             str_const_ref );
}

/*
tests_protocol_orthogonality
  Tests the following:
  - the Alternative and Foldable protocols are independent: opt<T> is an
    Alternative but not a Foldable, and the bag<T> fixture is a Foldable but not
    an Alternative.
*/
static bool
tests_protocol_orthogonality()
{
    const bool opt_is_alt   = ::djinterp::is_alternative< opt<int> >::value;   // true
    const bool opt_not_fold = ::djinterp::is_foldable< opt<int> >::value;      // false
    const bool bag_is_fold  = ::djinterp::is_foldable< bag<int> >::value;      // true
    const bool bag_not_alt  = ::djinterp::is_alternative< bag<int> >::value;   // false

    return ( opt_is_alt      &&
             (!opt_not_fold) &&
             bag_is_fold     &&
             (!bag_not_alt) );
}


///////////////////////////////////////////////////////////////////////////////
///                INNER TYPE:  alternative_value_type                       ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_alt_value_type_opt
  Tests the following:
  - alternative_value_type_t recovers the inner type of opt<T> for scalar and
    class element types.
*/
static bool
tests_alt_value_type_opt()
{
    const bool int_inner =
        std::is_same< ::djinterp::alternative_value_type_t< opt<int> >,
                      int >::value;

    const bool str_inner =
        std::is_same< ::djinterp::alternative_value_type_t< opt<std::string> >,
                      std::string >::value;

    return (int_inner && str_inner);
}

/*
tests_alt_value_type_decays
  Tests the following:
  - alternative_value_type_t decays its argument, so a cv / reference form yields
    the same inner type as the bare form.
*/
static bool
tests_alt_value_type_decays()
{
    return std::is_same<
        ::djinterp::alternative_value_type_t< const opt<double>& >,
        double >::value;
}

/*
tests_value_type_present_absent
  Tests the following:
  - the alternative_traits value_type surface is present for an alternative and
    absent for a non-alternative, detected the SFINAE-safe way (probing the
    undefined primary trait, not alternative_value_type<T>::type).
*/
static bool
tests_value_type_present_absent()
{
    const bool present_opt   = has_alt_value_type< opt<int> >::value;
    const bool absent_int    = has_alt_value_type< int >::value;
    const bool absent_string = has_alt_value_type< std::string >::value;
    const bool absent_bag    = has_alt_value_type< bag<int> >::value;   // foldable, not alt

    return ( present_opt      &&
             (!absent_int)    &&
             (!absent_string) &&
             (!absent_bag) );
}


///////////////////////////////////////////////////////////////////////////////
///                THE opt<T> INSTANCE                                       ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_alt_traits_markers
  Tests the following:
  - the opt<T> instance publishes is_specialized == true_type and value_type ==
    the inner type,
  - and its empty()/choice() static operations behave (empty is disengaged;
    choice keeps the first engaged operand), the fixture the generic ops rely on.
*/
static bool
tests_alt_traits_markers()
{
    const bool specialized =
        ::djinterp::alternative_traits< opt<int> >::is_specialized::value;

    const bool value_is_int =
        std::is_same<
            ::djinterp::alternative_traits< opt<int> >::value_type,
            int >::value;

    const opt<int> e = ::djinterp::alternative_traits< opt<int> >::empty();
    const opt<int> c = ::djinterp::alternative_traits< opt<int> >::choice(
        opt<int>(1), opt<int>(2));

    return ( specialized      &&
             value_is_int     &&
             (!e.engaged)      &&
             (c == opt<int>(1)) );
}


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

/*
tests_is_alternative_v
  Tests the following:
  - the is_alternative_v shorthand agrees with is_alternative<>::value for both
    an alternative and a non-alternative.  (C++14+.)
*/
static bool
tests_is_alternative_v()
{
    return ( ::djinterp::is_alternative_v< opt<int> > &&
             (!::djinterp::is_alternative_v<int>) );
}

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

/*
tests_concept_alternative
  Tests the following:
  - the Alternative concept is satisfied by opt<T> and not by a non-alternative
    scalar.  (C++20.)
*/
static bool
tests_concept_alternative()
{
    const bool opt_ok = ::djinterp::Alternative< opt<int> >;
    const bool int_no = ::djinterp::Alternative< int >;

    return ( opt_ok &&
             (!int_no) );
}

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
alternative_protocol_block()
{
    dt::block_spec block;

    block.name       = "0+I. protocol & traits";
    block.descriptor =
        "is_alternative / value_type / instance markers / concept / orthogonality";

    block.tests.push_back(dt::test_spec{
        "is_alternative: opt fixture",
        "is_alternative<opt<T>> is true",
        &tests_is_alternative_opt_true });

    block.tests.push_back(dt::test_spec{
        "is_alternative: int",
        "is_alternative<int> is false",
        &tests_is_alternative_int_false });

    block.tests.push_back(dt::test_spec{
        "is_alternative: std::string",
        "is_alternative<string> is false",
        &tests_is_alternative_string_false });

    block.tests.push_back(dt::test_spec{
        "is_alternative: cv/ref decay",
        "const/ref/rvalue/volatile alternatives still detected",
        &tests_is_alternative_cvref_decays });

    block.tests.push_back(dt::test_spec{
        "protocol orthogonality",
        "Alternative and Foldable are independent protocols",
        &tests_protocol_orthogonality });

    block.tests.push_back(dt::test_spec{
        "value_type: opt fixture",
        "alternative_value_type_t recovers inner type",
        &tests_alt_value_type_opt });

    block.tests.push_back(dt::test_spec{
        "value_type: cv/ref decay",
        "alternative_value_type_t decays its argument",
        &tests_alt_value_type_decays });

    block.tests.push_back(dt::test_spec{
        "value_type: present vs absent",
        "trait value_type exists for alternatives, absent otherwise",
        &tests_value_type_present_absent });

    block.tests.push_back(dt::test_spec{
        "instance: markers + operations",
        "is_specialized/value_type; empty disengaged; choice keeps first",
        &tests_alt_traits_markers });

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    block.tests.push_back(dt::test_spec{
        "is_alternative_v shorthand",
        "is_alternative_v agrees with is_alternative<>::value (C++14+)",
        &tests_is_alternative_v });
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    block.tests.push_back(dt::test_spec{
        "Alternative concept",
        "Alternative satisfied by opt, not by int (C++20)",
        &tests_concept_alternative });
#endif

    return block;
}


NS_END  // testing
NS_END  // djinterp
