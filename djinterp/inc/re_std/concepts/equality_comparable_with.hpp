/*******************************************************************************
* djinterp [re_std]                                 equality_comparable_with.hpp
*
* equality_comparable_with concept header:
*   TypeA and TypeB are mutually equality-comparable.
*
*   IMPLEMENTS THE C++20 SPELLING.  C++23's P2404R3 replaced the
* common_reference_with conjunct with an exposition-only
* comparison-common-type-with, which additionally checks that the common
* reference behaves consistently under conversion.  re_std ships the C++20 form
* because that is what the rest of this module targets; the refinement is
* tracked on the roadmap rather than silently mixed in.
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
* path:      /inc/re_std/concepts/equality_comparable_with.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CONCEPTS_EQUALITY_COMPARABLE_WITH_HPP
#define RE_STD_CONCEPTS_EQUALITY_COMPARABLE_WITH_HPP 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "equality_comparable.hpp"
#include "common_reference_with.hpp"

namespace re_std
{

// equality_comparable_with
//   concept: both types are equality_comparable, share a common reference,
// and are comparable with each other.
template<typename TypeA, typename TypeB>
concept equality_comparable_with
    =  equality_comparable<TypeA>
    && equality_comparable<TypeB>
    && common_reference_with<
           const typename remove_reference<TypeA>::type&,
           const typename remove_reference<TypeB>::type&>
    && equality_comparable<
           typename common_reference<
               const typename remove_reference<TypeA>::type&,
               const typename remove_reference<TypeB>::type&>::type>
    && internal::weakly_equality_comparable_with<TypeA, TypeB>;

}  // re_std
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_CONCEPTS_EQUALITY_COMPARABLE_WITH_HPP
