/*******************************************************************************
* djinterp [re_std]                                           is_placeholder.hpp
*
* is_placeholder trait header:
* trait: detects bind placeholder types.
*   Yields an `integral_constant<int, N>` where `N` is the 1-based
* index of the placeholder, or `0` if `Type` is not a placeholder.
* Like `is_bind_expression`, this is the user customisation point for
* recognising user-defined placeholder types.
*
*
* path:      /inc/re_std/functional/is_placeholder.hpp
* link(s):   TBA
* author(s): re_std                                          created: 2026.05.07
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_FUNCTIONAL_IS_PLACEHOLDER_HPP
#define RE_STD_FUNCTIONAL_IS_PLACEHOLDER_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "re_std/type_traits/type_traits.hpp"

namespace re_std
{

// is_placeholder
//   trait: primary template; 0 for non-placeholder types.
template<typename Type>
struct is_placeholder : integral_constant<int, 0>
{};

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

// is_placeholder_v (C++17+)
template<typename Type>
RE_STD_CONSTEXPR int is_placeholder_v = is_placeholder<Type>::value;

#endif // RE_STD_LANG_HAS_VARIABLE_TEMPLATES

}  // re_std

#endif  // floor, for now


#endif  // RE_STD_FUNCTIONAL_IS_PLACEHOLDER_HPP
