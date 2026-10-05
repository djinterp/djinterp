/*******************************************************************************
* djinterp [re_std]                                            bool_constant.hpp
*
* bool_constant alias header:
*   Provides the bool_constant alias template as
* integral_constant<bool, Value>. Mirrors the C++17 std::bool_constant
* interface but is available on any compiler with alias templates.
*
*   PORTABILITY:
*   Requires alias templates (C++11+). Not available on C++98/03;
* use integral_constant<bool, Value> directly instead.
*
*
* path:      /inc/re_std/type_traits/bool_constant.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_BOOL_CONSTANT_HPP
#define RE_STD_TYPE_TRAITS_BOOL_CONSTANT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"

// gate: requires alias templates
#if RE_STD_LANG_HAS_ALIAS_TEMPLATES


namespace re_std
{


// =============================================================================
// I.   BOOL_CONSTANT
// =============================================================================

// bool_constant
//   alias: integral_constant<bool, Value> helper for boolean traits.
template<bool Value>
using bool_constant = integral_constant<bool, Value>;


}  // re_std


#endif  // RE_STD_LANG_HAS_ALIAS_TEMPLATES


#endif  // RE_STD_TYPE_TRAITS_BOOL_CONSTANT_HPP
