/*******************************************************************************
* djinterp [re_std]                                                true_type.hpp
*
* true_type typedef header:
*   Provides the true_type typedef as integral_constant<bool, true>. Used
* as a base class for boolean traits that report true.
*
*
* path:      /inc/re_std/type_traits/true_type.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_TRUE_TYPE_HPP
#define RE_STD_TYPE_TRAITS_TRUE_TYPE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"


namespace re_std
{


// =============================================================================
// I.   TRUE_TYPE
// =============================================================================

// true_type
//   typedef: integral_constant<bool, true>. Base class for boolean traits
// that report true.
typedef integral_constant<bool, true> true_type;


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_TRUE_TYPE_HPP
