/******************************************************************************
* djinterp [restd]                                           bool_constant.hpp
*
* bool_constant alias header:
*   Provides the bool_constant alias template as
* integral_constant<bool, _Value>. Mirrors the C++17 std::bool_constant
* interface but is available on any compiler with alias templates.
*
*   PORTABILITY:
*   Requires alias templates (C++11+). Not available on C++98/03;
* use integral_constant<bool, _Value> directly instead.
*
*
* path:      /inc/djinterp/restd/type_traits/bool_constant.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_BOOL_CONSTANT_
#define DJINTERP_RESTD_TYPE_TRAITS_BOOL_CONSTANT_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"

// gate: requires alias templates
#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


NS_RESTD


// =============================================================================
// I.   BOOL_CONSTANT
// =============================================================================

// bool_constant
//   alias: integral_constant<bool, _Value> helper for boolean traits.
template<bool _Value>
using bool_constant = integral_constant<bool, _Value>;


NS_END  // restd


#endif  // D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


#endif  // DJINTERP_RESTD_TYPE_TRAITS_BOOL_CONSTANT_
