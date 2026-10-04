/***********************************************************************
* re_std                                             is_placeholder.hpp
*
* trait: detects bind placeholder types.
*   Yields an `integral_constant<int, N>` where `N` is the 1-based
* index of the placeholder, or `0` if `_Type` is not a placeholder.
* Like `is_bind_expression`, this is the user customisation point for
* recognising user-defined placeholder types.
*
*
* path:      /inc/djinterp/re_std/functional/is_placeholder.hpp
* link(s):   TBA
* author(s): re_std                                      date: 2026.05.07
***********************************************************************/

#ifndef DJINTERP_RE_STD_FUNCTIONAL_IS_PLACEHOLDER_
#define DJINTERP_RE_STD_FUNCTIONAL_IS_PLACEHOLDER_ 1

#include "djinterp.hpp"
#include "re_std/type_traits/type_traits.hpp"

namespace re_std
{

// is_placeholder
//   trait: primary template; 0 for non-placeholder types.
template<typename _Type>
struct is_placeholder : integral_constant<int, 0>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// is_placeholder_v (C++17+)
template<typename _Type>
D_CONSTEXPR int is_placeholder_v = is_placeholder<_Type>::value;

#endif // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

} // namespace re_std

#endif  // DJINTERP_RE_STD_FUNCTIONAL_IS_PLACEHOLDER_
