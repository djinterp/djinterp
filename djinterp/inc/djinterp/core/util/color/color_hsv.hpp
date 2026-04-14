/******************************************************************************
* djinterp [color]                                               color_hsv.hpp
*
*   HSV (Hue, Saturation, Value) color model for the djinterp color
* module. Hue is stored in degrees [0, 360), saturation and value
* in [0, 1], with optional alpha.
*
* 
* path:      /inc/djinterp/util/color/color_hsv.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.12
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    HSV COLOR MODEL
      ----------------
      i.    hsv
            a. model_tag, value_type
            b. channels: h, s, v, a
            c. hsv()                    (default constructor)
            d. hsv(_h, _s, _v, _a)     (parameterized constructor)
            e. operator==
*/

#ifndef DJINTERP_COLOR_HSV_
#define DJINTERP_COLOR_HSV_ 1

#include "../../djinterp.hpp"
#include "./color.hpp"


NS_DJINTERP
NS_COLOR


// ================================================================
//  hsv
// ================================================================

// hsv
//   struct: represents a color in the HSV color model. Hue is
// in degrees [0, 360); saturation and value are normalized to
// [0, 1]. Carries optional alpha in [0, 1].
struct hsv
{
    using model_tag  = hsv_tag;
    using value_type = channel_t;

    value_type h;
    value_type s;
    value_type v;
    value_type a;

    // hsv (default)
    //   constructor: initializes to opaque black (h=0, s=0,
    // v=0, a=1).
    D_CONSTEXPR hsv()
        : h(0), s(0), v(0), a(1)
    {}

    // hsv (parameterized)
    //   constructor: initializes from individual channel
    // values. Hue is normalized to [0, 360); saturation,
    // value, and alpha are clamped to [0, 1].
    D_CONSTEXPR hsv(
        value_type _h,
        value_type _s,
        value_type _v,
        value_type _a = 1
    )
        : h(internal::normalize_hue(_h)),
          s(clamp_channel(_s, value_type(0), value_type(1))),
          v(clamp_channel(_v, value_type(0), value_type(1))),
          a(clamp_channel(_a, value_type(0), value_type(1)))
    {}

    // operator==
    //   function: constexpr equality comparison.
    D_CONSTEXPR bool
    operator==(
        const hsv& _other
    ) const
    {
        return ( (h == _other.h) &&
                 (s == _other.s) &&
                 (v == _other.v) &&
                 (a == _other.a) );
    }
};


NS_END  // color
NS_END  // djinterp

#endif  // DJINTERP_COLOR_HSV_
