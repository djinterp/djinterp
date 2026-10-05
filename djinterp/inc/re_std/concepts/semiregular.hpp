/*******************************************************************************
* djinterp [re_std]                                              semiregular.hpp
*
* semiregular concept header:
*   Type is copyable and default-initializable.
*
* The classic "behaves like a built-in type, except for comparison" bundle.
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
* path:      /inc/re_std/concepts/semiregular.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CONCEPTS_SEMIREGULAR_HPP
#define RE_STD_CONCEPTS_SEMIREGULAR_HPP 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "copyable.hpp"
#include "default_initializable.hpp"

namespace re_std
{

// semiregular
//   concept: Type is copyable and default_initializable.
template<typename Type>
concept semiregular = copyable<Type> && default_initializable<Type>;

}  // re_std
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_CONCEPTS_SEMIREGULAR_HPP
