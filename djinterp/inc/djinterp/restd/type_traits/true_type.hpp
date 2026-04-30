/******************************************************************************
* djinterp [restd]                                               true_type.hpp
*
* true_type typedef header:
*   Provides the true_type typedef as integral_constant<bool, true>. Used
* as a base class for boolean traits that report true.
*
*
* path:      /inc/djinterp/restd/type_traits/true_type.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_TRUE_TYPE_
#define DJINTERP_RESTD_TYPE_TRAITS_TRUE_TYPE_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"


NS_RESTD


// =============================================================================
// I.   TRUE_TYPE
// =============================================================================

// true_type
//   typedef: integral_constant<bool, true>. Base class for boolean traits
// that report true.
typedef integral_constant<bool, true> true_type;


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_TRUE_TYPE_
