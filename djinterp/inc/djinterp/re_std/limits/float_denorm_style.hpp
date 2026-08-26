/***********************************************************************
* re_std                                          float_denorm_style.hpp
*
* the float_denorm_style subnormal-support enumeration:
*   the plain enumeration naming a floating-point type's subnormal (denormal)
*   support, used as numeric_limits<T>::has_denorm. A plain enum (matching std)
*   so it works on C++98. std deprecated this enum and has_denorm in C++23;
*   re_std still provides it for portability and does not attach a deprecation
*   attribute. C++98 baseline.
*
*
* path:      /inc/djinterp/re_std/limits/float_denorm_style.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                       date: 2026.06.05
***********************************************************************/

#ifndef DJINTERP_RE_STD_LIMITS_FLOAT_DENORM_STYLE_
#define DJINTERP_RE_STD_LIMITS_FLOAT_DENORM_STYLE_ 1

// djinterp
#include "djinterp.hpp"

NS_RESTD

    // float_denorm_style
    //   enum: subnormal support (numeric_limits<T>::has_denorm). Deprecated in
    // std since C++23; retained here for portability.
    enum float_denorm_style
    {
        denorm_indeterminate = -1,
        denorm_absent        = 0,
        denorm_present       = 1
    };

NS_END  // re_std

#endif  // DJINTERP_RE_STD_LIMITS_FLOAT_DENORM_STYLE_
