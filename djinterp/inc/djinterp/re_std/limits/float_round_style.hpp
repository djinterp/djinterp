/***********************************************************************
* restd                                            float_round_style.hpp
*
* the float_round_style rounding-mode enumeration:
*   the plain (non-scoped) enumeration naming a floating-point type's rounding
*   behaviour, used as numeric_limits<T>::round_style. A plain enum (not enum
*   class, matching std) so it works unchanged on C++98. C++98 baseline.
*
*
* path:      /inc/djinterp/re_std/limits/float_round_style.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                       date: 2026.06.05
***********************************************************************/

#ifndef RESTD_LIMITS_FLOAT_ROUND_STYLE_
#define RESTD_LIMITS_FLOAT_ROUND_STYLE_ 1

// djinterp
#include "djinterp.hpp"

NS_RESTD

    // float_round_style
    //   enum: floating-point rounding mode (numeric_limits<T>::round_style).
    enum float_round_style
    {
        round_indeterminate       = -1,
        round_toward_zero         = 0,
        round_to_nearest          = 1,
        round_toward_infinity     = 2,
        round_toward_neg_infinity = 3
    };

NS_END  // restd

#endif  // RESTD_LIMITS_FLOAT_ROUND_STYLE_
