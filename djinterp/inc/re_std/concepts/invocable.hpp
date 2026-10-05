/*******************************************************************************
* djinterp [re_std]                                                invocable.hpp
*
* invocable concept header:
*   Func can be invoked with Args.
*
*   Expressed through re_std::invoke, so pointer-to-member-function and
* pointer-to-member-data callables are recognised alongside ordinary functors -
* a plain `f(args...)` requires-expression would reject them.
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
* path:      /inc/re_std/concepts/invocable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CONCEPTS_INVOCABLE_HPP
#define RE_STD_CONCEPTS_INVOCABLE_HPP 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "../functional/invoke.hpp"

namespace re_std
{

// invocable
//   concept: Func is callable with Args... under the INVOKE protocol.
template<typename Func, typename... Args>
concept invocable
    = requires(Func&& f, Args&&... args)
      {
          invoke(static_cast<Func&&>(f), static_cast<Args&&>(args)...);
      };

}  // re_std
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_CONCEPTS_INVOCABLE_HPP
