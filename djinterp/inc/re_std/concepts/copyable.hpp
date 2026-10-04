/******************************************************************************
* re_std [concepts]                                                copyable.hpp
*
*   _Type is movable and copy-assignable from every spelling.
*
*   The three assignable_from conjuncts mirror copy_constructible's cv and
* value-category spread, for the same reason: a type assignable from _Type& but
* not from const _Type& is not actually copyable in generic code.
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
* path:      /inc/djinterp/re_std/concepts/copyable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_CONCEPTS_COPYABLE_
#define DJINTERP_RE_STD_CONCEPTS_COPYABLE_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../concepts/copy_constructible.hpp"
#include "../concepts/movable.hpp"
#include "../concepts/assignable_from.hpp"

NS_RESTD

// copyable
//   concept: _Type is movable and assignable from lvalue, const lvalue and
// const rvalue _Type.
template<typename _Type>
concept copyable
    =  copy_constructible<_Type>
    && movable<_Type>
    && assignable_from<_Type&, _Type&>
    && assignable_from<_Type&, const _Type&>
    && assignable_from<_Type&, const _Type>;

NS_END  // re_std
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // DJINTERP_RE_STD_CONCEPTS_COPYABLE_
