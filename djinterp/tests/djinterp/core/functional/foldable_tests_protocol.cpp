/******************************************************************************
* djinterp [test]                                   foldable_tests_protocol.cpp
*
*   Section 0 + I of the foldable.hpp suite: the protocol and its detection
* vocabulary.  Covers is_foldable (and its _v shorthand), the std::vector
* foldable instance's markers and direct fold_left, foldable_value_type / _t
* (both the well-formed and the soft-failure arms, the latter via the
* has_value_type detector), and the C++20 Foldable concept.  The seq<T> fixture
* participates so detection and value-type extraction are proven on a non-std
* foldable as well.
*
* path:      /tests/djinterp/core/functional/foldable_tests_protocol.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "foldable_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///                DETECTION:  is_foldable                                   ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_is_foldable_vector_true
  Tests the following:
  - is_foldable is true for the header's own std::vector instance.
*/
static bool
tests_is_foldable_vector_true()
{
    return ( ::djinterp::is_foldable< std::vector<int> >::value &&
             ::djinterp::is_foldable< std::vector<std::string> >::value );
}

/*
tests_is_foldable_seq_true
  Tests the following:
  - is_foldable is true for the non-std seq<T> fixture, proving detection keys
    on the foldable_traits specialization, not on std::vector specifically.
*/
static bool
tests_is_foldable_seq_true()
{
    return ( ::djinterp::is_foldable< seq<int> >::value &&
             ::djinterp::is_foldable< seq<char> >::value );
}

/*
tests_is_foldable_int_false
  Tests the following:
  - is_foldable is false for a scalar with no foldable_traits specialization.
*/
static bool
tests_is_foldable_int_false()
{
    return (!::djinterp::is_foldable<int>::value);
}

/*
tests_is_foldable_string_false
  Tests the following:
  - is_foldable is false for std::string: this header specializes the protocol
    for std::vector only, so an unrelated standard type is (correctly) not a
    foldable here.
*/
static bool
tests_is_foldable_string_false()
{
    return (!::djinterp::is_foldable<std::string>::value);
}

/*
tests_is_foldable_cvref_decays
  Tests the following:
  - is_foldable strips cv-qualifiers and references before deciding, so const /
    reference / rvalue / volatile forms of a foldable are still foldable.
*/
static bool
tests_is_foldable_cvref_decays()
{
    const bool as_const_ref  = ::djinterp::is_foldable< const std::vector<int>& >::value;
    const bool as_rvalue_ref = ::djinterp::is_foldable< std::vector<int>&& >::value;
    const bool as_volatile   = ::djinterp::is_foldable< volatile std::vector<int> >::value;
    const bool seq_const_ref = ::djinterp::is_foldable< const seq<char>& >::value;

    return ( as_const_ref  &&
             as_rvalue_ref &&
             as_volatile   &&
             seq_const_ref );
}


///////////////////////////////////////////////////////////////////////////////
///                INNER TYPE:  foldable_value_type / has_value_type         ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_value_type_vector
  Tests the following:
  - foldable_value_type_t<vector<T>> recovers the element type T for scalar and
    class element types.
*/
static bool
tests_value_type_vector()
{
    const bool int_elem =
        std::is_same< ::djinterp::foldable_value_type_t< std::vector<int> >,
                      int >::value;

    const bool str_elem =
        std::is_same< ::djinterp::foldable_value_type_t< std::vector<std::string> >,
                      std::string >::value;

    return (int_elem && str_elem);
}

/*
tests_value_type_seq
  Tests the following:
  - foldable_value_type_t recovers the element type of the non-std seq<T>
    fixture, confirming extraction is generic over the protocol.
*/
static bool
tests_value_type_seq()
{
    return std::is_same< ::djinterp::foldable_value_type_t< seq<char> >,
                         char >::value;
}

/*
tests_value_type_decays
  Tests the following:
  - foldable_value_type_t decays its argument, so a cv / reference form of a
    foldable yields the same element type as the bare form.
*/
static bool
tests_value_type_decays()
{
    return std::is_same<
        ::djinterp::foldable_value_type_t< const std::vector<double>& >,
        double >::value;
}

/*
tests_value_type_present_absent
  Tests the following:
  - the foldable_traits value_type surface (what foldable_value_type extracts)
    is present for foldables and absent for non-foldables, detected the
    SFINAE-safe way (probing the undefined primary trait, not the
    unconditionally-declared foldable_value_type<T>::type).
*/
static bool
tests_value_type_present_absent()
{
    const bool present_vector = has_traits_value_type< std::vector<int> >::value;
    const bool present_seq    = has_traits_value_type< seq<int> >::value;
    const bool absent_int     = has_traits_value_type< int >::value;
    const bool absent_string  = has_traits_value_type< std::string >::value;

    return ( present_vector &&
             present_seq    &&
             (!absent_int)  &&
             (!absent_string) );
}


///////////////////////////////////////////////////////////////////////////////
///                THE std::vector INSTANCE                                  ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_vector_traits_markers
  Tests the following:
  - the std::vector instance publishes is_specialized == true_type,
  - and value_type == the element type.
*/
static bool
tests_vector_traits_markers()
{
    const bool specialized =
        ::djinterp::foldable_traits< std::vector<int> >::is_specialized::value;

    const bool value_is_int =
        std::is_same<
            ::djinterp::foldable_traits< std::vector<int> >::value_type,
            int >::value;

    return (specialized && value_is_int);
}

/*
tests_vector_traits_fold_left_direct
  Tests the following:
  - the instance's own static fold_left folds vector elements left-to-right,
    exercised directly (not through the generic ::djinterp::fold_left wrapper)
    so the delegation target is verified in isolation.
*/
static bool
tests_vector_traits_fold_left_direct()
{
    std::vector<int> xs;
    int              sum;

    xs.push_back(2);
    xs.push_back(3);
    xs.push_back(5);

    sum = ::djinterp::foldable_traits< std::vector<int> >::fold_left(
        xs,
        0,
        [](int _acc, int _x) -> int
        {
            return _acc + _x;
        });

    return (sum == 10);
}


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

/*
tests_is_foldable_v
  Tests the following:
  - the is_foldable_v shorthand agrees with is_foldable<>::value for both a
    foldable and a non-foldable.  (C++14+.)
*/
static bool
tests_is_foldable_v()
{
    return ( ::djinterp::is_foldable_v< std::vector<int> > &&
             ::djinterp::is_foldable_v< seq<int> >         &&
             (!::djinterp::is_foldable_v<int>) );
}

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

/*
tests_concept_foldable
  Tests the following:
  - the Foldable concept is satisfied by std::vector and by the seq<T> fixture,
  - and is NOT satisfied by a non-foldable scalar.  (C++20.)
*/
static bool
tests_concept_foldable()
{
    const bool vector_ok = ::djinterp::Foldable< std::vector<int> >;
    const bool seq_ok    = ::djinterp::Foldable< seq<int> >;
    const bool int_no    = ::djinterp::Foldable< int >;

    return ( vector_ok &&
             seq_ok    &&
             (!int_no) );
}

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
foldable_protocol_block()
{
    dt::block_spec block;

    block.name       = "0+I. protocol & traits";
    block.descriptor =
        "is_foldable / value_type / vector instance / concept";

    block.tests.push_back(dt::test_spec{
        "is_foldable: std::vector",
        "is_foldable<vector<T>> is true",
        &tests_is_foldable_vector_true });

    block.tests.push_back(dt::test_spec{
        "is_foldable: seq fixture",
        "is_foldable<seq<T>> is true (non-std foldable)",
        &tests_is_foldable_seq_true });

    block.tests.push_back(dt::test_spec{
        "is_foldable: int",
        "is_foldable<int> is false",
        &tests_is_foldable_int_false });

    block.tests.push_back(dt::test_spec{
        "is_foldable: std::string",
        "is_foldable<string> is false (not specialized here)",
        &tests_is_foldable_string_false });

    block.tests.push_back(dt::test_spec{
        "is_foldable: cv/ref decay",
        "const/ref/rvalue/volatile foldables still detected",
        &tests_is_foldable_cvref_decays });

    block.tests.push_back(dt::test_spec{
        "value_type: std::vector",
        "foldable_value_type_t recovers element type",
        &tests_value_type_vector });

    block.tests.push_back(dt::test_spec{
        "value_type: seq fixture",
        "foldable_value_type_t recovers seq element type",
        &tests_value_type_seq });

    block.tests.push_back(dt::test_spec{
        "value_type: cv/ref decay",
        "foldable_value_type_t decays its argument",
        &tests_value_type_decays });

    block.tests.push_back(dt::test_spec{
        "value_type: present vs absent",
        "trait value_type exists for foldables, absent otherwise",
        &tests_value_type_present_absent });

    block.tests.push_back(dt::test_spec{
        "vector instance: markers",
        "is_specialized == true_type, value_type == element",
        &tests_vector_traits_markers });

    block.tests.push_back(dt::test_spec{
        "vector instance: direct fold_left",
        "instance fold_left folds elements left-to-right",
        &tests_vector_traits_fold_left_direct });

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    block.tests.push_back(dt::test_spec{
        "is_foldable_v shorthand",
        "is_foldable_v agrees with is_foldable<>::value (C++14+)",
        &tests_is_foldable_v });
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    block.tests.push_back(dt::test_spec{
        "Foldable concept",
        "Foldable satisfied by vector/seq, not by int (C++20)",
        &tests_concept_foldable });
#endif

    return block;
}


NS_END  // testing
NS_END  // djinterp
