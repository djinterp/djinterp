/*******************************************************************************
* djinterp [re_std]                                                 copyable.hpp
*
* copyable concept header:
*   Type is movable and copy-assignable from every spelling.
*
*   The three assignable_from conjuncts mirror copy_constructible's cv and
* value-category spread, for the same reason: a type assignable from Type& but
* not from const Type& is not actually copyable in generic code.
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
* path:      /inc/re_std/concepts/copyable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CONCEPTS_COPYABLE_HPP
#define RE_STD_CONCEPTS_COPYABLE_HPP 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "copy_constructible.hpp"
#include "movable.hpp"
#include "assignable_from.hpp"

namespace re_std
{

// copyable
//   concept: Type is movable and assignable from lvalue, const lvalue and
// const rvalue Type.
template<typename Type>
concept copyable
    =  copy_constructible<Type>
    && movable<Type>
    && assignable_from<Type&, Type&>
    && assignable_from<Type&, const Type&>
    && assignable_from<Type&, const Type>;

}  // re_std
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_CONCEPTS_COPYABLE_HPP
