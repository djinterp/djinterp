/******************************************************************************
* re_std [concepts]                                               invocable.hpp
*
*   _Func can be invoked with _Args.
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
* immediate, localised compile error.  Test D_ENV_LANG_IS_CPP20_OR_HIGHER, or
* use the trait-shaped equivalents in re_std::type_traits, which reach C++98.
*
*
* path:      /inc/djinterp/re_std/concepts/invocable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_CONCEPTS_INVOCABLE_
#define RESTD_CONCEPTS_INVOCABLE_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "../functional/invoke.hpp"

NS_DJINTERP
NS_RESTD

// invocable
//   concept: _Func is callable with _Args... under the INVOKE protocol.
template<typename _Func, typename... _Args>
concept invocable
    = requires(_Func&& f, _Args&&... args)
      {
          invoke(static_cast<_Func&&>(f), static_cast<_Args&&>(args)...);
      };

NS_END  // re_std
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // RESTD_CONCEPTS_INVOCABLE_
