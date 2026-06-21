/******************************************************************************
* djinterp [color]                                                color_hsv.hpp
*
*   C++ ergonomic layer for the HSV color model. `hsv` derives from the
* shared-kernel POD (color_hsv.h) without adding state, providing constexpr
* construction, comparison, validation, and canonicalizing clamp that
* forward to the C kernel. Conversions are supplied by the color_convert
* facade.
*
*
* path:      /inc/djinterp/util/color/color_hsv.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.06.20
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    hsv
      ---
      a. model_tag, value_type, channels
      b. constructors / converting constructor
      c. operator==
      d. is_valid / clamp
*/

#ifndef DJINTERP_COLOR_HSV_HPP_
#define DJINTERP_COLOR_HSV_HPP_ 1

#include "../../djinterp.hpp"
#include "./color_common.hpp"
#include "./color_hsv.h"


NS_DJINTERP


// hsv
//   struct: HSV color model. Wraps d_color_hsv with constexpr
// construction and self-operations.
struct hsv : d_color_hsv
{
    using model_tag  = hsv_tag;
    using value_type = channel_t;

    // hsv (default)
    //   constructor: zeroed (black).
    D_CONSTEXPR hsv()
        : d_color_hsv{ value_type(0), value_type(0), value_type(0) }
    {}

    // hsv (parameterized)
    //   constructor: from raw channels.
    D_CONSTEXPR hsv(
        value_type _h,
        value_type _s,
        value_type _v
    )
        : d_color_hsv{ _h, _s, _v }
    {}

    // hsv (converting)
    //   constructor: wraps a kernel-produced POD result.
    D_CONSTEXPR hsv(
        const d_color_hsv& _pod
    )
        : d_color_hsv(_pod)
    {}

    // operator==
    //   compare: exact channel equality.
    D_CONSTEXPR bool
    operator==(
        const hsv& _other
    ) const
    {
        return ( (h == _other.h) &&
                 (s == _other.s) &&
                 (v == _other.v) );
    }

    // is_valid
    //   query: h in [0, 360), s and v in [0, 1].
    D_CONSTEXPR_INLINE bool
    is_valid() const
    {
        return d_color_hsv_is_valid(*this);
    }

    // clamp
    //   transform: wrap hue, clamp s and v to [0, 1].
    D_CONSTEXPR_INLINE hsv
    clamp() const
    {
        return d_color_hsv_clamp(*this);
    }
};


NS_END  // djinterp


#endif  // DJINTERP_COLOR_HSV_HPP_
