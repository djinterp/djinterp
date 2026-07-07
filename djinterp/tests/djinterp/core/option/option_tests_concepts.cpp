/******************************************************************************
* djinterp [test]                                    option_tests_concepts.cpp
*
*   Section IV of the option.hpp suite: the C++20 concept analogs.
*
*     Option<_Type>        - satisfied iff is_option_v<_Type> (so it strips
*                            cv/ref, exactly like the trait it parallels).
*     UnaryOption<_Type>   - Option<_Type> AND _Type::has_args == false.
*     ArgsOption<_Type>    - Option<_Type> AND _Type::has_args == true, with a
*                            reachable _Type::args_type.
*
*   These exist only where the toolchain provides concepts; below C++20 the
* whole section is gated out and the block provider yields an empty block, so
* the runner needs no version gate at its call site.
*
*   The concepts are exercised three ways for rigor: as constexpr booleans
* (tracking the trait, including the cv/ref asymmetry between Option, which
* cleans, and UnaryOption/ArgsOption, which read the member directly and so
* soft-fail on a reference), as mutually-exclusive / jointly-exhaustive
* classifiers over option<> types, and as actual OVERLOAD CONSTRAINTS that
* steer template selection.
*
*
* path:      /tests/djinterp/core/option/option_tests_concepts.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

// std
#include <type_traits>
// djinterp
#include "option_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

namespace
{
    // constrained-overload probe: two classify() overloads, each accepting a
    // different concept, so which one is viable proves the concept steers
    // selection (not merely that it evaluates to a bool).
    template<UnaryOption _T>
    constexpr int classify(_T) { return 1; }

    template<ArgsOption _T>
    constexpr int classify(_T) { return 2; }
}


// concept_option_tracks_trait
//   Option is exactly the concept face of is_option_v: true for options
// (including cv/ref-decorated ones, since it cleans), false for non-options
// and for the duck-typed decoy.
bool
concept_option_tracks_trait()
{
    constexpr bool ok =
        Option<option<opt_key::alpha>>                &&
        Option<option<opt_key::alpha, arg_a>>         &&
        Option<option<opt_key::alpha>&>               &&   // Option cleans
        Option<const option<opt_key::alpha>>          &&
        Option<const option<opt_key::alpha, arg_a>&>  &&
        !Option<int>                                  &&
        !Option<void>                                 &&
        !Option<option_decoy>;

    static_assert(ok, "Option tracks is_option_v (cleans cv/ref)");
    return ok;
}

// concept_unary_positive_negative
//   UnaryOption holds for a unary option and fails for an args option and for
// a non-option.
bool
concept_unary_positive_negative()
{
    constexpr bool ok =
        UnaryOption<option<opt_key::alpha>>       &&
        UnaryOption<option<nullptr>>              &&
        !UnaryOption<option<opt_key::alpha, arg_a>>   &&
        !UnaryOption<int>;

    static_assert(ok, "UnaryOption: true for unary, false otherwise");
    return ok;
}

// concept_args_positive_negative
//   ArgsOption holds for an args option of any arity and fails for a unary
// option and for a non-option.
bool
concept_args_positive_negative()
{
    constexpr bool ok =
        ArgsOption<option<opt_key::alpha, arg_a>>            &&
        ArgsOption<option<opt_key::alpha, arg_a, arg_b>>     &&
        !ArgsOption<option<opt_key::alpha>>                  &&
        !ArgsOption<int>;

    static_assert(ok, "ArgsOption: true for args form, false otherwise");
    return ok;
}

// concept_unary_args_exclusive
//   over option<> types the two shape concepts partition the space: exactly
// one of UnaryOption / ArgsOption holds (their values differ), and neither
// holds for a non-option.
bool
concept_unary_args_exclusive()
{
    constexpr bool ok =
        (UnaryOption<option<opt_key::alpha>>
             != ArgsOption<option<opt_key::alpha>>)                 &&
        (UnaryOption<option<opt_key::alpha, arg_a>>
             != ArgsOption<option<opt_key::alpha, arg_a>>)          &&
        (!UnaryOption<int> && !ArgsOption<int>);

    static_assert(ok, "UnaryOption and ArgsOption partition option<> types");
    return ok;
}

// concept_reference_edge
//   the documented cv/ref asymmetry: Option cleans (so Option<option<K>&> is
// true), but UnaryOption / ArgsOption read _Type::has_args directly, so on a
// reference type the nested requirement is unsatisfied - a well-formed false,
// not a hard error.
bool
concept_reference_edge()
{
    constexpr bool ok =
        Option<option<opt_key::alpha>&>            &&   // cleans -> true
        !UnaryOption<option<opt_key::alpha>&>      &&   // member access on ref -> false
        !ArgsOption<option<opt_key::alpha, arg_a>&>;    // member access on ref -> false

    static_assert(ok, "Option cleans; UnaryOption/ArgsOption soft-fail on references");
    return ok;
}

// concept_constrains_overloads
//   the concepts actually CONSTRAIN: a unary option selects the UnaryOption
// overload (1), an args option selects the ArgsOption overload (2), resolved
// entirely at compile time.
bool
concept_constrains_overloads()
{
    constexpr bool ok =
        (classify(option<opt_key::alpha>{})        == 1)  &&
        (classify(option<nullptr>{})               == 1)  &&
        (classify(option<opt_key::alpha, arg_a>{}) == 2)  &&
        (classify(option<opt_key::alpha, arg_a, arg_b>{}) == 2);

    static_assert(ok, "UnaryOption/ArgsOption steer overload resolution");
    return ok;
}

#endif  // C++20 concepts available


// ---------------------------------------------------------------------------
// block provider  (empty below C++20: there is no concept surface to test)
// ---------------------------------------------------------------------------
::djinterp::test::block_spec
option_concepts_block()
{
    ::djinterp::test::block_spec b;
    b.name       = "concepts";
    b.descriptor = "Option / UnaryOption / ArgsOption C++20 concept analogs";

#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS
    b.tests = {
        { "concept_option_tracks_trait",
          "Option is the concept face of is_option_v (cleans cv/ref)",
          &concept_option_tracks_trait },
        { "concept_unary_positive_negative",
          "UnaryOption: true for unary options, false for args/non-options",
          &concept_unary_positive_negative },
        { "concept_args_positive_negative",
          "ArgsOption: true for args options, false for unary/non-options",
          &concept_args_positive_negative },
        { "concept_unary_args_exclusive",
          "UnaryOption and ArgsOption partition the option<> space",
          &concept_unary_args_exclusive },
        { "concept_reference_edge",
          "Option cleans; UnaryOption/ArgsOption soft-fail on references",
          &concept_reference_edge },
        { "concept_constrains_overloads",
          "the shape concepts steer overload resolution, not just bools",
          &concept_constrains_overloads },
    };
#else
    // Pre-C++20: option.hpp exposes no concept surface, so nothing to verify.
    b.descriptor = "Option / UnaryOption / ArgsOption (skipped: concepts require C++20)";
#endif

    return b;
}


NS_END  // testing
NS_END  // djinterp
