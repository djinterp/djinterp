/*******************************************************************************
* djinterp [re_std]                                               false_type.hpp
*
* false_type typedef header:
*   Provides the false_type typedef as integral_constant<bool, false>.
* Used as a base class for boolean traits that report false.
*
*
* path:      /inc/re_std/type_traits/false_type.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_FALSE_TYPE_HPP
#define RE_STD_TYPE_TRAITS_FALSE_TYPE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"


namespace re_std
{


// =============================================================================
// I.   FALSE_TYPE
// =============================================================================

// false_type
//   typedef: integral_constant<bool, false>. Base class for boolean
// traits that report false.
typedef integral_constant<bool, false> false_type;


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_FALSE_TYPE_HPP
