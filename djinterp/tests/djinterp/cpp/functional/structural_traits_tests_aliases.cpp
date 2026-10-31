/*******************************************************************************
* djinterp [cpp]                             structural_traits_tests_aliases.cpp
*
*
* path:      /tests/djinterp/cpp/functional/structural_traits_tests_aliases.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]  structural_traits_tests_aliases.cpp
//   Section IV -- the _v variable-template shorthands (C++14+).

// std
#include <string>
#include <type_traits>
// djinterp
#include "structural_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_has_match_result_type_v_agrees
  The member-type detector's _v companion (emitted by the trait macro) is exactly
  the trait's value.
  Tests the following:
  - the two agree, positively and negatively
*/
bool tests_has_match_result_type_v_agrees()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    ok = ok && (has_match_result_type_v<DMrtOnly> ==
                has_match_result_type<DMrtOnly>::value);
    ok = ok && (has_match_result_type_v<DMrtData> ==
                has_match_result_type<DMrtData>::value);

    static_assert(has_match_result_type_v<DPatScan>, "positive");
    static_assert(!has_match_result_type_v<DPatNone>, "negative");
    static_assert(!has_match_result_type_v<DMrtData>, "data member, not a type");
#endif

    return ok;
}


/*
tests_has_find_method_v_agrees
  has_find_method_v is the trait's value (it re-applies clean_t, which is
  idempotent, so the two cannot drift).
  Tests the following:
  - the two agree, positively and negatively, and through cv-ref spellings
*/
bool tests_has_find_method_v_agrees()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    ok = ok && ((has_find_method_v<DPatScan, std::string, int>) ==
                (has_find_method<DPatScan, std::string, int>::value));
    ok = ok && ((has_find_method_v<DPatNone, std::string, int>) ==
                (has_find_method<DPatNone, std::string, int>::value));

    static_assert(has_find_method_v<DPatScan, std::string, int>, "positive");
    static_assert(!has_find_method_v<DPatNonconst, std::string, int>,
                  "non-const find");
    static_assert(has_find_method_v<const DPatScan&, const std::string&, int&>,
                  "clean_t applied");
#endif

    return ok;
}


/*
tests_arity_v_agree
  The trilogy's three shorthands agree with their traits.
  Tests the following:
  - nullary, unary, and binary, each positively and negatively
  - the mutable-lvalue contract survives the shorthand
*/
bool tests_arity_v_agree()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    ok = ok && (is_nullary_callable_v<DNilConst> ==
                is_nullary_callable<DNilConst>::value);
    ok = ok && ((is_unary_callable_v<DConstStep, int>) ==
                (is_unary_callable<DConstStep, int>::value));
    ok = ok && ((is_binary_callable_v<sum_reducer, int, int>) ==
                (is_binary_callable<sum_reducer, int, int>::value));

    static_assert(is_nullary_callable_v<DNilMut>, "stateful nullary");
    static_assert(!is_nullary_callable_v<DNilRref>, "&&-qualified rejected");
    static_assert(is_unary_callable_v<DMutStep, int>, "stateful unary");
    static_assert(!is_unary_callable_v<in_place, int&>, "int& undetectable");
    static_assert(is_binary_callable_v<DTallyReducer, int, int>,
                  "stateful reducer");
    static_assert(!is_binary_callable_v<DConstStep, int, int>, "wrong arity");
#endif

    return ok;
}


/*
tests_optional_like_v_agree
  The two source-protocol shorthands agree with their traits.
  Tests the following:
  - produces_optional_like_v and is_unfold_step_v, positively and negatively
*/
bool tests_optional_like_v_agree()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    ok = ok && (produces_optional_like_v<DSrcPull> ==
                DProducesOptionalLike<DSrcPull>::value);
    ok = ok && ((is_unfold_step_v<step_count, int>) ==
                (is_unfold_step<step_count, int>::value));

    static_assert(produces_optional_like_v<src_ptr>, "pointer source");
    static_assert(!produces_optional_like_v<DSrcInt>, "no deref half");
    static_assert(!produces_optional_like_v<DSrcDeref>, "no bool half");
    static_assert(is_unfold_step_v<step_count, int>, "unfold step");
    static_assert(!is_unfold_step_v<DStepPlain, int>, "not optional-like");
#endif

    return ok;
}


/*
tests_aliases_are_constant_expressions
  The shorthands are constexpr bools, usable wherever a constant is required.
  Tests the following:
  - as a template argument, as an array bound, and in a constexpr initialiser
*/
bool tests_aliases_are_constant_expressions()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    typedef std::integral_constant<bool, is_nullary_callable_v<DNilConst> > c1;
    static_assert(c1::value, "template argument");

    int arr[produces_optional_like_v<DSrcPull> ? 2 : 1];
    ok = ok && (sizeof(arr) / sizeof(arr[0]) == 2u);

    D_CONSTEXPR bool b = is_unfold_step_v<step_count, int>;
    static_assert(b, "constexpr initialiser");
    static_assert(std::is_same<decltype(is_nullary_callable_v<DNilConst>),
                               const bool>::value, "typed bool");
#endif

    return ok;
}


/*
tests_aliases_gating
  The shorthands exist exactly where the standard allows them: gated on variable
  templates (C++14+), with the ::value forms as the always-present floor -- the
  header degrades to C++11.
  Tests the following:
  - the gate agrees with the language level
  - the ::value forms work regardless of the gate
*/
bool tests_aliases_gating()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // gate open: the shorthands must be usable.
    ok = ok && (is_nullary_callable_v<DNilConst>);
    ok = ok && (produces_optional_like_v<DSrcPull>);
#else
    // gate closed: this must be a pre-C++14 build.
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        ok = false;         // shorthands missing on a standard that has them
    #endif
#endif

    // the ::value forms are the floor, available on every standard.
    ok = ok && (is_nullary_callable<DNilConst>::value);
    ok = ok && (is_unary_callable<DConstStep, int>::value);
    ok = ok && (is_binary_callable<sum_reducer, int, int>::value);
    ok = ok && (DProducesOptionalLike<DSrcPull>::value);
    ok = ok && (is_unfold_step<step_count, int>::value);
    ok = ok && (has_match_result_type<DPatScan>::value);
    ok = ok && (has_find_method<DPatScan, std::string, int>::value);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
