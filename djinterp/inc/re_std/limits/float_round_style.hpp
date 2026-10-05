/*******************************************************************************
* djinterp [re_std]                                        float_round_style.hpp
*
* the float_round_style rounding-mode enumeration:
*   the plain (non-scoped) enumeration naming a floating-point type's rounding
*   behaviour, used as numeric_limits<T>::round_style. A plain enum (not enum
*   class, matching std) so it works unchanged on C++98. C++98 baseline.
*
*
* path:      /inc/re_std/limits/float_round_style.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.05
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_LIMITS_FLOAT_ROUND_STYLE_HPP
#define RE_STD_LIMITS_FLOAT_ROUND_STYLE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

namespace re_std
{

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

}  // re_std

#endif  // RE_STD_LIMITS_FLOAT_ROUND_STYLE_HPP
