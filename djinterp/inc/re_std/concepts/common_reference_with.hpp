/*******************************************************************************
* djinterp [re_std]                                    common_reference_with.hpp
*
* common_reference_with concept header:
*   TypeA and TypeB share a common reference type.
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
* immediate, localised compile error.  Test RE_STD_LANG_IS_CPP20_OR_HIGHER, or
* use the trait-shaped equivalents in re_std::type_traits, which reach C++98.
*
*
* path:      /inc/re_std/concepts/common_reference_with.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CONCEPTS_COMMON_REFERENCE_WITH_HPP
#define RE_STD_CONCEPTS_COMMON_REFERENCE_WITH_HPP 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "same_as.hpp"
#include "convertible_to.hpp"

namespace re_std
{

// common_reference_with
//   concept: both types convert to a shared common reference type.
template<typename TypeA, typename TypeB>
concept common_reference_with
    =  same_as<typename common_reference<TypeA, TypeB>::type,
               typename common_reference<TypeB, TypeA>::type>
    && convertible_to<TypeA,
                      typename common_reference<TypeA, TypeB>::type>
    && convertible_to<TypeB,
                      typename common_reference<TypeA, TypeB>::type>;

}  // re_std
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_CONCEPTS_COMMON_REFERENCE_WITH_HPP
