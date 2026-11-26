/*******************************************************************************
* djinterp [cpp][math]                                           math_common.hpp
*
* Common foundation for the math subsystem.
*   Establishes the math namespace macro and the shared environment include
* that every math header builds on. Including this header (directly or
* transitively) is sufficient to obtain the NS_MATH / NS_DJINTERP / NS_INTERNAL
* namespace-open macros and the version-gating helpers from djinterp.hpp.
*
* HOW TO USE
* ----------
*   Included by every math header; a caller need not. It gives NS_MATH and the
* framework root's namespace macros and level gates.
*
* OPTIONS
* -------
*   None: nothing in this header is chosen at run time.
*
* CONFIGURATION
* -------------
*   None of its own: it compiles at every level from ISO strict C++98.
*
*
* path:      /inc/djinterp/cpp/modules/math/math_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.04
*                                                            revised: 2026.11.24
*******************************************************************************/

#ifndef DJINTERP_MATH_MATH_COMMON_HPP
#define DJINTERP_MATH_MATH_COMMON_HPP 1

// djinterp
#include "../../djinterp.hpp"


// D_KEYWORD_MATH
//   keyword: resolves to `math`.  Marks a unit of code as part of the maths
// namespace.
#define D_KEYWORD_MATH              math

// NS_MATH
//   namespace: the maths subsystem namespace.
#define NS_MATH                     D_NAMESPACE(D_KEYWORD_MATH)


#endif  // DJINTERP_MATH_MATH_COMMON_HPP
