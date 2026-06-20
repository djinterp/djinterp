/******************************************************************************
* djinterp [math]                                              math_common.hpp
*
* Compile-time mathematical expressions and functions.
*   This header provides template types for representing mathematical 
* expressions, constants, functions, and coordinate systems at compile time.
* No tag types are used - all detection is structural via SFINAE.
*
* STRUCTURAL REQUIREMENTS (for SFINAE detection):
*   Constants: static constexpr value, static constexpr degree == 0
*   Functions: static evaluate(_x) method, static constexpr arity
*   Polynomials: static constexpr degree, static evaluate(_x) method
*   Number bases: static constexpr radix
*
* 
* path:      /inc/djinterp/math/math_common.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.02.04
******************************************************************************/

#ifndef DJINTERP_MATH_COMMON_
#define DJINTERP_MATH_COMMON_ 1

#include <cstddef>
#include <cstdint>
#include <array>
#include <ratio>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../../core/djinterp.hpp"
#include "../interval/interval.hpp"



// D_KEYWORD_MATH
//   keyword: resolves to `math`.  Marks a unit of code as part
// of the maths subframework.
#define D_KEYWORD_MATH              math

// NS_MATH
//   namespace: the parse subframework namespace.
#define NS_MATH                     D_NAMESPACE(D_KEYWORD_MATH)


NS_DJINTERP  // djinterp
NS_MATH      // math





NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_COMMON_