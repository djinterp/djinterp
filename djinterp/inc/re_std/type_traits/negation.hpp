/*******************************************************************************
* djinterp [re_std]                                                 negation.hpp
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
* path:      /inc/re_std/type_traits/negation.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_NEGATION_HPP
#define RE_STD_TYPE_TRAITS_NEGATION_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./bool_constant.hpp"
#include "./integral_constant.hpp"


namespace re_std
{


// =============================================================================
// I.   NEGATION
// =============================================================================

#if RE_STD_LANG_HAS_ALIAS_TEMPLATES

    // negation
    //   trait: bool_constant<!Trait::value>.
    template<typename Trait>
    struct negation : bool_constant<!static_cast<bool>(Trait::value)>
    {};

#endif  // RE_STD_LANG_HAS_ALIAS_TEMPLATES


// =============================================================================
// II.  NEGATION_V (C++14+ variable template)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // negation_v
    //   variable: convenience for negation<Trait>::value.
    template<typename Trait>
    RE_STD_CONSTEXPR bool negation_v = negation<Trait>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_NEGATION_HPP
