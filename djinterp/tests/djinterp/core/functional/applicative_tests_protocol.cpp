/******************************************************************************
* djinterp [test]                               applicative_tests_protocol.cpp
*
*   Section 0 + I of the applicative.hpp suite: the protocol and its detection
* vocabulary.  Covers is_applicative (and its _v shorthand), the monad-bridge
* instance's markers (is_specialized / value_type / rebind) and the direct
* ident instance's markers, applicative_value_type / _t (well-formed and
* SFINAE-safe absent arms), the C++20 Applicative concept, and the layering of
* the monadic protocols (box is monad + functor + applicative; the direct ident
* is functor + applicative but NOT a monad; a scalar is none).
*
* path:      /tests/djinterp/core/functional/applicative_tests_protocol.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "applicative_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///                DETECTION:  is_applicative                                ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_is_applicative_box_bridge
  Tests the following:
  - is_applicative is true for the box<T> monad -- i.e. the monad bridge makes
    every monad an applicative with no per-type wiring.
*/
static bool
tests_is_applicative_box_bridge()
{
    return ( ::djinterp::is_applicative< box<int> >::value &&
             ::djinterp::is_applicative< box<std::string> >::value );
}

/*
tests_is_applicative_ident_direct
  Tests the following:
  - is_applicative is true for the directly-specialized (non-monad) ident<T>,
    proving detection keys on any applicative_traits specialization, bridge or
    direct.
*/
static bool
tests_is_applicative_ident_direct()
{
    return ( ::djinterp::is_applicative< ident<int> >::value &&
             ::djinterp::is_applicative< ident<std::string> >::value );
}

/*
tests_is_applicative_int_false
  Tests the following:
  - is_applicative is false for a scalar (no applicative_traits, and not a
    monad, so the bridge does not apply).
*/
static bool
tests_is_applicative_int_false()
{
    return (!::djinterp::is_applicative<int>::value);
}

/*
tests_is_applicative_string_false
  Tests the following:
  - is_applicative is false for an unrelated standard type.
*/
static bool
tests_is_applicative_string_false()
{
    return (!::djinterp::is_applicative<std::string>::value);
}

/*
tests_is_applicative_cvref_decays
  Tests the following:
  - is_applicative strips cv-qualifiers and references before deciding, for both
    a bridge applicative (box) and a direct one (ident).
*/
static bool
tests_is_applicative_cvref_decays()
{
    const bool box_const_ref = ::djinterp::is_applicative< const box<int>& >::value;
    const bool box_rvalue    = ::djinterp::is_applicative< box<int>&& >::value;
    const bool ident_volatile= ::djinterp::is_applicative< volatile ident<int> >::value;
    const bool ident_cref    = ::djinterp::is_applicative< const ident<double>& >::value;

    return ( box_const_ref  &&
             box_rvalue     &&
             ident_volatile &&
             ident_cref );
}

/*
tests_protocol_orthogonality
  Tests the following:
  - the monadic protocol layering holds: box is a monad AND a functor AND an
    applicative; the direct ident is a functor and an applicative but NOT a
    monad; and a scalar is none of the three.
*/
static bool
tests_protocol_orthogonality()
{
    const bool box_monad = ::djinterp::is_monad< box<int> >::value;        // true
    const bool box_func  = ::djinterp::is_functor< box<int> >::value;      // true
    const bool box_appl  = ::djinterp::is_applicative< box<int> >::value;  // true

    const bool id_monad  = ::djinterp::is_monad< ident<int> >::value;      // false
    const bool id_func   = ::djinterp::is_functor< ident<int> >::value;    // true
    const bool id_appl   = ::djinterp::is_applicative< ident<int> >::value;// true

    const bool int_monad = ::djinterp::is_monad< int >::value;             // false
    const bool int_func  = ::djinterp::is_functor< int >::value;           // false
    const bool int_appl  = ::djinterp::is_applicative< int >::value;       // false

    return ( box_monad && box_func && box_appl &&
             (!id_monad) && id_func && id_appl &&
             (!int_monad) && (!int_func) && (!int_appl) );
}


///////////////////////////////////////////////////////////////////////////////
///                INNER TYPE:  applicative_value_type                       ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_app_value_type_box
  Tests the following:
  - applicative_value_type_t recovers the inner type of the bridge applicative
    box<T> for scalar and class element types.
*/
static bool
tests_app_value_type_box()
{
    const bool int_inner =
        std::is_same< ::djinterp::applicative_value_type_t< box<int> >,
                      int >::value;

    const bool str_inner =
        std::is_same< ::djinterp::applicative_value_type_t< box<std::string> >,
                      std::string >::value;

    return (int_inner && str_inner);
}

/*
tests_app_value_type_ident
  Tests the following:
  - applicative_value_type_t recovers the inner type of the direct applicative
    ident<T>.
*/
static bool
tests_app_value_type_ident()
{
    return std::is_same< ::djinterp::applicative_value_type_t< ident<int> >,
                         int >::value;
}

/*
tests_app_value_type_decays
  Tests the following:
  - applicative_value_type_t decays its argument.
*/
static bool
tests_app_value_type_decays()
{
    return std::is_same<
        ::djinterp::applicative_value_type_t< const box<double>& >,
        double >::value;
}

/*
tests_app_value_type_present_absent
  Tests the following:
  - the applicative_traits value_type surface is present for applicatives
    (bridge and direct) and absent for non-applicatives, detected the
    SFINAE-safe way (probing the undefined primary trait).
*/
static bool
tests_app_value_type_present_absent()
{
    const bool present_box   = has_app_value_type< box<int> >::value;
    const bool present_ident = has_app_value_type< ident<int> >::value;
    const bool absent_int    = has_app_value_type< int >::value;
    const bool absent_string = has_app_value_type< std::string >::value;

    return ( present_box      &&
             present_ident    &&
             (!absent_int)    &&
             (!absent_string) );
}


///////////////////////////////////////////////////////////////////////////////
///                INSTANCE MARKERS  (bridge + direct)                       ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_bridge_traits_markers
  Tests the following:
  - the monad-bridge instance publishes is_specialized == true_type, value_type
    == the inner type, and rebind<U> == box<U> (carried through monad_rebind).
*/
static bool
tests_bridge_traits_markers()
{
    const bool specialized =
        ::djinterp::applicative_traits< box<int> >::is_specialized::value;

    const bool value_is_int =
        std::is_same<
            ::djinterp::applicative_traits< box<int> >::value_type,
            int >::value;

    const bool rebinds =
        std::is_same<
            ::djinterp::applicative_traits< box<int> >::rebind<char>,
            box<char> >::value;

    return (specialized && value_is_int && rebinds);
}

/*
tests_direct_traits_markers
  Tests the following:
  - the direct ident instance publishes is_specialized == true_type and
    value_type == the inner type.
*/
static bool
tests_direct_traits_markers()
{
    const bool specialized =
        ::djinterp::applicative_traits< ident<int> >::is_specialized::value;

    const bool value_is_int =
        std::is_same<
            ::djinterp::applicative_traits< ident<int> >::value_type,
            int >::value;

    return (specialized && value_is_int);
}


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

/*
tests_is_applicative_v
  Tests the following:
  - the is_applicative_v shorthand agrees with is_applicative<>::value across a
    bridge applicative, a direct applicative, and a non-applicative.  (C++14+.)
*/
static bool
tests_is_applicative_v()
{
    return ( ::djinterp::is_applicative_v< box<int> >   &&
             ::djinterp::is_applicative_v< ident<int> > &&
             (!::djinterp::is_applicative_v<int>) );
}

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

/*
tests_concept_applicative
  Tests the following:
  - the Applicative concept is satisfied by box and ident and not by a scalar.
    (C++20.)
*/
static bool
tests_concept_applicative()
{
    const bool box_ok   = ::djinterp::Applicative< box<int> >;
    const bool ident_ok = ::djinterp::Applicative< ident<int> >;
    const bool int_no   = ::djinterp::Applicative< int >;

    return ( box_ok && ident_ok && (!int_no) );
}

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
applicative_protocol_block()
{
    dt::block_spec block;

    block.name       = "0+I. protocol & traits";
    block.descriptor =
        "is_applicative / value_type / bridge + direct markers / concept / layering";

    block.tests.push_back(dt::test_spec{
        "is_applicative: box (bridge)",
        "monad bridge makes box an applicative",
        &tests_is_applicative_box_bridge });

    block.tests.push_back(dt::test_spec{
        "is_applicative: ident (direct)",
        "direct specialization is detected",
        &tests_is_applicative_ident_direct });

    block.tests.push_back(dt::test_spec{
        "is_applicative: int",
        "is_applicative<int> is false",
        &tests_is_applicative_int_false });

    block.tests.push_back(dt::test_spec{
        "is_applicative: std::string",
        "is_applicative<string> is false",
        &tests_is_applicative_string_false });

    block.tests.push_back(dt::test_spec{
        "is_applicative: cv/ref decay",
        "const/ref/rvalue/volatile applicatives still detected",
        &tests_is_applicative_cvref_decays });

    block.tests.push_back(dt::test_spec{
        "protocol layering",
        "box: monad+functor+applicative; ident: not a monad; int: none",
        &tests_protocol_orthogonality });

    block.tests.push_back(dt::test_spec{
        "value_type: box (bridge)",
        "applicative_value_type_t recovers inner type",
        &tests_app_value_type_box });

    block.tests.push_back(dt::test_spec{
        "value_type: ident (direct)",
        "applicative_value_type_t recovers inner type",
        &tests_app_value_type_ident });

    block.tests.push_back(dt::test_spec{
        "value_type: cv/ref decay",
        "applicative_value_type_t decays its argument",
        &tests_app_value_type_decays });

    block.tests.push_back(dt::test_spec{
        "value_type: present vs absent",
        "trait value_type exists for applicatives, absent otherwise",
        &tests_app_value_type_present_absent });

    block.tests.push_back(dt::test_spec{
        "bridge instance: markers",
        "is_specialized/value_type/rebind for the monad bridge",
        &tests_bridge_traits_markers });

    block.tests.push_back(dt::test_spec{
        "direct instance: markers",
        "is_specialized/value_type for the direct ident",
        &tests_direct_traits_markers });

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    block.tests.push_back(dt::test_spec{
        "is_applicative_v shorthand",
        "is_applicative_v agrees with is_applicative<>::value (C++14+)",
        &tests_is_applicative_v });
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    block.tests.push_back(dt::test_spec{
        "Applicative concept",
        "Applicative satisfied by box/ident, not by int (C++20)",
        &tests_concept_applicative });
#endif

    return block;
}


NS_END  // testing
NS_END  // djinterp
