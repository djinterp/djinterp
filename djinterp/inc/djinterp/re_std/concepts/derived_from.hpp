/******************************************************************************
* re_std [concepts]                                            derived_from.hpp
*
*   _Derived is publicly and unambiguously derived from _Base.
*
*   The convertibility half is not implied by is_base_of: a PRIVATE or
* AMBIGUOUS base still satisfies is_base_of, but the pointer conversion is
* ill-formed.  Testing both is what makes this concept mean "usable as a _Base",
* which is what callers actually want.  The cv-qualifiers on the pointer test are
* deliberate - they keep the check working for cv-qualified _Derived.
*
*   C++20 ONLY - AND THAT IS NOT A GAP.
*   `concept` is a core language keyword with no builtin behind it, so unlike
* re_std's intrinsic-backed traits there is nothing to detect and nothing to
* back-port.  Below C++20 this header is EMPTY rather than degraded: a concept
* that does not exist cannot give a wrong answer, and naming one is an
* immediate, localised compile error.  Test D_ENV_LANG_IS_CPP20_OR_HIGHER, or
* use the trait-shaped equivalents in re_std::type_traits, which reach C++98.
*
*
* path:      /inc/djinterp/re_std/concepts/derived_from.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_CONCEPTS_DERIVED_FROM_
#define RESTD_CONCEPTS_DERIVED_FROM_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"

NS_DJINTERP
NS_RESTD

// derived_from
//   concept: _Derived is a public, unambiguous base-derived relation to _Base.
template<typename _Derived, typename _Base>
concept derived_from
    =  is_base_of<_Base, _Derived>::value
    && is_convertible<const volatile _Derived*,
                      const volatile _Base*>::value;

NS_END  // re_std
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // RESTD_CONCEPTS_DERIVED_FROM_
