/*******************************************************************************
* djinterp [re_std]                                                predicate.hpp
*
* predicate concept header:
*   Func is a regular_invocable returning something boolean-testable.
*
*   boolean_testable, not convertible_to<bool>: see boolean_testable.hpp for
* why the negation clause matters here.
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
* path:      /inc/re_std/concepts/predicate.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CONCEPTS_PREDICATE_HPP
#define RE_STD_CONCEPTS_PREDICATE_HPP 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "regular_invocable.hpp"
#include "boolean_testable.hpp"

namespace re_std
{

// predicate
//   concept: Func is regular_invocable and its result is usable as a
// condition.
template<typename Func, typename... Args>
concept predicate
    =  regular_invocable<Func, Args...>
    && internal::boolean_testable<
           typename invoke_result<Func, Args...>::type>;

}  // re_std
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_CONCEPTS_PREDICATE_HPP
