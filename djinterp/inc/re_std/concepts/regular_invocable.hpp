/*******************************************************************************
* djinterp [re_std]                                        regular_invocable.hpp
*
* regular_invocable concept header:
*   Func is invocable and equality-preserving.
*
*   SYNTACTICALLY IDENTICAL to invocable - the difference is a semantic
* requirement (equality-preservation) that no compiler can check.  It exists so
* that generic code can DOCUMENT which one it needs; std says the same.  Shipped
* as its own concept rather than an alias so that the distinction survives in
* diagnostics and in constraint normalisation.
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
* path:      /inc/re_std/concepts/regular_invocable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CONCEPTS_REGULAR_INVOCABLE_HPP
#define RE_STD_CONCEPTS_REGULAR_INVOCABLE_HPP 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "invocable.hpp"

namespace re_std
{

// regular_invocable
//   concept: invocable, and required (semantically) to be
// equality-preserving.  Not checkable; see the header note.
template<typename Func, typename... Args>
concept regular_invocable = invocable<Func, Args...>;

}  // re_std
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_CONCEPTS_REGULAR_INVOCABLE_HPP
