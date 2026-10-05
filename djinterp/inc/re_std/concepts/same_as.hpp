/*******************************************************************************
* djinterp [re_std]                                                  same_as.hpp
*
* same_as concept header:
*   T and U are the same type, cv-qualification included.
*
*   Written as a two-way conjunction of an internal helper rather than a bare
* is_same_v.  That is not redundancy: subsumption compares NORMALISED constraint
* expressions, and the symmetric form is what lets `same_as<T, U>` and
* `same_as<U, T>` subsume each other, so an overload constrained on one is not
* ambiguous against an overload constrained on the other.  A single-atom
* definition would break every overload set that relies on it.
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
* path:      /inc/re_std/concepts/same_as.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CONCEPTS_SAME_AS_HPP
#define RE_STD_CONCEPTS_SAME_AS_HPP 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"

namespace re_std
{

namespace internal
{

    template<typename TypeA, typename TypeB>
    concept same_as_impl = is_same<TypeA, TypeB>::value;

}  // internal

// same_as
//   concept: TypeA and TypeB name the same type.
template<typename TypeA, typename TypeB>
concept same_as
    =  internal::same_as_impl<TypeA, TypeB>
    && internal::same_as_impl<TypeB, TypeA>;

}  // re_std
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_CONCEPTS_SAME_AS_HPP
