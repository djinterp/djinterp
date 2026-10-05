/*******************************************************************************
* djinterp [re_std]                                             derived_from.hpp
*
* derived_from concept header:
*   Derived is publicly and unambiguously derived from Base.
*
*   The convertibility half is not implied by is_base_of: a PRIVATE or
* AMBIGUOUS base still satisfies is_base_of, but the pointer conversion is
* ill-formed.  Testing both is what makes this concept mean "usable as a Base",
* which is what callers actually want.  The cv-qualifiers on the pointer test are
* deliberate - they keep the check working for cv-qualified Derived.
*
*   C++20 ONLY - AND THAT IS NOT A GAP.
*   `concept` is a core language keyword with no builtin behind it, so unlike
* re_std's intrinsic-backed traits there is nothing to detect and nothing to
* back-port.  Below C++20 this header is EMPTY rather than degraded: a concept
* that does not exist cannot give a wrong answer, and naming one is an
* immediate, localised compile error.  Test RE_STD_LANG_IS_CPP20_OR_HIGHER, or
* use the trait-shaped equivalents in re_std::type_traits, which reach C++98.
*
*
* path:      /inc/re_std/concepts/derived_from.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CONCEPTS_DERIVED_FROM_HPP
#define RE_STD_CONCEPTS_DERIVED_FROM_HPP 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"

namespace re_std
{

// derived_from
//   concept: Derived is a public, unambiguous base-derived relation to Base.
template<typename Derived, typename Base>
concept derived_from
    =  is_base_of<Base, Derived>::value
    && is_convertible<const volatile Derived*,
                      const volatile Base*>::value;

}  // re_std
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_CONCEPTS_DERIVED_FROM_HPP
