/******************************************************************************
* re_std [concepts]                                       regular_invocable.hpp
*
*   _Func is invocable and equality-preserving.
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
* immediate, localised compile error.  Test D_ENV_LANG_IS_CPP20_OR_HIGHER, or
* use the trait-shaped equivalents in re_std::type_traits, which reach C++98.
*
*
* path:      /inc/djinterp/re_std/concepts/regular_invocable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_CONCEPTS_REGULAR_INVOCABLE_
#define DJINTERP_RE_STD_CONCEPTS_REGULAR_INVOCABLE_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../concepts/invocable.hpp"

NS_RESTD

// regular_invocable
//   concept: invocable, and required (semantically) to be
// equality-preserving.  Not checkable; see the header note.
template<typename _Func, typename... _Args>
concept regular_invocable = invocable<_Func, _Args...>;

NS_END  // re_std
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // DJINTERP_RE_STD_CONCEPTS_REGULAR_INVOCABLE_
