/*******************************************************************************
* djinterp [re_std]                                              common_with.hpp
*
* common_with concept header:
*   TypeA and TypeB share a common type they both convert to.
*
*   Stronger than common_reference_with, and the last two clauses are the
* reason: they force the common TYPE and the common REFERENCE to agree.  Without
* them a pair of types could have a common_type that is unrelated to their
* common_reference, and generic code that mixes value and reference contexts
* would silently pick different types in different places.
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
* path:      /inc/re_std/concepts/common_with.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CONCEPTS_COMMON_WITH_HPP
#define RE_STD_CONCEPTS_COMMON_WITH_HPP 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "same_as.hpp"
#include "common_reference_with.hpp"

namespace re_std
{

// common_with
//   concept: both types convert to a shared common type, consistently with
// their common reference type.
template<typename TypeA, typename TypeB>
concept common_with
    =  same_as<typename common_type<TypeA, TypeB>::type,
               typename common_type<TypeB, TypeA>::type>
    && requires {
           static_cast<typename common_type<TypeA, TypeB>::type>(
               declval<TypeA>());
           static_cast<typename common_type<TypeA, TypeB>::type>(
               declval<TypeB>());
       }
    && common_reference_with<
           typename add_lvalue_reference<const TypeA>::type,
           typename add_lvalue_reference<const TypeB>::type>
    && common_reference_with<
           typename add_lvalue_reference<
               typename common_type<TypeA, TypeB>::type>::type,
           typename common_reference<
               typename add_lvalue_reference<const TypeA>::type,
               typename add_lvalue_reference<const TypeB>::type>::type>;

}  // re_std
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_CONCEPTS_COMMON_WITH_HPP
