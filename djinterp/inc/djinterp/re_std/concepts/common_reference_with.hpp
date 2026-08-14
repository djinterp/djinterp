/******************************************************************************
* re_std [concepts]                                   common_reference_with.hpp
*
*   _TypeA and _TypeB share a common reference type.
*
*   The order-independence check is the substance: common_reference_t must
* give the same answer whichever way round the arguments are written.  A
* user-specialised basic_common_reference that is not symmetric would otherwise
* silently produce a concept that holds in one direction and not the other.
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
* path:      /inc/djinterp/re_std/concepts/common_reference_with.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_CONCEPTS_COMMON_REFERENCE_WITH_
#define RESTD_CONCEPTS_COMMON_REFERENCE_WITH_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../concepts/same_as.hpp"
#include "../concepts/convertible_to.hpp"

NS_DJINTERP
NS_RESTD

// common_reference_with
//   concept: both types convert to a shared common reference type.
template<typename _TypeA, typename _TypeB>
concept common_reference_with
    =  same_as<typename common_reference<_TypeA, _TypeB>::type,
               typename common_reference<_TypeB, _TypeA>::type>
    && convertible_to<_TypeA,
                      typename common_reference<_TypeA, _TypeB>::type>
    && convertible_to<_TypeB,
                      typename common_reference<_TypeA, _TypeB>::type>;

NS_END  // re_std
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // RESTD_CONCEPTS_COMMON_REFERENCE_WITH_
