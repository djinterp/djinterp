/******************************************************************************
* djinterp [re_std]                                             false_type.hpp
*
* false_type typedef header:
*   Provides the false_type typedef as integral_constant<bool, false>.
* Used as a base class for boolean traits that report false.
*
*
* path:      /inc/djinterp/re_std/type_traits/false_type.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_FALSE_TYPE_
#define DJINTERP_RE_STD_TYPE_TRAITS_FALSE_TYPE_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"


NS_RESTD


// =============================================================================
// I.   FALSE_TYPE
// =============================================================================

// false_type
//   typedef: integral_constant<bool, false>. Base class for boolean
// traits that report false.
typedef integral_constant<bool, false> false_type;


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_FALSE_TYPE_
