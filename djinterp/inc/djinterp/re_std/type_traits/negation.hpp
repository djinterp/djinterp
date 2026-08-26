/******************************************************************************
* djinterp [re_std]                                               negation.hpp
*
* negation trait header:
*   Logical NOT of a type trait. Yields a bool integral_constant whose
* value is the negation of the wrapped trait's value. Mirrors C++17
* std::negation but available on C++11+.
*
*     negation<true_type>::value      -> false
*     negation<false_type>::value     -> true
*     negation<is_integral<int>>::value -> false
*
*   PORTABILITY:
*   Requires alias templates (for bool_constant) and integral_constant.
* On C++98/03, where neither bool_constant nor the C++17 logical-ops
* surface exists meaningfully, this header omits the trait. Code paths
* that need negation must themselves be gated on the alias-templates
* feature macro.
*
*
* path:      /inc/djinterp/re_std/type_traits/negation.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_NEGATION_
#define DJINTERP_RE_STD_TYPE_TRAITS_NEGATION_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./bool_constant.hpp"
#include "./integral_constant.hpp"


NS_RESTD


// =============================================================================
// I.   NEGATION
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    // negation
    //   trait: bool_constant<!_Trait::value>.
    template<typename _Trait>
    struct negation : bool_constant<!static_cast<bool>(_Trait::value)>
    {};

#endif  // D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


// =============================================================================
// II.  NEGATION_V (C++14+ variable template)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // negation_v
    //   variable: convenience for negation<_Trait>::value.
    template<typename _Trait>
    D_CONSTEXPR bool negation_v = negation<_Trait>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_NEGATION_
