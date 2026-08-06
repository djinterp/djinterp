/******************************************************************************
* djinterp [math]                                              math_common.hpp
*
* Common foundation for the math subsystem.
*   Establishes the math namespace macro and the shared environment include
* that every math header builds on. Including this header (directly or
* transitively) is sufficient to obtain the NS_MATH / NS_DJINTERP / NS_INTERNAL
* namespace-open macros and the version-gating helpers from djinterp.hpp.
*
* path:      /inc/djinterp/math/math_common.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.02.04
******************************************************************************/

#ifndef DJINTERP_MATH_COMMON_
#define DJINTERP_MATH_COMMON_ 1

#include "../djinterp.hpp"


// D_KEYWORD_MATH
//   keyword: resolves to `math`.  Marks a unit of code as part of the maths
// namespace.
#define D_KEYWORD_MATH              math

// NS_MATH
//   namespace: the maths subsystem namespace.
#define NS_MATH                     D_NAMESPACE(D_KEYWORD_MATH)


#endif  // DJINTERP_MATH_COMMON_
