/******************************************************************************
* re_std [concepts]                                      copy_constructible.hpp
*
*   _Type is copy-constructible from every cv/value-category spelling.
*
*   Six conjuncts, testing _Type&, const _Type& and const _Type in both the
* construct and convert directions, on top of move_constructible.  The apparently
* redundant `const _Type` (a const PRVALUE) is the one that catches a class whose
* copy constructor takes a non-const reference: such a type is copyable from an
* lvalue but not from a const temporary, and would break as soon as generic code
* passed it through a const-returning function.
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
* path:      /inc/djinterp/re_std/concepts/copy_constructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_CONCEPTS_COPY_CONSTRUCTIBLE_
#define RESTD_CONCEPTS_COPY_CONSTRUCTIBLE_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../concepts/move_constructible.hpp"
#include "../concepts/constructible_from.hpp"
#include "../concepts/convertible_to.hpp"

NS_DJINTERP
NS_RESTD

// copy_constructible
//   concept: _Type is constructible and convertible from _Type&,
// const _Type& and const _Type, as well as from an rvalue.
template<typename _Type>
concept copy_constructible
    =  move_constructible<_Type>
    && constructible_from<_Type, _Type&>       && convertible_to<_Type&, _Type>
    && constructible_from<_Type, const _Type&> && convertible_to<const _Type&, _Type>
    && constructible_from<_Type, const _Type>  && convertible_to<const _Type, _Type>;

NS_END  // re_std
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // RESTD_CONCEPTS_COPY_CONSTRUCTIBLE_
