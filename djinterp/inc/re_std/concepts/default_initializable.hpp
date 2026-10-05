/*******************************************************************************
* djinterp [re_std]                                    default_initializable.hpp
*
* default_initializable concept header:
*   Type can be value- and default-initialized.
*
*   Three separate checks, and each rejects something the others accept.
* constructible_from<Type> alone admits `const int`, whose default-initialization
* is ill-formed; `requires { Type{}; }` rejects types with an explicit default
* ctor used in copy-list-init; and `requires { ::new Type; }` is what actually
* catches const-qualified and reference types.  The ::new appears only inside an
* unevaluated requires-expression, so nothing is ever allocated.
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
* path:      /inc/re_std/concepts/default_initializable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CONCEPTS_DEFAULT_INITIALIZABLE_HPP
#define RE_STD_CONCEPTS_DEFAULT_INITIALIZABLE_HPP 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "constructible_from.hpp"

namespace re_std
{

// default_initializable
//   concept: Type is default-initializable and value-initializable.
template<typename Type>
concept default_initializable
    =  constructible_from<Type>
    && requires { Type{}; }
    && requires { ::new Type; };

}  // re_std
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_CONCEPTS_DEFAULT_INITIALIZABLE_HPP
