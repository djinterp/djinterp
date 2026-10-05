/*******************************************************************************
* djinterp [re_std]                                       copy_constructible.hpp
*
* copy_constructible concept header:
*   Type is copy-constructible from every cv/value-category spelling.
*
*   Six conjuncts, testing Type&, const Type& and const Type in both the
* construct and convert directions, on top of move_constructible.  The apparently
* redundant `const Type` (a const PRVALUE) is the one that catches a class whose
* copy constructor takes a non-const reference: such a type is copyable from an
* lvalue but not from a const temporary, and would break as soon as generic code
* passed it through a const-returning function.
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
* path:      /inc/re_std/concepts/copy_constructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CONCEPTS_COPY_CONSTRUCTIBLE_HPP
#define RE_STD_CONCEPTS_COPY_CONSTRUCTIBLE_HPP 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "move_constructible.hpp"
#include "constructible_from.hpp"
#include "convertible_to.hpp"

namespace re_std
{

// copy_constructible
//   concept: Type is constructible and convertible from Type&,
// const Type& and const Type, as well as from an rvalue.
template<typename Type>
concept copy_constructible
    =  move_constructible<Type>
    && constructible_from<Type, Type&>       && convertible_to<Type&, Type>
    && constructible_from<Type, const Type&> && convertible_to<const Type&, Type>
    && constructible_from<Type, const Type>  && convertible_to<const Type, Type>;

}  // re_std
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_CONCEPTS_COPY_CONSTRUCTIBLE_HPP
